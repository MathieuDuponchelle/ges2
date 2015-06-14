#include "gst/controller/controller.h"
#include "ges-object.h"

#define GES_OBJECT_PRIV(self) (ges_object_get_instance_private (GES_OBJECT (self)))

/**
 * SECTION: gesobject
 *
 * #GESObject is the base class for all objects that can be edited.
 *
 * The properties it exposes are:
 *
 * - Timing properties such as start, inpoint and duration.
 * - The type of the media contained in it.
 * - #GstControlSources for all of its controllable properties,
 *   to let you set keyframes on them.
 */

/* Structure definitions */

typedef struct _GESObjectPrivate
{
  GESMediaType media_type;
  GstClockTime inpoint;
  GstClockTime duration;
  GstClockTime start;
  guint video_track_index;
  guint audio_track_index;
  GList *children;
  GList *control_sources;
} GESObjectPrivate;

static void ges_object_child_proxy_init (GstChildProxyInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESObject, ges_object, G_TYPE_INITIALLY_UNOWNED,
    G_ADD_PRIVATE (GESObject)
    G_IMPLEMENT_INTERFACE (GST_TYPE_CHILD_PROXY, ges_object_child_proxy_init))

enum
{
  PROP_0,
  PROP_MEDIA_TYPE,
  PROP_INPOINT,
  PROP_DURATION,
  PROP_START,
  PROP_VIDEO_TRACK_INDEX,
  PROP_AUDIO_TRACK_INDEX,
  PROP_LAST,
};

static GParamSpec *properties[PROP_LAST];

/* API */

/**
 * ges_object_set_media_type:
 * @object: a #GESObject
 * @media_type: the new #GESMediaType
 *
 * Set the #GESObject:media-type of @object
 *
 * Note that not all subclasses of #GESObject support changing the media
 * type after construction.
 *
 * Returns: %TRUE if @media_type could be set, %FALSE otherwise
 */
gboolean
ges_object_set_media_type (GESObject *object, GESMediaType media_type)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (object);

  if (klass->set_media_type && klass->set_media_type (object, media_type)) {
    priv->media_type = media_type;
    return TRUE;
  } else
    GST_ERROR_OBJECT (object, "could not set media type to %d, check your code", media_type);

  return FALSE;
}

/**
 * ges_object_set_inpoint:
 * @object: a #GESObject
 * @inpoint: the new inpoint in nanoseconds
 *
 * Set the #GESObject:inpoint of @object
 *
 * {{ set_object_inpoint.markdown }}
 *
 * Returns: %TRUE if @inpoint could be set, %FALSE otherwise
 */
gboolean
ges_object_set_inpoint (GESObject *object, GstClockTime inpoint)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (object);

  if (klass->set_inpoint && klass->set_inpoint (object, inpoint)) {
    priv->inpoint = inpoint;
    return TRUE;
  } else
    GST_ERROR_OBJECT (object, "could not set inpoint to %" GST_TIME_FORMAT, GST_TIME_ARGS (inpoint));

  return FALSE;
}

/**
 * ges_object_set_duration:
 * @object: a #GESObject
 * @duration: the new duration in nanoseconds
 *
 * Set the #GESObject:duration of @object
 *
 * {{ set_object_duration.markdown }}
 *
 * Returns: %TRUE if @duration could be set, %FALSE otherwise
 */
gboolean
ges_object_set_duration (GESObject *object, GstClockTime duration)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (object);

  if (klass->set_duration && klass->set_duration (object, duration)) {
    priv->duration = duration;
    return TRUE;
  } else
    GST_ERROR_OBJECT (object, "could not set duration to %" GST_TIME_FORMAT, GST_TIME_ARGS (duration));

  return FALSE;
}

/**
 * ges_object_set_start:
 * @object: a #GESObject
 * @inpoint: the new start in nanoseconds
 *
 * Set the #GESObject:start of @object
 *
 * Returns: %TRUE if @start could be set, %FALSE otherwise
 */
gboolean
ges_object_set_start (GESObject *object, GstClockTime start)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (object);

  if (klass->set_start && klass->set_start (object, start)) {
    priv->start = start;
    return TRUE;
  } else
    GST_ERROR_OBJECT (object, "could not set start to %" GST_TIME_FORMAT, GST_TIME_ARGS (start));

  return FALSE;
}

