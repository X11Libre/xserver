shared_module(
    'geode_drv',
    [
        'src/cimarron.c',
        'src/durango.c',
        'src/geode_common.c',
        'src/geode_dcon.c',
        'src/geode_ddc.c',
        'src/geode_driver.c',
        'src/geode_msr.c',
        'src/lx_cursor.c',
        'src/lx_display.c',
        'src/lx_driver.c',
        'src/lx_exa.c',
        'src/lx_memory.c',
        'src/lx_output.c',
        'src/lx_panel.c',
        'src/lx_video.c',
        'src/panel.c',

        'src/gx_cursor.c',
        'src/gx_driver.c',
        'src/gx_exa.c',
        'src/gx_randr.c',
        'src/gx_regacc.c',
        'src/gx_rotate.c',
        'src/gx_video.c',
    ],
    name_prefix: '',
    include_directories: [
        inc,
        xorg_inc,
        include_directories(['src', 'src/gfx', 'src/panel', 'src/cim'])
    ],
    c_args: [xorg_c_args, '-DHAVE_LX', '-DPNL_SUP', '-DHAVE_GX', '-DOPT_ACCEL'],
    dependencies: [
        common_dep,
    ],
    install: true,
    install_dir: join_paths(module_abi_dir, 'drivers'),
)

# Test that we don't have any unresolved symbols from our module to Xorg.
xorg_build_root = join_paths(meson.project_build_root(), 'hw', 'xfree86')
symbol_test_args = []
symbol_test_args += join_paths(xorg_build_root, 'libxorgserver.so')
symbol_test_args += join_paths(xorg_build_root, 'dixmods', 'libshadow.so')
symbol_test_args += join_paths(xorg_build_root, 'dixmods', 'libglx.so')
symbol_test_args += join_paths(xorg_build_root, 'drivers', 'video', 'geode', 'geode_drv.so')

test('video-geode symbol test',
    xorg_symbol_test,
    args: symbol_test_args,
)

shared_module(
    'ztv_drv',
    [
        'src/z4l.c',
    ],
    name_prefix: '',
    include_directories: [
        inc,
        xorg_inc,
        include_directories(['src', 'src/gfx', 'src/panel', 'src/cim'])
    ],
    c_args: [xorg_c_args, '-DHAVE_LX', '-DPNL_SUP', '-DHAVE_GX', '-DOPT_ACCEL'],
    dependencies: [
        common_dep,
    ],
    install: true,
    install_dir: join_paths(module_abi_dir, 'drivers'),
)
