if is_mingw
    srcs_libinox += 'random/windows.c'
elif is_darwin
    srcs_libinox += 'random/arc4random_buf.c'
else
    srcs_libinox += 'random/getrandom.c'
endif
