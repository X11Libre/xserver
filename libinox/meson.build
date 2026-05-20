
cc = meson.get_compiler('c')
platform_is_windows = host_machine.system() == 'windows'
platform_is_mingw = platform_is_windows and cc.get_define('__MINGW32__') != ''
platform_is_darwin = host_machine.system() == 'darwin'
platform_is_cygwin = host_machine.system() == 'cygwin'

srcs_libinox = []
libs_libinox = []

subdir('random')

libinox = static_library('libinox',
    srcs_libinox,
    dependencies: libs_libinox,
)
