#include "ges-editable.h"

G_DEFINE_INTERFACE (GESEditable, ges_editable,
    G_TYPE_OBJECT);

static void
ges_editable_default_init (GESEditableInterface * iface)
{
  iface->set_inpoint = NULL;
  iface->set_duration = NULL;
  iface->set_start = NULL;
  iface->get_nle_objects = NULL;
  iface->set_track_index = NULL;
  iface->get_track_index = NULL;
  iface->get_inpoint = NULL;
  iface->get_duration = NULL;
  iface->get_start = NULL;

  g_object_interface_install_property (iface,
      g_param_spec_flags ("media-type",
        "Media type",
        "Media type",
        GES_TYPE_MEDIA_TYPE,
        GES_MEDIA_TYPE_UNKNOWN,
        G_PARAM_READWRITE));
}

gboolean
ges_editable_set_inpoint (GESEditable *editable, GstClockTime inpoint)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->set_inpoint)
    return iface->set_inpoint (editable, inpoint);

  return FALSE;
}
gboolean
ges_editable_set_duration (GESEditable *editable, GstClockTime duration)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->set_duration)
    return iface->set_duration (editable, duration);

  return FALSE;
}

gboolean
ges_editable_set_start (GESEditable *editable, GstClockTime start)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->set_start)
    return iface->set_start (editable, start);

  return FALSE;
}

GstClockTime
ges_editable_get_inpoint (GESEditable *editable)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->get_inpoint)
    return iface->get_inpoint (editable);

  return GST_CLOCK_TIME_NONE;
}

GstClockTime
ges_editable_get_start (GESEditable *editable)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->get_start)
    return iface->get_start (editable);

  return GST_CLOCK_TIME_NONE;
}

GstClockTime
ges_editable_get_duration (GESEditable *editable)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->get_duration)
    return iface->get_duration (editable);

  return GST_CLOCK_TIME_NONE;
}

GList *
ges_editable_get_nle_objects (GESEditable *editable)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->get_nle_objects)
    return iface->get_nle_objects (editable);

  return NULL;
}

gboolean
ges_editable_set_track_index (GESEditable *editable, GESMediaType media_type, guint index)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->set_track_index)
    return iface->set_track_index (editable, media_type, index);

  return FALSE;
}
