#
# run common display info tools like xdpyinfo
#
if get_option('xvfb')
    xvfb_args = [
        xvfb_server.full_path(),
        '-screen',
        'scrn',
        '1280x1024x24'
    ]

    testlist = [
        ['xdpyinfo', ['xdpyinfo', '-ext', 'all']],
        ['xrandr',   ['xrandr', '--verbose']],
        ['xvinfo',   ['xvinfo']],
    ]

    # Only keep tools that are actually installed: simple-xinit execs the client
    # by name at runtime, so a missing client (e.g. Cygwin CI ships none of
    # these) would make the test fail rather than skip. Filter them out up front.
    avail_tests = []
    foreach test_walk: testlist
        if find_program(test_walk[1][0], required: false).found()
            avail_tests += [test_walk]
        else
            message('display-info: skipping "' + test_walk[0] + '" — ' + test_walk[1][0] + ' not found')
        endif
    endforeach

    # These tests each start their own X server (Xvfb, or Xephyr nested inside
    # Xvfb) via simple-xinit -displayfd. When several of them start at the same
    # instant the servers race on the same display numbers ("/tmp/.X11-unix/Xn
    # already in use" / "server already running"), and the losing client ends up
    # with a display it cannot open ("unable to open display :N"). Run them
    # serialised so the display allocation can't collide. They are short, so the
    # lost parallelism is negligible.
    is_parallel_info = false

    # run tests on xvfb
    foreach test_walk: avail_tests
        test(test_walk[0],
            simple_xinit,
            args: [
                test_walk[1],
                '--',
                xvfb_args
            ],
            suite: 'xvfb display info',
            timeout: 300,
            is_parallel: is_parallel_info,
        )
    endforeach

    # run tests on xephyr inside xvfb
    if get_option('xephyr') and build_glamor
        foreach test_walk: avail_tests
            test(test_walk[0],
                simple_xinit,
                args: [
                    simple_xinit.full_path(),
                    test_walk[1],
                    '----',
                    xephyr_server.full_path(),
                    '-schedMax', '2000',
                    '--',
                    xvfb_args
                ],
                suite: 'xehypr info',
                timeout: 300,
                is_parallel: is_parallel_info,
            )
        endforeach
    endif
    if get_option('xephyr') and build_glamor
        foreach test_walk: avail_tests
            test(test_walk[0],
                simple_xinit,
                args: [
                    simple_xinit.full_path(),
                    test_walk[1],
                    '----',
                    xephyr_server.full_path(),
                    '-glamor',
                    '-schedMax', '2000',
                    '--',
                    xvfb_args
                ],
                suite: 'xehypr-glamor display info',
                timeout: 300,
                is_parallel: is_parallel_info,
            )
        endforeach
    endif
endif
