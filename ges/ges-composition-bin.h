#ifndef _GES_COMPOSITION_BIN
#define _GES_COMPOSITION_BIN

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GES_TYPE_COMPOSITION_BIN (ges_composition_bin_get_type ())

G_DECLARE_FINAL_TYPE(GESCompositionBin, ges_composition_bin, GES, COMPOSITION_BIN, GstBin)

GESCompositionBin *ges_composition_bin_new (void);
void ges_composition_bin_set_nle_objects (GESCompositionBin *self, GList *nleobjects);

G_END_DECLS

#endif
