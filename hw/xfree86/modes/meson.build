srcs_xorg_modes = [
    'xf86Crtc.c',
    'xf86Cursors.c',
    'xf86gtf.c',
    'xf86EdidModes.c',
    'xf86Modes.c',
    'xf86RandR12.c',
    'xf86Rotate.c',
]

if build_dga
    srcs_xorg_modes += 'xf86DiDGA.c'
endif

xorg_modes = static_library('xorg_modes',
    srcs_xorg_modes,
    include_directories: [ inc, xorg_inc ],
    dependencies: [ common_dep, libxcvt_dep ],
    c_args: xorg_c_args,
)
