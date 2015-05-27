#include "ges-timeline.h"
#include "ges-clip.h"
#include "ges-editable.h"
#include "nle.h"

/* Structure definitions */

typedef struct _GESTimelinePrivate
{
  GList *compositions;
  GList *nleobjects;
  guint expected_async_done;
  guint group_id;
  GESMediaType media_type;
} GESTimelinePrivate;

struct _GESTimeline
{
  GstBin parent;
  GESTimelinePrivate *priv;
};

static void ges_editable_interface_init (GESEditableInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESTimeline, ges_timeline, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EDITABLE, ges_editable_interface_init))

/* Interface implementation */

static gboolean
_set_inpoint (GESEditable *editable, GstClockTime inpoint)
{
  GESTimeline *timeline = GES_TIMELINE (editable);
  GList *tmp;

  for (tmp = timeline->priv->nleobjects; tmp; tmp = tmp->next) {
    g_object_set (tmp->data, "inpoint", inpoint, NULL);
  }

  return TRUE;
}

static gboolean
_set_duration (GESEditable *editable, GstClockTime duration)
{
  GESTimeline *timeline = GES_TIMELINE (editable);
  GList *tmp;

  for (tmp = timeline->priv->nleobjects; tmp; tmp = tmp->next) {
    g_object_set (tmp->data, "duration", duration, NULL);
  }

  return TRUE;
}

static gboolean
_set_start (GESEditable *editable, GstClockTime start)
{
  GESTimeline *timeline = GES_TIMELINE (editable);
  GList *tmp;

  for (tmp = timeline->priv->nleobjects; tmp; tmp = tmp->next) {
    g_object_set (tmp->data, "start", start, NULL);
  }

  return TRUE;
}

static GList *
_get_nle_objects (GESEditable *editable)
{
  return g_list_copy (GES_TIMELINE (editable)->priv->nleobjects);
}

static void
ges_editable_interface_init (GESEditableInterface * iface)
{
  iface->set_inpoint = _set_inpoint;
  iface->set_duration = _set_duration;
  iface->set_start = _set_start;
  iface->get_nle_objects = _get_nle_objects;
}

/* Implementation */

static GList *
_get_compositions (GESTimeline *self, GESMediaType media_type)
{
  GList *res = NULL, *tmp;

  if (media_type == GES_MEDIA_TYPE_UNKNOWN)
    return g_list_copy (self->priv->compositions);

  for (tmp = self->priv->compositions; tmp; tmp = tmp->next) {
    GstCaps *caps;
    GstStructure *structure;

    g_object_get (tmp->data, "caps", &caps, NULL);
    structure = gst_caps_get_structure (caps, 0);

    if (media_type == GES_MEDIA_TYPE_AUDIO && gst_structure_has_name (structure, GES_RAW_AUDIO_CAPS))
      res = g_list_append (res, tmp->data);
    else if (media_type == GES_MEDIA_TYPE_VIDEO && gst_structure_has_name (structure, GES_RAW_VIDEO_CAPS))
      res = g_list_append (res, tmp->data);

    gst_caps_unref (caps);
  }

  return res;
}

static GstElement *
_get_first_composition (GESTimeline *self, GESMediaType media_type)
{
  GstElement *res;
  GList *compositions = _get_compositions (self, media_type);

  if (!compositions)
    return NULL;

  res = GST_ELEMENT (compositions->data);
  g_list_free (compositions);

  return res;
}

static GstPadProbeReturn
_pad_probe_cb (GstPad * mixer_pad, GstPadProbeInfo * info,
    GESTimeline * timeline)
{
  GstEvent *event = GST_PAD_PROBE_INFO_EVENT (info);
  if (GST_EVENT_TYPE (event) == GST_EVENT_STREAM_START) {
    if (timeline->priv->group_id == -1) {
      if (!gst_event_parse_group_id (event, &timeline->priv->group_id))
        timeline->priv->group_id = gst_util_group_id_next ();
    }

    info->data = gst_event_make_writable (event);
    gst_event_set_group_id (GST_PAD_PROBE_INFO_EVENT (info),
        timeline->priv->group_id);

    return GST_PAD_PROBE_REMOVE;
  }

  return GST_PAD_PROBE_OK;
}

