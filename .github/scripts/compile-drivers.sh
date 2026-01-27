#!/bin/bash

set -e

. .github/scripts/util.sh

export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig/:$PKG_CONFIG_PATH"
export ACLOCAL_PATH="/usr/share/aclocal:/usr/local/share/aclocal"

mkdir -p $DRV_BUILD_DIR
cd $DRV_BUILD_DIR

build_xf86drv_ac    input-elographics       25.0.0
build_xf86drv_ac    input-evdev             25.0.0
build_xf86drv_ac    input-joystick          25.0.0
build_xf86drv_ac    input-keyboard          25.0.0
build_xf86drv_ac    input-libinput          25.0.0
build_xf86drv_ac    input-mouse             25.0.0
build_xf86drv_ac    input-synaptics         25.0.0
build_xf86drv_ac    input-vmmouse           25.0.0
build_xf86drv_ac    input-void              25.0.0
build_xf86drv_ac    input-wacom             25.0.0

build_xf86drv_ac    video-amdgpu            25.0.0
build_xf86drv_ac    video-apm               25.0.0
build_xf86drv_ac    video-ark               25.0.0
build_xf86drv_ac    video-ast               25.0.0
build_xf86drv_ac    video-ati               25.0.0
build_xf86drv_ac    video-chips             25.0.0
build_xf86drv_ac    video-cirrus            25.0.0
build_xf86drv_ac    video-dummy             25.0.0
build_xf86drv_ac    video-fbdev             25.0.0
build_xf86drv_ac    video-freedreno         25.0.0
build_xf86drv_ac    video-geode             25.0.0
build_xf86drv_ac    video-i128              25.0.0
build_xf86drv_ac    video-i740              25.0.0
build_xf86drv_ac    video-intel             25.0.1
build_xf86drv_ac    video-mach64            25.0.0
build_xf86drv_ac    video-mga               25.0.0
build_xf86drv_ac    video-neomagic          25.0.0
build_xf86drv_ac    video-nested            25.0.0
build_xf86drv_ac    video-nouveau           25.0.0
build_xf86drv_ac    video-nv                25.0.0
build_xf86drv_ac    video-omap              25.0.0
build_xf86drv_ac    video-qxl               25.0.0
build_xf86drv_ac    video-r128              25.0.0
build_xf86drv_ac    video-rendition         25.0.0
build_xf86drv_ac    video-s3virge           25.0.0
build_xf86drv_ac    video-savage            25.0.0
build_xf86drv_ac    video-siliconmotion     25.0.0
build_xf86drv_ac    video-sis               25.0.0
build_xf86drv_ac    video-sisusb            25.0.0
build_xf86drv_ac    video-suncg14           25.0.0
build_xf86drv_ac    video-suncg3            25.0.0
build_xf86drv_ac    video-suncg6            25.0.0
build_xf86drv_ac    video-sunffb            25.0.0
build_xf86drv_ac    video-suntcx            25.0.0
build_xf86drv_ac    video-sunleo            25.0.0
build_xf86drv_ac    video-tdfx              25.0.0
build_xf86drv_ac    video-trident           25.0.0
build_xf86drv_ac    video-vesa              25.0.0
build_xf86drv_ac    video-vmware            25.0.0
build_xf86drv_ac    video-v4l               25.0.0
build_xf86drv_ac    video-vbox              25.0.0
build_xf86drv_ac    video-voodoo            25.0.0
build_xf86drv_ac    video-xgi               25.0.0
