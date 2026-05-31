#!/bin/bash

set -e

. .github/scripts/util.sh

mkdir -p $X11_BUILD_DIR
(
    cd $X11_BUILD_DIR
    build_meson   drm               $(fdo_mirror drm)                          $PKG_LIBDRM_REF   -Domap=enabled -Dfreedreno=enabled
    build_meson   xorgproto         $(fdo_mirror xorgproto)                    $PKG_XORGPROTO_REF
)

# build Xserver SDK
echo -n > .meson_environment
echo "export MESON_BUILDDIR=$MESON_BUILDDIR" >> .meson_environment
echo "export PKG_CONFIG_PATH=$PKG_CONFIG_PATH" >> .meson_environment
.github/scripts/meson-build.sh --skip-test
sudo meson install --no-rebuild -C "$MESON_BUILDDIR"
sudo mkdir -p /usr/local/lib/$MACHINE/xorg/modules # /home/runner/x11/lib/xorg/modules
sudo chown -R runner /usr/local/lib/$MACHINE/xorg/modules # /home/runner/x11/lib/xorg/modules

# copy over xserver SDK autoconf .m4 file so drivers can find it
cp $X11_PREFIX/share/aclocal/xorg-server.m4 /usr/share/aclocal
