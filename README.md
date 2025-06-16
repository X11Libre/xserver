# XLibre Xserver

**XLibre** is a major fork of the Xorg Xserver, created to push forward X server technology through extensive code cleanups, architectural improvements, and enhanced functionality. This project was born out of the need to maintain active development and innovation in the X server ecosystem, ensuring this critical infrastructure continues evolving to meet modern computing demands.

---

## Project Background

The decision to fork from the original Xorg project stemmed from concerns about its stagnation and resistance to substantial improvements that would benefit the broader community. By establishing XLibre, we strive to foster an environment where technical merit drives development decisions, free from unnecessary barriers or politics.

Following our public announcement on **June 6th, 2025**, our original development infrastructure on freedesktop.org was unexpectedly removed—including repositories, issue tracking, and merge requests. This forced us to relocate development to alternative platforms. Though disruptive, it reinforced our commitment to project independence and the importance of decentralized development infrastructure.

---

## Project Philosophy and Community

- **Independence:** XLibre operates completely free from corporate control, political influence, or external pressures.
- **Technical Merit:** Decisions are driven solely by technical excellence, community needs, and the goal of creating the most robust, feature-rich X server.
- **Inclusive Community:** Welcomes developers and users from all backgrounds worldwide, emphasizing respectful communication, technical competence, and shared interest in improving X server technology.
- **Transparency:** We maintain transparent technical discussions with no political maneuvering or corporate interference.
- **Respect and Neutrality:** We focus exclusively on technical advancement, remaining neutral on corporate and political matters.

---

## Technical Vision and Development Goals

- **Modernization:** Revamp the X server architecture while maintaining compatibility with existing applications and workflows.
- **Performance:** Optimize rendering pipelines, memory management, and inter-process communication to reduce latency and improve throughput across diverse hardware.
- **Security:** Harden the codebase against attacks, implement modern security practices, and improve privilege separation.
- **Code Quality:** Simplify and clean the codebase to reduce technical debt accrued over decades.
- **Features:** Add enhancements benefiting end users and system administrators.
- **Benchmarking:** Regular testing shows consistent improvements over baseline Xorg.

---

## Upgrade Notice

- **Module ABI Changes:** Drivers **must** be recompiled against the new Xserver version to avoid crashes or startup failures.
- **Input Driver Issues:** If the console locks up (no input, including VT switch), likely due to input driver version mismatch. We recommend preparing SSH access or a fallback VT switch timer (`chvt 1`) to avoid cold reboots.
- **Proprietary Nvidia Drivers:** May break due to lagging updates in Nvidia's codebase. We are working on workarounds but cannot guarantee stability.
- **Xorg Drivers:** Most should work once recompiled, with some exceptions. See `.gitlab-ci.yml` for tested versions and branches.

---

## Driver Repositories

Redhat deleted and banned all X11Libre repositories on freedesktop.org. We have moved all driver repositories to GitHub:

