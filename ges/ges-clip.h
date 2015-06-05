#ifndef _GES_CLIP
#define _GES_CLIP

#include <gst/gst.h>
#include <ges-object.h>
#include <ges-transition.h>

G_BEGIN_DECLS

#define GES_TYPE_CLIP (ges_clip_get_type ())

G_DECLARE_DERIVABLE_TYPE(GESClip, ges_clip, GES, CLIP, GESObject)

struct _GESClipClass
{
  GESObjectClass parent;
  GstElement *(*make_element) (GESClip *clip);
};

gboolean ges_clip_set_transition (GESClip *clip, GESTransition *transition);
GESTransition *ges_clip_get_transition (GESClip *clip);

G_END_DECLS

#endif
