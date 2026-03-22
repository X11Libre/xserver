build_kdrive = build_xephyr or build_xfbdev

build_kdrive_kbd = false
build_kdrive_mouse = false
build_kdrive_evdev = false
build_kdrive_tslib = false

if build_xfbdev
    build_kdrive_kbd = get_option('kdrive_kbd')
    build_kdrive_mouse = get_option('kdrive_mouse')
    build_kdrive_evdev = get_option('kdrive_evdev')
    build_kdrive_tslib = get_option('kdrive_tslib')
    tslib_dep = dependency('tslib', required : build_kdrive_tslib)
endif

if build_kdrive
    subdir('src')
endif

if build_xephyr
    subdir('ephyr')
endif

if build_xfbdev
    subdir('linux')
    subdir('fbdev')
endif

if build_kdrive
    xkdrive_man = configure_file(
        input: 'Xkdrive.man',
        output: 'Xkdrive.1',
        configuration: manpage_config,
    )
    install_man(xkdrive_man)
endif
