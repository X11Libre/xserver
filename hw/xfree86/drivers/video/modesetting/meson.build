modesetting_srcs = [
    'dri2.c',
    'driver.c',
    'drmmode_display.c',
    'dumb_bo.c',
    'pageflip.c',
    'present.c',
    'vblank.c',
]

modesetting_drv_lib = shared_module(
    'modesetting_drv',
    modesetting_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: [
        common_dep,
        udev_dep,
        libdrm_dep,
        gbm_dep,
    ],

    install: true,
    install_dir: module_dir / 'drivers',
)

# Test that we don't have any unresolved symbols from our module to Xorg.
xorg_build_root = meson.project_build_root() / 'hw' / 'xfree86'
symbol_test_args = []
symbol_test_args += xorgserver_lib.full_path()
symbol_test_args += shadow_lib.full_path()
if build_glx
  symbol_test_args += glx_lib.full_path()
endif
if build_glamor
    symbol_test_args += glamoregl_lib.full_path()
endif
symbol_test_args += modesetting_drv_lib.full_path()

install_man(configure_file(
    input: 'modesetting.man',
    output: 'modesetting.4',
    configuration: manpage_config,
))

test('modesetting symbol test',
    xorg_symbol_test,
    args: symbol_test_args,
)