static void
_ghost_srcpad (GESTimeline *self, GstElement *element)
{
  GstPad *pad, *ghostpad;
  gchar *padname;

  pad = gst_element_get_static_pad (element, "src");

  /* ghost it ! */
  GST_DEBUG ("Ghosting pad and adding it to ourself");
  padname = g_strdup_printf ("comp_%p_src", element);
  ghostpad = gst_ghost_pad_new (padname, pad);
  g_free (padname);
  gst_pad_set_active (ghostpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (self), ghostpad);

  gst_pad_add_probe (pad,
      GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      (GstPadProbeCallback) _pad_probe_cb, self, NULL);
}

static void
_add_expandable_operation (GstElement *composition, const gchar *element_name, guint priority)
{
  GstElement *expandable = gst_element_factory_make ("nleoperation", NULL);
  GstElement *element = gst_element_factory_make (element_name, NULL);

  gst_bin_add (GST_BIN (expandable), element);

  g_object_set (expandable, "expandable", TRUE, "priority", priority, NULL);
  gst_bin_add (GST_BIN (composition), expandable);
}

static void
_add_expandable_source (GstElement *composition, const gchar *element_name, guint priority)
{
  GstElement *expandable = gst_element_factory_make ("nlesource", NULL);
  GstElement *element = gst_element_factory_make (element_name, NULL);

  gst_bin_add (GST_BIN (expandable), element);

  g_object_set (expandable, "expandable", TRUE, "priority", priority, NULL);
  gst_bin_add (GST_BIN (composition), expandable);
}

static GstElement *
_create_composition (GESTimeline *self, const gchar *caps_string, const gchar *name)
{
  GstElement *composition = gst_element_factory_make ("nlecomposition", name);
  GstElement *capsfilter = gst_element_factory_make ("capsfilter", NULL);
  GstElement *wrapper = gst_element_factory_make ("nlesource", NULL);

  GST_ERROR_OBJECT (self, "creating composition %s with caps %s", name, caps_string);
  g_object_set (capsfilter, "caps", gst_caps_from_string (caps_string), NULL);
  g_object_set (composition, "caps", gst_caps_from_string (caps_string), NULL);
  g_object_set (wrapper, "caps", gst_caps_from_string (caps_string), NULL);

  gst_bin_add (GST_BIN (wrapper), composition);
  gst_bin_add_many (GST_BIN(self), wrapper, capsfilter, NULL);
  gst_element_link (wrapper, capsfilter);

  self->priv->compositions = g_list_append (self->priv->compositions, composition);
  self->priv->nleobjects = g_list_append (self->priv->nleobjects, wrapper);

  GST_ERROR_OBJECT (wrapper, "wrapper was created");
  _ghost_srcpad (self, capsfilter);

  return composition;
}

static void
_make_nle_objects (GESTimeline *self)
{
  GstElement *composition;

  if (self->priv->media_type & GES_MEDIA_TYPE_AUDIO) {
    composition = _create_composition (self, GES_RAW_AUDIO_CAPS, "audio-composition");
    _add_expandable_operation (composition, "audiomixer", 0);
    _add_expandable_source (composition, "audiotestsrc", 1);
  }

  if (self->priv->media_type & GES_MEDIA_TYPE_VIDEO) {
    composition = _create_composition (self, GES_RAW_VIDEO_CAPS, "video-composition");
    _add_expandable_operation (composition, "compositor", 0);
    _add_expandable_source (composition, "videotestsrc", 1);
  }

  gst_element_no_more_pads (GST_ELEMENT (self));
}

