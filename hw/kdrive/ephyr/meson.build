srcs = [
    'ephyr.c',
    'ephyrinit.c',
    'ephyrcursor.c',
    'ephyr_draw.c',
    'hostx.c',
]

xephyr_dep = [
    common_dep,
    dependency('xcb'),
    dependency('xcb-shape'),
    dependency('xcb-render'),
    dependency('xcb-renderutil'),
    dependency('xcb-aux'),
    dependency('xcb-image'),
    dependency('xcb-icccm'),
    dependency('xcb-shm', version : '>=1.9.3'),
    dependency('xcb-keysyms'),
    dependency('xcb-randr'),
    dependency('xcb-xkb'),
]

xephyr_glamor = []
if build_glamor
    srcs += 'ephyr_glamor.c'
    if build_xv
        srcs += 'ephyr_glamor_xv.c'
    endif
    xephyr_glamor += glamor
    xephyr_dep += epoxy_dep
    xephyr_dep += dependency('xcb-glx')
endif

if build_xv
    srcs += 'ephyrvideo.c'
    xephyr_dep += dependency('xcb-xv')
endif

# mingw needs this for nanosleep()
if host_machine.system() == 'windows'
    xephyr_dep += cc.find_library('pthread', required : true, static: true)
endif

xephyr_server = executable(
    'Xephyr',
    srcs,
    include_directories: [
        inc,
        include_directories('../src')
    ],
    dependencies: xephyr_dep,
    link_with: [
        libxserver_main,
        libxserver_exa,
        xephyr_glamor,
        kdrive,
        libxserver_fb,
        libxserver,
        libxserver_config,
        libxserver_xkb_stubs,
        libxserver_xi_stubs,
        libxserver_glx,
        libglxvnd,
    ],
    install: true,
)

xephyr_man = configure_file(
    input: 'man/Xephyr.man',
    output: 'Xephyr.1',
    configuration: manpage_config,
)
install_man(xephyr_man)
