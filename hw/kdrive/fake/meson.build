srcs = [
    'fake.c',
    'fakeinit.c',
    '../../stubs/ddxBeforeReset.c',
]

if build_glamor
    srcs += 'fake_glamor.c'
endif

xfake_server = executable(
    'Xfake',
    srcs,
    include_directories: [
        inc,
        include_directories('../src'),
        include_directories('../os/stub'),
    ],
    dependencies: common_dep,
    link_with: [
        libxserver_main,
        kdrive,
        stub,
        libxserver_fb,
        libxserver,
        libxserver_glx,
        libxserver_config,
        libxserver_xkeyboard_stubs,
        libglxvnd,
    ],
    install: true,
)
