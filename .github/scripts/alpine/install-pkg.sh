#!/bin/sh

set -e

echo "--> update apk index"
apk update

echo "--> install build dependencies"
# X libs and most -dev packages live in the community repo, which is
# enabled by default in the official alpine image.
apk add --no-cache \
	bash \
	build-base \
	cairo-dev \
	ca-certificates \
	dbus-dev \
	eudev-dev \
	expat-dev \
	font-util-dev \
	git \
	libbsd-dev \
	libdrm-dev \
	libepoxy-dev \
	libevdev-dev \
	libffi-dev \
	libgcrypt-dev \
	libinput-dev \
	libpciaccess-dev \
	libtirpc-dev \
	libunwind-dev \
	libx11-dev \
	libxau-dev \
	libxaw-dev \
	libxcb-dev \
	libxcvt-dev \
	libxdmcp-dev \
	libxext-dev \
	libxfixes-dev \
	libxfont2-dev \
	libxi-dev \
	libxinerama-dev \
	libxkbcommon-dev \
	libxkbfile-dev \
	libxmu-dev \
	libxpm-dev \
	libxrandr-dev \
	libxrender-dev \
	libxres-dev \
	libxshmfence-dev \
	libxt-dev \
	libxtst-dev \
	libxv-dev \
	mesa-dev \
	meson \
	nettle-dev \
	pango-dev \
	pixman-dev \
	pkgconf \
	py3-mako \
	xcb-util-dev \
	xcb-util-image-dev \
	xcb-util-keysyms-dev \
	xcb-util-renderutil-dev \
	xcb-util-wm-dev \
	xkbcomp \
	xkeyboard-config \
	xorgproto
