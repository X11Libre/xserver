srcs_dri3 = [
    'dri3.c',
    'dri3_request.c',
    'dri3_screen.c',
]

libxserver_dri3 = []
if build_dri3
    libxserver_dri3 = static_library('xserver_dri3',
        srcs_dri3,
        include_directories: inc,
        dependencies: [ common_dep, libdrm_dep ],
        )
endif
