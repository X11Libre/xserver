driver_video_ast_srcs = [
    'src/ast_2dtool.c',
    'src/ast_accel.c',
    'src/ast_cursor.c',
    'src/ast_driver.c',
    'src/ast_mode.c',
    'src/ast_tool.c',
    'src/ast_vgatool.c'
]

shared_module(
    'ast_drv',
    driver_video_ast_srcs,
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
symbol_test_args += join_paths(xorg_build_root, 'drivers', 'video-ast', 'ast_drv.so')

test('video-ast symbol test',
    xorg_symbol_test,
    args: symbol_test_args,
)
