#!/bin/sh

set -e

echo "--> enable EPEL and CRB repositories"
# Many X -devel packages live in CRB (CodeReady Builder) and EPEL rather
# than BaseOS/AppStream.
dnf -y install dnf-plugins-core epel-release
dnf config-manager --set-enabled crb
dnf -y makecache

echo "--> install build dependencies"
dnf -y install \
	cairo-devel \
	dbus-devel \
	expat-devel \
	gcc \
	git \
	libbsd-devel \
	libdrm-devel \
	libepoxy-devel \
	libevdev-devel \
	libffi-devel \
	libgcrypt-devel \
	libinput-devel \
	libpciaccess-devel \
	libtirpc-devel \
	libunwind-devel \
	libX11-devel \
	libXau-devel \
	libXaw-devel \
	libxcb-devel \
	libXdmcp-devel \
	libXext-devel \
	libXfixes-devel \
	libXfont2-devel \
	libXi-devel \
	libXinerama-devel \
	libxkbcommon-devel \
	libxkbfile-devel \
	libXmu-devel \
	libXpm-devel \
	libXrandr-devel \
	libXrender-devel \
	libXres-devel \
	libxshmfence-devel \
	libXt-devel \
	libXtst-devel \
	libXv-devel \
	libxcvt-devel \
	make \
	mesa-libEGL-devel \
	mesa-libGL-devel \
	mesa-libGLES-devel \
	mesa-libgbm-devel \
	meson \
	nettle-devel \
	ninja-build \
	pango-devel \
	pixman-devel \
	pkgconf-pkg-config \
	python3-mako \
	systemd-devel \
	xcb-util-devel \
	xcb-util-image-devel \
	xcb-util-keysyms-devel \
	xcb-util-renderutil-devel \
	xcb-util-wm-devel \
	xkbcomp \
	xorg-x11-font-utils \
	xorg-x11-proto-devel \
	xkeyboard-config
