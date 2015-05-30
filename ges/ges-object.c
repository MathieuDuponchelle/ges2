#include "ges-object.h"

#define GES_OBJECT_PRIV(self) (ges_object_get_instance_private (GES_OBJECT (self)))

/* Structure definitions */

typedef struct _GESObjectPrivate
{
  GESMediaType media_type;
  GstClockTime inpoint;
  GstClockTime duration;
  GstClockTime start;
  guint track_index;
} GESObjectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GESObject, ges_object, G_TYPE_OBJECT)

/* API */

gboolean
ges_object_set_media_type (GESObject *self, GESMediaType media_type)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (self);

  if (klass->set_media_type && klass->set_media_type (self, media_type)) {
    priv->media_type = media_type;
    return TRUE;
  } else
    GST_ERROR_OBJECT (self, "could not set media type to %d, check your code", media_type);

  return FALSE;
}

gboolean
ges_object_set_inpoint (GESObject *self, GstClockTime inpoint)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (self);

  if (klass->set_inpoint && klass->set_inpoint (self, inpoint)) {
    priv->inpoint = inpoint;
    return TRUE;
  } else
    GST_ERROR_OBJECT (self, "could not set inpoint to %" GST_TIME_FORMAT, GST_TIME_ARGS (inpoint));

  return FALSE;
}

gboolean
ges_object_set_duration (GESObject *self, GstClockTime duration)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (self);

  if (klass->set_duration && klass->set_duration (self, duration)) {
    priv->duration = duration;
    return TRUE;
  } else
    GST_ERROR_OBJECT (self, "could not set duration to %" GST_TIME_FORMAT, GST_TIME_ARGS (duration));

  return FALSE;
}

gboolean
ges_object_set_start (GESObject *self, GstClockTime start)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (self);

  if (klass->set_start && klass->set_start (self, start)) {
    priv->start = start;
    return TRUE;
  } else
    GST_ERROR_OBJECT (self, "could not set start to %" GST_TIME_FORMAT, GST_TIME_ARGS (start));

  return FALSE;
}

gboolean
ges_object_set_track_index (GESObject *self, GESMediaType media_type, guint track_index)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (self);

  if (klass->set_track_index && klass->set_track_index (self, media_type, track_index)) {
    priv->track_index = track_index;
    return TRUE;
  } else
    GST_ERROR_OBJECT (self, "could not set track index to %d", track_index);

  return FALSE;
}

GESMediaType
ges_object_get_media_type (GESObject *self)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);

  return priv->media_type;
}

GstClockTime
ges_object_get_inpoint (GESObject *self)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);

  return priv->inpoint;
}

GstClockTime
ges_object_get_start (GESObject *self)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);

  return priv->start;
}

GstClockTime
ges_object_get_duration (GESObject *self)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);

  return priv->duration;
}

guint
ges_object_get_track_index (GESObject *self, GESMediaType media_type)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);

  return priv->track_index;
}

GList *
ges_object_get_nle_objects (GESObject *self)
{
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (self);

  if (klass->get_nle_objects)
    return klass->get_nle_objects (self);

  return NULL;
}

/* GObject initialization */

enum
{
  PROP_0,
  PROP_MEDIA_TYPE,
  PROP_INPOINT,
  PROP_DURATION,
  PROP_START,
  PROP_TRACK_INDEX,
};

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESObject *self = GES_OBJECT (object);

  switch (property_id) {
    case PROP_MEDIA_TYPE:
      ges_object_set_media_type (self, g_value_get_flags (value));
      break;
    case PROP_INPOINT:
      ges_object_set_inpoint (self, g_value_get_uint64 (value));
      break;
    case PROP_DURATION:
      ges_object_set_duration (self, g_value_get_uint64 (value));
      break;
    case PROP_START:
      ges_object_set_start (self, g_value_get_uint64 (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  switch (property_id) {
    case PROP_MEDIA_TYPE:
      g_value_set_flags (value, priv->media_type);
      break;
    case PROP_INPOINT:
      g_value_set_uint64 (value, priv->inpoint);
      break;
    case PROP_DURATION:
      g_value_set_uint64 (value, priv->duration);
      break;
    case PROP_START:
      g_value_set_uint64 (value, priv->start);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_object_class_init (GESObjectClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;

  g_object_class_install_property (g_object_class, PROP_MEDIA_TYPE,
      g_param_spec_flags ("media-type", "Media Type", "The GESMediaType of the object", GES_TYPE_MEDIA_TYPE,
          GES_MEDIA_TYPE_UNKNOWN, G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class, PROP_INPOINT,
      g_param_spec_uint64 ("inpoint", "Inpoint", "The inpoint of the object", 0, G_MAXUINT64,
          0, G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class, PROP_DURATION,
      g_param_spec_uint64 ("duration", "Duration", "The duration of the object", 0, G_MAXUINT64,
          0, G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class, PROP_START,
      g_param_spec_uint64 ("start", "Start", "The start of the object", 0, G_MAXUINT64,
          0, G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class, PROP_TRACK_INDEX,
      g_param_spec_uint ("track-index", "Track index", "The index of the containing track", 0, G_MAXUINT,
          0, G_PARAM_READWRITE));

  klass->set_media_type = NULL;
  klass->set_inpoint = NULL;
  klass->set_duration = NULL;
  klass->set_start = NULL;
  klass->set_track_index = NULL;
}

static void
ges_object_init (GESObject *self)
{
}
