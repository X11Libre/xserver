srcs_dpms = [
    'dpms.c',
]

libxserver_dpms = []
if build_dpms
    libxserver_dpms = static_library('xserver_dpms',
        srcs_dpms,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
