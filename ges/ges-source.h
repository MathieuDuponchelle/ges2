#ifndef _GES_SOURCE
#define _GES_SOURCE

#include <gst/gst.h>
#include <ges-object.h>
#include <ges-transition.h>

G_BEGIN_DECLS

#define GES_TYPE_SOURCE (ges_source_get_type ())

G_DECLARE_DERIVABLE_TYPE(GESSource, ges_source, GES, SOURCE, GESObject)

struct _GESSourceClass
{
  GESObjectClass parent;
  GstElement *(*make_element) (GESSource *source);
};

gboolean ges_source_set_transition (GESSource *source, GESTransition *transition);
GESTransition *ges_source_get_transition (GESSource *source);

G_END_DECLS

#endif
