XLibre Xserver
===============

X11Libre...

A free and open "community and contributor driven" open source fork of xorg in the spirit of the UNIX philosophy.

Upgrade notice
--------------

* Module ABIs have changed - drivers MUST be recompiled against this Xserver
  verison, otherwise the Xserver can crash or not even start up correctly.

* If your console is locked up (no input possible, not even VT switch), then
  most likely the input driver couldn't be loaded due version mismatch.
  When unsure, Better be prepared to ssh into your machine from another one
  or set a timer that's calling `chvt 1` after certain time, so you'll don't
  need a cold reboot.

* Proprietary NVidia drivers might break: they still haven't managed to do
  do even simple cleanups to catch up with Xorg master for about a year.
  All attempts to get into direct mail contact have failed. We're trying to
  work around this, but cannot give any guarantees.

* Most xorg drivers should run as-is (need recompile!), with some exceptions.
  See .gitlab-ci.yml for the versions/branches built along w/ Xlibre.


Driver repositories
-------------------

Since Redhat had deleted and banned all X11Libre repositories from freedesktop.org,
the driver repositories are now moved to github:

| Driver | Git repository |
| --- | --- |
| xf86-input-elographics:   | https://github.com/X11Libre/xf86-input-elographics    |
| xf86-input-evdev:         | https://github.com/X11Libre/xf86-input-evdev          |
| xf86-input-joystick:      | https://github.com/X11Libre/xf86-input-joystick       |
| xf86-input-keyboard:      | https://github.com/X11Libre/xf86-input-keyboard       |
| xf86-input-libinput:      | https://github.com/X11Libre/xf86-input-libinput       |
| xf86-input-mouse:         | https://github.com/X11Libre/xf86-input-mouse          |
| xf86-input-synaptics:     | https://github.com/X11Libre/xf86-input-synaptics      |
| xf86-input-vmmouse:       | https://github.com/X11Libre/xf86-input-vmmouse        |
| xf86-input-amdgpu:        | https://github.com/X11Libre/xf86-video-amdgpu         |
| xf86-input-apm:           | https://github.com/X11Libre/xf86-video-apm            |
| xf86-input-ark:           | https://github.com/X11Libre/xf86-video-ark            |
| xf86-input-ast:           | https://github.com/X11Libre/xf86-video-ast            |
| xf86-input-ati:           | https://github.com/X11Libre/xf86-video-ati            |
| xf86-input-chips:         | https://github.com/X11Libre/xf86-video-chips          |
| xf86-input-cirrus:        | https://github.com/X11Libre/xf86-video-cirrus         |
| xf86-input-dummy:         | https://github.com/X11Libre/xf86-video-dummy          |
| xf86-input-fbdev:         | https://github.com/X11Libre/xf86-video-fbdev          |
| xf86-input-freedreno:     | https://github.com/X11Libre/xf86-video-freedreno      |
| xf86-input-geode:         | https://github.com/X11Libre/xf86-video-geode          |
| xf86-input-i128:          | https://github.com/X11Libre/xf86-video-i128           |
| xf86-input-i740:          | https://github.com/X11Libre/xf86-video-i740           |
| xf86-input-i740:          | https://github.com/X11Libre/xf86-video-i740           |
| xf86-input-intel:         | https://github.com/X11Libre/xf86-video-intel          |
| xf86-input-mach64:        | https://github.com/X11Libre/xf86-video-mach64         |
| xf86-input-mga:           | https://github.com/X11Libre/xf86-video-mga            |
| xf86-input-neomagic:      | https://github.com/X11Libre/xf86-video-neomagic       |
| xf86-input-nested:        | https://github.com/X11Libre/xf86-video-nested         |
| xf86-input-nouveau:       | https://github.com/X11Libre/xf86-video-nouveau        |
| xf86-input-nv:            | https://github.com/X11Libre/xf86-video-nv             |
| xf86-input-omap:          | https://github.com/X11Libre/xf86-video-omap           |
| xf86-input-qxl:           | https://github.com/X11Libre/xf86-video-qxl            |
| xf86-input-r128:          | https://github.com/X11Libre/xf86-video-r128           |
| xf86-input-rendition:     | https://github.com/X11Libre/xf86-video-rendition      |
| xf86-input-s3virge:       | https://github.com/X11Libre/xf86-video-s3virge        |
| xf86-input-savage:        | https://github.com/X11Libre/xf86-video-savage         |
| xf86-input-siliconmotion: | https://github.com/X11Libre/xf86-video-siliconmotion  |
| xf86-input-sis:           | https://github.com/X11Libre/xf86-video-sis            |
| xf86-input-sisusb:        | https://github.com/X11Libre/xf86-video-sisusb         |
| xf86-input-suncg14:       | https://github.com/X11Libre/xf86-video-suncg14        |
| xf86-input-suncg3:        | https://github.com/X11Libre/xf86-video-suncg3         |
| xf86-input-suncg6:        | https://github.com/X11Libre/xf86-video-suncg6         |
| xf86-input-sunffb:        | https://github.com/X11Libre/xf86-video-sunffb         |
| xf86-input-sunleo:        | https://github.com/X11Libre/xf86-video-sunleo         |
| xf86-input-suntcx:        | https://github.com/X11Libre/xf86-video-suntcx         |
| xf86-input-tdfx:          | https://github.com/X11Libre/xf86-video-tdfx           |
| xf86-input-trident:       | https://github.com/X11Libre/xf86-video-trident        |
| xf86-input-v4l:           | https://github.com/X11Libre/xf86-video-v4l            |
| xf86-input-vesa:          | https://github.com/X11Libre/xf86-video-vesa           |
| xf86-input-vmware:        | https://github.com/X11Libre/xf86-video-vmware         |
| xf86-input-voodoo:        | https://github.com/X11Libre/xf86-video-voodoo         |
| xf86-input-wsfb:          | https://github.com/X11Libre/xf86-video-wsfb           |
| xf86-input-xgi:           | https://github.com/X11Libre/xf86-video-xgi            |


Contact
-------

* Mailing list: https://www.freelists.org/list/xlibre
* Telegram channel: https://t.me/x11dev
