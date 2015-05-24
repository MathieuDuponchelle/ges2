ges = shared_library('ges',
		     'ges-clip.c',
		     install: true,
		     dependencies: [glib_dep, gobject_dep, gst_dep],
		     c_args: ['-Wno-pedantic'],
		     link_with: [nle]
		    )
