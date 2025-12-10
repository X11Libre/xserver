#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

build_meson   rendercheck       https://github.com/X11Libre/rendercheck                  rendercheck-1.6
if [ "$X11_OS" = "Linux" ]; then
build_meson   drm               https://github.com/X11Libre/drm                          libdrm-2.4.121   -Domap=enabled
fi
build_meson   libxcvt           https://github.com/X11Libre/libxcvt                      libxcvt-0.1.0
build_meson   xorgproto         https://github.com/X11Libre/xorgproto                    xorgproto-2024.1

# really must be build via autoconf instead of meson, otherwise piglit wont find the test programs
build_ac_xts  xts               https://github.com/X11Libre/xts                          aae51229af810efba24412511f60602fab53eded

clone_source piglit             https://github.com/X11Libre/piglit                       28d1349844eacda869f0f82f551bcd4ac0c4edfe

echo '[xts]' > piglit/piglit.conf
echo "path=$X11_BUILD_DIR/xts" >> piglit/piglit.conf
