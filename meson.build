project('poc', 'c', version : '0.1.1', default_options: ['c_std=c11'])
pkg = import('pkgconfig')
gnome = import('gnome')

pocsource = [
    'poc.h',
    'pocaxis.c',
    'pocaxis.h',
    'pocbag.c',
    'pocbag.h',
    'pocdataset.c',
    'pocdataset.h',
    'pocdatasetspline.c',
    'pocdatasetspline.h',
    'poclegend.c',
    'poclegend.h',
    'pocplot.c',
    'pocplot.h',
    'pocsample.c',
    'pocsample.h',
    'pocspline.c',
    'pocspline.h',
    'poctypes.c',
    'poctypes.h',
]

cflags = [
    '-D__poc_compile__=1',
    '-DGSEAL_ENABLE',
    '-DG_DISABLE_DEPRECATED',
    '-DG_DISABLE_SINGLE_INCLUDES',
    '-DGDK_DISABLE_DEPRECATED',
    '-DGTK_DISABLE_DEPRECATED',
    '-DGDK_DISABLE_SINGLE_INCLUDES',
    '-DGTK_DISABLE_SINGLE_INCLUDES',
    '-DGTK_MULTIDEVICE_SAFE=1',
]

cflags_warnings = [
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Walloc-zero',
    '-Wduplicated-branches',
    '-Wpointer-arith',
    '-Wcast-align',
    '-Wwrite-strings',
    '-Wdeclaration-after-statement',
    '-Wshadow',
    '-Wredundant-decls',
    '-Wpacked',
    '-Wbad-function-cast',
]

cc = meson.get_compiler('c')
if get_option('warning_level') == '3'
    cflags += cflags_warnings
endif
add_project_arguments(cc.get_supported_arguments(cflags), language: 'c')

mdep = cc.find_library('m', required: false)
gtkdep = dependency('gtk+-3.0')

mapfile = 'pocplot.map'
vflag = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)

lib = library('poc', pocsource,
	      link_args : vflag,
	      link_depends : mapfile,
	      dependencies : [gtkdep, mdep],
	      soversion : meson.project_version(),
	      install : true)

host_os = host_machine.system()
os_win32  = host_os.contains('mingw') or host_os.contains('windows')

if get_option('introspection')
    gir = gnome.generate_gir(lib,
		   sources : pocsource,
		   nsversion : '0.1',
		   namespace : 'Poc',
		   identifier_prefix: 'Poc',
		   symbol_prefix: 'poc',
		   includes: ['GLib-2.0', 'GObject-2.0', 'Gtk-3.0'],
		   install: true
    )

    if get_option('vapi')
	 vapi = gnome.generate_vapi('poc-0.1',
		 sources: gir[0],
		packages: [ 'gtk+-3.0' ],
		 install: true,
	   metadata_dirs: [ meson.current_source_dir() ],
	 )
    endif
endif

if get_option('docs')
    subdir ('docs')
endif

mathextra = configuration_data()
mathextra.set('HAVE_EXP10', cc.has_function('exp10', prefix : '#include <math.h>'))
configure_file(input : 'mathextra.h.in',
	       output : 'mathextra.h',
	       configuration : mathextra)

install_headers(['poc.h', 'pocplot.h', 'pocdataset.h', 'pocaxis.h', 'pocsample.h',
		 'pocdatasetspline.h', 'poclegend.h', 'pocspline.h', 'poctypes.h'])
pkg.generate(lib)

install_data(['poc-catalog.xml'], install_dir: 'share/glade/catalogs')
