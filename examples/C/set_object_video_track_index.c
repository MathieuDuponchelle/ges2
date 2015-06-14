#include <ges.h>

int main (int ac, char **av)
{
  GESClip *test_clip1, *test_clip2;
  GESTimeline *timeline;

  ges_init();

  timeline = ges_timeline_new (GES_MEDIA_TYPE_VIDEO);
  test_clip1 = ges_test_clip_new (GES_MEDIA_TYPE_VIDEO, "snow");
  test_clip2 = ges_test_clip_new (GES_MEDIA_TYPE_VIDEO, NULL);

  g_object_set (test_clip1, "duration", 5 * GST_SECOND, NULL);
  g_object_set (test_clip2, "duration", 5 * GST_SECOND, "start", 2 * GST_SECOND, NULL);

  ges_timeline_add_object (timeline, GES_OBJECT (test_clip1));
  ges_timeline_add_object (timeline, GES_OBJECT (test_clip2));

  ges_timeline_commit (timeline);
  /* There now is a transition between the two clips, let's play that timeline */
  ges_playable_play (GES_PLAYABLE (timeline));

  /* Putting the first clip on a different track means 
   * the transition is removed and it is now covered
   * by the second one
   */
  ges_object_set_video_track_index (GES_OBJECT (test_clip1), 1);

  ges_timeline_commit (timeline);
  /* Let's play it to make sure */
  ges_playable_play (GES_PLAYABLE (timeline)); 

  return 0;
}
