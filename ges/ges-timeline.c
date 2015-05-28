#include "ges-timeline.h"
#include "ges-clip.h"
#include "ges-editable.h"
#include "ges-playable.h"
#include "nle.h"

/* Structure definitions */

typedef struct _GESTimelinePrivate
{
  GList *compositions;
  GList *nleobjects;
  guint expected_async_done;
  guint group_id;
  GESMediaType media_type;
  GList *ghostpads;
  gboolean is_playable;
} GESTimelinePrivate;

struct _GESTimeline
{
  GstBin parent;
  GESTimelinePrivate *priv;
};

#define NLE_OBJECT_OLD_PARENT (g_quark_from_string ("nle_object_old_parent_quark"))

static void ges_editable_interface_init (GESEditableInterface * iface);
static void ges_playable_interface_init (GESPlayableInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESTimeline, ges_timeline, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EDITABLE, ges_editable_interface_init)
    G_IMPLEMENT_INTERFACE (GES_TYPE_PLAYABLE, ges_playable_interface_init))

/* Interface implementation */

/* GESEditable */

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

static gboolean
_set_track_index (GESEditable *editable, GESMediaType media_type, guint index)
{
  GESTimeline *self = GES_TIMELINE (editable);
  GList *tmp;

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next)
  {
    GstCaps *caps;
    GstStructure *structure;
    GESMediaType object_type;
    guint new_priority;

    g_object_get (tmp->data, "caps", &caps, NULL);
    structure = gst_caps_get_structure (caps, 0);
    gst_caps_unref (caps);

    if (gst_structure_has_name (structure, GES_RAW_AUDIO_CAPS))
      object_type = GES_MEDIA_TYPE_AUDIO;
    else if (gst_structure_has_name (structure, GES_RAW_VIDEO_CAPS))
      object_type = GES_MEDIA_TYPE_VIDEO;
    else
      continue;

    if (object_type & media_type) {
      new_priority = (TRACK_PRIORITY_HEIGHT * index) + TIMELINE_PRIORITY_OFFSET;
      g_object_set (tmp->data, "priority", new_priority, NULL);
    }
  }

  return TRUE;
}

static void
ges_editable_interface_init (GESEditableInterface * iface)
{
  iface->set_inpoint = _set_inpoint;
  iface->set_duration = _set_duration;
  iface->set_start = _set_start;
  iface->get_nle_objects = _get_nle_objects;
  iface->set_track_index = _set_track_index;
}

/* GESPlayable */

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
_expose_nle_objects (GESTimeline *self)
{
  GList *tmp, *gpads;

  gpads = self->priv->ghostpads;

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next)
  {
    GstElement *capsfilter = gst_element_factory_make ("capsfilter", NULL);
    GstCaps *caps;
    GstElement *old_parent;
    GstPad *pad = gst_element_get_static_pad (capsfilter, "src");

    g_object_get (tmp->data, "composition", &old_parent, NULL);

    if (old_parent) {
      gst_object_ref (tmp->data);
      gst_bin_remove (GST_BIN (old_parent), GST_ELEMENT (tmp->data));
      g_object_set_qdata (tmp->data, NLE_OBJECT_OLD_PARENT, gst_object_ref (old_parent));
      nle_object_commit (NLE_OBJECT (old_parent), TRUE);
    }

    g_object_get (tmp->data, "caps", &caps, NULL);
    g_object_set (capsfilter, "caps", gst_caps_copy (caps), NULL);
    gst_caps_unref (caps);

    gst_bin_add_many (GST_BIN (self), capsfilter, tmp->data, NULL);
    gst_element_link (tmp->data, capsfilter);

    gst_ghost_pad_set_target (GST_GHOST_PAD (gpads->data), pad);
    gst_pad_set_active (GST_PAD(gpads->data), TRUE);

    gst_pad_add_probe (pad,
        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        (GstPadProbeCallback) _pad_probe_cb, self, NULL);

    gpads = gpads->next;
  }

  gst_element_no_more_pads (GST_ELEMENT (self));
}

static gboolean
_remove_child (GValue * item, GValue * ret G_GNUC_UNUSED, GstBin * bin)
{
  GstElement *child = g_value_get_object (item);

  gst_bin_remove (bin, child);

  return TRUE;
}

static void
_empty_bin (GstBin * bin)
{
  GstIterator *children;

  children = gst_bin_iterate_elements (bin);

  while (G_UNLIKELY (gst_iterator_fold (children,
              (GstIteratorFoldFunction) _remove_child, NULL,
              bin) == GST_ITERATOR_RESYNC)) {
    gst_iterator_resync (children);
  }

  gst_iterator_free (children);
}

