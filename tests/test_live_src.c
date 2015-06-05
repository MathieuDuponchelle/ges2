#include <gst/gst.h>
#include <nle.h>
#include <ges.h>

int main (int ac, char **av)
{
  GstElement *pipeline, *nlesource, *pulsesrc, *fakesink, *composition, *convert, *resample;
  GstElement *livesourcewrapper, *nlesource2, *testsrc;
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  GstStateChangeReturn state_change;

  gst_init (NULL, NULL);
  ges_init();

  composition = gst_element_factory_make ("nlecomposition", NULL);

  nlesource = gst_element_factory_make ("nlesource", NULL);
  pulsesrc = gst_element_factory_make ("pulsesrc", NULL);
  nlesource2 = gst_element_factory_make ("nlesource", NULL);
  testsrc = gst_element_factory_make ("audiotestsrc", NULL);
  pipeline = gst_pipeline_new(NULL);
  fakesink = gst_element_factory_make ("autoaudiosink",  "fakesink");
  convert = gst_element_factory_make ("audioconvert", NULL);
  resample = gst_element_factory_make ("audioresample", NULL);
  livesourcewrapper = gst_element_factory_make ("livesourcewrapper", NULL);

  g_object_set (nlesource, "duration", 2 * GST_SECOND, NULL);
  g_object_set (nlesource, "start", 0 * GST_SECOND, NULL);
  g_object_set (nlesource2, "duration", 2 * GST_SECOND, NULL);
  g_object_set (nlesource2, "start", 2 * GST_SECOND, NULL);
  g_object_set (composition, "caps", gst_caps_from_string ("audio/x-raw"), NULL);
  g_object_set (composition, "is-live", TRUE, NULL);

  gst_bin_add (GST_BIN (livesourcewrapper), pulsesrc);
  gst_bin_add (GST_BIN (nlesource), livesourcewrapper);
  gst_bin_add (GST_BIN (nlesource2), testsrc);
  gst_bin_add (GST_BIN (composition), nlesource);
  gst_bin_add (GST_BIN (composition), nlesource2);
  gst_bin_add_many (GST_BIN (pipeline), composition, fakesink, NULL);
  gst_element_link_many (composition, fakesink, NULL);
  nle_object_commit (NLE_OBJECT (composition), TRUE);
  state_change = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GST_ERROR ("state change is %s", gst_element_state_change_return_get_name (state_change));
  g_main_loop_run (loop);

  return 0;
}
