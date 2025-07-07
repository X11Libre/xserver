#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

build_meson   rendercheck       https://gitlab.freedesktop.org/xorg/test/rendercheck     rendercheck-1.6
if [ "$X11_OS" = "Linux" ]; then
build_meson   drm               https://gitlab.freedesktop.org/mesa/drm                  libdrm-2.4.121   "" \
    -Domap=enabled
fi
build_meson   libxcvt           https://gitlab.freedesktop.org/xorg/lib/libxcvt          libxcvt-0.1.0
build_ac      xorgproto         https://gitlab.freedesktop.org/xorg/proto/xorgproto      xorgproto-2024.1
if [ "$X11_OS" = "Linux" ]; then
build_meson   wayland           https://gitlab.freedesktop.org/wayland/wayland           1.21.0           "" \
    -Dtests=false -Ddocumentation=false -Ddtd_validation=false
build_meson   wayland-protocols https://gitlab.freedesktop.org/wayland/wayland-protocols 1.38
build_meson   libdecor          https://gitlab.freedesktop.org/libdecor/libdecor         0.1.1            "" \
    -D{demo,install_demo}=false
build_meson   libei             https://gitlab.freedesktop.org/libinput/libei            1.0.0            "" \
    -Dtests=disabled -Ddocumentation=[] -Dliboeffis=enabled
fi
if [ "$X11_OS" = "Darwin" ]; then
build_ac      xset              https://gitlab.freedesktop.org/xorg/app/xset             xset-1.2.5
fi
build_ac_xts  xts               https://gitlab.freedesktop.org/xorg/test/xts             master     12a887c2c72c4258962b56ced7b0aec782f1ffed

clone_source piglit             https://gitlab.freedesktop.org/mesa/piglit               main # 265896c86f90cb72e8f218ba6a3617fca8b9a1e3

echo '[xts]' > piglit/piglit.conf
echo "path=$X11_BUILD_DIR/xts" >> piglit/piglit.conf
