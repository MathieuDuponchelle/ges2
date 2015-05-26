#ifndef _GES_EDITABLE
#define _GES_EDITABLE

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GES_TYPE_EDITABLE ges_editable_get_type ()

G_DECLARE_INTERFACE (GESEditable, ges_editable, GES, EDITABLE, GObject)

struct _GESEditableInterface
{
  GTypeInterface g_iface;

  gboolean (*set_inpoint) (GESEditable *self, GstClockTime inpoint);
  gboolean (*set_duration) (GESEditable *self, GstClockTime duration);
  gboolean (*set_start) (GESEditable *self, GstClockTime start);
};

gboolean ges_editable_set_inpoint (GESEditable *editable, GstClockTime inpoint);
gboolean ges_editable_set_duration (GESEditable *editable, GstClockTime duration);
gboolean ges_editable_set_start (GESEditable *editable, GstClockTime start);

G_END_DECLS

#endif
