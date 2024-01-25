#hdrs_miext_namespace = [
#	'damage.h',
#	'damagestr.h',
#]

srcs_miext_namespace = [
	'namespace.c',
	'ns.c',
	'clientpriv.c',
	'hook-receive.c',
	'hook-clientstate.c',
	'hook-send.c',
]

libxserver_miext_namespace = static_library('libxserver_miext_namespace',
	srcs_miext_namespace,
	include_directories: inc,
	dependencies: common_dep,
)

# if build_xorg
#    install_data(hdrs_miext_namespace, install_dir: xorgsdkdir)
# endif
