srcs_nexus = [
    'nexus.c',
]

libxserver_nexus = static_library('xserver_nexus',
    srcs_nexus,
    include_directories: inc,
    dependencies: common_dep,
)

nexus_dep = declare_dependency(
    link_with: libxserver_nexus,
    include_directories: inc,
    dependencies: common_dep,
)