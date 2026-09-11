srcs_config = [
    'config.c',
]

config_dep = [common_dep]


if build_udev
    srcs_config += 'udev.c'
    config_dep += udev_dep
endif

if host_machine.system() == 'netbsd' or host_machine.system() == 'openbsd'
    srcs_config += 'wscons.c'
endif

libxserver_config = static_library('xserver_config',
    srcs_config,
    include_directories: inc,
    dependencies: config_dep,
)
