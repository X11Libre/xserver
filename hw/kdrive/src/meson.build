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
endif

kdrive_data = configuration_data()

configure_file(input : 'kdrive-config.h.meson.in',
               output : 'kdrive-config.h',
               configuration : kdrive_data)

kdrive = static_library('kdrive',
    srcs_kdrive,
    include_directories: inc,
    dependencies: common_dep,
    link_with: libxserver_miext_shadow,
)
