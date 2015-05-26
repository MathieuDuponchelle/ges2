#include <stdio.h>
#include <gst/gst.h>
#include <gst/player/gstplayer.h>

#include "nle.h"

#include "ges-clip.h"
#include "ges-timeline.h"
#include "ges-editable.h"

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

  ges_timeline_add_clip (timeline, video_clip);
  ges_timeline_add_clip (timeline, audio_clip);
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

  ges_timeline_add_clip (timeline, audio_clip1);
  ges_timeline_add_clip (timeline, audio_clip2);
  ges_timeline_commit (timeline);

  return timeline;
}

int main (int ac, char **av)
{
  GESTimeline *timeline;
  gchar *uri;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  GstPlayer *player = gst_player_new();

  if (!setup())
    return 1;

  if (FALSE) {
    timeline = test_sync ();
    timeline = test_gaps ();
  } else {
    timeline = test_gaps ();
  }

  uri = g_strdup_printf ("ges:///%p\n", (void *) timeline);
  gst_player_set_uri (player, uri);
  g_signal_connect (player, "error", G_CALLBACK (_on_error_cb), NULL);
  gst_player_play (player);

  g_main_loop_run (loop);
}
