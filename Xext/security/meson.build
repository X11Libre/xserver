srcs_security = [
    'security.c',
]

libxserver_security = []
if build_xsecurity
    libxserver_security = static_library('xserver_security',
        srcs_security,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
