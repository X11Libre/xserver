srcs_kdrive = [
    'kcmap.c',
    'kdrive.c',
    'kinfo.c',
    'kinput.c',
    'kmode.c',
    'kshadow.c',
    '../../../mi/miinitext.c',
    '../../../mi/miinitext.h',
]

if build_xv
    srcs_kdrive += 'kxv.c'
    if build_glamor
        srcs_kdrive += 'kd_glamor_xv.c'
    endif
endif

kdrive_data = configuration_data()

kdrive_data.set('KDRIVE_KBD', build_kdrive_kbd ? '1' : false)
kdrive_data.set('KDRIVE_MOUSE', build_kdrive_mouse ? '1' : false)
kdrive_data.set('KDRIVE_EVDEV', build_kdrive_evdev ? '1' : false)
kdrive_data.set('KDRIVE_TSLIB', build_kdrive_tslib ? '1' : false)

configure_file(input : 'kdrive-config.h.meson.in',
               output : 'kdrive-config.h',
               configuration : kdrive_data)

kdrive = static_library('kdrive',
    srcs_kdrive,
    include_directories: inc,
    dependencies: common_dep,
    link_with: libxserver_miext_shadow,
)
