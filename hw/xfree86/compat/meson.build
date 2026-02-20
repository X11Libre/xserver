srcs_xorg_compat = [
    'clientexception.c',
    'log.c',
    'nvidiabug.c',
    'ones.c',
    'xf86Helper.c',
]

if get_option('legacy_nvidia_340x')
    srcs_xorg_compat += 'timercheck.c'
    srcs_xorg_compat += 'geeventinit.c'
endif

xorg_compat = static_library('xorg_compat',
    srcs_xorg_compat,
    dependencies: common_dep,
    include_directories: [inc, xorg_inc, top_dir_inc],
)

install_data('10-quirks.conf',
             install_dir: join_paths(get_option('datadir'), 'X11/xorg.conf.d'))
nvidia_conf = configuration_data()
nvidia_conf.set('libdir', join_paths(get_option('prefix'),get_option('libdir')))
configure_file(
    input: '10-nvidia.conf.in',
    output: '10-nvidia.conf',
    configuration: nvidia_conf,
    install_dir: join_paths(get_option('datadir'), 'X11/xorg.conf.d'),
)
