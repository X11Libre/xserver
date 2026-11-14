#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Check that every server symbol a module needs is exported by a server.
#
#   check-symbols.sh <module.so> <server-workdir>
#   check-symbols.sh --prefix <dir> <module.so>
#
# The module's strong undefined symbols, minus those its own libraries and
# the server's libraries provide, must each be defined by the server's Xorg
# or a module under its module directory. Exit 1 lists the rest.
#
# The server is one staged by build-server.sh, or with --prefix one
# installed in <dir> (<dir>/bin/Xorg, module directory from the
# xorg-server.pc that pkg-config finds, which must be under <dir>).

set -eu

usage() {
	echo "usage: $0 <module.so> <server-workdir>" >&2
	echo "       $0 --prefix <dir> <module.so>" >&2
	exit 2
}

if [ "${1:-}" = --prefix ]; then
	[ $# -eq 3 ] || usage
	root=$(cd "$2" && pwd)
	drv=$3
	xorg=$root/bin/Xorg
	moduledir=$(pkg-config --variable=moduledir xorg-server)
else
	[ $# -eq 2 ] || usage
	drv=$1
	root=$(cd "$2/root" && pwd)
	xorg=$root/usr/bin/Xorg
	pcdir=$("$(dirname "$0")/staged-pc.sh" "$2") || exit 2
	moduledir=$(PKG_CONFIG_PATH=$pcdir pkg-config --variable=moduledir xorg-server)
fi
[ -f "$xorg" ] || { echo "check-symbols: no $xorg" >&2; exit 2; }
case $moduledir in
"$root"/*) ;;
*) echo "check-symbols: moduledir '$moduledir' is outside $root" >&2
   exit 2 ;;
esac
[ -d "$moduledir" ] || { echo "check-symbols: no module dir $moduledir" >&2; exit 2; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

defined() {
	nm -D --defined-only "$1" 2>/dev/null | tr -s ' ' | cut -d' ' -f3 | cut -d@ -f1
}

nm -D --undefined-only "$drv" | grep ' U ' | tr -s ' ' | cut -d' ' -f3 |
	cut -d@ -f1 | sort -u >"$tmp/undef"

{ ldd "$drv"; ldd "$xorg"; } | grep '=> /' | tr -s ' \t' ' ' | cut -d' ' -f4 |
	sort -u | while read -r lib; do defined "$lib"; done | sort -u >"$tmp/libs"

{
	defined "$xorg"
	find "$moduledir" -name '*.so' | while read -r mod; do defined "$mod"; done
} | sort -u >"$tmp/server"

comm -23 "$tmp/undef" "$tmp/libs" | comm -23 - "$tmp/server" >"$tmp/missing"

if [ -s "$tmp/missing" ]; then
	echo "check-symbols: FAIL: $drv needs symbols $root does not export" \
		"(modules from $moduledir):"
	sed 's/^/  /' "$tmp/missing"
	exit 1
fi
echo "check-symbols: PASS ($(comm -12 "$tmp/undef" "$tmp/server" | wc -l)" \
	"server symbols resolved)"
