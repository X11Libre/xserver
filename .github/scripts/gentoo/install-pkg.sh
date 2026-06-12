#!/bin/sh

set -e

echo "--> sync portage tree (snapshot)"
# emerge-webrsync pulls a portage snapshot tarball, much faster than rsync.
emerge-webrsync --quiet

echo "--> configure portage for CI (binpkgs, no sandbox)"
# - getbinpkg + the binhost preconfigured in the stage3 image let us pull
#   prebuilt packages instead of compiling the whole X stack from source.
# - the sandbox features need disabling inside an unprivileged container.
cat >> /etc/portage/make.conf <<EOF

# --- xlibre CI build settings ---
ACCEPT_LICENSE="*"
USE="\${USE} gbm egl gles2 udev -systemd"
FEATURES="\${FEATURES} getbinpkg -sandbox -usersandbox -ipc-sandbox -network-sandbox -pid-sandbox"
EMERGE_DEFAULT_OPTS="--jobs=$(nproc) --quiet-build=y --getbinpkg --binpkg-respect-use=y --autounmask-continue=y"
EOF

echo "--> install build dependencies"
emerge --oneshot \
	dev-build/meson \
	dev-libs/expat \
	dev-libs/libbsd \
	dev-libs/libevdev \
	dev-libs/libffi \
	dev-libs/libgcrypt \
	dev-libs/libinput \
	dev-libs/nettle \
	dev-libs/spice-protocol \
	dev-python/mako \
	dev-vcs/git \
	media-fonts/font-util \
	media-libs/libglvnd \
	media-libs/mesa \
	net-libs/libtirpc \
	sys-apps/dbus \
	sys-libs/libunwind \
	virtual/libudev \
	virtual/pkgconfig \
	x11-apps/xkbcomp \
	x11-base/xorg-proto \
	x11-libs/cairo \
	x11-libs/libdrm \
	x11-libs/libepoxy \
	x11-libs/libpciaccess \
	x11-libs/libX11 \
	x11-libs/libXau \
	x11-libs/libXaw \
	x11-libs/libxcb \
	x11-libs/libXdmcp \
	x11-libs/libXext \
	x11-libs/libXfixes \
	x11-libs/libXfont2 \
	x11-libs/libXi \
	x11-libs/libXinerama \
	x11-libs/libxkbfile \
	x11-libs/libXmu \
	x11-libs/libXpm \
	x11-libs/libXrandr \
	x11-libs/libXrender \
	x11-libs/libXres \
	x11-libs/libxshmfence \
	x11-libs/libXt \
	x11-libs/libXtst \
	x11-libs/libXv \
	x11-libs/libxcvt \
	x11-libs/pango \
	x11-libs/pixman \
	x11-libs/xcb-util \
	x11-libs/xcb-util-image \
	x11-libs/xcb-util-keysyms \
	x11-libs/xcb-util-renderutil \
	x11-libs/xcb-util-wm \
	x11-misc/xkeyboard-config
