XLibre Xserver
==============

XLibre is a fork of the Xorg Xserver, with lots of code cleanups and enhanced
functionality.

XLibre intends to deliver an independent X server that will last long into the
future, with no conflicts of interest or corporate affiliations.

Anyone interested in bringing X forward is welcome to contribute.

Upgrade notice
--------------

* Module ABIs have changed - drivers MUST be recompiled against this Xserver
  version, otherwise the Xserver can crash or fail to start up correctly.

* If your console is locked up (no input possible, not even VT switch), then
  most likely the input driver couldn't be loaded due to a version mismatch.
  When unsure, it's best be prepared to SSH into your machine from another one
  or set a timer to call `chvt 1` after a certain duration of time, so that you
  don't need to cold reboot.

* Proprietary Nvidia drivers might break: they still haven't managed to do
  do even simple cleanups to catch up with Xorg master for about a year.
  All attempts to get into direct mail contact have failed. We're trying to
  work around this, but cannot give any guarantees.

* Most Xorg drivers should run as-is (once recompiled!), with some exceptions.
  See `.gitlab-ci.yml` for the versions/branches built along with XLibre.


Driver repositories
-------------------

Since Red Hat has purged all XLibre repositories from freedesktop.org, the
driver repositories have been migrated to GitHub:

| Driver                    | Git repository                                        |
| ------------------------- | ----------------------------------------------------- |
| xf86-input-elographics:   | https://github.com/X11Libre/xf86-input-elographics    |
| xf86-input-evdev:         | https://github.com/X11Libre/xf86-input-evdev          |
| xf86-input-joystick:      | https://github.com/X11Libre/xf86-input-joystick       |
| xf86-input-keyboard:      | https://github.com/X11Libre/xf86-input-keyboard       |
| xf86-input-libinput:      | https://github.com/X11Libre/xf86-input-libinput       |
| xf86-input-mouse:         | https://github.com/X11Libre/xf86-input-mouse          |
| xf86-input-synaptics:     | https://github.com/X11Libre/xf86-input-synaptics      |
| xf86-input-vmmouse:       | https://github.com/X11Libre/xf86-input-vmmouse        |
| xf86-video-amdgpu:        | https://github.com/X11Libre/xf86-video-amdgpu         |
| xf86-video-apm:           | https://github.com/X11Libre/xf86-video-apm            |
| xf86-video-ark:           | https://github.com/X11Libre/xf86-video-ark            |
| xf86-video-ast:           | https://github.com/X11Libre/xf86-video-ast            |
| xf86-video-ati:           | https://github.com/X11Libre/xf86-video-ati            |
| xf86-video-chips:         | https://github.com/X11Libre/xf86-video-chips          |
| xf86-video-cirrus:        | https://github.com/X11Libre/xf86-video-cirrus         |
| xf86-video-dummy:         | https://github.com/X11Libre/xf86-video-dummy          |
| xf86-video-fbdev:         | https://github.com/X11Libre/xf86-video-fbdev          |
| xf86-video-freedreno:     | https://github.com/X11Libre/xf86-video-freedreno      |
| xf86-video-geode:         | https://github.com/X11Libre/xf86-video-geode          |
| xf86-video-i128:          | https://github.com/X11Libre/xf86-video-i128           |
| xf86-video-i740:          | https://github.com/X11Libre/xf86-video-i740           |
| xf86-video-i740:          | https://github.com/X11Libre/xf86-video-i740           |
| xf86-video-intel:         | https://github.com/X11Libre/xf86-video-intel          |
| xf86-video-mach64:        | https://github.com/X11Libre/xf86-video-mach64         |
| xf86-video-mga:           | https://github.com/X11Libre/xf86-video-mga            |
| xf86-video-neomagic:      | https://github.com/X11Libre/xf86-video-neomagic       |
| xf86-video-nested:        | https://github.com/X11Libre/xf86-video-nested         |
| xf86-video-nouveau:       | https://github.com/X11Libre/xf86-video-nouveau        |
| xf86-video-nv:            | https://github.com/X11Libre/xf86-video-nv             |
| xf86-video-omap:          | https://github.com/X11Libre/xf86-video-omap           |
| xf86-video-qxl:           | https://github.com/X11Libre/xf86-video-qxl            |
| xf86-video-r128:          | https://github.com/X11Libre/xf86-video-r128           |
| xf86-video-rendition:     | https://github.com/X11Libre/xf86-video-rendition      |
| xf86-video-s3virge:       | https://github.com/X11Libre/xf86-video-s3virge        |
| xf86-video-savage:        | https://github.com/X11Libre/xf86-video-savage         |
| xf86-video-siliconmotion: | https://github.com/X11Libre/xf86-video-siliconmotion  |
| xf86-video-sis:           | https://github.com/X11Libre/xf86-video-sis            |
| xf86-video-sisusb:        | https://github.com/X11Libre/xf86-video-sisusb         |
| xf86-video-suncg14:       | https://github.com/X11Libre/xf86-video-suncg14        |
| xf86-video-suncg3:        | https://github.com/X11Libre/xf86-video-suncg3         |
| xf86-video-suncg6:        | https://github.com/X11Libre/xf86-video-suncg6         |
| xf86-video-sunffb:        | https://github.com/X11Libre/xf86-video-sunffb         |
| xf86-video-sunleo:        | https://github.com/X11Libre/xf86-video-sunleo         |
| xf86-video-suntcx:        | https://github.com/X11Libre/xf86-video-suntcx         |
| xf86-video-tdfx:          | https://github.com/X11Libre/xf86-video-tdfx           |
| xf86-video-trident:       | https://github.com/X11Libre/xf86-video-trident        |
| xf86-video-v4l:           | https://github.com/X11Libre/xf86-video-v4l            |
| xf86-video-vesa:          | https://github.com/X11Libre/xf86-video-vesa           |
| xf86-video-vmware:        | https://github.com/X11Libre/xf86-video-vmware         |
| xf86-video-voodoo:        | https://github.com/X11Libre/xf86-video-voodoo         |
| xf86-video-wsfb:          | https://github.com/X11Libre/xf86-video-wsfb           |
| xf86-video-xgi:           | https://github.com/X11Libre/xf86-video-xgi            |


Contact
-------

- Mailing list: https://www.freelists.org/list/xlibre
- Telegram channel: https://t.me/x11dev
- Matrix room: https://matrix.to/#/#xlibre:matrix.org