/**
 * ges_object_set_video_track_index:
 * @object: a #GESObject
 * @track_index: The new track index.
 *
 * {{ set_object_video_track_index.markdown }}
 *
 * Set the #GESObject:video-track-index of @object.
 *
 * Returns: %TRUE if @track_index could be set, %FALSE otherwise
 */
gboolean
ges_object_set_video_track_index (GESObject *object, guint track_index)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (object);

  if (klass->set_track_index && klass->set_track_index (object, GES_MEDIA_TYPE_VIDEO, track_index)) {
    priv->video_track_index = track_index;
    g_object_notify_by_pspec(G_OBJECT (object), properties[PROP_VIDEO_TRACK_INDEX]);
    return TRUE;
  } else
    GST_ERROR_OBJECT (object, "could not set track index to %d", track_index);

  return FALSE;
}

/**
 * ges_object_set_audio_track_index:
 * @object: a #GESObject
 * @track_index: The new track index.
 *
 * Set the #GESObject:audio-track-index of @object.
 *
 * Returns: %TRUE if @track_index could be set, %FALSE otherwise
 */
gboolean
ges_object_set_audio_track_index (GESObject *object, guint track_index)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (object);

  if (klass->set_track_index && klass->set_track_index (object, GES_MEDIA_TYPE_AUDIO, track_index)) {
    priv->audio_track_index = track_index;
    g_object_notify_by_pspec(G_OBJECT (object), properties[PROP_AUDIO_TRACK_INDEX]);
    return TRUE;
  } else
    GST_ERROR_OBJECT (object, "could not set track index to %d", track_index);

  return FALSE;
}

/**
 * ges_object_get_media_type:
 * @object: a #GESObject
 *
 * Get the #GESObject:media-type of @object
 *
 * Returns: The #GESMediaType of @object
 */
GESMediaType
ges_object_get_media_type (GESObject *object)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  return priv->media_type;
}

/**
 * ges_object_get_inpoint:
 * @object: a #GESObject
 *
 * Get the #GESObject:inpoint of @object
 *
 * Returns: The inpoint of @object
 */
GstClockTime
ges_object_get_inpoint (GESObject *object)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  return priv->inpoint;
}

/**
 * ges_object_get_start:
 * @object: a #GESObject
 *
 * Get the #GESObject:start of @object
 *
 * Returns: The start of @object
 */
GstClockTime
ges_object_get_start (GESObject *object)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  return priv->start;
}

/**
 * ges_object_get_duration:
 * @object: a #GESObject
 *
 * Get the #GESObject:duration of @object
 *
 * Returns: The duration of @object
 */
GstClockTime
ges_object_get_duration (GESObject *object)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  return priv->duration;
}

/**
 * ges_object_get_video_track_index:
 * @object: a #GESObject
 *
 * Get the #GESObject:video-track-index of @object
 *
 * Returns: The video track index of @object
 */
guint
ges_object_get_video_track_index (GESObject *object)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  return priv->video_track_index;
}

/**
 * ges_object_get_audio_track_index:
 * @object: a #GESObject
 *
 * Get the #GESObject:audio-track-index of @object
 *
 * Returns: The audio track index of @object
 */
guint
ges_object_get_audio_track_index (GESObject *object)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  return priv->audio_track_index;
}

