srcs_randr = [
    'randr.c',
    'rrcrtc.c',
    'rrdispatch.c',
    'rrinfo.c',
    'rrlease.c',
    'rrmode.c',
    'rrmonitor.c',
    'rroutput.c',
    'rrpointer.c',
    'rrproperty.c',
    'rrprovider.c',
    'rrproviderproperty.c',
    'rrscreen.c',
    'rrtransform.c',
]

if build_xinerama
    srcs_randr += 'rrxinerama.c'
endif

libxserver_randr = static_library('xserver_randr',
    srcs_randr,
    include_directories: inc,
    dependencies: common_dep,
)
