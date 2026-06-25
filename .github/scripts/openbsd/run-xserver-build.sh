#!/bin/sh

set -e

./.github/scripts/openbsd/install-pkg.sh

echo "--> running xserver build ...."
export MESON_BUILDDIR=_build

# OpenBSD's base X.Org lives under /usr/X11R6 (libs, headers, .pc files); pkg_add
# packages (meson deps, epoll-shim, ...) install under /usr/local. Point
# meson/pkg-config and the compiler/linker at both trees.
PKG_CONFIG_PATH="/usr/X11R6/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/local/share/pkgconfig:/usr/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
CPPFLAGS="-I/usr/X11R6/include -I/usr/local/include ${CPPFLAGS:-}"
LDFLAGS="-L/usr/X11R6/lib -L/usr/local/lib ${LDFLAGS:-}"
export PKG_CONFIG_PATH CPPFLAGS LDFLAGS

rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args
# tests not wired up for this platform yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
