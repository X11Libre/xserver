#!/bin/sh
#
# Build the X server inside an OpenIndiana (illumos) VM. Mirrors the BSD
# run-xserver-build.sh scripts: install deps, then meson setup/compile/install.
#

set -e

./.github/scripts/solaris/install-pkg.sh

echo "--> running xserver build ...."

PATH="/usr/bin:/usr/sbin:/sbin:$PATH"
export PATH

# OpenIndiana installs into the standard /usr layout; build 64-bit (the illumos
# gcc still defaults to 32-bit) and point pkg-config at the amd64 .pc dir plus
# the noarch share/pkgconfig (xorgproto protocol files).
CFLAGS="-m64 ${CFLAGS:-}"
LDFLAGS="-m64 ${LDFLAGS:-}"
PKG_CONFIG_PATH="/usr/lib/amd64/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:${PKG_CONFIG_PATH:-}"
export CFLAGS LDFLAGS PKG_CONFIG_PATH

export MESON_BUILDDIR=_build

rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args
# tests not wired up for this platform yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
meson install --no-rebuild -C "$MESON_BUILDDIR" $MESON_INSTALL_ARGS
