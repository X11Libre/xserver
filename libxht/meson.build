xht_c_args = ['-DHAVE_CONFIG_H']

xht_headers = files('xht.h')
xht_sources = files('xht.c')

libxht = static_library('xht',
  xht_sources,
  c_args : xht_c_args,
  install : true,
)

install_headers(xht_headers, subdir : 'xorg')

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(
    name: 'libxht',
    version: meson.project_version(),
    libraries: libxht,
    description: 'X server hashtable library',
)
