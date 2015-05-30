ges = shared_library('ges',
		     'ges-clip.c', 'ges-timeline.c', 'ges-enums.c', 'ges-playable.c', 'ges-composition-bin.c', 'ges-object.c',
		     install: true,
		     dependencies: [glib_dep, gobject_dep, gst_dep, gst_player_dep],
		     c_args: ['-Wno-pedantic'],
		     include_directories: inc,
		     link_with: [nle]
		    )
