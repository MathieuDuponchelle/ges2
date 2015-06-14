#include <ges.h>

int main (int ac, char **av)
{
  GESClip *clip;

  if (ac != 3)
    return 1;

  ges_init();

  clip = ges_uri_clip_new (av[1], GES_MEDIA_TYPE_VIDEO);
  if (!ges_object_set_inpoint (GES_OBJECT (clip), g_ascii_strtoll (av[2], NULL, 0))) {
    GST_ERROR ("couldn't set inpoint !");
    return 1;
  }

  ges_playable_play (GES_PLAYABLE (clip));
  return 0;
}
