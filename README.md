XLibre Xserver
===============
XLibre represents a significant fork of the Xorg Xserver, designed to advance X server technology through comprehensive code cleanups, architectural improvements, and enhanced functionality. This project emerged from the need to maintain active development and innovation in the X server ecosystem, ensuring that this critical piece of infrastructure continues to evolve and meet modern computing demands.
The decision to create this independent fork was driven by concerns about the future direction and development pace of the original Xorg project. We observed a pattern of stagnation and resistance to substantial improvements that would benefit the broader community. By establishing XLibre, we aim to create an environment where technical merit drives decisions and where contributors can focus on advancing the technology without unnecessary barriers or political considerations.
Following the public announcement of XLibre on June 6th, 2025, our original development infrastructure on freedesktop.org was unexpectedly removed, including repositories, issue tracking, and merge requests. This action necessitated the immediate relocation of our development efforts to alternative platforms. While disruptive, this event reinforced our commitment to maintaining project independence and demonstrated the importance of having robust, decentralized development infrastructure.

Project Philosophy and Community
--------------------------------

XLibre operates as a completely independent project, free from corporate control, political influence, or external organizational pressures. Our development decisions are guided solely by technical excellence, community needs, and the goal of creating the most robust and feature-rich X server implementation possible. We maintain strict neutrality regarding corporate interests and political movements, focusing exclusively on technical advancement.
Our community welcomes developers, contributors, and users from all backgrounds, countries, and walks of life. We believe that diversity of thought, experience, and perspective strengthens our technical work. The only requirements for participation are technical competence, respectful communication, and a genuine interest in improving X server technology. We foster an environment where ideas are evaluated based on their technical merit rather than the identity or background of their proposers.
We are committed to maintaining an inclusive development environment where all contributors can participate productively. This means establishing clear technical standards, maintaining respectful discourse, and ensuring that project decisions are made 
transparently through technical discussion rather than political maneuvering or corporate influence. Our goal is to create software that serves users effectively, regardless of their personal characteristics or beliefs.

Technical Vision and Development Goals
--------------------------------
The XLibre project aims to modernize and enhance the X server architecture while maintaining compatibility with existing applications and workflows. Our development priorities include improving performance, enhancing security, simplifying the codebase, and adding features that benefit end users and system administrators. We are particularly focused on addressing long-standing architectural limitations and technical debt that has accumulated in the codebase over decades of development.
Performance optimization represents a core focus area for XLibre. We are implementing numerous improvements to rendering pipelines, memory management, and inter-process communication mechanisms. These enhancements are designed to reduce latency, improve throughput, and provide more consistent performance across different hardware configurations and usage patterns. Our benchmarking efforts consistently demonstrate measurable improvements over the baseline Xorg implementation.
Security enhancements form another critical component of our development strategy. We are systematically reviewing and hardening the codebase against various attack vectors, implementing modern security practices, and improving privilege separation mechanisms. These efforts include both proactive security measures and reactive responses to newly discovered vulnerabilities. Our security-focused development approach helps ensure that XLibre can be deployed confidently in security-sensitive environments.

Upgrade notice
--------------

* Module ABIs have changed - drivers MUST be recompiled against this Xserver
  version, otherwise the Xserver can crash or fail to start up correctly.

* If your console is locked up (no input possible, not even VT switch), then
  most likely the input driver couldn't be loaded due to a version mismatch.
  When unsure, it's best be prepared to ssh into your machine from another one
  or set a timer that's calling `chvt 1` after certain time, so you don't
  need a cold reboot.

* Proprietary Nvidia drivers might break: they still haven't managed to do
  do even simple cleanups to catch up with Xorg master for about a year.
  All attempts to get into direct mail contact have failed. We're trying to
  work around this, but cannot give any guarantees.

* Most Xorg drivers should run as-is (once recompiled!), with some exceptions.
  See `.gitlab-ci.yml` for the versions/branches built along with Xlibre.


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

|  |  |
| --- | --- |
| Mailing list:                     | https://www.freelists.org/list/xlibre |
| Telegram channel:                 | https://t.me/x11dev |
| Matrix room (mirror of tg group): | https://matrix.to/#/#xlibre:matrix.org |
