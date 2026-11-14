#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Print a pkg-config directory whose xorg-server.pc points at a server
# staged by build-server.sh.
#
#   staged-pc.sh <server-workdir>
#
# The staged xorg-server.pc keeps prefix=/usr. Write a copy to
# <server-workdir>/pkgconfig with prefix set to the staged root; its other
# paths are relative to ${prefix}. Every other package keeps coming from the
# system. (The PKG_CONFIG_XORG_SERVER_PREFIX override is not honoured by
# every pkg-config: Ubuntu's pkgconf 2.5.1 ignores it.)

set -eu

[ $# -eq 1 ] || { echo "usage: $0 <server-workdir>" >&2; exit 2; }

wd=$(cd "$1" && pwd)
root=$wd/root

pc=
for f in "$root"/usr/lib*/pkgconfig/xorg-server.pc \
	"$root"/usr/lib/*/pkgconfig/xorg-server.pc; do
	[ -f "$f" ] || continue
	[ -z "$pc" ] || { echo "staged-pc: more than one xorg-server.pc under $root" >&2; exit 1; }
	pc=$f
done
[ -n "$pc" ] || { echo "staged-pc: no xorg-server.pc under $root" >&2; exit 1; }

grep -q '^prefix=/usr$' "$pc" || {
	echo "staged-pc: $pc does not have prefix=/usr" >&2
	exit 1
}

mkdir -p "$wd/pkgconfig"
{
	echo "prefix=$root/usr"
	grep -v '^prefix=' "$pc"
} >"$wd/pkgconfig/xorg-server.pc"
echo "$wd/pkgconfig"
