if inox_is_mingw
    srcs_libinox += 'random/windows.c'
    libs_lininox += compiler.find_library('bcrypt')
elif inox_is_darwin
    srcs_libinox += 'random/arc4random_buf.c'
else
    srcs_libinox += 'random/getrandom.c'
endif
