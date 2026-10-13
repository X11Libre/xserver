if get_option('xf86-input-inputtest')
    subdir('inputtest')
endif

if get_option('xf86-input-null')
    subdir('null')
endif

if host_machine.system() == 'gnu' and get_option('xf86-input-hurd')
    subdir('hurd')
endif
