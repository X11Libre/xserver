foreach src : files([
	'common.css',
	'default.css',
])
	name = fs.name(src)
	publican_inputs += fs.copyfile(
		name,
		install: true,
		install_dir: publican_install_prefix + '/html/css',
	)
endforeach
