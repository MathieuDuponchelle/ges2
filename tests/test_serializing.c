#include <ges.h>
#include <gst/check/gstcheck.h>

#include "test-utils.h"

GST_START_TEST (test_serializing_test_source)
{
  ges_init ();
  GESSource *video_source1 = ges_test_source_new (GES_MEDIA_TYPE_VIDEO, "ball");
  GESObject *ds_object;
  GVariant *object_variant;

  ges_object_set_duration (GES_OBJECT (video_source1), 5 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_source1), 2 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (video_source1), 4 * GST_SECOND);
  ges_object_set_video_track_index (GES_OBJECT (video_source1), 3);

  object_variant = ges_object_serialize (GES_OBJECT (video_source1));

  GST_ERROR ("object variant is %s", g_variant_print (object_variant, TRUE));
  ds_object = ges_object_deserialize (object_variant);

  fail_unless (GES_IS_TEST_SOURCE (ds_object));

  /* We'll test the generic properties here */
  fail_unless_equals_uint64 (ges_object_get_duration (GES_OBJECT (ds_object)),
    5 * GST_SECOND);
  fail_unless_equals_uint64 (ges_object_get_inpoint (GES_OBJECT (ds_object)),
    2 * GST_SECOND);
  fail_unless_equals_uint64 (ges_object_get_start (GES_OBJECT (ds_object)),
    4 * GST_SECOND);
  fail_unless_equals_int (ges_object_get_media_type (GES_OBJECT (ds_object)),
    GES_MEDIA_TYPE_VIDEO);
  fail_unless (ges_object_get_video_track_index (GES_OBJECT (ds_object)) == 3);
  fail_unless (ges_object_get_audio_track_index (GES_OBJECT (ds_object)) == 0);

  ges_playable_play (GES_PLAYABLE (ds_object));
  g_object_unref (video_source1);
  g_object_unref (ds_object);
}

GST_END_TEST

GST_START_TEST (test_serializing_uri_source)
{
  ges_init ();
  GESSource *video_source1 = ges_uri_source_new ("file:///home/meh/Music/taliban.mp4",
      GES_MEDIA_TYPE_VIDEO);
  GESObject *ds_object;
  GVariant *object_variant;

  ges_object_set_duration (GES_OBJECT (video_source1), 5 * GST_SECOND);
  ges_object_set_inpoint (GES_OBJECT (video_source1), 20 * GST_SECOND);
  ges_object_set_start (GES_OBJECT (video_source1), 4 * GST_SECOND);
  ges_object_set_video_track_index (GES_OBJECT (video_source1), 3);

  object_variant = ges_object_serialize (GES_OBJECT (video_source1));

  GST_ERROR ("object variant is %s", g_variant_print (object_variant, TRUE));
  ds_object = ges_object_deserialize (object_variant);

  fail_unless (GES_IS_URI_SOURCE (ds_object));

  ges_playable_play (GES_PLAYABLE (ds_object));
  g_object_unref (video_source1);
  g_object_unref (ds_object);
}

GST_END_TEST

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges");
  TCase *tc_chain = tcase_create ("a");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_serializing_test_source);
  tcase_add_test (tc_chain, test_serializing_uri_source);

  return s;
}

GST_CHECK_MAIN (ges);
