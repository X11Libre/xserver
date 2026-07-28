#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net>

set -e

./.github/scripts/rhel/install-pkg.sh

echo "--> running xserver build ...."
export MESON_BUILDDIR=_build

rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args
# tests not working yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
meson install --no-rebuild  -C "$MESON_BUILDDIR" $MESON_INSTALL_ARGS
