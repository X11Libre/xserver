#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

build_meson   rendercheck       $(fdo_mirror rendercheck)                  rendercheck-1.6
if [ "$X11_OS" = "Linux" ]; then
build_meson   drm               $(fdo_mirror drm)                          libdrm-2.4.121   -Domap=enabled -Dfreedreno=enabled
fi
build_meson   libxcvt           $(fdo_mirror libxcvt)                      libxcvt-0.1.0
build_meson   xorgproto         $(fdo_mirror xorgproto)                    xorgproto-2024.1

# really must be build via autoconf instead of meson, otherwise piglit wont find the test programs
build_ac_xts  xts               $(fdo_mirror xts)                          6cf94400a09abecd6b86e4eb6441741acecd51f6

clone_source piglit             $(fdo_mirror piglit)                       59111996534f875ca88bce51f21fa2e6564895da
