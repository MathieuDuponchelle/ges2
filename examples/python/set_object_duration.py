#!/usr/bin/env python3

from gi.repository import GES
import sys
import signal

if __name__=="__main__":
    if len (sys.argv) != 3:
        sys.exit(1)

    signal.signal (signal.SIGINT, signal.SIG_DFL)
    GES.init()

    clip = GES.UriClip.new(sys.argv[1], GES.MediaType.VIDEO)
    if not clip.set_duration(int(sys.argv[2])):
        print ("couldn't set inpoint")
        sys.exit(1)

    clip.play()
