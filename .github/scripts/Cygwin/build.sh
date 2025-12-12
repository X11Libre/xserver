#!/bin/sh
set -x
ccache -s
meson setup --prefix=/usr -Dxv=false -Dxf86bigfont=true -Dxephyr=true -Dxnest=true -Dxvfb=true -Dxwin=true -Dxorg=true -Dpciaccess=false -Dint10=false -Dglamor=false build
meson configure build
ninja -C build
ccache -s
ninja -C build test
ninja -C build install
