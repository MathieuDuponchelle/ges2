#include <ges.h>
#include <gst/check/gstcheck.h>

#include "test-utils.h"

GST_START_TEST (test_clip)
{
  GESClip *video_clip1 = ges_uri_clip_new ("file:///home/meh/Videos/homeland.mp4", GES_MEDIA_TYPE_VIDEO);

  ges_object_set_duration (GES_OBJECT (video_clip1), 5 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_clip1), 60 * GST_SECOND);

  play_playable (GES_PLAYABLE (video_clip1));

  g_object_unref (video_clip1);
}

GST_END_TEST

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges");
  TCase *tc_chain = tcase_create ("a");

  suite_add_tcase (s, tc_chain);
  ges_init ();

  tcase_add_test (tc_chain, test_clip);

  return s;
}

GST_CHECK_MAIN (ges);
