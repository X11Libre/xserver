srcs_panoramiX = [
    'panoramiX.c',
    'panoramiXprocs.c',
]

libxserver_xinerama = []
if build_xinerama
    libxserver_xinerama = static_library('xserver_xinerama',
        srcs_panoramiX,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
