#ifndef _GES_CLIP
#define _GES_CLIP

#include <gst/gst.h>
#include <ges-object.h>
#include <ges-transition.h>

G_BEGIN_DECLS

#define GES_TYPE_CLIP (ges_clip_get_type ())

G_DECLARE_FINAL_TYPE(GESClip, ges_clip, GES, CLIP, GESObject)

GESClip *ges_clip_new (const gchar *uri, GESMediaType media_type);
gboolean ges_clip_set_transition (GESClip *clip, GESTransition *transition);
GESTransition *ges_clip_get_transition (GESClip *clip);

G_END_DECLS

#endif
