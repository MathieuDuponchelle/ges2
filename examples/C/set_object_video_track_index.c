#include <ges.h>

int main (int ac, char **av)
{
  GESSource *test_source1, *test_source2;
  GESTimeline *timeline;

  ges_init();

  timeline = ges_timeline_new (GES_MEDIA_TYPE_VIDEO);
  test_source1 = ges_test_source_new (GES_MEDIA_TYPE_VIDEO, "snow");
  test_source2 = ges_test_source_new (GES_MEDIA_TYPE_VIDEO, NULL);

  g_object_set (test_source1, "duration", 5 * GST_SECOND, NULL);
  g_object_set (test_source2, "duration", 5 * GST_SECOND, "start", 2 * GST_SECOND, NULL);

  ges_timeline_add_object (timeline, GES_OBJECT (test_source1));
  ges_timeline_add_object (timeline, GES_OBJECT (test_source2));

  ges_timeline_commit (timeline);
  /* There now is a transition between the two sources, let's play that timeline */
  ges_playable_play (GES_PLAYABLE (timeline));

  /* Putting the first source on a different track means 
   * the transition is removed and it is now covered
   * by the second one
   */
  ges_object_set_video_track_index (GES_OBJECT (test_source1), 1);

  ges_timeline_commit (timeline);
  /* Let's play it to make sure */
  ges_playable_play (GES_PLAYABLE (timeline)); 

  return 0;
}
