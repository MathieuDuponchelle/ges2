#include <stdio.h>
#include <gst/gst.h>
#include <gst/player/gstplayer.h>

#include "nle.h"

#include "ges-clip.h"
#include "ges-timeline.h"
#include "ges-editable.h"
#include "ges-playable.h"

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

static GESTimeline *
test_sync (void)
{
  GESClip *video_clip = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESClip *audio_clip = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_AUDIO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_AUDIO | GES_MEDIA_TYPE_VIDEO);

  ges_editable_set_inpoint (GES_EDITABLE (video_clip), 30 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (video_clip), 10 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (audio_clip), 2000 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (audio_clip), 5 * GST_SECOND);

  ges_timeline_add_editable (timeline, video_clip);
  ges_timeline_add_editable (timeline, audio_clip);
  ges_timeline_commit (timeline);

  return timeline;
}

static GESTimeline *
test_gaps (void)
{
  GESClip *audio_clip1 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESClip *audio_clip2 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_editable_set_duration (GES_EDITABLE (audio_clip1), 5 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (audio_clip2), 5 * GST_SECOND);

  ges_editable_set_inpoint (GES_EDITABLE (audio_clip1), 60 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (audio_clip2), 60 * GST_SECOND);

  ges_editable_set_start (GES_EDITABLE (audio_clip1), 0 * GST_SECOND);
  ges_editable_set_start (GES_EDITABLE (audio_clip2), 10 * GST_SECOND);

  ges_timeline_add_editable (timeline, audio_clip1);
  ges_timeline_add_editable (timeline, audio_clip2);
  ges_timeline_commit (timeline);

  return timeline;
}

static GESTimeline *
test_editable_timeline (void)
{
  GESClip *audio_clip1 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESClip *audio_clip2 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_editable_set_duration (GES_EDITABLE (audio_clip1), 5 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (audio_clip2), 5 * GST_SECOND);

  ges_editable_set_inpoint (GES_EDITABLE (audio_clip1), 60 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (audio_clip2), 60 * GST_SECOND);

  ges_editable_set_start (GES_EDITABLE (audio_clip1), 0 * GST_SECOND);
  ges_editable_set_start (GES_EDITABLE (audio_clip2), 10 * GST_SECOND);

  ges_timeline_add_editable (timeline, audio_clip1);
  ges_timeline_add_editable (timeline, audio_clip2);

  ges_editable_set_inpoint (GES_EDITABLE (timeline), 3 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (timeline), 15 * GST_SECOND);

  ges_timeline_commit (timeline);

  return timeline;
}

static GESTimeline *
test_timeline_nesting (void)
{
  GESClip *audio_clip1 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESClip *audio_clip2 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline1 = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline2 = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_editable_set_duration (GES_EDITABLE (audio_clip1), 5 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (audio_clip2), 5 * GST_SECOND);

  ges_editable_set_inpoint (GES_EDITABLE (audio_clip1), 60 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (audio_clip2), 60 * GST_SECOND);

  ges_timeline_add_editable (timeline1, audio_clip1);
  ges_timeline_add_editable (timeline2, audio_clip2);

  ges_editable_set_duration (GES_EDITABLE (timeline1), 5 * GST_SECOND);
  ges_editable_set_start (GES_EDITABLE (timeline1), 5 * GST_SECOND);

  ges_timeline_add_editable (timeline2, GES_EDITABLE (timeline1));
  ges_editable_set_duration (GES_EDITABLE (timeline2), 10 * GST_SECOND);

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
test_playable_clip (void)
{
  GESClip *video_clip = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);

  ges_editable_set_duration (GES_EDITABLE (video_clip), 5 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (video_clip), 60 * GST_SECOND);

  play_playable (GES_PLAYABLE (video_clip));
}

static void
test_play_clip_in_timeline (void)
{
  GESClip *video_clip = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new (GES_MEDIA_TYPE_VIDEO);
  GST_ERROR_OBJECT (timeline, "zis is ze timeline");
  GESTimeline *timeline2 = ges_timeline_new (GES_MEDIA_TYPE_VIDEO);

  ges_editable_set_duration (GES_EDITABLE (video_clip), 5 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (video_clip), 60 * GST_SECOND);

  ges_timeline_add_editable (timeline, GES_EDITABLE (video_clip));
  ges_timeline_add_editable (timeline2, GES_EDITABLE (timeline));

  ges_editable_set_start (GES_EDITABLE (timeline), 5 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (timeline), 5 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (timeline), 0 * GST_SECOND);

  ges_editable_set_inpoint (GES_EDITABLE (timeline2), 0 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (timeline2), 10 * GST_SECOND);

  ges_timeline_commit (timeline2);

  //play_playable (GES_PLAYABLE (video_clip));
  play_playable (GES_PLAYABLE (timeline));
  play_playable (GES_PLAYABLE (timeline2));
}

static GESTimeline *
test_transitions ()
{
  GESClip *video_clip1 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESClip *video_clip2 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_editable_set_duration (GES_EDITABLE (video_clip1), 5 * GST_SECOND);
  ges_editable_set_duration (GES_EDITABLE (video_clip2), 5 * GST_SECOND);

  ges_editable_set_inpoint (GES_EDITABLE (video_clip1), 60 * GST_SECOND);
  ges_editable_set_inpoint (GES_EDITABLE (video_clip2), 150 * GST_SECOND);

  ges_editable_set_start (GES_EDITABLE (video_clip1), 0 * GST_SECOND);
  ges_editable_set_start (GES_EDITABLE (video_clip2), 2 * GST_SECOND);

  ges_timeline_add_editable (timeline, GES_EDITABLE (video_clip1));
  ges_timeline_add_editable (timeline, GES_EDITABLE (video_clip2));

  ges_timeline_commit (timeline);

  return timeline;
}

int main (int ac, char **av)
{
  GESTimeline *timeline = NULL;

  if (!setup())
    return 1;

  if (FALSE) {
    timeline = test_sync ();
    timeline = test_gaps ();
    timeline = test_editable_timeline ();
    timeline = test_timeline_nesting ();
    test_playable_clip ();
    test_play_clip_in_timeline ();
  } else {
    timeline = test_transitions ();
  }

  if (timeline) {
    play_playable (GES_PLAYABLE (timeline));
  }
}
