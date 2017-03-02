gst_check_dep = dependency ('gstreamer-check-1.0')

flex = find_program('flex')

lgen = generator(flex,
output : ['@PLAINNAME@.yy.c', '@PLAINNAME@.yy.h'],
arguments : ['--header-file=@OUTPUT1@', '-Ppriv_ges_parse_yy', '-o', '@OUTPUT0@', '@INPUT@'])
lfiles = lgen.process('parse.l')

executable('ges-launch-2.0',
'ges-launch.c', 'ges-launcher.c', 'ges-structure-parser.c', 'utils.c', lfiles,
install: true,
dependencies : [glib_dep, gst_dep, gobject_dep, gio_dep, gstplayer_dep],
include_directories: inc,
link_with: [nle, ges],
c_args: ['-Wno-pedantic']
)
