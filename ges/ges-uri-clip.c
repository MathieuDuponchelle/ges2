#include "ges-timeline.h"
#include "ges-uri-clip.h"

/* Structure definitions */

#define GES_URI_CLIP_PRIV(self) (ges_uri_clip_get_instance_private (GES_URI_CLIP (self)))

typedef struct _GESUriClipPrivate
{
  gchar *uri;
} GESUriClipPrivate;

struct _GESUriClip
{
  GESClip parent;
};

G_DEFINE_TYPE_WITH_CODE (GESUriClip, ges_uri_clip, GES_TYPE_CLIP,
    G_ADD_PRIVATE (GESUriClip)
    )

/* Implementation */

void
ges_uri_clip_set_uri (GESUriClip *self, const gchar *uri)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (self);

  if (priv->uri) {
    GST_ERROR_OBJECT (self, "changing uri is not supported yet");
  }

  priv->uri = g_strdup (uri);
}

GESClip *
ges_uri_clip_new (const gchar *uri, GESMediaType media_type)
{
  GESClip *res = g_object_new (GES_TYPE_URI_CLIP, "uri", uri, "media-type", media_type, NULL);

  return res;
}

static GstElement *
_make_element (GESClip *clip)
{
  GESMediaType media_type;
  GstElement *decodebin;
  GstCaps *caps;
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (clip);
  g_object_get (clip, "media-type", &media_type, NULL);

  if (media_type == GES_MEDIA_TYPE_VIDEO)
    caps = gst_caps_from_string (GES_RAW_VIDEO_CAPS);
  else if (media_type == GES_MEDIA_TYPE_AUDIO)
    caps = gst_caps_from_string (GES_RAW_AUDIO_CAPS);
  else {
    GST_ERROR_OBJECT (clip, "media type not supported : %d", media_type);
    return NULL;
  }

  decodebin = gst_element_factory_make ("uridecodebin", NULL);
  g_object_set (decodebin, "caps", caps,
      "expose-all-streams", FALSE, "uri", priv->uri, NULL);
  gst_caps_unref (caps);

  return decodebin;
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
  GESUriClip *self = GES_URI_CLIP (object);

  switch (property_id) {
    case PROP_URI:
      ges_uri_clip_set_uri (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (object);

  switch (property_id) {
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (object);

  if (priv->uri)
    g_free (priv->uri);

  G_OBJECT_CLASS (ges_uri_clip_parent_class)->dispose (object);
}

static void
ges_uri_clip_class_init (GESUriClipClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GESClipClass *clip_class = GES_CLIP_CLASS (klass);

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->dispose = _dispose;

  g_object_class_install_property (g_object_class, PROP_URI,
      g_param_spec_string ("uri", "URI", "uri of the resource", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  clip_class->make_element = _make_element;
}

static void
ges_uri_clip_init (GESUriClip *self)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (self);

  priv->uri = NULL;
}
