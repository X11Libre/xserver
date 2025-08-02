driver_video_amdgpu_srcs = [
    'src/amdgpu_bo_helper.c',
    'src/amdgpu_dri2.c',
    'src/amdgpu_dri3.c',
    'src/amdgpu_drm_queue.c',
    'src/amdgpu_glamor_wrappers.c',
    'src/amdgpu_glamor.c',
    'src/amdgpu_kms.c',
    'src/amdgpu_misc.c',
    'src/amdgpu_pixmap.c',
    'src/amdgpu_present.c',
    'src/amdgpu_probe.c',
    'src/amdgpu_sync.c',
    'src/amdgpu_video.c',
    'src/drmmode_display.c',
]

shared_module(
    'amdgpu_drv',
    driver_video_amdgpu_srcs,
    name_prefix: '',

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: [
        common_dep,
        dependency('libdrm')
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
symbol_test_args += join_paths(xorg_build_root, 'drivers', 'video-amdgpu', 'andgpu_drv.so')

test('video-amdgpu symbol test',
    xorg_symbol_test,
    args: symbol_test_args,
)

install_man(configure_file(
    input: 'man/amdgpu.man',
    output: 'amdgpu.4',
    configuration: manpage_config,
))
