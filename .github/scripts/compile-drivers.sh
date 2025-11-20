#!/bin/bash

set -e

. .github/scripts/util.sh

export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig/:$PKG_CONFIG_PATH"
export ACLOCAL_PATH="/usr/share/aclocal:/usr/local/share/aclocal"
export X11L_DRV_GIT=https://github.com/X11Libre/

mkdir -p $DRV_BUILD_DIR
cd $DRV_BUILD_DIR

build_drv_ac xf86-input-elographics $X11L_DRV_GIT/xf86-input-elographics xlibre-xf86-input-elographics-1.4.4.2
build_drv_ac xf86-input-evdev       $X11L_DRV_GIT/xf86-input-evdev       xlibre-xf86-input-evdev-2.11.0.2
build_drv_ac xf86-input-libinput    $X11L_DRV_GIT/xf86-input-libinput    xlibre-xf86-input-libinput-1.5.1.0
build_drv_ac xf86-input-mouse       $X11L_DRV_GIT/xf86-input-mouse       xlibre-xf86-input-mouse-1.9.5.2
build_drv_ac xf86-input-synaptics   $X11L_DRV_GIT/xf86-input-synaptics   xlibre-xf86-input-synaptics-1.10.0.2

build_drv_ac xf86-video-amdgpu      $X11L_DRV_GIT/xf86-video-amdgpu      xlibre-xf86-video-amdgpu-23.0.0.5
build_drv_ac xf86-video-apm         $X11L_DRV_GIT/xf86-video-apm         xlibre-xf86-video-apm-1.3.0.3
build_drv_ac xf86-video-ati         $X11L_DRV_GIT/xf86-video-ati         xlibre-xf86-video-ati-22.0.0.3
build_drv_ac xf86-video-dummy       $X11L_DRV_GIT/xf86-video-dummy       xlibre-xf86-video-dummy-0.4.1.3
build_drv_ac xf86-video-intel       $X11L_DRV_GIT/xf86-video-intel       xlibre-xf86-video-intel-3.0.0.3
build_drv_ac xf86-video-nouveau     $X11L_DRV_GIT/xf86-video-nouveau     xlibre-xf86-video-nouveau-1.0.18.3
build_drv_ac xf86-video-omap        $X11L_DRV_GIT/xf86-video-omap        xlibre-xf86-video-omap-0.4.5.2
build_drv_ac xf86-video-qxl         $X11L_DRV_GIT/xf86-video-qxl         xlibre-xf86-video-qxl-0.1.6.2
build_drv_ac xf86-video-r128        $X11L_DRV_GIT/xf86-video-r128        xlibre-xf86-video-r128-6.13.0.2
build_drv_ac xf86-video-vesa        $X11L_DRV_GIT/xf86-video-vesa        xlibre-xf86-video-vesa-2.6.0.2
build_drv_ac xf86-video-vmware      $X11L_DRV_GIT/xf86-video-vmware      xlibre-xf86-video-vmware-13.4.0.3

build_drv_ac xf86-input-mouse       $X11L_DRV_GIT/xf86-input-mouse       master
build_drv_ac xf86-input-keyboard    $X11L_DRV_GIT/xf86-input-keyboard    master
