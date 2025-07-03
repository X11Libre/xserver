srcs_mi = [
    'miarc.c',
    'micmap.c',
    'micopy.c',
    'midash.c',
    'midispcur.c',
    'mieq.c',
    'miexpose.c',
    'mifillarc.c',
    'migc.c',
    'miglblt.c',
    'mioverlay.c',
    'mipointer.c',
    'mipoly.c',
    'mipolypnt.c',
    'mipolyrect.c',
    'mipolyseg.c',
    'mipolytext.c',
    'mipushpxl.c',
    'miscrinit.c',
    'misprite.c',
    'mivaltree.c',
    'miwideline.c',
    'miwindow.c',
    'mizerarc.c',
    'mizerclip.c',
    'mizerline.c',
]

hdrs_mi = [
  'micmap.h',
  'micoord.h',
  'migc.h',
  'mi.h',
  'miline.h',
  'mioverlay.h',
  'mipointer.h',
  'mipointrst.h',
  'mistruct.h',
  'mizerarc.h',
]

libxserver_mi = static_library('xserver_mi',
    srcs_mi,
    include_directories: inc,
    dependencies: [
        common_dep,
        m_dep,
    ],
)

if build_xorg
    install_data(hdrs_mi, install_dir: xorgsdkdir)
endif
