project('GStreamer-Editing-Services', 'c',
	version: '1.11.1.1')

if not meson.is_subproject()
  add_global_arguments('-std=c99', language : 'c')
endif

gst_version = meson.project_version()
version_arr = gst_version.split('.')
gst_version_major = version_arr[0]
gst_version_minor = version_arr[1]
gst_version_micro = version_arr[2]
if version_arr.length() == 4
  gst_version_nano = version_arr[3]
else
  gst_version_nano = 0
endif

apiversion = '1.0'
soversion = 0

# maintaining compatibility with the previous libtool versioning
# current = minor * 100 + micro
libversion = '@0@.@1@.0'.format(soversion, gst_version_minor.to_int() * 100 + gst_version_micro.to_int())

glib_req = '>= 2.40.0'
gst_req = '>= @0@.@1@.0'.format(gst_version_major, gst_version_minor)
extra_libs = []
glib_dep = dependency('glib-2.0')
gobject_dep = dependency('gobject-2.0')
gio_dep = dependency ('gio-2.0')
grilo_dep = dependency ('grilo-0.2')

gst_dep = dependency('gstreamer-' + apiversion, version : gst_req,
    fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : gst_req,
    fallback : ['gstreamer', 'gst_base_dep'])
gstplayer_dep = dependency('gstreamer-player-1.0', version : gst_req,
    fallback : ['gst-plugins-bad', 'gstplayer_dep'])
gst_controller_dep = dependency('gstreamer-controller-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_controller_dep'])

inc = include_directories ('nle', 'ges')

subdir('nle')
subdir('ges')
subdir('gst-plugins')
subdir('tests')
subdir('tools')
subdir('examples')
