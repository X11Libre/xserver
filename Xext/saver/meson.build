srcs_saver = [
    'byteswap.c',
    'saver.c'
]

libxserver_saver = []
if build_screensaver
    libxserver_saver = static_library('xserver_saver',
        srcs_saver,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
