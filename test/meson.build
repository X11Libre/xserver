simple_xinit = executable(
    'simple-xinit',
    'simple-xinit.c',
    include_directories: inc,
    dependencies: [ xproto_dep ],
)

piglit_env = environment()
piglit_env.set('XSERVER_DIR', meson.project_source_root())
piglit_env.set('XSERVER_BUILDDIR', meson.project_build_root())

gles20_env = environment()
gles20_env.set('XSERVER_DIR', meson.project_source_root())
gles20_env.set('XSERVER_BUILDDIR', meson.project_build_root())
gles20_env.set('MESA_GLES_VERSION_OVERRIDE', '2.0')

some_ops = ' -o clear,src,dst,over,xor,disjointover'
gles2_working_formats = ' -f '+ ','.join(['a8',
                                          'a8r8g8b8',
                                          'x8r8g8b8',
                                          'b8g8r8a8',
                                          'b8g8r8x8',
                                          'r8g8b8',
                                          'r5g5b5',
                                          'b5g5r5',
                                          'r5g6b5',
                                          'b5g6r5',
                                          'b8g8r8',
                                          'x8b8g8r8',
                                          'x2r10g10b10',
                                          'x2b10g10r10'])
rendercheck_tests_noblend = [
    ['blend/All/a8r8g8b8', '-t blend -f a8r8g8b8'],
    ['blend/All/x8r8g8b8', '-t blend -f a8r8g8b8,x8r8g8b8'],
    ['blend/All/a2r10g10b10', '-t blend -f a8r8g8b8,a2r10g10b10'],
    ['composite/Some/a8r8g8b8', '-t composite -f a8r8g8b8' + some_ops],
    ['composite/Some/x8r8g8b8', '-t composite -f a8r8g8b8,x8r8g8b8' + some_ops],
    ['composite/Some/a2r10g10b10', '-t composite -f a8r8g8b8,a2r10g10b10' + some_ops],
    ['ca composite/Some/a8r8g8b8', '-t cacomposite -f a8r8g8b8' + some_ops],
    ['ca composite/Some/x8r8g8b8', '-t cacomposite -f a8r8g8b8,x8r8g8b8' + some_ops],
    ['ca composite/Some/a2r10g10b10', '-t cacomposite -f a8r8g8b8,a2r10g10b10' + some_ops],
    ['fill', '-t fill'],
    ['bug7366', '-t bug7366'],
    ['destination coordinates', '-t dcoords'],
    ['source coordinates', '-t scoords'],
    ['mask coordinates', '-t mcoords'],
    ['translated source coordinates', '-t tscoords'],
    ['translated mask coordinates', '-t tmcoords'],
    ['LibreOffice xRGB', '-t libreoffice_xrgb'],
    ['GTK ARGB vs xBGR', '-t gtk_argb_xbgr'],
]

if get_option('test_rendercheck_triangles')
    message('testsuite: rendercheck triangles enabled - expected to fail on Xephyr')
    rendercheck_tests_noblend += [ ['triangles', '-t triangles'] ]
else
    message('testsuite: rendercheck triangles disabled')
endif

rendercheck_blend = [
    ['blend/Clear', '-t blend -o clear'],
    ['blend/Src', '-t blend -o src'],
    ['blend/Over', '-t blend -o over'],
]
#A8 test failing on GLES 2.0 llvmpipe before mesa 23.
rendercheck_a8 = [
    ['ca composite/Some/a8', '-t cacomposite -f a8r8g8b8,a8' + some_ops],
]
#Exclude 15bpp for now due to GLES limitation (see glamor.c:470)
rendercheck_blend_gles2 = [
    ['blend/Clear', '-t blend -o clear' + gles2_working_formats],
    ['blend/Src', '-t blend -o src' + gles2_working_formats],
    ['blend/Over', '-t blend -o over' + gles2_working_formats],
]
rendercheck_tests = rendercheck_blend + rendercheck_tests_noblend + rendercheck_a8
rendercheck_tests_gles2_success = rendercheck_blend_gles2 + rendercheck_tests_noblend
rendercheck_tests_gles3 = rendercheck_blend_gles2 + rendercheck_tests_noblend + rendercheck_a8
rendercheck = find_program('rendercheck', required:false)

subdir('info')

