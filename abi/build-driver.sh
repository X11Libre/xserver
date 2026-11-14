#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Build a driver with debug info against a server staged by
# build-server.sh.
#
#   build-driver.sh <server-workdir> <driver-builddir> <driver-srcdir>
#
# Only the xorg-server package is redirected to the staged root (through
# the xorg-server.pc copy staged-pc.sh writes); every other dependency keeps
# coming from the system. meson drivers build out of tree; autotools drivers
# are copied into <driver-builddir> first, since their autogen.sh may run
# configure in the source tree.

set -eu

[ $# -eq 3 ] || {
	echo "usage: $0 <server-workdir> <driver-builddir> <driver-srcdir>" >&2
	exit 2
}

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$1/root" && pwd)
builddir=$2
srcdir=$(cd "$3" && pwd)

pcdir=$("$here/staged-pc.sh" "$1")
PKG_CONFIG_PATH=$pcdir${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
export PKG_CONFIG_PATH

sdkdir=$(pkg-config --variable=sdkdir xorg-server)
echo "build-driver: xorg-server $(pkg-config --modversion xorg-server)" \
	"sdkdir $sdkdir"
case $sdkdir in
"$root"/*) ;;
*) echo "build-driver: sdkdir $sdkdir is outside $root" >&2; exit 1 ;;
esac

rm -rf "$builddir"
if [ -f "$srcdir/meson.build" ]; then
	meson setup "$builddir" "$srcdir" -Dbuildtype=debug
	ninja -C "$builddir"
else
	mkdir -p "$builddir"
	(cd "$srcdir" && tar --exclude=.git -cf - .) | tar -xf - -C "$builddir"
	cd "$builddir"
	NOCONFIGURE=1 ./autogen.sh
	CFLAGS="-g -O0" ./configure
	make -j"$(nproc 2>/dev/null || echo 2)"
fi
