#ifndef _GES_TEST_SOURCE
#define _GES_TEST_SOURCE

#include <gst/gst.h>
#include <ges-source.h>

G_BEGIN_DECLS

#define GES_TYPE_TEST_SOURCE (ges_test_source_get_type ())

G_DECLARE_FINAL_TYPE(GESTestSource, ges_test_source, GES, TEST_SOURCE, GESSource)

GESSource *ges_test_source_new (GESMediaType media_type, const gchar *pattern);

G_END_DECLS

#endif
