project(
    'hawk-desktop', 'c',
    version: '1.0.0',
    meson_version: '>=1.3.0',
    default_options: [
        'warning_level=3',
        'c_std=c17',
        'libdir=/usr/lib/x86_64-linux-gnu'
    ]
)

# Add custom install script to handle library path
meson.add_install_script('sh', '-c', 'mkdir -p /etc/ld.so.conf.d && echo /usr/share/hlib > /etc/ld.so.conf.d/hawk.conf && ldconfig')

# Find dependencies - use GTK3 since that's what's installed
gtk_dep = dependency('gtk+-3.0')
x11_dep = dependency('x11')
pthread_dep = dependency('threads')
pango_dep = dependency('pango')

subdir('hawkwm')
# subdir('hawkdm')

sources = [
    'src/main.c',
]

# Create executable
executable(
    'hawk-session',
    sources,
    dependencies: [gtk_dep, x11_dep, pthread_dep, hawkwm_dep],
    install: true,
    install_dir: '/usr/bin'
)