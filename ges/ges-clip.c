#include "ges-clip.h"
#include "ges-editable.h"

/* Structure definitions */

typedef struct _GESClipPrivate
{
  gchar *uri;
  GESMediaType media_type;
  GstElement *nleobject;
} GESClipPrivate;

struct _GESClip
{
  GstElement parent;
  GESClipPrivate *priv;
};

static void ges_editable_interface_init (GESEditableInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESClip, ges_clip, GST_TYPE_ELEMENT,
    G_IMPLEMENT_INTERFACE (GES_TYPE_EDITABLE, ges_editable_interface_init))

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

/* Interface implementation */

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

static void
ges_editable_interface_init (GESEditableInterface * iface)
{
  iface->set_inpoint = _set_inpoint;
  iface->set_duration = _set_duration;
  iface->set_start = _set_start;
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
_pad_added_cb (GstElement *element, GstPad *srcpad, GstPad *sinkpad)
{
  gst_element_no_more_pads (element);
  gst_pad_link (srcpad, sinkpad);
  gst_object_unref (sinkpad);
}

static void
_make_nle_object (GESClip *self)
{
  GstElement *decodebin, *rate, *converter;
  GstElement *topbin;
  GstPad *srcpad, *ghost;

  topbin = gst_bin_new (NULL);
  decodebin = gst_element_factory_make ("uridecodebin", NULL);

  if (self->priv->media_type == GES_MEDIA_TYPE_VIDEO) {
    converter = gst_element_factory_make ("videoconvert", NULL);
    rate = gst_element_factory_make ("videorate", NULL);
    gst_bin_add_many (GST_BIN(topbin), decodebin, converter, rate, NULL); 
    gst_element_link (converter, rate);
  } else {
    converter = gst_element_factory_make ("audioconvert", NULL);
    rate = gst_element_factory_make ("audioresample", NULL);
    gst_bin_add_many (GST_BIN(topbin), decodebin, converter, rate, NULL); 
    gst_element_link (converter, rate);
  }


  g_signal_connect (decodebin, "pad-added",
      G_CALLBACK (_pad_added_cb),
      gst_element_get_static_pad (converter, "sink"));


  srcpad = gst_element_get_static_pad (rate, "src");
  ghost = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (ghost, TRUE);
  gst_element_add_pad (topbin, ghost);

  gst_object_unref (srcpad);

  self->priv->nleobject = gst_element_factory_make ("nlesource", NULL);

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
_constructed (GObject *object)
{
  GESClip *clip = GES_CLIP (object);

  _make_nle_object (clip);
}

static void
ges_clip_class_init (GESClipClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESClipPrivate));

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->constructed = _constructed;

  g_object_class_install_property (g_object_class, PROP_URI,
      g_param_spec_string ("uri", "URI", "uri of the resource", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_MEDIA_TYPE,
      g_param_spec_flags ("media-type", "Media Type", "media type", GES_TYPE_MEDIA_TYPE, GES_MEDIA_TYPE_UNKNOWN,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ges_clip_init (GESClip *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_CLIP, GESClipPrivate);
}
