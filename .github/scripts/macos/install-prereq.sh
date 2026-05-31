#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

build_meson   rendercheck       $(fdo_mirror rendercheck)                  $PKG_RENDERCHECK_REF
build_meson   libxcvt           $(fdo_mirror libxcvt)                      $PKG_LIBXCVT_REF
build_meson   xorgproto         $(fdo_mirror xorgproto)                    $PKG_XORGPROTO_REF
build_ac      xset              $(fdo_mirror xset)                         xset-1.2.5
