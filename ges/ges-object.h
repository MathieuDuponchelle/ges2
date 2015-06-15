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

  /**
   * GESObjectClass::set_media_type:
   *
   * Implement this method if your subclass supports changing its media type
   * after construction.
   */
  gboolean (*set_media_type)  (GESObject *object, GESMediaType media_type);
  /**
   * GESObjectClass::set_inpoint:
   *
   * Implement this method if your subclass supports setting its inpoint
   */
  gboolean (*set_inpoint)     (GESObject *object, GstClockTime inpoint);
  /**
   * GESObjectClass::set_duration:
   *
   * Implement this method if your subclass supports changing its duration
   */
  gboolean (*set_duration)    (GESObject *object, GstClockTime duration);
  /**
   * GESObjectClass::set_start:
   *
   * Implement this method if your subclass supports changing its start
   */
  gboolean (*set_start)       (GESObject *object, GstClockTime start);
  /**
   * GESObjectClass::set_video_track_index:
   *
   * Implement this method if your subclass supports changing its video track index
   */
  gboolean (*set_track_index) (GESObject *object, GESMediaType media_type, guint track_index);

  /**
   * GESObjectClass::get_nle_objects:
   *
   * When defining a new subclass of #GESObject, you will need to implement this
   * method to return nle objects that can be put in a composition
   *
   * If you're solely interested in using existing #GESObjects, ignore this
   * virtual method
   */
  GList *  (*get_nle_objects) (GESObject *object);

  GVariant * (*serialize) (GESObject *object);
  gboolean   (*deserialize) (GESObject *object, GVariant *variant);
};

GESMediaType ges_object_get_media_type  (GESObject *object);
GstClockTime ges_object_get_inpoint     (GESObject *object);
GstClockTime ges_object_get_duration    (GESObject *object);
GstClockTime ges_object_get_start       (GESObject *object);
guint        ges_object_get_video_track_index (GESObject *object);
guint        ges_object_get_audio_track_index (GESObject *object);

gboolean ges_object_set_media_type  (GESObject *object, GESMediaType media_type);
gboolean ges_object_set_inpoint     (GESObject *object, GstClockTime inpoint);
gboolean ges_object_set_duration    (GESObject *object, GstClockTime duration);
gboolean ges_object_set_start       (GESObject *object, GstClockTime start);
gboolean ges_object_set_video_track_index (GESObject *object, guint track_index);
gboolean ges_object_set_audio_track_index (GESObject *object, guint track_index);

GVariant *ges_object_serialize (GESObject *object);
GESObject *ges_object_deserialize (GVariant *object_variant);

GstControlSource *
ges_object_get_interpolation_control_source (GESObject * object,
    const gchar * property_name, GType binding_type);

G_END_DECLS

#endif
