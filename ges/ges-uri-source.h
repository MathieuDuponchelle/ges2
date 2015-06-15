#ifndef _GES_URI_SOURCE
#define _GES_URI_SOURCE

#include <gst/gst.h>
#include <ges-source.h>

G_BEGIN_DECLS

#define GES_TYPE_URI_SOURCE (ges_uri_source_get_type ())

G_DECLARE_FINAL_TYPE(GESUriSource, ges_uri_source, GES, URI_SOURCE, GESSource)

GESSource *ges_uri_source_new (const gchar *uri, GESMediaType media_type);
void ges_uri_source_set_uri (GESUriSource *self, const gchar *uri);

G_END_DECLS

#endif
