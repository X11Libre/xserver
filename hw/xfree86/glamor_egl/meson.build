glamoregl_src = [
    'glamor_xf86_egl.c',
    'glamor_eglmodule.c',
    'glamor_xf86_xv.c',
]

shared_module(
    'glamoregl',
    glamoregl_src,

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: [
        common_dep,
        dependency('libdrm', version: '>= 2.4.46'),
        gbm_dep,
    ],
    link_with: [glamor, libxserver_glx],

    install: true,
    install_dir: module_abi_dir,
)
