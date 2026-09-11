srcs_dri3 = [
    'dri3.c',
    'dri3_request.c',
    'dri3_screen.c',
]

dri3_deps = [ common_dep, libdrm_dep ]

if build_lttngust
    srcs_dri3 += [ 'dri3_tp.c' ]
    dri3_deps += [ lttng_ust_dep ]
endif

libxserver_dri3 = []
if build_dri3
    libxserver_dri3 = static_library('xserver_dri3',
        srcs_dri3,
        include_directories: inc,
        dependencies: dri3_deps,
        )
endif
