#include "ges.h"
#include "nle.h"
  
struct _elements_entry
{
  const gchar *name;
  GType (*type) (void);
};

static struct _elements_entry _elements[] = {
  {"nlesource", nle_source_get_type},
  {"nlecomposition", nle_composition_get_type},
  {"nleoperation", nle_operation_get_type},
  {"nleurisource", nle_urisource_get_type},
  {NULL, 0}
};

gboolean
ges_init (void)
{
  guint i = 0;

  gst_init (NULL, NULL);

  for (; _elements[i].name; i++)
    if (!(gst_element_register (NULL,
                _elements[i].name, GST_RANK_NONE, (_elements[i].type) ())))
      return FALSE;

  nle_init_ghostpad_category ();

  return TRUE;
}
