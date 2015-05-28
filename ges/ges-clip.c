#include "ges-timeline.h"
#include "ges-clip.h"
#include "ges-editable.h"
#include "ges-playable.h"

/* Structure definitions */

typedef struct _GESClipPrivate
{
  gchar *uri;
  GESMediaType media_type;
  GstElement *nleobject;
  GstObject *old_parent;
  GstPad *ghostpad;
  GstPad *static_sinkpad;
} GESClipPrivate;

struct _GESClip
{
  GstBin parent;
  GESClipPrivate *priv;
};

static void ges_editable_interface_init (GESEditableInterface * iface);
static void ges_playable_interface_init (GESPlayableInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESClip, ges_clip, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EDITABLE, ges_editable_interface_init)
    G_IMPLEMENT_INTERFACE (GES_TYPE_PLAYABLE, ges_playable_interface_init))

/* API */

GESMediaType
ges_clip_get_media_type (GESClip *self)
{
  return self->priv->media_type;
}

void
ges_clip_set_uri (GESClip *self, const gchar *uri)
{
  self->priv->uri = g_strdup (uri);
}

GESClip *
ges_clip_new (const gchar *uri, GESMediaType media_type)
{
  return g_object_new (GES_TYPE_CLIP, "uri", uri, "media-type", media_type, NULL);
}

GstElement *
ges_clip_get_nleobject (GESClip *self)
{
  return self->priv->nleobject;
}

/* Implementation */

static void
_pad_added_cb (GstElement *element, GstPad *srcpad, GESClip *self)
{
  gst_element_no_more_pads (element);
  gst_pad_link (srcpad, self->priv->static_sinkpad);
}

static void
_make_nle_object (GESClip *self)
{
  GstElement *decodebin, *rate, *converter;
  GstElement *topbin;
  GstPad *srcpad, *ghost;

  topbin = gst_bin_new (NULL);
  decodebin = gst_element_factory_make ("uridecodebin", NULL);

  self->priv->nleobject = gst_element_factory_make ("nlesource", NULL);

  if (self->priv->media_type == GES_MEDIA_TYPE_VIDEO) {
    converter = gst_element_factory_make ("videoconvert", NULL);
    rate = gst_element_factory_make ("videorate", NULL);
    gst_bin_add_many (GST_BIN(topbin), decodebin, converter, rate, NULL); 
    gst_element_link (converter, rate);
    g_object_set (self->priv->nleobject, "caps", gst_caps_from_string(GES_RAW_VIDEO_CAPS), NULL);
  } else {
    converter = gst_element_factory_make ("audioconvert", NULL);
    rate = gst_element_factory_make ("audioresample", NULL);
    gst_bin_add_many (GST_BIN(topbin), decodebin, converter, rate, NULL); 
    gst_element_link (converter, rate);
    g_object_set (self->priv->nleobject, "caps", gst_caps_from_string(GES_RAW_AUDIO_CAPS), NULL);
  }

  self->priv->static_sinkpad = gst_element_get_static_pad (converter, "sink");
  g_signal_connect (decodebin, "pad-added",
      G_CALLBACK (_pad_added_cb),
      self);

  srcpad = gst_element_get_static_pad (rate, "src");
  ghost = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (ghost, TRUE);
  gst_element_add_pad (topbin, ghost);

  gst_object_unref (srcpad);

  if (self->priv->media_type == GES_MEDIA_TYPE_VIDEO)
    g_object_set (decodebin, "caps", gst_caps_from_string (GES_RAW_VIDEO_CAPS),
        "expose-all-streams", FALSE, "uri", self->priv->uri, NULL);
  else
    g_object_set (decodebin, "caps", gst_caps_from_string (GES_RAW_AUDIO_CAPS),
        "expose-all-streams", FALSE, "uri", self->priv->uri, NULL);

  if (!gst_bin_add (GST_BIN (self->priv->nleobject), topbin))
    return;
}

