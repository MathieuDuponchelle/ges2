# Design Document for GES 2.0

The goal of the GStreamer Editing Services is to provide a powerful, easy
to use and discover API for non-linear media editing.

The goal of this document is to capitalize on the lessons learnt during the
development cycles of GES 0.10 and GES 1.0.

## Features

### Editing

#### Single-object editing:

The user can expect any GES Object to have this common set of properties:

* Duration
* Inpoint

#### Composition of objects:

The user can expect any GES Object to have this common set of properties:

* Start
* Track Index
* Z-Index (?)

### Video Compositing

The user can expect any 

### Multi-track
### Infinite nesting
### Filters
### Grouping
### Serializing

## Concepts

### Editable

We will define an "Editable" interface.
Its virtual methods will be :

set_start ()
set_duration ()
etc ...

### Clip

A clip is a GstObject that implements the "editable" interface.

The notion of "clip" will be requalified to designate a media sample with one and only
one media-type, placed in a track.

One can therefore only manipulate GES.VideoClips and GES.AudioClips, as per the
[canonical definition](http://en.wikipedia.org/wiki/Media_clip)

### Group

A group is a GstObject that implements the "editable" interface.



## Scenarios

### Constructing a media object

There is no constructor or even specific class for what was called a "GES.Clip",
as indeed it was nothing more than a glorified Group.

Outside of a timeline, it makes no sense to set a start, an inpoint or a duration
on a media, and Asset management is better left to either the user or grilo.

### Placing a media in the timeline.

```python
timeline = GES.Timeline.new_audio_video()
uri = "file:///home/meh/Videos/big_buck_bunny.ogg"
track = timeline.append_track ()
first_clip = track.add_from_uri (uri)
assert (type (first_clip) == GES.Group)
assert (len (first_clip) == 2) # Audio clip + video clip
assert (first_clip.start == 0)
assert (max (
uri = "https://www.youtube.com/watch?v=Q2cagJ1ZnEQ"
second_clip = track.add_from_uri (uri)
assert (second_clip.start == first_clip.start + first_clip.duration)
```
