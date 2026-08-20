srcs_composite = [
    'compalloc.c',
    'compext.c',
    'compinit.c',
    'compoverlay.c',
    'compwindow.c',
]

compext_deps = common_dep

if build_lttngust
    srcs_composite += [ 'compext_tp.c' ]
    compext_deps += [ lttng_ust_dep ]
endif



libxserver_composite = static_library('xserver_composite',
    srcs_composite,
    include_directories: inc,
    dependencies: compext_deps,
)
