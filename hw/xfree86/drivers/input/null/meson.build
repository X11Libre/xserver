nullinputdrv_srcs = [
    'null.c',
]

shared_module(
    'nullinput_drv',
    nullinputdrv_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: [common_dep],

    install: true,
    install_dir: join_paths(module_abi_dir, 'input'),

    link_with: xserver_exec,
)
