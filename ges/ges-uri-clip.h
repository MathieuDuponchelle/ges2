#ifndef _GES_URI_CLIP
#define _GES_URI_CLIP

#include <gst/gst.h>
#include <ges-clip.h>

G_BEGIN_DECLS

#define GES_TYPE_URI_CLIP (ges_uri_clip_get_type ())

G_DECLARE_FINAL_TYPE(GESUriClip, ges_uri_clip, GES, URI_CLIP, GESClip)

GESClip *ges_uri_clip_new (const gchar *uri, GESMediaType media_type);
void ges_uri_clip_set_uri (GESUriClip *self, const gchar *uri);

G_END_DECLS

#endif
