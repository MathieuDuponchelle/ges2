#ifndef _GES_TIMELINE
#define _GES_TIMELINE

#include <glib-object.h>
#include <gst/gst.h>

#include <ges.h>

#include <ges-clip.h>

G_BEGIN_DECLS

#define GES_TYPE_TIMELINE (ges_timeline_get_type ())

G_DECLARE_FINAL_TYPE(GESTimeline, ges_timeline, GES, TIMELINE, GstBin)

GESTimeline *ges_timeline_new (void);
gboolean ges_timeline_add_clip (GESTimeline *self, GESClip *clip);
gboolean ges_timeline_commit (GESTimeline *timeline);
GList *ges_timeline_get_compositions_by_media_type (GESTimeline *timeline, GESMediaType media_type);

G_END_DECLS

#endif
