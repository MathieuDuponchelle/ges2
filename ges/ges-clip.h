#ifndef _GES_CLIP
#define _GES_CLIP

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GES_TYPE_CLIP (ges_clip_get_type ())

G_DECLARE_FINAL_TYPE(GESClip, ges_clip, GES, CLIP, GstElement)

GESClip *ges_clip_new (const gchar *uri);
GstElement * ges_clip_get_nleobject (GESClip *self);

G_END_DECLS

#endif
