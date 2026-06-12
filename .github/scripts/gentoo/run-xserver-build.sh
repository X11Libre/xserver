#!/bin/sh

set -e

# Build dependencies are baked into the prebuilt GHCR image (see
# .github/docker/gentoo/Dockerfile + .github/workflows/gentoo-image.yml), so we
# only run meson here. Gentoo uses the standard /usr[/lib64] layout, so no extra
# PKG_CONFIG_PATH is needed.
echo "--> running xserver build ...."
export MESON_BUILDDIR=_build

rm -rf "$MESON_BUILDDIR"
meson setup "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
meson compile -v -C "$MESON_BUILDDIR" $jobcount $ninja_args
# tests not working yet
# meson test -C "$MESON_BUILDDIR" --print-errorlogs $MESON_TEST_ARGS
meson install --no-rebuild  -C "$MESON_BUILDDIR" $MESON_INSTALL_ARGS