if get_option('xvfb')
    xvfb_args = [
        xvfb_server.full_path(),
        '-screen',
        'scrn',
        '1280x1024x24',
    ]

    test('XTS',
        find_program('scripts/xvfb-piglit.sh'),
        env: piglit_env,
        timeout: 1200,
        suite: 'xvfb'
    )

    if rendercheck.found()
        foreach rctest: rendercheck_tests
            test(rctest[0],
                 simple_xinit,
                 # Use full_path so people can copy and paste the
                 # command line from testlog.txt easily.
                 args: [
                     rendercheck.full_path(),
                     rctest[1].split(' '),
                     '--',
                     xvfb_args,
                 ],
                 suite: 'xvfb'
                )
        endforeach
    endif

    if get_option('xephyr') and build_glamor
        foreach testsuite : ['','-gles2','-gles3']
            test_env = piglit_env
            if(testsuite == '-gles2')
                test_env = gles20_env
            endif
            test('XTS',
                find_program('scripts/xephyr-glamor' + testsuite + '-piglit.sh'),
                env: piglit_env,
                timeout: 1200,
                suite: 'xephyr-glamor' + testsuite,
            )
        endforeach
        test_parameters = [
                [rendercheck_tests, '', piglit_env, '', false],
                [rendercheck_tests_gles2_success, '_gles2', gles20_env, '-gles2', false],
                [rendercheck_a8, '_gles2', gles20_env, '-gles2', true],
                [rendercheck_tests_gles3, '_gles2', piglit_env, '-gles3', false]
            ]

        if rendercheck.found()
            foreach testsuite : test_parameters
                foreach rctest : testsuite[0]
                    test(rctest[0],
                        simple_xinit,
                        args: [simple_xinit.full_path(),
                                rendercheck.full_path(),
                                rctest[1].split(' '),
                                '----',
                                xephyr_server.full_path(),
                                '-glamor' + testsuite[1],
                                '-glamor-skip-present',
                                '-schedMax', '2000',
                                '--',
                                xvfb_args,
                            ],
                        env: testsuite[2],
                        suite: 'xephyr-glamor' + testsuite[3],
                        should_fail: testsuite[4],
                        timeout: 300,
                        )
                endforeach
            endforeach
        endif
    endif
endif

subdir('bigreq')
subdir('damage')
subdir('sync')
subdir('bugs')

if build_xorg
# Tests that require at least some DDX functions in order to fully link
# For now, requires xf86 ddx, could be adjusted to use another
    unit_sources = [
     '../mi/miinitext.c',
     '../mi/miinitext.h',
     '../mi/micmap.c',
     '../mi/micmap.h',
     'fixes.c',
     'input.c',
     'list.c',
     'misc.c',
     'signal-logging.c',
     'string.c',
     'test_xkb.c',
     'tests-common.c',
     'tests.c',
     'touch.c',
     'xfree86.c',
     'xtest.c',
    ]
    unit_c_args = ['-DXORG_TESTS']
    unit_includes = [inc, xorg_inc]

    if build_res
        unit_sources += ['hashtabletest.c']
        unit_c_args += ['-DRES_TESTS']
    endif

    if meson.get_compiler('c').has_link_argument('-Wl,-wrap')
       # LTO breaks with -Wl,-wrap on certain configurations
       unit_c_args += ['-fno-lto']
       unit_sources += [
        'xi1/protocol-xchangedevicecontrol.c',
        'xi2/protocol-common.c',
        'xi2/protocol-xiqueryversion.c',
        'xi2/protocol-xiquerydevice.c',
        'xi2/protocol-xiselectevents.c',
        'xi2/protocol-xigetselectedevents.c',
        'xi2/protocol-xisetclientpointer.c',
        'xi2/protocol-xigetclientpointer.c',
        'xi2/protocol-xiquerypointer.c',
        'xi2/protocol-xipassivegrabdevice.c',
        'xi2/protocol-xiwarppointer.c',
        'xi2/protocol-eventconvert.c',
        'xi2/xi2.c',
       ]
       unit_c_args += ['-DLDWRAP_TESTS']
       unit_includes += [include_directories('xi1', 'xi2')]
       ldwraps = [
        '-Wl,-wrap,dixLookupWindow',
        '-Wl,-wrap,dixLookupResourceOwner',
        '-Wl,-wrap,WriteToClient',
        '-Wl,-wrap,dixLookupWindow',
        '-Wl,-wrap,XISetEventMask',
        '-Wl,-wrap,AddResource',
        '-Wl,-wrap,GrabButton',
       ]
    else
       ldwraps = []
       message('ld -wrap required for xi1 & xi2 unit tests, skipping')
    endif

    unit = executable('tests',
         unit_sources,
         c_args: unit_c_args,
         dependencies: [x11_dep, pixman_dep, randrproto_dep, inputproto_dep, libxcvt_dep],
         include_directories: unit_includes,
         link_args: ldwraps,
         link_with: xorg_link,
    )

    test('unit', unit)
endif
