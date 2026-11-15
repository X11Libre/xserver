#!/bin/bash

set -e

. .github/scripts/util.sh

export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig/:$PKG_CONFIG_PATH"
export ACLOCAL_PATH="/usr/share/aclocal:/usr/local/share/aclocal"
export X11L_DRV_GIT=https://github.com/X11Libre/

mkdir -p $DRV_BUILD_DIR
cd $DRV_BUILD_DIR

build_drv_ac xf86-input-elographics $X11L_DRV_GIT/xf86-input-elographics master
build_drv_ac xf86-input-evdev       $X11L_DRV_GIT/xf86-input-evdev       master
build_drv_ac xf86-input-libinput    $X11L_DRV_GIT/xf86-input-libinput    master
build_drv_ac xf86-input-mouse       $X11L_DRV_GIT/xf86-input-mouse       master
build_drv_ac xf86-input-synaptics   $X11L_DRV_GIT/xf86-input-synaptics   master

build_drv_ac xf86-video-amdgpu      $X11L_DRV_GIT/xf86-video-amdgpu      xf86-video-amdgpu-23.0.0
build_drv_ac xf86-video-ati         $X11L_DRV_GIT/xf86-video-ati         xf86-video-ati-22.0.0
build_drv_ac xf86-video-intel       $X11L_DRV_GIT/xf86-video-intel       master
build_drv_ac xf86-video-nouveau     $X11L_DRV_GIT/xf86-video-nouveau     xf86-video-nouveau-1.0.18
build_drv_ac xf86-video-omap        $X11L_DRV_GIT/xf86-video-omap        master
build_drv_ac xf86-video-qxl         $X11L_DRV_GIT/xf86-video-qxl         master
build_drv_ac xf86-video-r128        $X11L_DRV_GIT/xf86-video-r128        master
build_drv_ac xf86-video-vesa        $X11L_DRV_GIT/xf86-video-vesa        xf86-video-vesa-2.6.0
build_drv_ac xf86-video-vmware      $X11L_DRV_GIT/xf86-video-vmware      xf86-video-vmware-13.4.0

# doesn't compile on Linux
# build_drv_ac xf86-input-mouse       $X11L_DRV_GIT/xf86-input-mouse       master
# build_drv_ac xf86-input-keyboard    $X11L_DRV_GIT/xf86-input-keyboard    master

# every server symbol a module needs must be exported by this server;
# libdrm and libxcvt are in $X11_PREFIX, outside the server's libdir
abi_tools=$(pkg-config --variable=abi_tools_dir xorg-server)
prefix=$(pkg-config --variable=prefix xorg-server)
failed=
# a GitHub annotation, shown at the top of the run and in its notification
annotate() {
    local msg=${1//'%'/%25}
    echo "::error::${msg//$'\n'/%0A}"
}
for drv in xf86-*; do
    modules=$(find "$drv" -path '*/.libs/*_drv.so')
    if [ -z "$modules" ]; then
        annotate "no *_drv.so built for $drv"
        failed="$failed $drv"
    fi
    for mod in $modules; do
        if ! out=$(LD_LIBRARY_PATH=$X11_PREFIX/lib/$MACHINE \
                "$abi_tools/check-symbols.sh" --prefix "$prefix" "$mod" 2>&1); then
            annotate "$out"
            failed="$failed $mod"
        else
            echo "$out"
        fi
    done
done
if [ -n "$failed" ]; then
    echo "symbol check failed:$failed" >&2
    exit 1
fi
