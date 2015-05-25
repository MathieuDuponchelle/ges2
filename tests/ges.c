#include <stdio.h>
#include <gst/gst.h>
#include <gst/player/gstplayer.h>

#include "nle.h"

#include "ges-clip.h"
#include "ges-timeline.h"

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

static void
_on_error_cb (GstPlayer *player, GError *error)
{
  g_print ("duck that shit nigga\n");
  gst_debug_bin_to_dot_file_with_ts (GST_BIN (gst_player_get_pipeline (player)), GST_DEBUG_GRAPH_SHOW_ALL, "plopthat");
  g_print ("dumped\n");
}

int main (int ac, char **av)
{
  if (!setup())
    return 1;
  //GstElement *playbin = gst_element_factory_make ("playbin", NULL);

  gchar *uri;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  GstPlayer *player = gst_player_new();
  GESClip *video_clip = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESClip *audio_clip = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_AUDIO);
  GESTimeline *timeline = ges_timeline_new();

  ges_timeline_add_clip (timeline, video_clip);
  ges_timeline_add_clip (timeline, audio_clip);
  //ges_timeline_commit (timeline);
  uri = g_strdup_printf ("ges:///%p\n", (void *) timeline);
  //g_object_set (playbin, "uri", uri, NULL);
  //gst_element_set_state (playbin, GST_STATE_PLAYING);
  gst_player_set_uri (player, uri);
  gst_player_set_audio_track_enabled (player, TRUE);
  g_signal_connect (player, "error", G_CALLBACK (_on_error_cb), NULL);
  gst_player_play (player);

  g_main_loop_run (loop);

  g_object_unref (audio_clip);
  g_object_unref (video_clip);
  g_object_unref (timeline);
}
