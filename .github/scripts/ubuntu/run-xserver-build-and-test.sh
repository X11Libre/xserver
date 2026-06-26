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
export GOXPROTO_DIR="$X11_BUILD_DIR/goxproto"

.github/scripts/ubuntu/install-prereq.sh

.github/scripts/meson-build.sh

echo '[xts]' > $X11_BUILD_DIR/piglit/piglit.conf
echo "path=$X11_BUILD_DIR/xts" >> $X11_BUILD_DIR/piglit/piglit.conf

# Run the test suite with a timeout-aware retry. The XTS suites on llvmpipe
# occasionally *hang* (a single subtest wedges) instead of failing — meson
# catches that as a per-test TIMEOUT (SIGTERM at the suite's `timeout:`), not a
# real failure ("Ok: 15, Fail: 0, Timeout: 1"). Raising the timeout only delays
# the hang, so instead we detect the TIMEOUT and re-run the tests — with
# --no-rebuild, so the existing build is reused (no recompile). A genuine test
# failure (no TIMEOUT in the log) is NOT retried and fails the job immediately,
# so real regressions are never masked.
testlog="$MESON_BUILDDIR/meson-logs/testlog.txt"
attempt=1
max_attempts=3
while true; do
    meson test -C "$MESON_BUILDDIR" --print-errorlogs ${retry_tests:+--no-rebuild} && break
    rc=$?
    if [ "$attempt" -lt "$max_attempts" ] && grep -q 'TIMEOUT' "$testlog" 2>/dev/null; then
        echo "=== meson test hit a TIMEOUT (likely a hung XTS subtest); re-running tests only, no rebuild (attempt $((attempt + 1))/$max_attempts)"
        attempt=$((attempt + 1))
        retry_tests=1
        continue
    fi
    echo "=== meson test failed (rc=$rc) and it was not a retryable TIMEOUT — failing" >&2
    exit "$rc"
done

.github/scripts/check-ddx-build.sh

.github/scripts/manpages-check
