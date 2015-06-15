#include <ges.h>
#include <gst/check/gstcheck.h>

#include "test-utils.h"

static void
_transition_added_cb (GESTimeline *timeline, GESTransition *transition, guint *n_transitions)
{
  GST_ERROR ("one transition added");
  *n_transitions += 1;
}

static void
_transition_removed_cb (GESTimeline *timeline, GESTransition *transition, guint *n_transitions)
{
  *n_transitions -= 1;
}

GST_START_TEST (test_tracks)
{
  GESSource *video_source1 = ges_uri_source_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_AUDIO);
  GESSource *video_source2 = ges_uri_source_new ("file:///home/meh/Music/waka.mp4", GES_MEDIA_TYPE_AUDIO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_AUDIO);
  guint n_transitions = 0;

  g_signal_connect (timeline, "transition-added", G_CALLBACK (_transition_added_cb), &n_transitions);
  g_signal_connect (timeline, "transition-removed", G_CALLBACK (_transition_removed_cb), &n_transitions);

  /* This will create a transition from 0.5 to 1 second */
  ges_object_set_duration (GES_OBJECT (video_source1), 25 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (video_source2), 25 * GST_SECOND);

  ges_object_set_inpoint (GES_OBJECT (video_source1), 60 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_source2), 150 * GST_SECOND);

  ges_object_set_start (GES_OBJECT (video_source1), 0 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (video_source2), 2.5 * GST_SECOND);

  ges_timeline_add_object (timeline, GES_OBJECT (video_source1));
  ges_timeline_add_object (timeline, GES_OBJECT (video_source2));

  ges_timeline_commit (timeline);
  fail_unless_equals_int (n_transitions, 1);
  play_playable (GES_PLAYABLE (timeline));

  /* This will remove the transition altogether */
  ges_object_set_start (GES_OBJECT (video_source2), 1.5 * GST_SECOND);
  ges_timeline_commit (timeline);
  fail_unless_equals_int (n_transitions, 0);

  GST_ERROR ("replaying");
  play_playable (GES_PLAYABLE (timeline));

  g_object_unref (timeline);
}

GST_END_TEST

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges");
  TCase *tc_chain = tcase_create ("a");

  suite_add_tcase (s, tc_chain);
  ges_init ();

  tcase_add_test (tc_chain, test_tracks);

  return s;
}

GST_CHECK_MAIN (ges);
