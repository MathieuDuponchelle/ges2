#ifndef _GES_TRACK
#define _GES_TRACK

#include <glib-object.h>
#include <gst/gst.h>

#include <ges.h>

#include <ges-editable.h>

G_BEGIN_DECLS

#define GES_TYPE_TRACK (ges_track_get_type ())

G_DECLARE_FINAL_TYPE(GESTrack, ges_track, GES, TRACK, GObject)

gboolean ges_track_add_editable (GESTrack *track, GESEditable *editable);

G_END_DECLS

#endif
