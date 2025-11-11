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
