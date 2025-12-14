#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
cd $X11_BUILD_DIR

build_meson   rendercheck       $(fdo_mirror rendercheck)                  rendercheck-1.6
build_meson   libxcvt           $(fdo_mirror libxcvt)                      libxcvt-0.1.0
build_meson   xorgproto         $(fdo_mirror xorgproto)                    xorgproto-2024.1
build_ac      xset              $(fdo_mirror xset)                         xset-1.2.5
