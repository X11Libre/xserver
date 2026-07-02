#!/bin/bash
#
# Build a debug Xvfb and run the pyxtest suite under valgrind memcheck.
#
# This is a dedicated, lightweight job: unlike run-xserver-build-and-test.sh
# it does not build Xorg/Xephyr/Xnest or fetch XTS/piglit/goxproto — pyxtest
# only exercises Xvfb here, and valgrind is slow enough already that we don't
# want to pay for the rest of the matrix too.

set -e

if [ ! "$MESON_BUILDDIR" ]; then
    echo "missing MESON_BUILDDIR" >&2
    exit 1
fi

echo "=== MESON_BUILDDIR=$MESON_BUILDDIR"

meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson compile -C "$MESON_BUILDDIR"

export XSERVER_BUILDDIR="$PWD/$MESON_BUILDDIR"
export XVFB_PATH="$XSERVER_BUILDDIR/hw/vfb/Xvfb"
export PYTHONDONTWRITEBYTECODE=1

pytest -v --tb=short -n auto --timeout=300 --valgrind test/pyxtest/
