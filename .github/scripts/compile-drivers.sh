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
