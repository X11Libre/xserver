srcs_xf86bigfont = [
    'xf86bigfont.c',
]

libxserver_xf86bigfont = []
if build_xf86bigfont
    libxserver_xf86bigfont = static_library('xserver_xf86bigfont',
        srcs_xf86bigfont,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
