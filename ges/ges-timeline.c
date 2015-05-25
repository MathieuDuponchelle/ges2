#include "ges-timeline.h"
#include "ges-clip.h"
#include "nle.h"

/* Structure definitions */

typedef struct _GESTimelinePrivate
{
  GList *nleobjects;
  guint expected_async_done;
  guint group_id;
} GESTimelinePrivate;

struct _GESTimeline
{
  GstBin parent;
  GESTimelinePrivate *priv;
};

G_DEFINE_TYPE (GESTimeline, ges_timeline, GST_TYPE_BIN)

/* Implementation */

static GList *
_get_compositions (GESTimeline *self, GESMediaType media_type)
{
  GList *res = NULL, *tmp;

  if (media_type == GES_MEDIA_TYPE_UNKNOWN)
    return g_list_copy (self->priv->nleobjects);

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next) {
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
_create_composition (GESTimeline *self, const gchar *caps_string, const gchar *name)
{
  GstElement *composition = gst_element_factory_make ("nlecomposition", name);
  GstElement *capsfilter = gst_element_factory_make ("capsfilter", NULL);

  g_object_set (capsfilter, "caps", gst_caps_from_string (caps_string), NULL);
  g_object_set (composition, "caps", gst_caps_from_string (caps_string), NULL);

  gst_bin_add_many (GST_BIN(self), composition, capsfilter, NULL);
  gst_element_link (composition, capsfilter);

  self->priv->nleobjects = g_list_append (self->priv->nleobjects, composition);
  _ghost_srcpad (self, capsfilter);
}

static void
_make_nle_objects (GESTimeline *self)
{
  _create_composition (self, GES_RAW_AUDIO_CAPS, "audio-composition");
  _create_composition (self, GES_RAW_VIDEO_CAPS, "video-composition");
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
        timeline->priv->expected_async_done = g_list_length (timeline->priv->nleobjects);
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
ges_timeline_add_clip (GESTimeline *self, GESClip *clip)
{
  GstElement *composition = _get_first_composition (self, ges_clip_get_media_type (clip));

  if (!composition)
    return FALSE;
  return gst_bin_add (GST_BIN (composition), ges_clip_get_nleobject (clip));
}

GESTimeline *
ges_timeline_new (void)
{
  return g_object_new (GES_TYPE_TIMELINE, NULL);
}

GList *
ges_timeline_get_nleobjects (GESTimeline *self)
{
  return self->priv->nleobjects;
}

gboolean
ges_timeline_commit (GESTimeline *self)
{
  GList *tmp;

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
};

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESTimeline *self = GES_TIMELINE (object);
  (void) self;
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
}

static void
_constructed (GObject *object)
{
  GESTimeline *timeline = GES_TIMELINE (object);

  _make_nle_objects (timeline);
}

static void
ges_timeline_class_init (GESTimelineClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GstBinClass *bin_class = GST_BIN_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTimelinePrivate));

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->constructed = _constructed;

  bin_class->handle_message = GST_DEBUG_FUNCPTR (ges_timeline_handle_message);
}

static void
ges_timeline_init (GESTimeline *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_TIMELINE, GESTimelinePrivate);

  self->priv->nleobjects = NULL;
  self->priv->expected_async_done = 0;
  self->priv->group_id = -1;
}
