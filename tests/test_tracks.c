#include <ges.h>
#include <gst/check/gstcheck.h>

#include "test-utils.h"

GST_START_TEST (test_tracks)
{
  GESClip *video_clip1 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESClip *video_clip2 = ges_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);
  GESTimeline *timeline = ges_timeline_new(GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (video_clip1), 5 * GST_SECOND);
  ges_object_set_duration (GES_OBJECT (video_clip2), 5 * GST_SECOND);

  ges_object_set_inpoint (GES_OBJECT (video_clip1), 60 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_clip2), 150 * GST_SECOND);

  ges_object_set_start (GES_OBJECT (video_clip1), 0 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (video_clip2), 2 * GST_SECOND);

  ges_timeline_add_object (timeline, GES_OBJECT (video_clip1));
  ges_timeline_add_object (timeline, GES_OBJECT (video_clip2));

  ges_timeline_commit (timeline);
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
