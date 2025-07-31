srcs_miext_sync = [
    'misync.c',
    'misyncfd.c',
]

hdrs_miext_sync = [
    'misync.h',
    'misyncfd.h',
    'misyncshm.h',
    'misyncstr.h',
]

if xshmfence_dep.found()
    srcs_miext_sync += 'misyncshm.c'
endif

libxserver_miext_sync = static_library('xserver_miext_sync',
    srcs_miext_sync,
    include_directories: inc,
    dependencies: [
        common_dep,
        xshmfence_dep,
    ],
)

if build_xorg
    install_data(hdrs_miext_sync, install_dir: xorgsdkdir)
endif
