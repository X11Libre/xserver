srcs_dri2 = [
    'dri2.c',
    'dri2ext.c',
]


libxserver_dri2 = []
if build_dri2
libxserver_dri2 = static_library('xserver_dri2',
    srcs_dri2,
    include_directories: inc,
    dependencies: [ common_dep, libdrm_dep ],
)
endif

if build_xorg
install_data('dri2.h', install_dir: xorgsdkdir)
endif
