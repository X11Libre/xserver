#!/bin/bash

set -e

if [ ! "$X11_BUILD_DIR" ]; then
    echo "missing X11_BUILD_DIR" >&2
    exit 1
fi

if [ ! "$MESON_BUILDDIR" ]; then
    echo "missing MESON_BUILDDIR" >&2
    exit 1
fi

echo "=== X11_BUILD_DIR=$X11_BUILD_DIR"
echo "=== MESON_BUILDDIR=$MESON_BUILDDIR"

export XTEST_DIR="$X11_BUILD_DIR/xts"
export PIGLIT_DIR="$X11_BUILD_DIR/piglit"

go install github.com/metux/mpbt/cmd/mpbt-builder@v0.1.2

~/go/bin/mpbt-builder \
    -root . \
    -solution .mpbt/cf/xlibre/solutions/github-ubuntu-xserver.yaml \
    build

( cd $X11_BUILD_DIR/xts && xvfb-run make tetexec.cfg )

echo '[xts]' > $X11_BUILD_DIR/piglit/piglit.conf
echo "path=$X11_BUILD_DIR/xts" >> $X11_BUILD_DIR/piglit/piglit.conf

meson test -C "$MESON_BUILDDIR" --print-errorlogs

.github/scripts/check-ddx-build.sh

.github/scripts/manpages-check
