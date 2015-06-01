#include "gst/controller/controller.h"
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
  GList *children;
} GESObjectPrivate;

static void ges_object_child_proxy_init (GstChildProxyInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESObject, ges_object, G_TYPE_OBJECT,
    G_ADD_PRIVATE (GESObject)
    G_IMPLEMENT_INTERFACE (GST_TYPE_CHILD_PROXY, ges_object_child_proxy_init))

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

static gchar *
_get_base_property_name (const gchar * name)
{
  gchar **names, **current;
  gchar *prop_name = NULL;

  names = current = g_strsplit (name, "::", -1);

  while (*current) {
    if (!current[1]) {
      prop_name = g_strdup (*current);
      g_free (*current);
      *current = NULL;
    }
    current++;
  }

  g_strfreev (names);

  return prop_name;
}

static GObject *
_lookup_element_for_property (GESObject *self, const gchar *property_name, GParamSpec **pspec)
{
  GList *objects = gst_child_proxy_lookup_all (GST_CHILD_PROXY (self), property_name);
  GObject *object;
  GObjectClass *klass;
  gchar *base_property_name;

  if (!objects) {
    GST_ERROR_OBJECT (self, "No such property %s in any of the elements we contain", property_name);
    return NULL;
  }

  if (g_list_length (objects) != 1) {
    GST_ERROR_OBJECT (self, "Too many objects expose a %s property, please be more specific, see gst_child_proxy_lookup_all documentation", property_name);
    g_list_free_full (objects, g_object_unref);
    return NULL;
  }

  base_property_name = _get_base_property_name (property_name);

  object = g_object_ref (objects->data);
  g_list_free_full (objects, g_object_unref);
  klass = G_OBJECT_GET_CLASS (object);

  *pspec = g_object_class_find_property (klass, base_property_name);
  g_free (base_property_name);

  return object;
}

GstControlSource *
ges_object_get_interpolation_control_source (GESObject * self,
    const gchar * property_name, GType binding_type)
{
  GParamSpec *pspec = NULL;
  GstControlSource *source = NULL;
  GstControlBinding *binding;
  GObject *object = _lookup_element_for_property (self, property_name, &pspec);
  gchar *base_property_name = _get_base_property_name (property_name);

  if (!object)
    goto beach;

  if (!GST_IS_OBJECT (object)) {
    GST_ERROR_OBJECT (self, "can't control a %s as it is not a GstObject", G_OBJECT_TYPE_NAME (object));
    goto beach;
  }

  if ((binding = gst_object_get_control_binding (GST_OBJECT (object), base_property_name))) {
    g_object_get (binding, "control-source", &source, NULL);
    g_object_unref (binding);
    goto beach;
  }

  if (binding_type != G_TYPE_NONE && binding_type != GST_TYPE_DIRECT_CONTROL_BINDING) {
    GST_ERROR_OBJECT (self, "We only support direct control bindings for now");
    goto beach;
  }

  source = gst_interpolation_control_source_new ();
  binding = gst_direct_control_binding_new (GST_OBJECT (object), base_property_name, source);

  if (!binding) {
    g_object_unref (source);
    source = NULL;
    goto beach;
  }

  gst_object_add_control_binding (GST_OBJECT (object), binding);

beach:
  g_free (base_property_name);
  return source;
}

/* GstChildProxy implementation */

static guint
_get_children_count (GstChildProxy *proxy)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (proxy);

  return g_list_length (priv->children);
}

static GObject *
_get_child_by_index (GstChildProxy *proxy, guint index)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (proxy);

  return g_object_ref (g_list_nth_data (priv->children, index));
}

static void
_child_added (GstChildProxy *proxy, GObject *child, const gchar *name)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (proxy);

  priv->children = g_list_append (priv->children, child);
}

static void
_child_removed (GstChildProxy *proxy, GObject *child, const gchar *name)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (proxy);

  priv->children = g_list_remove (priv->children, child);
}

static gchar *
_get_object_name (GstChildProxy *proxy, GObject *child)
{
  if (GES_IS_OBJECT (child))
    return g_strdup (""); /* FIXME */
  else if (GST_IS_OBJECT (child))
    return gst_object_get_name (GST_OBJECT (child));
  else
    return NULL;
}

static void
ges_object_child_proxy_init (GstChildProxyInterface *iface)
{
  iface->get_children_count = _get_children_count;
  iface->get_child_by_index = _get_child_by_index;
  iface->child_added = _child_added;
  iface->child_removed = _child_removed;
  iface->get_object_name = _get_object_name;
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
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);

  priv->children = NULL;
}
