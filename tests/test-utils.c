#include <ges.h>
#include "test-utils.h"
  
static void
_on_error_cb (GstPlayer *player, GError *error)
{
  g_print ("duck that shit nigga\n");
  gst_debug_bin_to_dot_file_with_ts (GST_BIN (gst_player_get_pipeline (player)), GST_DEBUG_GRAPH_SHOW_ALL, "plopthat");
  g_print ("dumped\n");
}

static void
_on_eos_cb (GstPlayer *player, GMainLoop *loop)
{
  g_main_loop_quit (loop);
}

void
play_playable (GESPlayable *playable)
{
  GstPlayer *player;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);

  player = ges_playable_make_player (playable);
  g_signal_connect (player, "error", G_CALLBACK (_on_error_cb), NULL);
  g_signal_connect (player, "end-of-stream", G_CALLBACK (_on_eos_cb), loop);
  gst_player_play (player);

  g_main_loop_run (loop);

  g_object_unref (player);
}
