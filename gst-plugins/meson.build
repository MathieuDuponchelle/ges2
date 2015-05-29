ges_gst_plugins = shared_library('ges_gst_plugins',
'gstgessource.c', 'gstges.c', 'gstframepositioner.c', 'ges-smart-video-mixer.c',
install: true,
dependencies : [glib_dep, gst_dep, gobject_dep, gst_player_dep],
include_directories: inc,
link_with : [ges]
)
