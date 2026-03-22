srcs_miext_sync = [
    'misync.c',
    'misyncfd.c',
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
