#!/bin/bash

set -e

# Packages which are needed by this script, but not for the xserver build
EPHEMERAL="
	libexpat-dev
	libgles2-mesa-dev
	libxkbcommon-dev
	x11-utils
	x11-xserver-utils
	xauth
	xvfb
"

apt-get update

apt-get install -y \
	$EPHEMERAL \
	autoconf \
	automake \
	build-essential \
	ca-certificates \
	libaudit-dev \
	libbsd-dev \
	libcairo2-dev \
	libdbus-1-dev \
	libdrm-dev \
	libegl1-mesa-dev \
	libepoxy-dev \
	libevdev2 \
	libexpat1 \
	libffi-dev \
	libgbm-dev \
	libgcrypt-dev \
	libgl1-mesa-dev \
	libgles2 \
	libglx-mesa0 \
	libinput10 \
	libinput-dev \
	libnvidia-egl-wayland-dev \
	libpciaccess-dev \
	libpixman-1-dev \
	libspice-protocol-dev \
	libsystemd-dev \
	libudev-dev \
	libunwind-dev \
	libx11-dev \
	libx11-xcb-dev \
	libxau-dev \
	libxaw7-dev \
	libxcb-glx0-dev \
	libxcb-icccm4-dev \
	libxcb-image0-dev \
	libxcb-keysyms1-dev \
	libxcb-randr0-dev \
	libxcb-render-util0-dev \
	libxcb-render0-dev \
	libxcb-shape0-dev \
	libxcb-shm0-dev \
	libxcb-util0-dev \
	libxcb-xf86dri0-dev \
	libxcb-xkb-dev \
	libxcb-xv0-dev \
	libxcb1-dev \
	libxdmcp-dev \
	libxext-dev \
	libxfixes-dev \
	libxfont-dev \
	libxi-dev \
	libxinerama-dev \
	libxkbcommon0 \
	libxkbfile-dev \
	libxmu-dev \
	libxmuu-dev \
	libxpm-dev \
	libxrender-dev \
	libxres-dev \
	libxshmfence-dev \
	libxt-dev \
	libxtst-dev \
	libxv-dev \
	mesa-common-dev \
	meson \
	nettle-dev \
	libpango1.0-dev \
	pkg-config \
	x11-xkb-utils \
	xfonts-utils \
	xkb-data \
	xtrans-dev \
	xutils-dev \
	libxaw7-dev \
	python3-mako
