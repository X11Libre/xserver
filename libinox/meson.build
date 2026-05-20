srcs_libinox = []

# FIXME: also need Cygwin and Mingw32

if host_machine.system() == 'darwin'
    srcs_libinox += 'random/arc4random_buf.c'
else
    srcs_libinox += 'random/getrandom.c'
endif

libinox = static_library('libinox',
    srcs_libinox,
)
