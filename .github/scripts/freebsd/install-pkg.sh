#!/bin/sh

set -e

echo "--> install extra dependencies"
pkg install -y \
    curl \
    git \
    libdrm \
    libepoll-shim \
    libX11 \
    libxkbfile \
    libxshmfence \
    libXfont2 \
    libxcvt \
    libglvnd \
    libepoxy \
    libudev-devd \
    mesa-dri \
    mesa-libs \
    meson \
    pixman \
    pkgconf \
    xcb-util-image \
    xcb-util-keysyms \
    xcb-util-renderutil \
    xcb-util-wm \
    xkbcomp \
    xorgproto
