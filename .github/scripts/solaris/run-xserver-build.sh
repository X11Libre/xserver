#!/bin/sh
#
# Build the X server inside an OmniOS (illumos) VM. Mirrors the BSD
# run-xserver-build.sh scripts: install deps, then meson setup/compile/install.
#

set -e

./.github/scripts/solaris/install-pkg.sh

echo "--> running xserver build ...."

# OmniOS Extra (ooce) ships meson/ninja/gcc/pkg-config under /opt/ooce/bin.
PATH="/opt/ooce/bin:/usr/bin:/usr/sbin:/sbin:$PATH"
export PATH

export MESON_BUILDDIR=_build

rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args
# tests not wired up for this platform yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
meson install --no-rebuild -C "$MESON_BUILDDIR" $MESON_INSTALL_ARGS
