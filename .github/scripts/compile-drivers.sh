#!/bin/bash

set -e

. .github/scripts/util.sh

export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig/:$PKG_CONFIG_PATH"
export ACLOCAL_PATH="/usr/share/aclocal:/usr/local/share/aclocal"

mkdir -p $DRV_BUILD_DIR
cd $DRV_BUILD_DIR

build_xf86drv_ac    input-elographics       1.4.4.2
build_xf86drv_ac    input-evdev             2.11.0.2
build_xf86drv_ac    input-joystick          1.6.4.2
build_xf86drv_ac    input-keyboard          2.1.0.2
build_xf86drv_ac    input-libinput          1.5.1.0
build_xf86drv_ac    input-synaptics         1.10.0.2
build_xf86drv_ac    input-vmmouse           13.2.0.4
build_xf86drv_ac    input-void              1.4.2.3
build_xf86drv_ac    input-wacom             1.2.3.3

build_xf86drv_ac    video-amdgpu            23.0.0.5
build_xf86drv_ac    video-apm               1.3.0.3
build_xf86drv_ac    video-ark               0.7.6.2
build_xf86drv_ac    video-ast               1.2.1
build_xf86drv_ac    video-ati               22.0.0.3
build_xf86drv_ac    video-chips             1.5.0.3
build_xf86drv_ac    video-cirrus            1.6.1
build_xf86drv_ac    video-dummy             0.4.1.3
build_xf86drv_ac    video-fbdev             0.5.2
build_xf86drv_ac    video-freedreno         1.4.1
build_xf86drv_ac    video-geode             2.18.1.3
build_xf86drv_ac    video-i128              1.4.1.2
build_xf86drv_ac    video-i740              1.4.0.2
build_xf86drv_ac    video-intel             3.0.0.3
build_xf86drv_ac    video-mach64            6.10.0.3
build_xf86drv_ac    video-mga               2.1.0.2
build_xf86drv_ac    video-neomagic          1.3.1.2
build_xf86drv_ac    video-nested            1.0.0.2
build_xf86drv_ac    video-nouveau           1.0.18.3
build_xf86drv_ac    video-nv                2.1.23.2
build_xf86drv_ac    video-omap              0.4.5.2
build_xf86drv_ac    video-qxl               0.1.6.2
build_xf86drv_ac    video-r128              6.13.0.2
build_xf86drv_ac    video-rendition         4.2.7.2
build_xf86drv_ac    video-s3virge           1.11.1.2
build_xf86drv_ac    video-savage            2.4.1.2
build_xf86drv_ac    video-siliconmotion     1.7.10.2
build_xf86drv_ac    video-sis               0.12.0.3
build_xf86drv_ac    video-sisusb            0.9.7.2
build_xf86drv_ac    video-suncg14           1.2.0.1
build_xf86drv_ac    video-suncg3            1.1.3.0
build_xf86drv_ac    video-suncg6            1.1.3.2
build_xf86drv_ac    video-sunffb            1.2.3.2
build_xf86drv_ac    video-suntcx            1.1.3.2
build_xf86drv_ac    video-sunleo            1.2.3.2
build_xf86drv_ac    video-tdfx              1.5.0.2
build_xf86drv_ac    video-trident           1.4.0.2
build_xf86drv_ac    video-vesa              2.6.0.2
build_xf86drv_ac    video-vmware            13.4.0.3
build_xf86drv_ac    video-v4l               0.3.0.2
build_xf86drv_ac    video-vbox              1.0.1.2
build_xf86drv_ac    video-voodoo            1.2.6.3
build_xf86drv_ac    video-xgi               1.6.1.3
