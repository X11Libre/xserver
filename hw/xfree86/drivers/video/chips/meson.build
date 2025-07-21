driver_video_chips_srcs = [
    'src/ct_bank.c',
    'src/ct_cursor.c',
    'src/ct_dga.c',
    'src/ct_shadow.c',
    'src/ct_accel.c',
    'src/ct_ddc.c',
    'src/ct_driver.c',
    'src/ct_exa.c',
    'src/ct_regs.c',
    'src/ct_video.c'
]

shared_module(
    'chips_drv',
    driver_video_chips_srcs,
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
symbol_test_args += join_paths(xorg_build_root, 'drivers', 'video-chips', 'chips_drv.so')

test('video-chips symbol test',
    xorg_symbol_test,
    args: symbol_test_args,
)

install_man(configure_file(
    input: 'man/chips.man',
    output: 'chips.4',
    configuration: manpage_config,
))

subdir('util')
