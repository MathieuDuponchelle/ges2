#ifndef _GES_TRANSITION
#define _GES_TRANSITION

#include <gst/gst.h>
#include <ges-object.h>

G_BEGIN_DECLS

#define GES_TYPE_TRANSITION (ges_transition_get_type ())

G_DECLARE_FINAL_TYPE(GESTransition, ges_transition, GES, TRANSITION, GObject)

GESTransition *
ges_transition_new (GESObject *fadeout_clip, GESObject *fadein_clip);
void ges_transition_reset (GESTransition *self);
void ges_transition_update (GESTransition *self);

G_END_DECLS

#endif
