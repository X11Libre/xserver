#!/bin/sh

# Run the go-x11proto XTS (wire-protocol round-trip) tests against a freshly
# built Xephyr.  Since the test environment is headless, we start an Xvfb first
# to host the Xephyr, then let the go-x11proto test connect to Xephyr via
# $DISPLAY.
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

set -e

# this times out on Travis, because the tests take too long.
if test "x$TRAVIS_BUILD_DIR" != "x"; then
    exit 77
fi

if [ -z "$GOXPROTO_DIR" ] || [ ! -d "$GOXPROTO_DIR" ]; then
    echo "GOXPROTO_DIR not set or not a directory -- skipping go XTS tests"
    exit 77
fi

if [ -z "$XSERVER_BUILDDIR" ]; then
    echo "XSERVER_BUILDDIR must be set"
    exit 1
fi

XVFB="$XSERVER_BUILDDIR/hw/vfb/Xvfb"
XEPHYR="$XSERVER_BUILDDIR/hw/kdrive/ephyr/Xephyr"

if [ ! -x "$XVFB" ]; then
    echo "Xvfb not found at $XVFB -- skipping go XTS tests"
    exit 77
fi
if [ ! -x "$XEPHYR" ]; then
    echo "Xephyr not found at $XEPHYR -- skipping go XTS tests"
    exit 77
fi

if ! command -v "${GO:-go}" >/dev/null 2>&1; then
    echo "Go not found -- skipping go XTS tests"
    exit 77
fi

# Start Xvfb with -displayfd to get an unused display number
DSPFIFO=$(mktemp /tmp/xts-go-xvfb.XXXXXX)
rm -f "$DSPFIFO"
mkfifo "$DSPFIFO"
"$XVFB" -displayfd 42 42>"$DSPFIFO" -screen scrn 1280x1024x24 -noreset +byteswappedclients +extension GLX +render &
XVFB_PID=$!
read -r XVFB_DISP < "$DSPFIFO"
rm -f "$DSPFIFO"

CLEANUP() {
    kill "$XEPHYR_PID" 2>/dev/null || true
    kill "$XVFB_PID" 2>/dev/null || true
    wait "$XEPHYR_PID" 2>/dev/null || true
    wait "$XVFB_PID" 2>/dev/null || true
}
trap CLEANUP EXIT INT TERM

# Start Xephyr on the next display, using Xvfb as its host
XEPHYR_DISPLAY=$((XVFB_DISP + 1))
DISPLAY=:$XVFB_DISP "$XEPHYR" \
    -glamor \
    -glamor-skip-present \
    -schedMax 2000 \
    -screen 1280x1024 \
    :$XEPHYR_DISPLAY &
XEPHYR_PID=$!

# Wait for the Xephyr socket to appear
for i in $(seq 1 50); do
    if [ -S "/tmp/.X11-unix/X$XEPHYR_DISPLAY" ]; then
        break
    fi
    sleep 0.1
done

if ! [ -S "/tmp/.X11-unix/X$XEPHYR_DISPLAY" ]; then
    echo "Xephyr did not start in time"
    exit 1
fi

# Run the go-xts tests against Xephyr via $DISPLAY.
# Set XTS_XSERVER to a nonexistent path so the test framework falls back to $DISPLAY.
export DISPLAY=:$XEPHYR_DISPLAY
export XTS_XSERVER="/nonexistent"

cd "$GOXPROTO_DIR"
${GO:-go} test ${GOTESTFLAGS:--count=1 -v} ./xts/...
