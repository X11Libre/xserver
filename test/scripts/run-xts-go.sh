#!/bin/sh

# Run the go-x11proto XTS (wire-protocol round-trip) tests against a freshly
# built Xvfb.  These are Go tests in the go-x11proto xts/ package that start
# their own server via -displayfd, so they never touch the host display.
#
# The test is skipped (exit 77) when GOXPROTO_DIR is unset or Go isn't found,
# so it's harmless on systems without the go-x11proto tree.
#
# Required environment:
#   GOXPROTO_DIR      root of the go-x11proto repository
#   XSERVER_BUILDDIR  build directory root
#
# Optional environment:
#   GO                go binary (default: go)
#   XTS_XSERVER       server binary (default: $XSERVER_BUILDDIR/hw/vfb/Xvfb)
#   XTS_XSERVER_ARGS  server arguments (see below for default)

set -e

if [ -z "$GOXPROTO_DIR" ] || [ ! -d "$GOXPROTO_DIR" ]; then
    echo "GOXPROTO_DIR not set or not a directory -- skipping go XTS tests"
    exit 77
fi

if [ -z "$XSERVER_BUILDDIR" ]; then
    echo "XSERVER_BUILDDIR must be set"
    exit 1
fi

if [ -z "$XTS_XSERVER" ]; then
    XTS_XSERVER="$XSERVER_BUILDDIR/hw/vfb/Xvfb"
fi
if [ ! -x "$XTS_XSERVER" ]; then
    echo "Server not found at $XTS_XSERVER -- skipping go XTS tests"
    exit 77
fi

if ! command -v "${GO:-go}" >/dev/null 2>&1; then
    echo "Go not found -- skipping go XTS tests"
    exit 77
fi

export XTS_XSERVER
: "${XTS_XSERVER_ARGS:="-noreset +byteswappedclients -screen scrn 1280x1024x24"}"
export XTS_XSERVER_ARGS

cd "$GOXPROTO_DIR"
exec ${GO:-go} test ${GOTESTFLAGS:--count=1 -v} ./xts/...
