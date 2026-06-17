srcs_xv = [
    'xvmain.c',
    'xvdisp.c',
    'xvmc.c',
]

libxserver_xv = []
if build_xv
    libxserver_xv = static_library('xserver_xv',
        srcs_xv,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
