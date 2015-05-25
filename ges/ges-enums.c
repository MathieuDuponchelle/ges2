#include "ges-enums.h"

#define C_ENUM(v) ((guint) v)

static const GFlagsValue media_types_values[] = {
  {C_ENUM (GES_MEDIA_TYPE_UNKNOWN), "GES_MEDIA_TYPE_UNKNOWN", "unknown"},
  {C_ENUM (GES_MEDIA_TYPE_AUDIO), "GES_MEDIA_TYPE_AUDIO", "audio"},
  {C_ENUM (GES_MEDIA_TYPE_VIDEO), "GES_MEDIA_TYPE_VIDEO", "video"},
  {0, NULL, NULL}
};

static void
register_ges_media_type_select_result (GType * id)
{
  *id = g_flags_register_static ("GESMediaType", media_types_values);
}

const gchar *
ges_media_type_name (GESMediaType type)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (media_types_values); i++) {
    if (type == media_types_values[i].value)
      return media_types_values[i].value_nick;
  }

  return "Unknown (mixed?) ";
}

GType
ges_media_type_get_type (void)
{
  static GType id;
  static GOnce once = G_ONCE_INIT;

  g_once (&once, (GThreadFunc) register_ges_media_type_select_result, &id);
  return id;
}
