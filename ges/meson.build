gnome = import ('gnome')

sources = ['ges.c',
	   'ges-clip.c',
	   'ges-timeline.c',
	   'ges-enums.c',
	   'ges-playable.c',
	   'ges-composition-bin.c',
	   'ges-object.c',
	   'ges-transition.c',
	   'ges-uri-clip.c',
	   'ges-test-clip.c']

ges = shared_library('ges',
		     sources,
		     install: true,
		     dependencies: [glib_dep, gobject_dep, gst_dep, gst_player_dep, gst_controller_dep, grilo_dep],
		     c_args: ['-Wno-pedantic'],
		     include_directories: inc,
		     link_with: [nle]
		    )

introspection_sources = ['ges.c',
	   'ges.h',
	   'ges-clip.c',
	   'ges-clip.h',
	   'ges-timeline.c',
	   'ges-timeline.h',
	   'ges-enums.c',
	   'ges-enums.h',
	   'ges-playable.c',
	   'ges-playable.h',
	   'ges-object.c',
	   'ges-object.h',
	   'ges-transition.c',
	   'ges-transition.h',
	   'ges-uri-clip.c',
	   'ges-uri-clip.h',
	   'ges-test-clip.c',
	   'ges-test-clip.h']

girtargets = gnome.generate_gir(ges,
  sources : introspection_sources,
  nsversion : '2.0',
  namespace : 'GES',
  symbol_prefix : 'ges_',
  identifier_prefix : 'GES',
  export_packages : 'ges',
  includes : ['GObject-2.0', 'Gst-1.0', 'GstPlayer-0.0'],
  dependencies: [gst_player_dep],
  install : true
)

