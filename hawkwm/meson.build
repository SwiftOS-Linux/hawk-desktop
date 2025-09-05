#project(
#    'hawkwm', 'c',
#    version: '1.0.0',
#    meson_version: '>=1.3.0',
#    default_options: ['warning_level=3', 'c_std=c17']
#)

hwm_sources = [
    'wm.h',
    'window-manager.c'
]

# Create library
hawkwm_lib = shared_library(
    'hawkwm',
    hwm_sources,
    dependencies: [gtk_dep, x11_dep, pthread_dep, pango_dep],
    install: true,
    install_dir: get_option('libdir'),
    install_rpath: get_option('prefix') / get_option('libdir')
)

# Create symlink in standard lib directory
meson.add_install_script('sh', '-c', 
    'ln -sf "$DESTDIR"' + get_option('prefix') / 'share/hlib/libhawkwm.so' + 
    ' "$DESTDIR"' + get_option('prefix') / get_option('libdir') + '/libhawkwm.so')

hawkwm_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: hawkwm_lib
)