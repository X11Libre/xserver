# RemoteDesktop Extension

srcs_remotedesktop = [
    'remotedesktopext.c',
]

vnc_deps = []
vnc_c_args = ['-DHAVE_REMOTE_DESKTOP']

if get_option('remote_desktop') == 'true' or get_option('remote_desktop') == 'auto'
    srcs_remotedesktop += [
        'vnc/vnc_backend.c',
    ]
    
    # OpenSSL for VNC TLS (optional - VNC works without it)
    openssl_dep = dependency('openssl', required: false)
    if openssl_dep.found()
        vnc_deps += openssl_dep
        vnc_c_args += ['-DHAVE_OPENSSL']
        message('VNC backend: OpenSSL found, TLS support enabled')
    else
        message('VNC backend: OpenSSL not found, TLS support disabled')
    endif
endif

libxserver_remotedesktop = static_library('xserver_remotedesktop',
    srcs_remotedesktop,
    include_directories: inc,
    dependencies: common_dep + vnc_deps,
    c_args: vnc_c_args,
)

# Install protocol header
install_headers('remotedesktopproto.h',
    subdir: 'X11/extensions',
)

# Install VNC backend header
install_headers('vnc/vnc_backend.h',
    subdir: 'X11/extensions/remotedesktop',
)