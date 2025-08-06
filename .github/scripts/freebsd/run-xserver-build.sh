#!/bin/sh

set -e

./.github/scripts/freebsd/install-pkg.sh

echo "--> running xserver build ...."
export MESON_BUILDDIR=_build

rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args
# tests not working yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
meson install --no-rebuild  -C "$MESON_BUILDDIR" $MESON_INSTALL_ARGS
# making trouble w/ git tree copied into the VM
# meson dist -C "$MESON_BUILDDIR" $MESON_DIST_ARGS
