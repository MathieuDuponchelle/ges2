#ifndef GES_INTERNAL
#define GES_INTERNAL

#include <ges-object.h>

GList *      ges_object_get_nle_objects (GESObject *object);

#define GET_FROM_TUPLE(v, t, n, val) G_STMT_START{         \
  GVariant *child = g_variant_get_child_value (v, n); \
  *val = g_variant_get_##t(child); \
  g_variant_unref (child); \
}G_STMT_END

#define GET_STRING_FROM_TUPLE(v, n, val) G_STMT_START{         \
  GVariant *child = g_variant_get_child_value (v, n); \
  *val = g_variant_get_string(child, NULL); \
  g_variant_unref (child); \
}G_STMT_END

const gchar *
_maybe_get_string_from_tuple (GVariant * tuple, guint index);

#endif
