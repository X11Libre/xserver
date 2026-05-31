# pytest-based test suite for the X server
#
# Uses pytest to launch X servers which allows us to e.g. send crafted protocol
# requests that exercise specific security fixes. Supports Xvfb, Xwayland,
# and Xorg, with optional valgrind and AddressSanitizer (ASAN) integration
# for detecting use-after-free and out-of-bounds memory access.
#
# Run with: meson test --suite pyxtest
# Or directly: pytest test/pyxtest/ -v

pymod = import('python')
pytest = find_program('pytest', 'pytest-3', required: false)

if pytest.found()
    pyxtest_env = environment()
    pyxtest_env.set('XSERVER_BUILDDIR', meson.project_build_root())
    pyxtest_env.set('XSERVER_DIR', meson.project_source_root())
    pyxtest_env.set('PYTHONDONTWRITEBYTECODE', '1')

    if build_xvfb
        pyxtest_env.set('XVFB_PATH', xvfb_server.full_path())
    endif

    # Tell the test suite if the server was built with AddressSanitizer
    if get_option('b_sanitize').contains('address')
        pyxtest_env.set('XSERVER_ASAN', '1')
    endif

    # We are *not* setting XORG_PATH in meson because we don't want
    # to start lots of Xorg instances as part of a meson test run.
    #
    # if build_xorg
    #     pyxtest_env.set('XORG_PATH', xorg_server.full_path())
    # endif

    pytest_args = [
        '-v',
        '--tb=short',
    ]

    # pytest-xdist speeds up the test suite by running tests in parallel,
    # but it's not required
    if pymod.find_installation('python3', modules: ['xdist'], required: false).found()
        pytest_args += ['-n', 'auto']
    endif

    # pytest-timeout means we can fail in pytest before hitting the meson limits
    if pymod.find_installation('python3', modules: ['pytest_timeout'], required: false).found()
        pytest_args += ['--timeout=120']
    endif

    # This needs to be kept in sync with the test_foo.py files in the tree
    tests_pyxtest = [
        'test_font.py',
        'test_glamor.py',
        'test_glx.py',
        'test_present.py',
        'test_randr.py',
        'test_record.py',
        'test_render.py',
        'test_screensaver.py',
        'test_shm.py',
        'test_sync.py',
        'test_vidmode.py',
        'test_xinerama.py',
        'test_xi.py',
        'test_xkb.py',
        'test_xres.py',
    ]

    test_list_data = configuration_data()
    test_list_data.set('TESTS', '\n'.join(tests_pyxtest))
    test_list_data.set('SOURCEDIR', meson.current_source_dir())
    test_list_check = configure_file(
        input: 'ensure-meson-tests.sh',
        output: 'ensure-meson-tests.sh',
        configuration: test_list_data,
    )
    test('ensure-meson-tests',
         test_list_check,
         suite:'pyxtest'
    )

    # As part of the normal meson test run we only run the Xvfb tests
    if build_xvfb
        foreach t: tests_pyxtest
            test(f'pyxtest-@t@',
            pytest,
            args: pytest_args + [files(t)],
            env: pyxtest_env,
            timeout: 600,
            suite: 'pyxtest',
            )
        endforeach
    endif
endif
