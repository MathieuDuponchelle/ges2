#include "ges-track.h"
#include "ges-editable.h"
#include "nle.h"

/* Structure definitions */

typedef struct _GESTrackPrivate
{
  GstElement *composition;
  GESMediaType media_type;
  guint index;
} GESTrackPrivate;

struct _GESTrack
{
  GObject parent;
  GESTrackPrivate *priv;
};

G_DEFINE_TYPE(GESTrack, ges_track, G_TYPE_OBJECT)

/* API */

gboolean
ges_track_add_editable (GESTrack *self, GESEditable *editable)
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
