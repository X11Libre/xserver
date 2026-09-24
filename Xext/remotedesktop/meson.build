# RemoteDesktop Extension

srcs_remotedesktop = [
    'remotedesktopext.c',
]

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