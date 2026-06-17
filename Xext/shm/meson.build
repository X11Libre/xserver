srcs_shm = [
    'shm.c',
]

libxserver_shm = []
if build_mitshm
    libxserver_shm = static_library('xserver_shm',
        srcs_shm,
        include_directories: inc,
        dependencies: common_dep,
    )
endif