static void
_unexpose_nle_objects (GESTimeline *self)
{
  GList *tmp;
  GList *gpads = self->priv->ghostpads;

  g_list_foreach (self->priv->nleobjects, (GFunc) gst_object_ref, NULL);
  _empty_bin (GST_BIN (self));

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next) {
    GstElement *old_parent;

    gst_ghost_pad_set_target (GST_GHOST_PAD (gpads->data), NULL);
    gst_pad_set_active (GST_PAD (gpads->data), FALSE);
    old_parent = GST_ELEMENT (g_object_get_qdata (tmp->data, NLE_OBJECT_OLD_PARENT));

    if (old_parent) {
      gst_bin_add (GST_BIN (old_parent), GST_ELEMENT (tmp->data));
      gst_object_unref (old_parent);
      nle_object_commit (NLE_OBJECT (old_parent), TRUE);
      g_object_set_qdata (tmp->data, NLE_OBJECT_OLD_PARENT, NULL);
    }

    gpads = gpads->next;
  }
}

static gboolean
_make_playable (GESPlayable *playable, gboolean is_playable)
{
  GESTimeline *self = GES_TIMELINE (playable);

  if (is_playable) {
    _expose_nle_objects (self);
    self->priv->is_playable = TRUE;
  } else {
    _unexpose_nle_objects (self);
  }

  return TRUE;
}

static void
ges_playable_interface_init (GESPlayableInterface * iface)
{
  iface->make_playable = _make_playable;
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

static void
_add_expandable_operation (GstElement *composition, const gchar *element_name, guint priority, const gchar *name)
{
  GstElement *expandable = gst_element_factory_make ("nleoperation", name);
  GstElement *element = gst_element_factory_make (element_name, NULL);

  gst_bin_add (GST_BIN (expandable), element);

  g_object_set (expandable, "expandable", TRUE, "priority", priority, NULL);
  gst_bin_add (GST_BIN (composition), expandable);
}

static void
_add_expandable_source (GstElement *composition, const gchar *element_name, guint priority, const gchar *name)
{
  GstElement *expandable = gst_element_factory_make ("nlesource", name);
  GstElement *element = gst_element_factory_make (element_name, NULL);

  gst_bin_add (GST_BIN (expandable), element);

  g_object_set (expandable, "expandable", TRUE, "priority", priority, NULL);
  gst_bin_add (GST_BIN (composition), expandable);
}

static GstElement *
_create_composition (GESTimeline *self, const gchar *caps_string, const gchar *name)
{
  GstPad *ghostpad;
  GstElement *composition = gst_element_factory_make ("nlecomposition", name);
  GstElement *wrapper = gst_element_factory_make ("nlesource", NULL);

  GST_DEBUG_OBJECT (self, "creating composition %s with caps %s", name, caps_string);
  g_object_set (composition, "caps", gst_caps_from_string (caps_string), NULL);
  g_object_set (wrapper, "caps", gst_caps_from_string (caps_string), NULL);

  gst_bin_add (GST_BIN (wrapper), composition);

  self->priv->compositions = g_list_append (self->priv->compositions, composition);
  self->priv->nleobjects = g_list_append (self->priv->nleobjects, wrapper);

  ghostpad = gst_ghost_pad_new_no_target (NULL, GST_PAD_SRC);
  gst_element_add_pad (GST_ELEMENT (self), ghostpad);
  self->priv->ghostpads = g_list_append (self->priv->ghostpads, ghostpad);

  return composition;
}

static void
_make_nle_objects (GESTimeline *self)
{
  GstElement *composition;

  if (self->priv->media_type & GES_MEDIA_TYPE_AUDIO) {
    composition = _create_composition (self, GES_RAW_AUDIO_CAPS, "audio-composition");
    _add_expandable_operation (composition, "audiomixer", 0, "timeline-audiomixer");
    _add_expandable_source (composition, "audiotestsrc", 1, "timeline-audio-background");
  }

  if (self->priv->media_type & GES_MEDIA_TYPE_VIDEO) {
    composition = _create_composition (self, GES_RAW_VIDEO_CAPS, "video-composition");
    _add_expandable_operation (composition, "compositor", 0, "timeline-videomixer");
    _add_expandable_source (composition, "videotestsrc", 1, "timeline-video-background");
  }
}

static void
ges_timeline_handle_message (GstBin * bin, GstMessage * message)
{
  GESTimeline *timeline = GES_TIMELINE (bin);

  if (!timeline->priv->is_playable)
    goto forward;

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
    gst_caps_unref (caps);

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
      gst_bin_remove (GST_BIN (parent), GST_ELEMENT (tmp->data));
      gst_object_unref (parent);
    }
    GST_DEBUG_OBJECT (tmp->data, "got added in composition %p", composition);
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
      GST_DEBUG_OBJECT (self, "timeline setting media type to %d\n", self->priv->media_type);
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
  self->priv->ghostpads = NULL;
  self->priv->expected_async_done = 0;
  self->priv->group_id = -1;
  self->priv->is_playable = FALSE;
}
