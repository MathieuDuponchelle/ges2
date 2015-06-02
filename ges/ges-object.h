#ifndef _GES_OBJECT
#define _GES_OBJECT

#include <glib-object.h>
#include <gst/gst.h>
#include <ges-enums.h>

G_BEGIN_DECLS

#define GES_TYPE_OBJECT ges_object_get_type ()

G_DECLARE_DERIVABLE_TYPE (GESObject, ges_object, GES, OBJECT, GInitiallyUnowned)

struct _GESObjectClass
{
  GObjectClass parent_class;

  gboolean (*set_media_type)  (GESObject *object, GESMediaType media_type);
  gboolean (*set_inpoint)     (GESObject *object, GstClockTime inpoint);
  gboolean (*set_duration)    (GESObject *object, GstClockTime duration);
  gboolean (*set_start)       (GESObject *object, GstClockTime start);
  gboolean (*set_track_index) (GESObject *object, GESMediaType media_type, guint track_index);
  GList *  (*get_nle_objects) (GESObject *object);
};

GESMediaType ges_object_get_media_type  (GESObject *self);
GstClockTime ges_object_get_inpoint     (GESObject *self);
GstClockTime ges_object_get_duration    (GESObject *self);
GstClockTime ges_object_get_start       (GESObject *self);
guint        ges_object_get_track_index (GESObject *self, GESMediaType media_type);
GList *      ges_object_get_nle_objects (GESObject *self);

gboolean ges_object_set_media_type  (GESObject *self, GESMediaType media_type);
gboolean ges_object_set_inpoint     (GESObject *self, GstClockTime inpoint);
gboolean ges_object_set_duration    (GESObject *self, GstClockTime duration);
gboolean ges_object_set_start       (GESObject *self, GstClockTime start);
gboolean ges_object_set_track_index (GESObject *self, GESMediaType media_type, guint track_index);

GstControlSource *
ges_object_get_interpolation_control_source (GESObject * object,
    const gchar * property_name, GType binding_type);

G_END_DECLS

#endif
