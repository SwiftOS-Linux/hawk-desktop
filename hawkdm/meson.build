#project(
#    'hawkdm', 'c',
#    version: '1.0.0',
#    meson_version: '>=1.3.0',
#    default_options: ['warning_level=3', 'c_std=c17']
#)

hdm_sources = [
    'scall.h',
    'scall.c'
]

# Create library
hawkdm_lib = shared_library(
    'hawkdm',
    hdm_sources,
    dependencies: [gtk_dep, x11_dep, pthread_dep, pango_dep],
    install: true,
    install_dir: '/usr/share/hlib'
)

hawkdm_dep = declare_dependency(
    include_directories: include_directories('.'),
    link_with: hawkdm_lib
)