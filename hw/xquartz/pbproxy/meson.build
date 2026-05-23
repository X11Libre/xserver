build_standalone_pbproxy = get_option('xpbproxy')

pbproxy_defs = [
    '-DXQUARTZ_DATA_DIR="@0@"'.format(xquartz_data_dir),
    bundle_id_def
]
if build_standalone_pbproxy
    pbproxy_defs += ['-DSTANDALONE_XPBPROXY']
endif

libapplewm_dep = dependency('applewm', version: '>=1.4')

libxpbproxy = static_library('xpbproxy',
    ['../NSUserDefaults+XQuartzDefaults.m',
     'main.m',
     'x-input.m',
     'x-selection.m'],
    include_directories: ['..'],
    dependencies: [applewmproto_dep, libapplewm_dep, dependency('xfixes'), dependency('x11')],
    objc_args: pbproxy_defs,
)

if cc.get_id() == 'clang'
  cocoa = dependency('Cocoa', method: 'extraframework')
  xpbproxy_linkargs = []
else
  xpbproxy_linkargs = ['-framework Cocoa']
endif

# standalone xpbproxy
if build_standalone_pbproxy
    executable('xpbproxy',
        'app-main.m',
        link_with: libxpbproxy,
        link_args: xpbproxy_linkargs,
        dependencies: [cocoa, dependency('x11')],
        objc_args: pbproxy_defs,
        install: true,
    )
endif
