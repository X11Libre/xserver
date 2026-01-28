mouse_drv_srcs = [
    'src/mouse.c',
    'src/pnp.c'
]

if host_machine.system() in [ 'netbsd', 'dragonfly', 'freebsd', 'openbsd' ]
    mouse_drv_srcs += [ 'src/bsd_mouse.c' ]
elif host_machine.system() in [ 'linux' ]
    mouse_drv_srcs += [ 'src/lnx_mouse.c' ]
elif host_machine.system() in [ 'sunos' ]
    mouse_drv_srcs += [ 'src/sun_mouse.c' ]
elif host_machine.system() in [ 'hurd' ]
    mouse_drv_srcs += [ 'src/hurd_mouse.c' ]
else
    error('unsupported OS: '+host_machine.system())
endif

shared_module(
    'mouse_drv',
    mouse_drv_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc, 'include'],
    c_args: xorg_c_args,
    dependencies: [common_dep],

    install: true,
    install_dir: join_paths(module_abi_dir, 'input'),

    link_with: e,
)

install_man(configure_file(
    input: 'man/mousedrv.man',
    output: 'mousedrv.4',
    configuration: manpage_config,
))
