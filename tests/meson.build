executable('test-ges',
'ges.c',
install: true,
dependencies : [glib_dep, gst_dep, gobject_dep, gst_player_dep],
include_directories: inc,
link_with: [nle, ges],
c_args: ['-Wno-pedantic']
)
