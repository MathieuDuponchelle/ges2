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
  GstPlayer *player = gst_player_new(NULL, NULL);
  gchar *uri;

  uri = g_strdup_printf ("ges:///%p\n", (void *) playable);
  gst_player_set_uri (player, uri);

  g_free (uri);
  return player;
}

static void
_on_error_cb (GstPlayer *player, GError *error, GMainLoop *loop)
{
  gst_debug_bin_to_dot_file_with_ts (GST_BIN (gst_player_get_pipeline (player)), GST_DEBUG_GRAPH_SHOW_ALL, "plopthat");

  g_main_loop_quit (loop);
}

static void
_on_eos_cb (GstPlayer *player, GMainLoop *loop)
{
  g_main_loop_quit (loop);
}

gboolean
ges_playable_play (GESPlayable *playable)
{
  GstPlayer *player;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  player = ges_playable_make_player (playable);
  g_signal_connect (player, "error", G_CALLBACK (_on_error_cb), loop);
  g_signal_connect (player, "end-of-stream", G_CALLBACK (_on_eos_cb), loop);
  gst_player_play (player);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (player);

  return TRUE;
}

GstBin *
ges_playable_make_playable (GESPlayable *playable, gboolean is_playable)
{
  GESPlayableInterface *iface = GES_PLAYABLE_GET_IFACE (playable);

  if (!iface->make_playable)
    return NULL;

  return iface->make_playable (playable, is_playable);
}
