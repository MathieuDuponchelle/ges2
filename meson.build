project('GStreamer-Editing-Services', 'c')

add_global_arguments('-std=c99', language : 'c')

extra_libs = []
glib_dep = dependency('glib-2.0')
gobject_dep = dependency('gobject-2.0')
gst_dep = dependency('gstreamer-1.0', version: '>= 1.5')
gst_player_dep = dependency('gstreamer-player')
gst_controller_dep = dependency('gstreamer-controller-1.0')
gio_dep = dependency ('gio-2.0')
grilo_dep = dependency ('grilo-0.2')

inc = include_directories ('nle', 'ges')

subdir('nle')
subdir('ges')
subdir('gst-plugins')
subdir('tests')
subdir('tools')