| Driver                       | Git Repository                                                |
|------------------------------|---------------------------------------------------------------|
| xf86-input-elographics        | https://github.com/X11Libre/xf86-input-elographics            |
| xf86-input-evdev             | https://github.com/X11Libre/xf86-input-evdev                  |
| xf86-input-joystick          | https://github.com/X11Libre/xf86-input-joystick               |
| xf86-input-keyboard          | https://github.com/X11Libre/xf86-input-keyboard               |
| xf86-input-libinput          | https://github.com/X11Libre/xf86-input-libinput               |
| xf86-input-mouse             | https://github.com/X11Libre/xf86-input-mouse                  |
| xf86-input-synaptics         | https://github.com/X11Libre/xf86-input-synaptics              |
| xf86-input-vmmouse           | https://github.com/X11Libre/xf86-input-vmmouse                |
| xf86-video-amdgpu            | https://github.com/X11Libre/xf86-video-amdgpu                 |
| xf86-video-apm               | https://github.com/X11Libre/xf86-video-apm                    |
| xf86-video-ark               | https://github.com/X11Libre/xf86-video-ark                    |
| xf86-video-ast               | https://github.com/X11Libre/xf86-video-ast                    |
| xf86-video-ati               | https://github.com/X11Libre/xf86-video-ati                    |
| xf86-video-chips             | https://github.com/X11Libre/xf86-video-chips                  |
| xf86-video-cirrus            | https://github.com/X11Libre/xf86-video-cirrus                 |
| xf86-video-dummy             | https://github.com/X11Libre/xf86-video-dummy                  |
| xf86-video-fbdev             | https://github.com/X11Libre/xf86-video-fbdev                  |
| xf86-video-freedreno         | https://github.com/X11Libre/xf86-video-freedreno              |
| xf86-video-geode             | https://github.com/X11Libre/xf86-video-geode                  |
| xf86-video-i128              | https://github.com/X11Libre/xf86-video-i128                   |
| xf86-video-i740              | https://github.com/X11Libre/xf86-video-i740                   |
| xf86-video-intel             | https://github.com/X11Libre/xf86-video-intel                  |
| xf86-video-mach64            | https://github.com/X11Libre/xf86-video-mach64                 |
| xf86-video-mga               | https://github.com/X11Libre/xf86-video-mga                    |
| xf86-video-neomagic          | https://github.com/X11Libre/xf86-video-neomagic               |
| xf86-video-nested            | https://github.com/X11Libre/xf86-video-nested                 |
| xf86-video-nouveau           | https://github.com/X11Libre/xf86-video-nouveau                |
| xf86-video-nv                | https://github.com/X11Libre/xf86-video-nv                     |
| xf86-video-omap              | https://github.com/X11Libre/xf86-video-omap                   |
| xf86-video-qxl               | https://github.com/X11Libre/xf86-video-qxl                    |
| xf86-video-r128              | https://github.com/X11Libre/xf86-video-r128                   |
| xf86-video-rendition         | https://github.com/X11Libre/xf86-video-rendition              |
| xf86-video-s3virge           | https://github.com/X11Libre/xf86-video-s3virge                |
| xf86-video-savage            | https://github.com/X11Libre/xf86-video-savage                 |
| xf86-video-siliconmotion     | https://github.com/X11Libre/xf86-video-siliconmotion          |
| xf86-video-sis               | https://github.com/X11Libre/xf86-video-sis                    |
| xf86-video-sisusb            | https://github.com/X11Libre/xf86-video-sisusb                 |
| xf86-video-suncg14           | https://github.com/X11Libre/xf86-video-suncg14                |
| xf86-video-suncg3            | https://github.com/X11Libre/xf86-video-suncg3                 |
| xf86-video-suncg6            | https://github.com/X11Libre/xf86-video-suncg6                 |
| xf86-video-sunffb            | https://github.com/X11Libre/xf86-video-sunffb                 |
| xf86-video-sunleo            | https://github.com/X11Libre/xf86-video-sunleo                 |
| xf86-video-suntcx            | https://github.com/X11Libre/xf86-video-suntcx                 |
| xf86-video-tdfx              | https://github.com/X11Libre/xf86-video-tdfx                   |
| xf86-video-trident           | https://github.com/X11Libre/xf86-video-trident                |
| xf86-video-v4l               | https://github.com/X11Libre/xf86-video-v4l                    |
| xf86-video-vesa              | https://github.com/X11Libre/xf86-video-vesa                   |
| xf86-video-vmware            | https://github.com/X11Libre/xf86-video-vmware                 |
| xf86-video-voodoo            | https://github.com/X11Libre/xf86-video-voodoo                 |
| xf86-video-wsfb              | https://github.com/X11Libre/xf86-video-wsfb                   |
| xf86-video-xgi               | https://github.com/X11Libre/xf86-video-xgi                    |

---

## Contact

| Channel                 | Link                                          |
|-------------------------|-----------------------------------------------|
| Mailing list            | [https://www.freelists.org/list/xlibre](https://www.freelists.org/list/xlibre) |
| Telegram channel        | [https://t.me/x11dev](https://t.me/x11dev)   |
| Matrix room (mirror of Telegram group) | [https://matrix.to/#/#xlibre:matrix.org](https://matrix.to/#/#xlibre:matrix.org) |

---
