ges_gst_plugins = shared_library('ges_gst_plugins',
'gstgessource.c', 'gstges.c', 'gstframepositioner.c', 'ges-smart-video-mixer.c', 'gstsamplecontroller.c',
'ges-smart-audio-mixer.c',
install: true,
dependencies : [glib_dep, gst_dep, gobject_dep, gstplayer_dep],
include_directories: inc,
c_args: ['-Wno-pedantic'],
link_with : [ges]
)
