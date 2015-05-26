#include "ges-editable.h"

G_DEFINE_INTERFACE (GESEditable, ges_editable,
    G_TYPE_OBJECT);

static void
ges_editable_default_init (GESEditableInterface * iface)
{
  iface->set_inpoint = NULL;
  iface->set_duration = NULL;
  iface->set_start = NULL;
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
