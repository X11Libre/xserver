srcs_linux = [
    'linux.c'
]

kdrive_linux_dep = common_dep

if build_kdrive_kbd
    srcs_linux += 'keyboard.c'
endif

if build_kdrive_mouse
    srcs_linux += 'mouse.c'
    srcs_linux += 'ms.c'
    srcs_linux += 'ps2.c'
endif

if build_kdrive_evdev
    srcs_linux += 'evdev/evdev.c'
    srcs_linux += 'evdev/evdev_autodetect.c'
endif

if build_kdrive_tslib
    srcs_linux += 'tslib.c'
    kdrive_linux_dep += tslib_dep
endif

linux = static_library('linux',
    srcs_linux,
    include_directories: [
        inc,
        include_directories('../src'),
    ],
    dependencies: kdrive_linux_dep,
)
