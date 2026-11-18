#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net>

set -e

# Proof-of-life markers for the CI lane, see "verify the build actually ran"
# in .github/workflows/build-xserver.yml. vmactions/dragonflybsd-vm loses the
# exit code of the command it runs over SSH, so a step that never completed
# still reports success; the workflow therefore gates on these files instead
# of on the step outcome. Both carry $GITHUB_SHA so a marker from an earlier
# attempt cannot vouch for a later one - which is why GITHUB_SHA has to be
# listed in the workflow step's envs.
XSBUILD_STARTED=/tmp/xsbuild-started
XSBUILD_COMPLETE=/tmp/xsbuild-complete
rm -f "$XSBUILD_STARTED" "$XSBUILD_COMPLETE"

./.github/scripts/DragonFlyBSD/install-pkg.sh

echo "--> running xserver build ...."
export MESON_BUILDDIR=_build

CFLAGS="$CFLAGS -Wno-typedef-redefinition"

# Written once the guest is alive and this script is running. If this is
# missing, the build never started.
echo "$GITHUB_SHA" > "$XSBUILD_STARTED"

rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args
# tests not working yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
meson install --no-rebuild  -C "$MESON_BUILDDIR" $MESON_INSTALL_ARGS
# making trouble w/ git tree copied into the VM
# meson dist -C "$MESON_BUILDDIR" $MESON_DIST_ARGS

# Written last: present only if the build ran to completion.
echo "$GITHUB_SHA" > "$XSBUILD_COMPLETE"
