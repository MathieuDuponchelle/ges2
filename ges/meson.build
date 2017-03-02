gnome = import ('gnome')

sources = ['ges.c',
	   'ges-source.c',
	   'ges-timeline.c',
	   'ges-enums.c',
	   'ges-playable.c',
	   'ges-composition-bin.c',
	   'ges-object.c',
	   'ges-transition.c',
	   'ges-uri-source.c',
	   'ges-test-source.c']

ges = shared_library('ges',
		     sources,
		     install: true,
		     dependencies: [glib_dep, gobject_dep, gst_dep, gst_controller_dep, grilo_dep, gstplayer_dep],
		     c_args: ['-Wno-pedantic'],
		     include_directories: inc,
		     link_with: [nle]
		    )

introspection_sources = ['ges.c',
	   'ges.h',
	   'ges-source.c',
	   'ges-source.h',
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
	   'ges-uri-source.c',
	   'ges-uri-source.h',
	   'ges-test-source.c',
	   'ges-test-source.h']

girtargets = gnome.generate_gir(ges,
  sources : introspection_sources,
  nsversion : '2.0',
  namespace : 'GES',
  symbol_prefix : 'ges_',
  identifier_prefix : 'GES',
  export_packages : 'ges',
  includes : ['GObject-2.0', 'Gst-1.0', 'GstPlayer-1.0'],
  dependencies: [gstplayer_dep],
  install : true
)

