#!/bin/bash

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
