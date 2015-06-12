#include "ges-playable.h"

G_DEFINE_INTERFACE (GESPlayable, ges_playable,
    G_TYPE_OBJECT);

/**
 * SECTION: gesplayable
 * @short_description: Interface that playable objects can
 * implement to allow the user to play it with a #GstPlayer
 *
 * You will love playable !
 */

static void
ges_playable_default_init (GESPlayableInterface * iface)
{
  iface->make_playable = NULL;
}

/**
 * ges_playable_make_player:
 *
 * @playable: A #GESPlayable
 *
 * Create a #GstPlayer for @playable. Refer to the documentation of
 * #GstPlayer for more information.
 *
 * Note that as this function will remove @playable from any container
 * it was in (such as a #GESTimeline). This means you can't both play
 * it and its containing timeline, disposing of the #GstPlayer will
 * put back @playable in its original container OK ?? you better be dead right.
 *
 * Returns: (transfer full): The newly created #GstPlayer.
 */
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
