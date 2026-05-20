
inox_cc = meson.get_compiler('c')
inox_is_windows = host_machine.system() == 'windows'
inox_is_mingw = inox_is_windows and inox_cc.get_define('__MINGW32__') != ''
inox_is_darwin = host_machine.system() == 'darwin'
inox_is_cygwin = host_machine.system() == 'cygwin'

srcs_libinox = []
libs_libinox = []

subdir('random')

libinox = static_library('libinox',
    srcs_libinox,
    dependencies: libs_libinox,
)

libinox_dep = declare_dependency(
    link_with: libinox,
    dependencies: libs_libinox,
)
