libcglcore = static_library('CGLCore',
     ['indirect.c', 'capabilities.c', 'visualConfigs.c'],
     c_args: ['-DGL_SILENCE_DEPRECATION'],
     include_directories: [inc, '..', '../xpr'],
     dependencies: [xproto_dep, pixman_dep],
)
