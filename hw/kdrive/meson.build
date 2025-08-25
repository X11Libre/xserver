build_xephyr = get_option('xephyr')
build_kdrive = build_xephyr

build_kdrive_kbd = false
build_kdrive_mouse = false
build_kdrive_evdev = false
build_kdrive_tslib = false

if build_kdrive
    subdir('src')
endif

if build_xephyr
    subdir('ephyr')
endif
