# SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
#
# Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>

hurdinputdrv_srcs = [
    'xf86-input-hurd.c',
]

shared_module(
    'hurdinput_drv',
    hurdinputdrv_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: [common_dep],

    install: true,
    install_dir: join_paths(module_abi_dir, 'input'),

    link_with: xserver_exec,
)
