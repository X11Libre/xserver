srcs_linux = [
    'linux.c'
]

if build_kdrive_kbd
    srcs_linux += 'keyboard.c'
endif

if build_kdrive_mouse
    srcs_linux += 'mouse.c'
    srcs_linux += 'ms.c'
    srcs_linux += 'ps2.c'
endif

if build_kdrive_evdev
    srcs_linux += 'evdev.c'
endif

if build_kdrive_tslib
    srcs_linux += 'tslib.c'
endif

linux = static_library('linux',
    srcs_linux,
    include_directories: [
        inc,
        include_directories('../src'),
    ],
    dependencies: common_dep,
)
