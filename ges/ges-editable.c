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

GList *
ges_editable_get_nle_objects (GESEditable *editable)
{
  GESEditableInterface *iface = GES_EDITABLE_GET_IFACE (editable);

  if (iface->get_nle_objects)
    return iface->get_nle_objects (editable);

  return NULL;
}
