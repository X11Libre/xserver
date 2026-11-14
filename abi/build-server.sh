#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Build an XLibre xserver with debug info and stage it under <workdir>/root
# (prefix /usr, installed with DESTDIR) for the driver ABI checks.
#
#   build-server.sh <ref> <workdir>
#
# <ref> is a tag, branch or commit. With XSERVER_SRC set to a local xserver
# clone the sources come from `git archive <ref>` there; otherwise <ref> is
# cloned from GitHub (tags and branch names only).

set -eu

[ $# -eq 2 ] || { echo "usage: $0 <ref> <workdir>" >&2; exit 2; }
ref=$1
mkdir -p "$2"
workdir=$(cd "$2" && pwd)
srcdir=$workdir/src

if [ -f "$workdir/root/.abi-staged" ]; then
	echo "build-server: reusing $workdir/root"
	exit 0
fi

if [ ! -f "$srcdir/meson.build" ]; then
	rm -rf "$srcdir"
	if [ -n "${XSERVER_SRC:-}" ]; then
		mkdir -p "$srcdir"
		git -C "$XSERVER_SRC" archive "$ref" | tar -x -C "$srcdir"
	else
		git clone --quiet --depth 1 --branch "$ref" \
			https://github.com/X11Libre/xserver "$srcdir"
	fi
fi

rm -rf "$workdir/build" "$workdir/root"
meson setup "$workdir/build" "$srcdir" \
	-Dprefix=/usr -Dbuildtype=debug -Dc_args=-fno-common \
	-Dnamespace=false -Dxselinux=false -Dxephyr=false -Dwerror=false \
	-Dxcsecurity=false -Dxorg=true -Dxvfb=false -Dxnest=false \
	-Ddocs=false -Dsuid_wrapper=false
ninja -C "$workdir/build"
DESTDIR="$workdir/root" meson install -C "$workdir/build" --no-rebuild --quiet
touch "$workdir/root/.abi-staged"
echo "build-server: staged $workdir/root"
