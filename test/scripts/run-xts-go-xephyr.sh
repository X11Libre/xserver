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

# Start Xvfb with -displayfd to get an unused display number.
# Use a single-digit fd (3): POSIX sh (e.g. dash, the default /bin/sh on
# Ubuntu) does not recognize multi-digit fd numbers in redirections, so
# "42>$DSPFIFO" would be parsed as the literal argument "42" plus a stdout
# redirect rather than a redirect of fd 42.
DSPFIFO=$(mktemp /tmp/xts-go-xvfb.XXXXXX)
rm -f "$DSPFIFO"
mkfifo "$DSPFIFO"
"$XVFB" -displayfd 3 3>"$DSPFIFO" -screen scrn 1280x1024x24 -noreset +byteswappedclients +extension GLX +render &
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

# Start Xephyr hosted by Xvfb, letting it pick a free display number itself via
# -displayfd (fd 4) rather than guessing XVFB_DISP+1.  meson runs the test suite
# in parallel, so a guessed number races with other tests' servers: Xephyr (no
# -displayfd) would fail to bind ("Cannot establish any listening sockets") while
# the socket of the colliding server still satisfies the old wait-loop, leaving
# $DISPLAY pointed at the wrong server and the go test hanging until timeout.
# As with Xvfb above, a single-digit fd is required for POSIX sh redirections.
EPHFIFO=$(mktemp /tmp/xts-go-xephyr.XXXXXX)
rm -f "$EPHFIFO"
mkfifo "$EPHFIFO"
DISPLAY=:$XVFB_DISP "$XEPHYR" \
    -glamor \
    -glamor-skip-present \
    -schedMax 2000 \
    -screen 1280x1024 \
    +byteswappedclients \
    -displayfd 4 4>"$EPHFIFO" &
XEPHYR_PID=$!
read -r XEPHYR_DISPLAY < "$EPHFIFO"
rm -f "$EPHFIFO"

if [ -z "$XEPHYR_DISPLAY" ]; then
    echo "Xephyr did not start in time"
    exit 1
fi

# Run the go-xts tests against Xephyr via $DISPLAY.
# Set XTS_XSERVER to a nonexistent path so the test framework falls back to $DISPLAY.
export DISPLAY=:$XEPHYR_DISPLAY
export XTS_XSERVER="/nonexistent"

cd "$GOXPROTO_DIR"
${GO:-go} test ${GOTESTFLAGS:--count=1 -v} ./xts/...
