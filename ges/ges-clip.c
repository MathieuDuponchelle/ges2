#include "ges-clip.h"

/* Structure definitions */

typedef struct _GESClipPrivate
{
  gchar *uri;
  GstElement *nleobject;
} GESClipPrivate;

struct _GESClip
{
  GstElement parent;
  GESClipPrivate *priv;
};

G_DEFINE_TYPE (GESClip, ges_clip, GST_TYPE_ELEMENT)

/* API */

void
ges_clip_set_uri (GESClip *self, const gchar *uri)
{
  self->priv->uri = g_strdup (uri);
}

GESClip *
ges_clip_new (const gchar *uri)
{
  return g_object_new (GES_TYPE_CLIP, "uri", uri, NULL);
}

GstElement *
ges_clip_get_nleobject (GESClip *self)
{
  return self->priv->nleobject;
}

/* GObject initialization */

enum
{
  PROP_0,
  PROP_URI,
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
  GstElement *decodebin, *videorate;
  GstElement *topbin;
  GstPad *srcpad, *ghost;

  topbin = gst_bin_new ("mybin");
  videorate = gst_element_factory_make ("videorate", NULL);
  decodebin = gst_element_factory_make ("uridecodebin", NULL);
  gst_bin_add_many (GST_BIN(topbin), decodebin, videorate, NULL); 

  g_signal_connect (decodebin, "pad-added",
      G_CALLBACK (_pad_added_cb),
      gst_element_get_static_pad (videorate, "sink"));

  srcpad = gst_element_get_static_pad (videorate, "src");
  ghost = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (ghost, TRUE);
  gst_element_add_pad (topbin, ghost);

  gst_object_unref (srcpad);

  self->priv->nleobject = gst_element_factory_make ("nlesource", NULL);

  g_object_set (decodebin, "caps", gst_caps_from_string ("video/x-raw"),
      "expose-all-streams", FALSE, "uri", self->priv->uri, NULL);

  if (!gst_bin_add (GST_BIN (self->priv->nleobject), topbin))
    return;

  g_object_set (self->priv->nleobject, "duration", 2 * GST_SECOND, "inpoint", 50 * GST_SECOND, NULL);
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
}

static void
ges_clip_init (GESClip *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_CLIP, GESClipPrivate);
}