static void
ges_timeline_handle_message (GstBin * bin, GstMessage * message)
{
  GESTimeline *timeline = GES_TIMELINE (bin);

  if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ELEMENT) {
    GstMessage *amessage = NULL;
    const GstStructure *mstructure = gst_message_get_structure (message);

    if (gst_structure_has_name (mstructure, "NleCompositionStartUpdate")) {
      if (g_strcmp0 (gst_structure_get_string (mstructure, "reason"), "Seek")) {
        GST_DEBUG_OBJECT (timeline,
            "A composition is starting an update because of %s"
            " not concidering async", gst_structure_get_string (mstructure,
                "reason"));

        goto forward;
      }

      GST_OBJECT_LOCK (timeline);
      if (timeline->priv->expected_async_done == 0) {
        amessage = gst_message_new_async_start (GST_OBJECT_CAST (bin));
        timeline->priv->expected_async_done = g_list_length (timeline->priv->compositions);
        GST_INFO_OBJECT (timeline, "Posting ASYNC_START %s",
            gst_structure_get_string (mstructure, "reason"));
      }
      GST_OBJECT_UNLOCK (timeline);

    } else if (gst_structure_has_name (mstructure, "NleCompositionUpdateDone")) {
      if (g_strcmp0 (gst_structure_get_string (mstructure, "reason"), "Seek")) {
        GST_DEBUG_OBJECT (timeline,
            "A composition is done updating because of %s"
            " not concidering async", gst_structure_get_string (mstructure,
                "reason"));

        goto forward;
      }

      GST_OBJECT_LOCK (timeline);
      timeline->priv->expected_async_done -= 1;
      GST_DEBUG ("expected async done is : %d", timeline->priv->expected_async_done);
      if (timeline->priv->expected_async_done == 0) {
        amessage = gst_message_new_async_done (GST_OBJECT_CAST (bin),
            GST_CLOCK_TIME_NONE);
        GST_DEBUG_OBJECT (timeline, "Posting ASYNC_DONE %s",
            gst_structure_get_string (mstructure, "reason"));
      }
      GST_OBJECT_UNLOCK (timeline);
    }

    if (amessage)
      gst_element_post_message (GST_ELEMENT_CAST (bin), amessage);
  }

forward:
  gst_element_post_message (GST_ELEMENT_CAST (bin), message);
}

/* API */

gboolean
ges_timeline_add_editable (GESTimeline *self, GESEditable *editable)
{
  GList *tmp;
  GList *nleobjects = ges_editable_get_nle_objects (editable);

  for (tmp = nleobjects; tmp; tmp = tmp->next) {
    GstCaps *caps;
    GstStructure *structure;
    GESMediaType media_type;
    GstElement *composition;
    GstObject *parent = gst_object_get_parent (tmp->data);

    g_object_get (tmp->data, "caps", &caps, NULL);
    structure = gst_caps_get_structure (caps, 0);

    if (gst_structure_has_name (structure, GES_RAW_AUDIO_CAPS))
      media_type = GES_MEDIA_TYPE_AUDIO;
    else if (gst_structure_has_name (structure, GES_RAW_VIDEO_CAPS))
      media_type = GES_MEDIA_TYPE_VIDEO;
    else
      continue;

    composition = _get_first_composition (self, media_type);
    if (!composition)
      continue;

    g_object_set (tmp->data, "priority", 2, NULL);
    if (parent) {
      gst_object_ref (tmp->data);
      gst_object_unparent (GST_OBJECT (tmp->data));
      gst_object_unref (parent);
    }
    gst_bin_add (GST_BIN (composition), GST_ELEMENT (tmp->data));
  }

  return TRUE;
}

GESTimeline *
ges_timeline_new (GESMediaType media_type)
{
  return g_object_new (GES_TYPE_TIMELINE, "media-type", media_type, NULL);
}

gboolean
ges_timeline_commit (GESTimeline *self)
{
  GList *tmp;

  for (tmp = self->priv->compositions; tmp; tmp = tmp->next) {
    nle_object_commit (NLE_OBJECT(tmp->data), TRUE);
  }

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next) {
    nle_object_commit (NLE_OBJECT(tmp->data), TRUE);
  }

  return TRUE;
}

GList *ges_timeline_get_compositions_by_media_type (GESTimeline *self, GESMediaType media_type)
{
  return _get_compositions (self, media_type);
}

/* GObject initialization */

enum
{
  PROP_0,
  PROP_MEDIA_TYPE,
};

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESTimeline *self = GES_TIMELINE (object);

  switch (property_id) {
    case PROP_MEDIA_TYPE:
      self->priv->media_type = g_value_get_flags (value);
      GST_ERROR ("timeline setting media type to %d\n", self->priv->media_type);
      _make_nle_objects (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESTimelinePrivate *priv = GES_TIMELINE (object)->priv;

  switch (property_id) {
    case PROP_MEDIA_TYPE:
      g_value_set_flags (value, priv->media_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_timeline_class_init (GESTimelineClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GstBinClass *bin_class = GST_BIN_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTimelinePrivate));

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;

  g_object_class_override_property (g_object_class, PROP_MEDIA_TYPE, "media-type");

  bin_class->handle_message = GST_DEBUG_FUNCPTR (ges_timeline_handle_message);
}

static void
ges_timeline_init (GESTimeline *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_TIMELINE, GESTimelinePrivate);

  self->priv->compositions = NULL;
  self->priv->nleobjects = NULL;
  self->priv->expected_async_done = 0;
  self->priv->group_id = -1;
}
