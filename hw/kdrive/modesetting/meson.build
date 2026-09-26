srcs = [
    'modesetting.c',
    'msinit.c',
    'ms_cursor.c',
    'ms_damage.c',
    'ms_gamma.c',
    'ms_gbm.c',
    'ms_query.c',
    'ms_randr.c',
    'ms_util.c',
    '../../stubs/ddxBeforeReset.c',
]

if build_glamor
    srcs += 'ms_glamor.c'
endif

xmodesetting_server = executable(
    'Xmodesetting',
    srcs,
    include_directories: [
        inc,
        include_directories('../src'),
        include_directories('../os/linux'),
        include_directories('../os/stub'),
    ],
    dependencies: [ common_dep, libdrm_dep, gbm_dep ],
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

xmodesetting_man = configure_file(
    input: 'Xmodesetting.man',
    output: 'Xmodesetting.1',
    configuration: manpage_config,
)
install_man(xmodesetting_man)
