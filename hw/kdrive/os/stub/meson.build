srcs_stub = [
    'kbd.c',
    'mouse.c',
    'os.c',
]

stub = static_library('stub',
    srcs_stub,
    include_directories: [
        inc,
        include_directories('../../src'),
    ],
    dependencies: common_dep,
)
