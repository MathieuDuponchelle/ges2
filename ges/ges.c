#include <grilo.h>
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

static void
configure_plugins (void)
{
  GrlConfig *config;
  GrlRegistry *registry;

  config = grl_config_new ("grl-youtube", NULL);
  /* FIXME maybe get one for us :) */
  grl_config_set_api_key (config, "AIzaSyCENhl8yDxDZbyhTF6p-ok-RefK07xdXUg");
  registry = grl_registry_get_default ();
  grl_registry_add_config (registry, config, NULL);
}

static void
initialize_grilo (void)
{
  GError *error = NULL;
  GrlRegistry *registry;

  grl_init (NULL, NULL);
  registry = grl_registry_get_default ();
  configure_plugins ();

  grl_registry_load_all_plugins (registry, &error);
  g_assert_no_error (error);
}


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

  initialize_grilo ();
  return TRUE;
}
