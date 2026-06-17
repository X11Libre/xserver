srcs_xres = [
    'xres.c',
]

libxserver_xres = []
if build_res
    libxserver_xres = static_library('xserver_xres',
        srcs_xres,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
