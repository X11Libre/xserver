# Driver ABI check tools, installed with the SDK (see README.md).

abi_shell_scripts = [
    'abigail-compare.sh',
    'build-driver.sh',
    'build-server.sh',
    'check-symbols.sh',
    'gate-driver.sh',
    'run-matrix.sh',
    'staged-pc.sh',
]

if build_xorg_sdk
    abi_tools_dir = get_option('datadir') / 'xorg-server' / 'abi'
    install_data(abi_shell_scripts + ['check-private-includes.py'],
                 install_dir: abi_tools_dir,
                 install_mode: 'rwxr-xr-x')
    install_data('common.sh', 'README.md',
                 install_dir: abi_tools_dir,
                 install_mode: 'rw-r--r--')
endif

if get_option('tests')
    sh = find_program('sh')
    foreach script : abi_shell_scripts + ['common.sh']
        test('abi-tools-syntax-' + script, sh,
             args: ['-n', files(script)],
             suite: 'abi-tools')
    endforeach
    python3 = find_program('python3', required: false)
    if python3.found()
        test('abi-tools-syntax-check-private-includes.py', python3,
             args: ['-c', 'import ast, sys; ast.parse(open(sys.argv[1]).read())',
                    files('check-private-includes.py')],
             suite: 'abi-tools')
    endif
endif
