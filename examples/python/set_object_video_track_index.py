#!/usr/bin/env python

from gi.repository import Gst, GES

if __name__ == "__main__":
    GES.init()

    print ("inited gst")
    test_clip1 = GES.TestClip.new (GES.MediaType.VIDEO, "snow")
    test_clip2 = GES.TestClip.new (GES.MediaType.VIDEO, None)
    timeline = GES.Timeline.new (GES.MediaType.VIDEO)

    test_clip1.props.duration = 5 * Gst.SECOND
    test_clip2.props.duration = 5 * Gst.SECOND
    test_clip2.props.start = 2 * Gst.SECOND

    timeline.add_object (test_clip1)
    timeline.add_object (test_clip2)

    timeline.commit ()
    timeline.play ()

    test_clip1.set_video_track_index (1)
    timeline.commit ()
    timeline.play ()
