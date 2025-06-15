#!/bin/bash

set -e

. .github/scripts/util.sh

export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig/:$PKG_CONFIG_PATH"
export ACLOCAL_PATH="/usr/share/aclocal:/usr/local/share/aclocal"
export XORG_DRV_GIT=https://gitlab.freedesktop.org/xorg/driver

mkdir -p $DRV_BUILD_DIR
cd $DRV_BUILD_DIR

# FIXME: add more drivers
build_drv_ac xf86-input-evdev     $XORG_DRV_GIT/xf86-input-evdev     xf86-input-evdev-2.10.6
build_drv_ac xf86-input-libinput  $XORG_DRV_GIT/xf86-input-libinput  xf86-input-libinput-1.4.0
build_drv_ac xf86-input-mouse     $XORG_DRV_GIT/xf86-input-mouse     xf86-input-mouse-1.9.5
build_drv_ac xf86-input-synaptics $XORG_DRV_GIT/xf86-input-synaptics xf86-input-synaptics-1.9.2
build_drv_ac xf86-video-amdgpu    $XORG_DRV_GIT/xf86-video-amdgpu    xf86-video-amdgpu-23.0.0
build_drv_ac xf86-video-ati       $XORG_DRV_GIT/xf86-video-ati       xf86-video-ati-22.0.0
build_drv_ac xf86-video-qxl       $XORG_DRV_GIT/xf86-video-qxl       master
build_drv_ac xf86-video-vesa      $XORG_DRV_GIT/xf86-video-vesa      xf86-video-vesa-2.6.0
build_drv_ac xf86-video-vmware    $XORG_DRV_GIT/xf86-video-vmware    xf86-video-vmware-13.4.0
