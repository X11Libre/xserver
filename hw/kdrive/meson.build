subdir('src')
subdir('ephyr')

xkdrive_man = configure_file(
    input: 'Xkdrive.man',
    output: 'Xkdrive.1',
    configuration: manpage_config,
)
install_man(xkdrive_man)
