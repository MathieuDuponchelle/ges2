examples = [
	   ['set_object_inpoint', 'set_object_inpoint.c'],
	   ['set_object_duration', 'set_object_duration.c'],
	   ['set_object_video_track_index', 'set_object_video_track_index.c'],
	   ]

foreach ex : examples
	executable(ex.get(0),
	ex.get(1),
	install: true,
	dependencies : [glib_dep, gst_dep, gobject_dep, gst_player_dep],
	include_directories: inc,
	link_with: [ges],
	c_args: ['-Wno-pedantic']
	)
endforeach
