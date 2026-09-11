srcs_damageext = [
	'damageext.c',
]

damageext_deps = [ common_dep ]

if build_lttngust
    srcs_damageext += [ 'damageext_tp.c' ]
    damageext_deps += [ lttng_ust_dep ]
endif


libxserver_damageext = static_library('xserver_damageext',
	srcs_damageext,
	include_directories: inc,
	dependencies: damageext_deps,
)
