modesetting_srcs = [
    'src/vesa.c',
]

shared_module(
    'vesa_drv',
    modesetting_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc, 'src'],
    c_args: xorg_c_args,
    dependencies: [
        common_dep,
    ],

    install: true,
    install_dir: join_paths(module_abi_dir, 'video'),
)

install_man(configure_file(
    input: 'man/vesa.man',
    output: 'vesa.4',
    configuration: manpage_config,
))

# Test that we don't have any unresolved symbols from our module to XLibre.
xorg_build_root = join_paths(meson.project_build_root(), 'hw', 'xfree86')
symbol_test_args = []
symbol_test_args += join_paths(xorg_build_root, 'libxorgserver.so')
symbol_test_args += join_paths(xorg_build_root, 'dixmods', 'libshadow.so')
symbol_test_args += join_paths(xorg_build_root, 'int10', 'libint10.so')
symbol_test_args += join_paths(xorg_build_root, 'drivers', 'video', 'vesa', 'vesa_drv.so')

test('vesa driver symbol test',
    xorg_symbol_test,
    args: symbol_test_args,
)