GList *
ges_object_get_nle_objects (GESObject *object)
{
  GESObjectClass *klass = GES_OBJECT_GET_CLASS (object);

  if (klass->get_nle_objects)
    return klass->get_nle_objects (object);

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

/**
 * ges_object_get_interpolation_control_source:
 * @object: a #GESObject
 * @property_name: The name of the property to control, see the documentation
 * of #GstChildProxy for more information on the naming scheme.
 * @binding_type: The #GType of #GstControlBinding to create, G_TYPE_NONE will
 * create a direct control binding.
 *
 * Get a #GstControlSource to control the property specified with @name, if
 * it one already exists it will be returned.
 *
 * Returns: a #GstControlSource if one existed or could be created.
 */
GstControlSource *
ges_object_get_interpolation_control_source (GESObject * self,
    const gchar * property_name, GType binding_type)
{
  GParamSpec *pspec = NULL;
  GstControlSource *source = NULL;
  GstControlBinding *binding;
  GObject *object = _lookup_element_for_property (self, property_name, &pspec);
  gchar *base_property_name = _get_base_property_name (property_name);
  GESObjectPrivate *priv = GES_OBJECT_PRIV (self);

  if (!object)
    goto beach;

  if (!GST_IS_OBJECT (object)) {
    GST_ERROR_OBJECT (self, "can't control a %s as it is not a GstObject", G_OBJECT_TYPE_NAME (object));
    goto beach;
  }

  if ((binding = gst_object_get_control_binding (GST_OBJECT (object), base_property_name))) {
    g_object_unref (object);
    g_object_get (binding, "control-source", &source, NULL);
    g_object_unref (binding);
    goto beach;
  }

  if (binding_type != G_TYPE_NONE && binding_type != GST_TYPE_DIRECT_CONTROL_BINDING) {
    GST_ERROR_OBJECT (self, "We only support direct control bindings for now");
    g_object_unref (object);
    goto beach;
  }

  source = gst_interpolation_control_source_new ();
  binding = gst_direct_control_binding_new (GST_OBJECT (object), base_property_name, source);

  if (!binding) {
    g_object_unref (source);
    g_object_unref (object);
    source = NULL;
    goto beach;
  }

  gst_object_add_control_binding (GST_OBJECT (object), binding);
  priv->control_sources = g_list_append (priv->control_sources, source);
  g_object_unref (object);

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
_dispose (GObject *object)
{
  GESObjectPrivate *priv = GES_OBJECT_PRIV (object);

  g_list_free (priv->children);
  g_list_free_full (priv->control_sources, g_object_unref);
}

static void
ges_object_class_init (GESObjectClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->dispose = _dispose;

  /**
   * GESObject:inpoint:
   *
   * Given a media, the point in media-time in nanoseconds at which the object
   * will start outputting data from that media. Do not confuse with #GESObject:start
   */
  properties[PROP_INPOINT] = 
      g_param_spec_uint64 ("inpoint", "Inpoint", "The inpoint of the object", 0, G_MAXUINT64,
          0, G_PARAM_READWRITE);

  /**
   * GESObject:duration:
   *
   * Given a media, the amount of data in nanoseconds the object will output from it.
   */
  properties[PROP_DURATION] =
      g_param_spec_uint64 ("duration", "Duration", "The duration of the object", 0, G_MAXUINT64,
          0, G_PARAM_READWRITE);

  /**
   * GESObject:start:
   *
   * This property has no effect when the object is not in a #GESTimeline.
   * When in a timeline, it represents the time in nanoseconds at which it will start
   * outputting data. For example if the object is put in an empty audio timeline
   * and set a start of 5 seconds, when playing the timeline silence will be
   * output for 5 seconds, then the contents of the object.
   */
  properties[PROP_START] =
      g_param_spec_uint64 ("start", "Start", "The start of the object", 0, G_MAXUINT64,
          0, G_PARAM_READWRITE);

  /**
   * GESObject:media-type:
   *
   * The type of media contained in the #GESObject, it can be a single
   * #GESMediaType, or a combination of #GESMediaType.
   */
  properties[PROP_MEDIA_TYPE] =
      g_param_spec_flags ("media-type", "Media Type", "The GESMediaType of the object",
        GES_TYPE_MEDIA_TYPE, GES_MEDIA_TYPE_UNKNOWN, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  /**
   * GESObject:video-track-index:
   *
   * This property has no effect when the object is not in a #GESTimeline.
   * When in a timeline, it is used to determine the video track to which the
   * object belongs, that is the order in which it will be mixed with other,
   * overlapping objects belonging to different tracks.
   *
   * For example if two video objects with the same duration, start and size
   * are put in a video timeline, only the object with the *lowest* track-index
   * will be visible when playing that timeline.
   *
   * This property will also be used to determine if transitions have to be
   * created between two video objects.
   */
  properties[PROP_VIDEO_TRACK_INDEX] =
      g_param_spec_uint ("video-track-index", "Video track index",
        "The index of the containing video track", 0, G_MAXUINT,
          0, G_PARAM_READWRITE);

  /**
   * GESObject:audio-track-index:
   *
   * This property has no effect when the object is not in a #GESTimeline.
   *
   * When in a timeline, it is used to determine the video track to which the
   * object belongs, in order to determine if transitions have to be created
   * between two audio objects.
   */
  properties[PROP_AUDIO_TRACK_INDEX] =
      g_param_spec_uint ("audio-track-index", "Audio track index",
        "The index of the containing audio track", 0, G_MAXUINT,
          0, G_PARAM_READWRITE);

  g_object_class_install_properties (g_object_class, PROP_LAST, properties);

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
  priv->control_sources = NULL;
}
