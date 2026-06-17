srcs_xext = [
    'bigreq.c',
    'geext.c',
    'sleepuntil.c',
    'sync.c',
    'xace.c',
    'xcmisc.c',
]


if build_hashtable
    srcs_xext += 'hashtable.c'
endif

if build_xsecurity
    srcs_xext += ['security.c']
endif


libxserver_xext = static_library('xserver_xext',
    srcs_xext,
    include_directories: inc,
    dependencies: common_dep,
)

subdir('xfixes')
subdir('xinput')
subdir('xkeyboard')

if build_xselinux
    subdir('xselinux')
endif

subdir('damage')
subdir('doublebuffer')
subdir('dri2')
subdir('dri3')
subdir('dpms')
subdir('panoramiX')
subdir('present')
subdir('render')
subdir('xf86bigfont')
subdir('composite')
if build_xwin or build_xquartz
    subdir('pseudoramiX')
endif
subdir('randr')
subdir('record')
subdir('shape')
subdir('saver')
subdir('glx')
subdir('shm')
subdir('xv')
subdir('xres')
subdir('xtest')
