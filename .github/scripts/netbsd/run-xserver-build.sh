#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net>

set -ex

echo "=== DEBUG: Starting run-xserver-build.sh ==="
echo "=== DEBUG: PWD = $(pwd) ==="
echo "=== DEBUG: MESON_ARGS = $MESON_ARGS ==="
echo "=== DEBUG: X11_BUILD_DIR = $X11_BUILD_DIR ==="
echo "=== DEBUG: X11_PREFIX = $X11_PREFIX ==="
ls -la

./.github/scripts/netbsd/install-pkg.sh

echo "=== DEBUG: install-pkg.sh completed ==="
echo "--> running xserver build ...."
export MESON_BUILDDIR=_build

echo "=== DEBUG: Setting up meson build directory ==="
rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS

echo "=== DEBUG: Meson setup completed, configuring ==="
meson configure "$MESON_BUILDDIR"

echo "=== DEBUG: Starting meson compile ==="
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args

echo "=== DEBUG: Meson compile completed, installing ==="
# tests not working yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
meson install --no-rebuild  -C "$MESON_BUILDDIR" $MESON_INSTALL_ARGS

echo "=== DEBUG: Build completed successfully ==="
# making trouble w/ git tree copied into the VM
# meson dist -C "$MESON_BUILDDIR" $MESON_DIST_ARGS
