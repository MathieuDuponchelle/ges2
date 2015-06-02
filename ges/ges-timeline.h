#ifndef _GES_TIMELINE
#define _GES_TIMELINE

#include <gst/gst.h>
#include <ges-object.h>

G_BEGIN_DECLS

#define GES_TYPE_TIMELINE (ges_timeline_get_type ())

#define TIMELINE_PRIORITY_OFFSET 2
#define TRACK_PRIORITY_HEIGHT 1000

G_DECLARE_FINAL_TYPE(GESTimeline, ges_timeline, GES, TIMELINE, GESObject)

GESTimeline *ges_timeline_new (GESMediaType media_type);
gboolean ges_timeline_add_object (GESTimeline *self, GESObject *object);
gboolean ges_timeline_commit (GESTimeline *timeline);
GList *ges_timeline_get_compositions_by_media_type (GESTimeline *timeline, GESMediaType media_type);

G_END_DECLS

#endif
