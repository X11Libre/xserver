srcs_xorg_compat = [
    'clientexception.c',
    'log.c',
    'nvidiabug.c',
    'ones.c',
    'timercheck.c',
    'xf86Helper.c',
]

xorg_compat = static_library('xorg_compat',
    srcs_xorg_compat,
    dependencies: common_dep,
    include_directories: [inc, xorg_inc, top_dir_inc],
)