static void
_expose_nle_object (GESClip *self)
{
  GstPad *pad;

  g_object_get (self->priv->nleobject, "composition", &self->priv->old_parent, NULL);

  if (self->priv->old_parent) {
    gst_object_ref (self->priv->nleobject);
    gst_bin_remove (GST_BIN (self->priv->old_parent), self->priv->nleobject);
  }

  gst_bin_add (GST_BIN (self), self->priv->nleobject);

  pad = gst_element_get_static_pad (self->priv->nleobject, "src");

  gst_ghost_pad_set_target (GST_GHOST_PAD (self->priv->ghostpad), pad);
  gst_pad_set_active (self->priv->ghostpad, TRUE);
}

static void
_unexpose_nle_object (GESClip *self)
{
  gst_ghost_pad_set_target (GST_GHOST_PAD (self->priv->ghostpad), NULL);
    gst_pad_set_active (GST_PAD (self->priv->ghostpad), FALSE);

  if (self->priv->old_parent) {
    gst_object_ref (self->priv->nleobject);
    gst_bin_remove (GST_BIN (self), self->priv->nleobject);
    gst_bin_add (GST_BIN (self->priv->old_parent), self->priv->nleobject);
    gst_object_unref (self->priv->old_parent);
    self->priv->old_parent = NULL;
  }
}

/* Interface implementation */

/* GESPlayable */

static gboolean
_make_playable (GESPlayable *playable, gboolean is_playable)
{
  GESClip *self = GES_CLIP (playable);

  if (is_playable)
    _expose_nle_object (self);
  else
    _unexpose_nle_object (self);

  return TRUE;
}

static void
ges_playable_interface_init (GESPlayableInterface * iface)
{
  iface->make_playable = _make_playable;
}

/* GESEditable */

static gboolean
_set_inpoint (GESEditable *editable, GstClockTime inpoint)
{
  GESClip *clip = GES_CLIP (editable);

  g_object_set (clip->priv->nleobject, "inpoint", inpoint, NULL);

  return TRUE;
}

static gboolean
_set_duration (GESEditable *editable, GstClockTime duration)
{
  GESClip *clip = GES_CLIP (editable);

  g_object_set (clip->priv->nleobject, "duration", duration, NULL);

  return TRUE;
}

static gboolean
_set_start (GESEditable *editable, GstClockTime start)
{
  GESClip *clip = GES_CLIP (editable);

  g_object_set (clip->priv->nleobject, "start", start, NULL);

  return TRUE;
}

static GList *
_get_nle_objects (GESEditable *editable)
{
  return g_list_append (NULL, GES_CLIP (editable)->priv->nleobject);
}

static gboolean
_set_track_index (GESEditable *editable, GESMediaType media_type, guint index)
{
  GESClip *self = GES_CLIP (editable);
  guint new_priority;

  if (!(media_type & self->priv->media_type))
      return FALSE;

  new_priority = (TRACK_PRIORITY_HEIGHT * index) + TIMELINE_PRIORITY_OFFSET;

  g_object_set (self->priv->nleobject, "priority", new_priority, NULL);

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

/* GObject initialization */

enum
{
  PROP_0,
  PROP_URI,
  PROP_MEDIA_TYPE,
};

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESClip *self = GES_CLIP (object);

  switch (property_id) {
    case PROP_URI:
      ges_clip_set_uri (self, g_value_get_string (value));
      break;
    case PROP_MEDIA_TYPE:
      self->priv->media_type = g_value_get_flags (value);
      _make_nle_object (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESClipPrivate *priv = GES_CLIP (object)->priv;

  switch (property_id) {
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;
    case PROP_MEDIA_TYPE:
      g_value_set_flags (value, priv->media_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_clip_class_init (GESClipClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESClipPrivate));

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;

  g_object_class_install_property (g_object_class, PROP_URI,
      g_param_spec_string ("uri", "URI", "uri of the resource", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_override_property (g_object_class, PROP_MEDIA_TYPE, "media-type");
}

static void
ges_clip_init (GESClip *self)
{
  gchar *padname;
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_CLIP, GESClipPrivate);

  self->priv->old_parent = NULL;
  padname = g_strdup_printf ("clip_%p_src", self->priv->nleobject);
  self->priv->ghostpad = gst_ghost_pad_new_no_target (padname, GST_PAD_SRC);
  g_free (padname);
  gst_element_add_pad (GST_ELEMENT (self), self->priv->ghostpad);
}
