#!/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net>

set -e

. .github/scripts/util.sh

export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig/:$PKG_CONFIG_PATH"
export ACLOCAL_PATH="/usr/share/aclocal:/usr/local/share/aclocal"

# copy over xserver SDK autoconf .m4 file so drivers can find it
cp $X11_PREFIX/share/aclocal/xorg-server.m4 /usr/share/aclocal

mkdir -p $DRV_BUILD_DIR
cd $DRV_BUILD_DIR

DRIVER_NAME=$(echo "$1" | sed -e 's~:.*~~')
DRIVER_VER=$(echo "$1" | sed -e 's~.*:~~')

echo "DRIVER_NAME=$DRIVER_NAME"
echo "DRIVER_VER=$DRIVER_VER"

build_xf86drv_ac "$DRIVER_NAME" "$DRIVER_VER"

# every server symbol the module needs must be exported by this server
# a GitHub annotation, shown at the top of the run and in its notification
annotate() {
    local msg=${1//'%'/%25}
    echo "::error::${msg//$'\n'/%0A}"
}
abi_tools=$(pkg-config --variable=abi_tools_dir xorg-server)
modules=$(find "xf86-$DRIVER_NAME" -path '*/.libs/*_drv.so')
[ -n "$modules" ] || { annotate "no *_drv.so built for $DRIVER_NAME"; exit 1; }
failed=
for mod in $modules; do
    if ! out=$("$abi_tools/check-symbols.sh" --prefix "$X11_PREFIX" "$mod" 2>&1); then
        annotate "$out"
        failed=1
    else
        echo "$out"
    fi
done
[ -z "$failed" ] || exit 1
