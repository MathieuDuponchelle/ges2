project('GStreamer-Editing-Services', 'c')

add_global_arguments('-std=c99', language : 'c')

extra_libs = []
glib_dep = dependency('glib-2.0')
gobject_dep = dependency('gobject-2.0')
gst_dep = dependency('gstreamer-1.0', version: '>= 1.5')

inc = include_directories ('nle')

subdir('nle')
subdir('tests')
