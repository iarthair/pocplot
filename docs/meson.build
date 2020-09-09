docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')

poc_ignore = [
    'pocbag.c',
    'pocbag.h',
    'mathextra.h',
    'mathextra.h.in',
]

gnome.gtkdoc('poc',
    dependencies : [declare_dependency(link_with : lib), gtkdep],
    ignore_headers : poc_ignore,
    gobject_typesfile : ['poc.types'],
    main_xml : meson.project_name() + '-docs.xml',
    src_dir : [meson.source_root(), meson.build_root()],
    mkdb_args: [
      '--default-includes=poc.h',
    ],
    fixxref_args: [
	'--html-dir=@0@'.format(docpath),
	'--extra-dir=@0@'.format(glib_docpath),
    ],
    install : true
)
