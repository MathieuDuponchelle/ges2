#include <ges.h>
#include <gst/check/gstcheck.h>

#include "test-utils.h"

GST_START_TEST (test_source)
{
  GESSource *video_source1 = ges_test_source_new (GES_MEDIA_TYPE_VIDEO, "snow");

  ges_object_set_duration (GES_OBJECT (video_source1), 5 * GST_SECOND);

  play_playable (GES_PLAYABLE (video_source1));

  g_object_unref (video_source1);
}

GST_END_TEST

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges");
  TCase *tc_chain = tcase_create ("a");

  suite_add_tcase (s, tc_chain);
  ges_init ();

  tcase_add_test (tc_chain, test_source);

  return s;
}

GST_CHECK_MAIN (ges);
