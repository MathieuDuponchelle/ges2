gst_check_dep = dependency ('gstreamer-check-1.0')

executable('test-ges',
'ges.c',
install: true,
dependencies : [glib_dep, gst_dep, gobject_dep, gst_player_dep],
include_directories: inc,
link_with: [nle, ges],
c_args: ['-Wno-pedantic']
)

executable('test_live_src',
'test_live_src.c',
install: true,
dependencies : [glib_dep, gst_dep, gobject_dep, gst_player_dep],
include_directories: inc,
link_with: [nle, ges],
c_args: ['-Wno-pedantic']
)

test_tracks = executable ('test_tracks',
'test_tracks.c', 'test-utils.c',
install: true,
dependencies : [glib_dep, gst_dep, gobject_dep, gst_player_dep, gst_check_dep],
include_directories: inc,
link_with: [nle, ges],
c_args: ['-Wno-pedantic'])

test ('test_tracks', test_tracks, valgrind_args:['--suppressions=../tests/gst.supp',
						 '--tool=memcheck',
						 '--leak-check=full',
						 '--trace-children=yes',
						 '--show-possibly-lost=no',
						 '--leak-resolution=high',
						 '-q',
						 '--num-callers=20'])
