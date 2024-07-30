srcs = [
    'Args.c',
    'Color.c',
    'Cursor.c',
    'Display.c',
    'Events.c',
    'Font.c',
    'GC.c',
    'GCOps.c',
    'Handlers.c',
    'Init.c',
    'Keyboard.c',
    'Pixmap.c',
    'Pointer.c',
    'Screen.c',
    'Visual.c',
    'Window.c',
    '../../mi/miinitext.c',
    '../../mi/miinitext.h',
    'xcb.c',
]

x11_xcb_dep = dependency('x11-xcb', required: true)
xcb_dep = dependency('xcb', required: true)
xcb_aux_dep = dependency('xcb-aux', required: true)

executable(
    'Xnest',
    srcs,
    include_directories: inc,
    dependencies: [
        common_dep,
        xnest_dep,
        xcb_dep,
        xcb_aux_dep,
        x11_xcb_dep,
    ],
    link_with: [
        libxserver_main,
        libxserver,
        libxserver_xi_stubs,
        libxserver_xkb_stubs,
    ],
    c_args: [ '-DHAVE_XNEST_CONFIG_H', '-DDISABLE_EXT_COMPOSITE', '-DDISABLE_EXT_DPMS', '-DISABLE_EXT_MITSHM' ],
    install: true,
)

install_man(configure_file(
    input: 'man/Xnest.man',
    output: 'Xnest.1',
    configuration: manpage_config,
))
