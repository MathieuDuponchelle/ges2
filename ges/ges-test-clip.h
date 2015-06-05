#ifndef _GES_TEST_CLIP
#define _GES_TEST_CLIP

#include <gst/gst.h>
#include <ges-clip.h>

G_BEGIN_DECLS

#define GES_TYPE_TEST_CLIP (ges_test_clip_get_type ())

G_DECLARE_FINAL_TYPE(GESTestClip, ges_test_clip, GES, TEST_CLIP, GESClip)

GESClip *ges_test_clip_new (GESMediaType media_type, const gchar *pattern);

G_END_DECLS

#endif
