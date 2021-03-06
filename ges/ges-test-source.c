#include "ges-test-source.h"
#include "ges-internal.h"

/* Structure definitions */

#define GES_TEST_SOURCE_PRIV(self) (ges_test_source_get_instance_private (GES_TEST_SOURCE (self)))

typedef struct _GESTestSourcePrivate
{
  gchar *pattern;
  GstElement *testsrc;
} GESTestSourcePrivate;

struct _GESTestSource
{
  GESSource parent;
};

G_DEFINE_TYPE_WITH_CODE (GESTestSource, ges_test_source, GES_TYPE_SOURCE,
    G_ADD_PRIVATE (GESTestSource)
    )

/* Implementation */

static void
ges_test_source_set_pattern (GESTestSource *self, const gchar *pattern)
{
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (self);

  priv->pattern = g_strdup (pattern);
}

/**
 * ges_test_source_new:
 * @media_type: The #GESMediaType of the test source to create
 * @pattern: (allow-none): The pattern or wave to produce, see gst-inspect-1.0
 * videotestsrc and gst-inspect-1.0 audiotestsrc for more info.
 *
 * Returns: A new #GESTestSource
 */
GESSource *
ges_test_source_new (GESMediaType media_type, const gchar *pattern)
{
  GESSource *res = g_object_new (GES_TYPE_TEST_SOURCE, "pattern", pattern, "media-type", media_type, NULL);

  return res;
}

static void
_set_pattern (GESTestSource *source)
{
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (source);
  gchar *pattern_enum_name = NULL;
  gchar *pattern_prop_name = NULL;
  GESMediaType media_type = ges_object_get_media_type (GES_OBJECT (source));
  GType pattern_type;
  GEnumClass *klass;
  GEnumValue *val;
  guint pattern = 0;

  if (!priv->testsrc)
    return;

  if (media_type == GES_MEDIA_TYPE_VIDEO) {
    pattern_enum_name = g_strdup ("GstVideoTestSrcPattern");
    pattern_prop_name = g_strdup ("pattern");
  } else if (media_type == GES_MEDIA_TYPE_AUDIO) {
    pattern_enum_name = g_strdup ("GstAudioTestSrcWave");
    pattern_prop_name = g_strdup ("wave");
  } else {
    GST_ERROR_OBJECT (source, "media type not supported : %d", media_type);
    return;
  }

  pattern_type = g_type_from_name (pattern_enum_name);
  klass = (GEnumClass *) g_type_class_ref (pattern_type);
  if (priv->pattern) {
    val = g_enum_get_value_by_nick (klass, priv->pattern);
    if (val)
      pattern = val->value;
  }

  g_object_set (priv->testsrc, pattern_prop_name, pattern, NULL);
  g_type_class_unref (klass);
  g_free (pattern_enum_name);
  g_free (pattern_prop_name);
}

static GstElement *
_make_element (GESSource *source)
{
  GESMediaType media_type;
  g_object_get (source, "media-type", &media_type, NULL);
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (source);

  if (media_type == GES_MEDIA_TYPE_VIDEO) {
    priv->testsrc = gst_element_factory_make ("videotestsrc", NULL);
  } else if (media_type == GES_MEDIA_TYPE_AUDIO) {
    priv->testsrc = gst_element_factory_make ("audiotestsrc", NULL);
  } else {
    GST_ERROR_OBJECT (source, "media type not supported : %d", media_type);
    return NULL;
  }

  _set_pattern (GES_TEST_SOURCE (source));
  return priv->testsrc;
}

static GVariant *
_serialize (GESObject *object)
{
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (object);

  return g_variant_new ("(ms)", priv->pattern);
}

static gboolean
_deserialize (GESObject *object, GVariant *variant)
{
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (object);

  priv->pattern = g_strdup (_maybe_get_string_from_tuple (variant, 0));
  _set_pattern (GES_TEST_SOURCE (object));

  return TRUE;
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
  GESTestSource *self = GES_TEST_SOURCE (object);

  switch (property_id) {
    case PROP_PATTERN:
      ges_test_source_set_pattern (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (object);

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
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (object);

  if (priv->pattern)
    g_free (priv->pattern);

  G_OBJECT_CLASS (ges_test_source_parent_class)->dispose (object);
}

static void
ges_test_source_class_init (GESTestSourceClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GESSourceClass *source_class = GES_SOURCE_CLASS (klass);
  GESObjectClass *ges_object_class = GES_OBJECT_CLASS (klass);

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->dispose = _dispose;

  g_object_class_install_property (g_object_class, PROP_PATTERN,
      g_param_spec_string ("pattern", "Pattern", "pattern of the resource", NULL,
          G_PARAM_READWRITE));

  source_class->make_element = _make_element;
  ges_object_class->serialize = _serialize;
  ges_object_class->deserialize = _deserialize;
}

static void
ges_test_source_init (GESTestSource *self)
{
  GESTestSourcePrivate *priv = GES_TEST_SOURCE_PRIV (self);

  priv->pattern = NULL;
}
