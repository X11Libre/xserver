# RemoteDesktop Extension

srcs_remotedesktop = [
    'remotedesktopext.c',
]

if get_option('remote_desktop') == 'true' or get_option('remote_desktop') == 'auto'
    srcs_remotedesktop += [
        'vnc/vnc_backend.c',
    ]
endif

libxserver_remotedesktop = static_library('xserver_remotedesktop',
    srcs_remotedesktop,
    include_directories: inc,
    dependencies: common_dep,
    c_args: ['-DHAVE_REMOTE_DESKTOP'],
)

# Install protocol header
install_headers('remotedesktopproto.h',
    subdir: 'X11/extensions',
)

# Install VNC backend header
install_headers('vnc/vnc_backend.h',
    subdir: 'X11/extensions/remotedesktop',
)