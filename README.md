<img src="./XLibre_Logo.svg" alt="XLibre Logo" width="200"/>

XLibre Xserver
===============

Xlibre is a fork of the [Xorg Xserver](https://gitlab.freedesktop.org/xorg/xserver)
with lots of code cleanups and enhanced functionality.

This fork was necessary since toxic elements within Xorg projects, moles
from BigTech, are boycotting any substantial work on Xorg, in order to
destroy the project, to eliminate competition of their own products.
Classic "embrace, extend, extinguish" tactics.

Right after journalists first began covering the planned fork Xlibre,
on June 6th 2025, Redhat employees started a purge on the Xlibre founder's
GitLab account on freedesktop.org: deleted the git repo, tickets, merge
requests, etc, and so fired the shot that the whole world heard.

This is an independent project, not at all affiliated with BigTech or any
of their subsidiaries or tax evasion tools, nor any political activists
groups, state actors, etc. It's explicitly free of any "DEI" or similar
discriminatory policies. Anybody who's treating others nicely is welcomed.

It doesn't matter which country you're coming from, your political views,
your race, your sex, your age, your food menu, whether you wear boots or
heels, whether you're furry or fairy, Conan or McKay, comic character, a
small furry creature from Alpha Centauri, or just a boring average person.
Anybody who's interested in bringing X forward is welcome.

Together we'll make X great again!

Upgrade notice
--------------

* Module ABIs have changed - drivers MUST be recompiled against this Xserver
  version, otherwise the Xserver can crash or fail to start up correctly.

* If your console is locked up (no input possible, not even VT switch), then
  most likely the input driver couldn't be loaded due to a version mismatch.
  When unsure, it's best to be prepared to ssh into your machine from another one
  or set a timer that's calling `chvt 1` after certain time, so you don't
  need a cold reboot.
  Or, make sure that you have magic `SysRq` key enabled (`Alt+PrtSc`)
  via sysctl (`kernel.sysrq=1`), then press following combination depending on keyboard
  layout to make kernel regain control over keyboard to make VT switching work:
  - QWERTY/AZERTY keyboard layout: `SysRq + R`
  - Dvorak/Colemak keyboard layout: `SysRq + P`

* Proprietary Nvidia drivers might break: they still haven't managed to do
  even simple cleanups to catch up with Xorg master for about a year.
  All attempts to get into direct mail contact have failed. We're trying to
  work around this, but cannot give any guarantees. But you can make it work
  by adding `Option "IgnoreABI" "1"` line to `ServerFlags` section in Xorg config.

* Most Xorg drivers should run as-is (once recompiled!), with some exceptions.
  See `.gitlab-ci.yml` for the versions/branches built along with Xlibre.


Driver repositories
-------------------

Since Redhat had deleted and banned all X11Libre repositories from freedesktop.org,
the driver repositories are now moved to GitHub:

| Driver | Git repository | Release tag |
| --- | --- | --- |
| xf86-input-elographics:   | https://github.com/X11Libre/xf86-input-elographics    | xlibre-xf86-input-elographics-1.4.4.1    |
| xf86-input-evdev:         | https://github.com/X11Libre/xf86-input-evdev          | xlibre-xf86-input-evdev-2.11.0.1         |
| xf86-input-void:          | https://github.com/X11Libre/xf86-input-void           | xlibre-xf86-input-void-1.4.2.1           |
| xf86-input-joystick:      | https://github.com/X11Libre/xf86-input-joystick       | xlibre-xf86-input-joystick-1.6.4.1       |
| xf86-input-keyboard:      | https://github.com/X11Libre/xf86-input-keyboard       | xlibre-xf86-input-keyboard-2.1.0.1       |
| xf86-input-libinput:      | https://github.com/X11Libre/xf86-input-libinput       | xlibre-xf86-input-libinput-1.5.0.1       |
| xf86-input-mouse:         | https://github.com/X11Libre/xf86-input-mouse          | xlibre-xf86-input-mouse-1.9.5.1          |
| xf86-input-synaptics:     | https://github.com/X11Libre/xf86-input-synaptics      | xlibre-xf86-input-synaptics-1.10.0.1     |
| xf86-input-vmmouse:       | https://github.com/X11Libre/xf86-input-vmmouse        | xlibre-xf86-input-vmmouse-13.2.0.1       |
| xf86-input-wacom:         | https://github.com/X11Libre/xf86-input-wacom          | xlibre-xf86-input-wacom-1.2.3.1          |
| xf86-video-amdgpu:        | https://github.com/X11Libre/xf86-video-amdgpu         | xlibre-xf86-video-amdgpu-23.0.0.1        |
| xf86-video-apm:           | https://github.com/X11Libre/xf86-video-apm            | xlibre-xf86-video-apm-1.3.0.1            |
| xf86-video-ark:           | https://github.com/X11Libre/xf86-video-ark            | xfree-xf86-video-ark-0.7.6.1             |
| xf86-video-ast:           | https://github.com/X11Libre/xf86-video-ast            | xlibre-xf86-video-ast-1.2.0              |
| xf86-video-ati:           | https://github.com/X11Libre/xf86-video-ati            | xfree-xf86-video-ati-22.0.0.1            |
| xf86-video-chips:         | https://github.com/X11Libre/xf86-video-chips          | xlibre-xf86-video-chips-1.5.0.1          |
| xf86-video-cirrus:        | https://github.com/X11Libre/xf86-video-cirrus         | xlibre-xf86-video-cirrus-1.6.0.1         |
| xf86-video-dummy:         | https://github.com/X11Libre/xf86-video-dummy          | xlibre-xf86-video-dummy-0.4.1.1          |
| xf86-video-fbdev:         | https://github.com/X11Libre/xf86-video-fbdev          | xlibre-xf86-video-fbdev-0.5.1.1          |
| xf86-video-freedreno:     | https://github.com/X11Libre/xf86-video-freedreno      | xlibre-xf86-video-freedreno-1.4.0.1      |
| xf86-video-geode:         | https://github.com/X11Libre/xf86-video-geode          | xlibre-xf86-video-geode-2.18.1.1         |
| xf86-video-i128:          | https://github.com/X11Libre/xf86-video-i128           | xlibre-xf86-video-i128-1.4.1.1           |
| xf86-video-i740:          | https://github.com/X11Libre/xf86-video-i740           | xlibre-xf86-video-i740-1.4.0.1           |
| xf86-video-intel:         | https://github.com/X11Libre/xf86-video-intel          | xlibre-xf86-video-intel-3.0.0.1          |
| xf86-video-mach64:        | https://github.com/X11Libre/xf86-video-mach64         | xlibre-xf86-video-mach64-6.10.0.1        |
| xf86-video-mga:           | https://github.com/X11Libre/xf86-video-mga            | xlibre-xf86-video-mga-2.1.0.1            |
| xf86-video-neomagic:      | https://github.com/X11Libre/xf86-video-neomagic       | xlibre-xf86-video-neomagic-1.3.1.1       |
| xf86-video-nested:        | https://github.com/X11Libre/xf86-video-nested         | xlibre-xf86-video-nested-1.0.0.1         |
| xf86-video-nouveau:       | https://github.com/X11Libre/xf86-video-nouveau        | xlibre-xf86-video-nouveau-1.0.18.1       |
| xf86-video-nv:            | https://github.com/X11Libre/xf86-video-nv             | xlibre-xf86-video-nv-2.1.23.1            |
| xf86-video-omap:          | https://github.com/X11Libre/xf86-video-omap           | xlibre-xf86-video-omap-0.4.5.1           |
| xf86-video-qxl:           | https://github.com/X11Libre/xf86-video-qxl            | xlibre-xf86-video-qxl-0.1.6.1            |
| xf86-video-r128:          | https://github.com/X11Libre/xf86-video-r128           | xlibre-xf86-video-r128-6.13.0.1          |
| xf86-video-rendition:     | https://github.com/X11Libre/xf86-video-rendition      | xlibre-xf86-video-rendition-4.2.7.1      |
| xf86-video-s3virge:       | https://github.com/X11Libre/xf86-video-s3virge        | xlibre-xf86-video-s3virge-1.11.1.1       |
| xf86-video-savage:        | https://github.com/X11Libre/xf86-video-savage         | xlibre-xf86-video-savage-2.4.1.1         |
| xf86-video-siliconmotion: | https://github.com/X11Libre/xf86-video-siliconmotion  | xlibre-xf86-video-siliconmotion-1.7.10.1 |
| xf86-video-sis:           | https://github.com/X11Libre/xf86-video-sis            | xlibre-xf86-video-sis-0.12.0.1           |
| xf86-video-sisusb:        | https://github.com/X11Libre/xf86-video-sisusb         | xlibre-xf86-video-sisusb-0.9.7.1         |
| xf86-video-suncg14:       | https://github.com/X11Libre/xf86-video-suncg14        | xlibre-xf86-video-suncg14-1.2.0          |
| xf86-video-suncg3:        | https://github.com/X11Libre/xf86-video-suncg3         | xlibre-xf86-video-suncg3-1.1.3.0         |
| xf86-video-suncg6:        | https://github.com/X11Libre/xf86-video-suncg6         | xlibre-xf86-video-suncg6-1.1.3.1         |
| xf86-video-sunffb:        | https://github.com/X11Libre/xf86-video-sunffb         | xlibre-xf86-video-sunffb-1.2.3.1         |
| xf86-video-sunleo:        | https://github.com/X11Libre/xf86-video-sunleo         | xlibre-xf86-video-sunleo-1.2.3.1         |
| xf86-video-suntcx:        | https://github.com/X11Libre/xf86-video-suntcx         | xlibre-xf86-video-suntcx-1.1.3.1         |
| xf86-video-tdfx:          | https://github.com/X11Libre/xf86-video-tdfx           | xlibre-xf86-video-tdfx-1.5.0.1           |
| xf86-video-trident:       | https://github.com/X11Libre/xf86-video-trident        | xlibre-xf86-video-trident-1.4.0.1        |
| xf86-video-vbox:          | https://github.com/X11Libre/xf86-video-vbox           | xlibre-xf86-video-vbox-1.0.1.1           |
| xf86-video-v4l:           | https://github.com/X11Libre/xf86-video-v4l            | xlibre-xf86-video-v4l-0.3.0.1            |
| xf86-video-vesa:          | https://github.com/X11Libre/xf86-video-vesa           | xlibre-xf86-video-vesa-2.6.0.1           |
| xf86-video-vmware:        | https://github.com/X11Libre/xf86-video-vmware         | xlibre-xf86-video-vmware-13.4.0.1        |
| xf86-video-voodoo:        | https://github.com/X11Libre/xf86-video-voodoo         | xlibre-xf86-video-voodoo-1.2.6.1         |
| xf86-video-wsfb:          | https://github.com/X11Libre/xf86-video-wsfb           | xlibre-xf86-video-wsfb-0.4.1.1           |
| xf86-video-xgi:           | https://github.com/X11Libre/xf86-video-xgi            | xlibre-xf86-video-xgi-1.6.1.1            |


Contact
-------

|  |  |
| --- | --- |
| Mailing list:                     | https://www.freelists.org/list/xlibre |
| Telegram channel:                 | https://t.me/x11dev |
| Matrix room (mirror of tg group): | https://matrix.to/#/#xlibre:matrix.org |
