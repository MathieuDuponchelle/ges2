#include "ges-playable.h"

G_DEFINE_INTERFACE (GESPlayable, ges_playable,
    G_TYPE_OBJECT);

static void
ges_playable_default_init (GESPlayableInterface * iface)
{
  iface->make_playable = NULL;
}

GstPlayer *
ges_playable_make_player (GESPlayable * playable)
{
  GstPlayer *player = gst_player_new();
  gchar *uri;

  uri = g_strdup_printf ("ges:///%p\n", (void *) playable);
  gst_player_set_uri (player, uri);

  g_free (uri);
  return player;
}

GstBin *
ges_playable_make_playable (GESPlayable *playable, gboolean is_playable)
{
  GESPlayableInterface *iface = GES_PLAYABLE_GET_IFACE (playable);

  if (!iface->make_playable)
    return NULL;

  return iface->make_playable (playable, is_playable);
}
