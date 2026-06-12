#!/bin/sh

set -e

echo "--> update xbps and system"
# the package manager itself must be updated before anything else
xbps-install -Syu xbps
xbps-install -Syu

echo "--> install build dependencies"
xbps-install -y \
	audit-devel \
	base-devel \
	cairo-devel \
	ca-certificates \
	dbus-devel \
	eudev-libudev-devel \
	expat-devel \
	font-util \
	git \
	libXau-devel \
	libXaw-devel \
	libXdmcp-devel \
	libXext-devel \
	libXfixes-devel \
	libXfont2-devel \
	libXi-devel \
	libXinerama-devel \
	libXmu-devel \
	libXpm-devel \
	libXrandr-devel \
	libXrender-devel \
	libXres-devel \
	libXt-devel \
	libXtst-devel \
	libXv-devel \
	libX11-devel \
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
	libxcb-devel \
	libxcvt-devel \
	libxkbcommon-devel \
	libxkbfile-devel \
	libxshmfence-devel \
	MesaLib-devel \
	meson \
	nettle-devel \
	pango-devel \
	pixman-devel \
	pkg-config \
	python3-Mako \
	spice-protocol \
	xcb-util-devel \
	xcb-util-image-devel \
	xcb-util-keysyms-devel \
	xcb-util-renderutil-devel \
	xcb-util-wm-devel \
	xkbcomp \
	xkeyboard-config \
	xorgproto
