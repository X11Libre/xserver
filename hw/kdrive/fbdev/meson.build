srcs = [
    'fbdev.c',
    'fbinit.c',
    '../../stubs/ddxBeforeReset.c',
]

if build_glamor
    srcs += 'fb_glamor.c'
endif

xfbdev_server = executable(
    'Xfbdev',
    srcs,
    include_directories: [
        inc,
        include_directories('../src'),
        include_directories('../os/linux'),
    ],
    dependencies: common_dep,
    link_with: [
        libxserver_main,
        kdrive,
        linux,
        libxserver_fb,
        libxserver,
        libxserver_glx,
        libxserver_config,
        libxserver_xkeyboard_stubs,
        libglxvnd,
    ],
    install: true,
)

xfbdev_man = configure_file(
    input: 'Xfbdev.man',
    output: 'Xfbdev.1',
    configuration: manpage_config,
)
install_man(xfbdev_man)
