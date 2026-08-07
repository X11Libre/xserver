#!/bin/sh

set -e

brew update

brew install \
	autoconf \
	automake \
	bdftopcf \
	libapplewm \
	libtool \
	libx11 \
	libxau \
	libxaw \
	libxcb \
	libxdmcp \
	libxext \
	libxfixes \
	libxfont2 \
	libxkbfile \
	libxi \
	libxmu \
	libxrender \
	libxt \
	libxtst \
	meson \
	mkfontscale \
	pkgconf \
	pixman \
	util-macros \
	xdpyinfo \
	xkbcomp \
	xkeyboard-config \

pip3 install --break-system-packages \
	mako
