srcs = [
    'fbdev.c',
    'fbinit.c',
    '../../stubs/ddxBeforeReset.c',
]

fbdev_glamor = []
fbdev_dep = []
if build_glamor and build_glx
    srcs += 'fb_glamor.c'
    fbdev_glamor += glamor
    fbdev_dep += epoxy_dep
endif

xfbdev_server = executable(
    'Xfbdev',
    srcs,
    include_directories: [
        inc,
        include_directories('../src'),
        include_directories('../linux'),
    ],
    dependencies: [ common_dep, fbdev_dep, ],
    link_with: [
        libxserver_main,
        fbdev_glamor,
        kdrive,
        linux,
        libxserver_fb,
        libxserver,
        libxserver_glx,
        libxserver_config,
        libxserver_xkb_stubs,
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
