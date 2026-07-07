foreach src : files([
	'icon.svg',
	'wayland.png',
])
	name = fs.name(src)
	publican_inputs += fs.copyfile(
		name,
		install: true,
		install_dir: publican_install_prefix + '/html/images',
	)
endforeach
