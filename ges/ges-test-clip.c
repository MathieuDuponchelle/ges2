#include "ges-test-clip.h"

/* Structure definitions */

#define GES_TEST_CLIP_PRIV(self) (ges_test_clip_get_instance_private (GES_TEST_CLIP (self)))

typedef struct _GESTestClipPrivate
{
  gchar *pattern;
} GESTestClipPrivate;

struct _GESTestClip
{
  GESClip parent;
};

G_DEFINE_TYPE_WITH_CODE (GESTestClip, ges_test_clip, GES_TYPE_CLIP,
    G_ADD_PRIVATE (GESTestClip)
    )

/* Implementation */

static void
ges_test_clip_set_pattern (GESTestClip *self, const gchar *pattern)
{
  GESTestClipPrivate *priv = GES_TEST_CLIP_PRIV (self);

  priv->pattern = g_strdup (pattern);
}

GESClip *
ges_test_clip_new (GESMediaType media_type, const gchar *pattern)
{
  GESClip *res = g_object_new (GES_TYPE_TEST_CLIP, "pattern", pattern, "media-type", media_type, NULL);

  return res;
}

static GstElement *
_make_element (GESClip *clip)
{
  GESMediaType media_type;
  GstElement *testsrc;
  g_object_get (clip, "media-type", &media_type, NULL);

  if (media_type == GES_MEDIA_TYPE_VIDEO) {
    testsrc = gst_element_factory_make ("videotestsrc", NULL);
    g_object_set (testsrc, "pattern", 0, NULL);
  } else if (media_type == GES_MEDIA_TYPE_AUDIO)
    testsrc = gst_element_factory_make ("audiotestsrc", NULL);
  else {
    GST_ERROR_OBJECT (clip, "media type not supported : %d", media_type);
    return NULL;
  }

  return testsrc;
}

/* GObject initialization */

enum
{
  PROP_0,
  PROP_PATTERN,
};

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESTestClip *self = GES_TEST_CLIP (object);

  switch (property_id) {
    case PROP_PATTERN:
      ges_test_clip_set_pattern (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESTestClipPrivate *priv = GES_TEST_CLIP_PRIV (object);

  switch (property_id) {
    case PROP_PATTERN:
      g_value_set_string (value, priv->pattern);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  GESTestClipPrivate *priv = GES_TEST_CLIP_PRIV (object);

  if (priv->pattern)
    g_free (priv->pattern);

  G_OBJECT_CLASS (ges_test_clip_parent_class)->dispose (object);
}

static void
ges_test_clip_class_init (GESTestClipClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GESClipClass *clip_class = GES_CLIP_CLASS (klass);

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->dispose = _dispose;

  g_object_class_install_property (g_object_class, PROP_PATTERN,
      g_param_spec_string ("pattern", "Pattern", "pattern of the resource", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  clip_class->make_element = _make_element;
}

static void
ges_test_clip_init (GESTestClip *self)
{
  GESTestClipPrivate *priv = GES_TEST_CLIP_PRIV (self);

  priv->pattern = NULL;
}
