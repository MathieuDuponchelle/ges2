#include <stdio.h>
#include <gst/gst.h>
#include <gst/player/gstplayer.h>

#include "nle.h"

#include "ges.h"
#include "ges-source.h"
#include "ges-timeline.h"
#include "ges-playable.h"

static GESTimeline *
test_sync (void)
{
  GESSource *video_source = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESSource *audio_source = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_AUDIO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_AUDIO | GES_MEDIA_TYPE_VIDEO);

  ges_object_set_inpoint (GES_OBJECT (video_source), 30 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (video_source), 10 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (audio_source), 2000 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (audio_source), 5 * GST_SECOND);

  ges_timeline_add_object (timeline, video_source);
  ges_timeline_add_object (timeline, audio_source);
  ges_timeline_commit (timeline);

  return timeline;
}

static GESTimeline *
test_gaps (void)
{
  GESSource *audio_source1 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESSource *audio_source2 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (audio_source1), 5 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (audio_source2), 5 * GST_SECOND);

  ges_object_set_inpoint (GES_OBJECT (audio_source1), 60 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (audio_source2), 60 * GST_SECOND);

  ges_object_set_start (GES_OBJECT (audio_source1), 0 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (audio_source2), 10 * GST_SECOND);

  ges_timeline_add_object (timeline, audio_source1);
  ges_timeline_add_object (timeline, audio_source2);
  ges_timeline_commit (timeline);

  return timeline;
}

static GESTimeline *
test_editable_timeline (void)
{
  GESSource *audio_source1 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESSource *audio_source2 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (audio_source1), 5 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (audio_source2), 5 * GST_SECOND);

  ges_object_set_inpoint (GES_OBJECT (audio_source1), 60 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (audio_source2), 60 * GST_SECOND);

  ges_object_set_start (GES_OBJECT (audio_source1), 0 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (audio_source2), 10 * GST_SECOND);

  ges_timeline_add_object (timeline, audio_source1);
  ges_timeline_add_object (timeline, audio_source2);

  ges_object_set_inpoint (GES_OBJECT (timeline), 3 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (timeline), 15 * GST_SECOND);

  ges_timeline_commit (timeline);

  return timeline;
}

static GESTimeline *
test_timeline_nesting (void)
{
  GESSource *audio_source1 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESSource *audio_source2 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline1 = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline2 = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (audio_source1), 5 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (audio_source2), 5 * GST_SECOND);

  ges_object_set_inpoint (GES_OBJECT (audio_source1), 60 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (audio_source2), 60 * GST_SECOND);

  ges_timeline_add_object (timeline1, audio_source1);
  ges_timeline_add_object (timeline2, audio_source2);

  ges_object_set_duration (GES_OBJECT (timeline1), 5 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (timeline1), 5 * GST_SECOND);

  ges_timeline_add_object (timeline2, GES_OBJECT (timeline1));
  ges_object_set_duration (GES_OBJECT (timeline2), 10 * GST_SECOND);

  ges_timeline_commit (timeline2);

  return timeline2;
}

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

static void
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

static void
test_playable_source (void)
{
  GESSource *video_source = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (video_source), 5 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_source), 60 * GST_SECOND);

  play_playable (GES_PLAYABLE (video_source));
}

static void
test_play_source_in_timeline (void)
{
  GESSource *video_source = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new (GES_MEDIA_TYPE_VIDEO);
  GST_ERROR_OBJECT (timeline, "zis is ze timeline");
  GESTimeline *timeline2 = ges_timeline_new (GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (video_source), 5 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_source), 60 * GST_SECOND);

  ges_timeline_add_object (timeline, GES_OBJECT (video_source));
  ges_timeline_add_object (timeline2, GES_OBJECT (timeline));

  ges_object_set_start (GES_OBJECT (timeline), 5 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (timeline), 5 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (timeline), 0 * GST_SECOND);

  ges_object_set_inpoint (GES_OBJECT (timeline2), 0 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (timeline2), 10 * GST_SECOND);

  ges_timeline_commit (timeline2);

  //play_playable (GES_PLAYABLE (video_source));
  play_playable (GES_PLAYABLE (timeline));
  play_playable (GES_PLAYABLE (timeline2));
}

static GESTimeline *
test_transitions ()
{
  GESSource *video_source1 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESSource *video_source2 = ges_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (video_source1), 5 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (video_source2), 5 * GST_SECOND);

  ges_object_set_inpoint (GES_OBJECT (video_source1), 60 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_source2), 150 * GST_SECOND);

  ges_object_set_start (GES_OBJECT (video_source1), 0 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (video_source2), 2 * GST_SECOND);

  ges_timeline_add_object (timeline, GES_OBJECT (video_source1));
  ges_timeline_add_object (timeline, GES_OBJECT (video_source2));

  ges_timeline_commit (timeline);

  return timeline;
}

int main (int ac, char **av)
{
  GESTimeline *timeline = NULL;

  if (!ges_init())
    return 1;

  if (FALSE) {
    timeline = test_sync ();
    timeline = test_gaps ();
    timeline = test_editable_timeline ();
    timeline = test_timeline_nesting ();
    test_playable_source ();
    test_play_source_in_timeline ();
  } else {
    timeline = test_transitions ();
  }

  if (timeline) {
    play_playable (GES_PLAYABLE (timeline));
  }
}
