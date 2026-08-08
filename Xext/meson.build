srcs_xext = [
    'xace.c',
]


if build_hashtable
    srcs_xext += 'hashtable.c'
endif


libxserver_xext = static_library('xserver_xext',
    srcs_xext,
    include_directories: inc,
    dependencies: common_dep,
)

subdir('bigreq')
subdir('geext')
subdir('xfixes')
subdir('xinput')
subdir('xkeyboard')

if build_xselinux
    subdir('xselinux')
endif

subdir('security')
subdir('damage')
subdir('doublebuffer')
subdir('dri2')
subdir('dri3')
subdir('dpms')
subdir('panoramiX')
subdir('present')
subdir('render')
subdir('composite')
if build_xwin or build_xquartz
    subdir('pseudoramiX')
endif
subdir('randr')
subdir('record')
subdir('xcmisc')
subdir('shape')
subdir('saver')
subdir('sync')
subdir('glx')
subdir('shm')
subdir('xv')
subdir('xres')
subdir('xtest')
