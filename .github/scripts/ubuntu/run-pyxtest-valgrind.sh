#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net>
#
# Build a debug Xvfb and run the pyxtest suite under valgrind memcheck.
#
# This is a dedicated, lightweight job: unlike run-xserver-build-and-test.sh
# it does not build Xorg/Xephyr/Xnest or fetch XTS/piglit/goxproto — pyxtest
# only exercises Xvfb here, and valgrind is slow enough already that we don't
# want to pay for the rest of the matrix too. It does still need a from-source
# xorgproto (util.sh's build_meson), since the Ubuntu runner image's
# presentproto is too old for meson.build's '>= 1.4' requirement.

set -e

if [ ! "$MESON_BUILDDIR" ]; then
    echo "missing MESON_BUILDDIR" >&2
    exit 1
fi

if [ ! "$X11_BUILD_DIR" ]; then
    echo "missing X11_BUILD_DIR" >&2
    exit 1
fi

. .github/scripts/util.sh

echo "=== MESON_BUILDDIR=$MESON_BUILDDIR"
echo "=== X11_PREFIX=$X11_PREFIX"
echo "=== PKG_CONFIG_PATH=$PKG_CONFIG_PATH"

mkdir -p "$X11_BUILD_DIR"
(
    cd "$X11_BUILD_DIR"
    build_meson xorgproto $(fdo_mirror xorgproto) "$PKG_XORGPROTO_REF"
)

meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson compile -C "$MESON_BUILDDIR"

export XSERVER_BUILDDIR="$PWD/$MESON_BUILDDIR"
export XVFB_PATH="$XSERVER_BUILDDIR/hw/vfb/Xvfb"
export PYTHONDONTWRITEBYTECODE=1

# Per-test valgrind XML (+ a .txt summary when a test has findings) goes
# here, independent of pass/fail — see test/pyxtest/conftest.py's
# --valgrind-log-dir. Without this, valgrind's own output never leaves an
# ephemeral temp file, so a green run leaves no trace of what it checked
# and a failing run's report isn't visible without an API round-trip.
export VALGRIND_LOG_DIR="$PWD/valgrind-logs"
mkdir -p "$VALGRIND_LOG_DIR"

# Run without -e so a failing/valgrind-tripped test still lets us write the
# job summary below before propagating the real exit code.
set +e
pytest -v --tb=short -n auto --timeout=300 --valgrind test/pyxtest/
PYTEST_STATUS=$?
set -e

if [ "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
        echo "### Valgrind memcheck summary"
        echo
        shopt -s nullglob
        findings=("$VALGRIND_LOG_DIR"/*.txt)
        if [ "${#findings[@]}" -eq 0 ]; then
            echo "No valgrind findings. Full per-test XML reports (leak-only included) are in the \`valgrind-logs\` artifact."
        else
            echo "${#findings[@]} test/server combination(s) with valgrind findings (full detail in the \`valgrind-logs\` artifact):"
            echo
            for f in "${findings[@]}"; do
                echo "- \`$(basename "${f%.txt}")\`"
            done
        fi
    } >>"$GITHUB_STEP_SUMMARY"
fi

exit "$PYTEST_STATUS"
