#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

build_meson   rendercheck       https://github.com/X11Libre/rendercheck                  rendercheck-1.6
build_meson   libxcvt           https://github.com/X11Libre/libxcvt                      libxcvt-0.1.0
build_meson   xorgproto         https://github.com/X11Libre/xorgproto                    xorgproto-2024.1
build_ac      xset              https://github.com/X11Libre/xset                         xset-1.2.5
