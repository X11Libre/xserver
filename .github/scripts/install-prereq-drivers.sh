#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

if [ "$X11_OS" = "Linux" ]; then
build_meson   drm               $(fdo_mirror drm)                          libdrm-2.4.121   -Domap=enabled -Dfreedreno=enabled
fi
build_meson   xorgproto         $(fdo_mirror xorgproto)                    xorgproto-2024.1
