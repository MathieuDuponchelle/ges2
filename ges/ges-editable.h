#ifndef _GES_EDITABLE
#define _GES_EDITABLE

#include <glib-object.h>
#include <gst/gst.h>
#include <ges.h>

G_BEGIN_DECLS

#define GES_TYPE_EDITABLE ges_editable_get_type ()

G_DECLARE_INTERFACE (GESEditable, ges_editable, GES, EDITABLE, GObject)

struct _GESEditableInterface
{
  GTypeInterface g_iface;

  gboolean     (*set_inpoint) (GESEditable *self, GstClockTime inpoint);
  gboolean     (*set_duration) (GESEditable *self, GstClockTime duration);
  gboolean     (*set_start) (GESEditable *self, GstClockTime start);
  GstClockTime (*get_inpoint) (GESEditable *self);
  GstClockTime (*get_duration) (GESEditable *self);
  GstClockTime (*get_start) (GESEditable *self);
  guint        (*get_track_index) (GESEditable *self, GESMediaType media_type);
  GList *      (*get_nle_objects) (GESEditable *self);
  gboolean     (*set_track_index) (GESEditable *self, GESMediaType media_type, guint index);

  GESMediaType media_type;
};

gboolean ges_editable_set_inpoint (GESEditable *editable, GstClockTime inpoint);
gboolean ges_editable_set_duration (GESEditable *editable, GstClockTime duration);
gboolean ges_editable_set_start (GESEditable *editable, GstClockTime start);

GstClockTime ges_editable_get_inpoint (GESEditable *editable);
GstClockTime ges_editable_get_duration (GESEditable *editable);
GstClockTime ges_editable_get_start (GESEditable *editable);

guint ges_editable_get_track_index (GESEditable *editable, GESMediaType media_type);

GList *  ges_editable_get_nle_objects (GESEditable *editable);
gboolean ges_editable_set_track_index (GESEditable *editable, GESMediaType media_type, guint index);

G_END_DECLS

#endif
