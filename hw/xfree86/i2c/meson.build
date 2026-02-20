srcs_xorg_i2c = [
    'xf86i2c.c'
]

xorg_i2c = static_library('xorg_i2c',
    srcs_xorg_i2c,
    include_directories: [inc, xorg_inc],
    dependencies: common_dep,
    c_args: xorg_c_args,
)
