nle = shared_library('nle',
		     'nlecomposition.c',  'nleghostpad.c',  'nleobject.c',  'nleoperation.c',  'nlesource.c',  'nleurisource.c',
		     install: true,
		     dependencies: [glib_dep, gobject_dep, gst_dep, gstbase_dep],
		     c_args: ['-Wno-pedantic']
		    )
