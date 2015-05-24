#include <stdio.h>
#include <gst/gst.h>
#include <gst/player/gstplayer.h>

#include "nle.h"

#include "ges-clip.h"

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
setup (void) {
  guint i = 0;

  gst_init (NULL, NULL);

  for (; _elements[i].name; i++)
    if (!(gst_element_register (NULL,
                _elements[i].name, GST_RANK_NONE, (_elements[i].type) ())))
      return FALSE;

  nle_init_ghostpad_category ();

  return TRUE;
}

int main (int ac, char **av)
{
  if (!setup())
    return 1;

  gchar *uri;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  GstPlayer *player = gst_player_new();
  GESClip *clip = ges_clip_new ("file:///home/meh/Videos/homeland.mp4");
  nle_object_commit (NLE_OBJECT(ges_clip_get_nleobject (clip)), TRUE);

  uri = g_strdup_printf ("ges:///%p\n", (void *) clip);
  gst_player_set_uri (player, uri);
  gst_player_play (player);

  g_main_loop_run (loop);

  g_object_unref (clip);
}
