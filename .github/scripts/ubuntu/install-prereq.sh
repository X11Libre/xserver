#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

build_meson   rendercheck       $(fdo_mirror rendercheck)                  $PKG_RENDERCHECK_REF
if [ "$X11_OS" = "Linux" ]; then
build_meson   drm               $(fdo_mirror drm)                          $PKG_LIBDRM_REF          -Domap=enabled -Dfreedreno=enabled
fi
build_meson   libxcvt           $(fdo_mirror libxcvt)                      $PKG_LIBXCVT_REF
build_meson   xorgproto         $(fdo_mirror xorgproto)                    $PKG_XORGPROTO_REF

# really must be build via autoconf instead of meson, otherwise piglit wont find the test programs
build_ac_xts  xts               $(fdo_mirror xts)                          $PKG_XTS_REF

clone_source piglit             $(fdo_mirror piglit)                       $PKG_PIGLIT_REF

clone_source goxproto           $(xl_mirror go-x11proto)                   $PKG_GOXPROTO_REF
