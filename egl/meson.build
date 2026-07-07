wayland_egl = library(
	'wayland-egl',
	sources: [
		'wayland-egl.c',
		wayland_client_protocol_h
	],
	include_directories: src_inc,
	version: meson.project_version(),
	install: true
)

if get_option('tests')
	wayland_egl_abi_check = executable('wayland-egl-abi-check', 'wayland-egl-abi-check.c')
	test('wayland-egl abi check', wayland_egl_abi_check)

	if get_option('default_library') != 'static'
		nm_path = find_program(['llvm-nm', 'nm']).full_path()
		wayland_egl_shared = wayland_egl
		if get_option('default_library') == 'both'
			wayland_egl_shared = wayland_egl.get_shared_lib()
		endif
		test(
			'wayland-egl symbols check',
			find_program('wayland-egl-symbols-check'),
			depends: wayland_egl,
			env: [
				'WAYLAND_EGL_LIB=@0@'.format(wayland_egl_shared.full_path()),
				'NM=@0@'.format(nm_path)
			]
		)
	endif
endif

install_headers([
	'wayland-egl.h',
	'wayland-egl-core.h',
	'wayland-egl-backend.h'
])

pkgconfig.generate(
	name: 'wayland-egl',
	description: 'Frontend wayland-egl library',
	version: '18.1.0',
	requires: 'wayland-client',
	libraries: wayland_egl
)

pkgconfig.generate(
	name: 'wayland-egl-backend',
	description: 'Backend wayland-egl interface',
	version: '3'
)

wayland_egl_dep = declare_dependency(
	link_with: wayland_egl,
	include_directories: [ root_inc, include_directories('.') ],
)

if meson.version().version_compare('>= 0.54.0')
	meson.override_dependency('wayland-egl', wayland_egl_dep)
endif
