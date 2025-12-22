shared_module('shadowfb',
    [ 'shadowfb.c', 'sfbmodule.c'],
    include_directories: [ inc, xorg_inc ],
    dependencies: common_dep,
    c_args: xorg_c_args,
    install: true,
    install_dir: module_abi_dir,
    link_with: xserver_exec,
)

install_data('shadowfb.h', install_dir: xorgsdkdir)
