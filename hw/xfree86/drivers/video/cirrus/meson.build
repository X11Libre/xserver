driver_video_cirrus_srcs = [
    'src/alp_driver.c',
    'src/alp_hwcurs.c',
    'src/alp_i2c.c',
    'src/cir_dga.c',
    'src/cir_driver.c',
    'src/CirrusClk.c',
    'src/cir_shadow.c',
    'src/lg_driver.c',
    'src/lg_hwcurs.c',
    'src/lg_i2c.c',
]

shared_module(
    'cirrus_drv',
    driver_video_cirrus_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
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
symbol_test_args += join_paths(xorg_build_root, 'drivers', 'video', 'cirrus', 'cirrus_drv.so')

test('video-cirrus symbol test',
    xorg_symbol_test,
    args: symbol_test_args,
)

install_man(configure_file(
    input: 'man/cirrus.man',
    output: 'cirrus.4',
    configuration: manpage_config,
))
