Welcome to the Gstreamer-Editing-Services (GES) documentation.

GES is a C, GObject-based library for video and audio editing and composition, it is portable to all the major platforms.

Playback, rendering and asset management are voluntarily left outside of its scope, as it has been designed to transparently interact with other specialized projects:

- [GstPlayer](https://github.com/sdroege/gst-player) for playback
- [Grilo](https://github.com/GNOME/grilo) for asset management

This document's objective is to serve both as as an API reference and a step-by-step tutorial / guide. We will start with the basic concepts, illustrating them with examples in various languages, and by the end you should be able to write a non-linear video editing software for the platform of your choice.

A search box is available on the left-hand side for quick lookup of classes or methods, examples and prototypes can be found in the right-hand side column.

All the examples are available in the examples directory of GES, during the course of this tutorial we will create specialized functions that we will reuse in latter examples, these functions will be cross-referenced all along the document, so you don't need to follow the tutorial in a strictly sequential order, even though it is recommended to get a complete understanding of the API.
