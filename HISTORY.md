# History of the XLibre fork

This file contains a brief historical overview of the events surrounding the fork of Xorg into XLibre. It includes copies of the original unmodified documents, including references to their sources, so that readers can form their own opinions.

The only two changes made were to reduce the original heading levels to improve readability and, where necessary, to add headings and short introductions to provide factual and chronological context. Spelling errors have been retained.


## Message of metux sent to the X.org mailing list

This is the e-mail sent from Enrico Weigelt alias metux to the X.org mailing list hosted at freedesktop.org on June 6, 2025.

---

### History repeats: Redhat censored me on freedesktop.org - Xlibre fork release coming in few days

**Enrico Weigelt, metux IT consult** [info at metux.net](mailto:xorg-devel%40lists.x.org?Subject=Re%3A%20History%20repeats%3A%20Redhat%20censored%20me%20on%20freedesktop.org%20-%20Xlibre%20fork%0A%20release%20coming%20in%20few%20days&In-Reply-To=%3C1a9d25cc-6f02-4fe3-a206-3388c07ab81f%40metux.net%3E "History repeats: Redhat censored me on freedesktop.org - Xlibre fork release coming in few days")  
_Fri Jun 6 13:50:32 UTC 2025_

-   Next message (by thread): [Reminder: Registration & Call for Proposals open for XDC 2025](https://lists.x.org/archives/xorg-devel/2025-June/059397.html)
-   **Messages sorted by:** [\[ date \]](https://lists.x.org/archives/xorg-devel/2025-June/date.html#59396) [\[ thread \]](https://lists.x.org/archives/xorg-devel/2025-June/thread.html#59396) [\[ subject \]](https://lists.x.org/archives/xorg-devel/2025-June/subject.html#59396) [\[ author \]](https://lists.x.org/archives/xorg-devel/2025-June/author.html#59396)

Hello everybody,


this morning, Redhat employees banned me from the freedesktop.org gitlab
infrastructure - so censored all my work (not just on Xorg). They killed
my account, my git repos, my tickets in Xorg and closed all my merge
requests. And then making fun on social media about it.

They fired the shot that's heared around the world.

So much for freedesktop.org being "independent" and embracing freedom. 
Perhaps we should nominate them for the next Orwell award.

It's now clear that freedesktop.org *is* the Redskirts, and they want
to kill X. By the way, the same corporation that tied to proprietarize a
lot of FOSS code, including the Linux kernel (and I've been one of those 
who warned them about terminating our license grants them).

My most evil heresies probably were:

a) forking Xorg and making *actual progress*<br>
b) talking to a journalist whose name must not be spoken in many other
    Redhat/IBM tax evasion outlets, like GNOME (they're also banning
    honorable long time contributors for just mentioning that name)<br>
c) inviting *anybody* to join me, without discrimination

I don't know why, but it really looks they're quite scared by one guy
that's just trying to actually bring X11 forward. Hard to find he right
words for telling how honored I'm about that.

This didn't actually surprise me, I knew this would be coming for about
a year now. Just didn't expect them to do such an extremely irrational
and dumb move. Now I'm taking great pleasure seeing the Streisand effect
kicking in (my inbox is exploding). Thanks for that great publicity.

It's not the first time this happens in FOSS world, and it's not the
first time it's happening in X: remember what Xfree86 board did to
the honorable Keith Packard, back about two decades ago - what lead to
the birth of Xorg and the death of Xfree86. Same is happening again.

History repeats itself.

And now the Redskirts placed me onto the same stage as the great
honorable Keith Packard. WOOOOW.

Just to be clear, I didn't want to fork, I tried my best to work
together with the Xorg team. But I knew for long time, this day would
come. Xorg has been captured by Redhat, in order to get rid of destroy
competition. The necessary consequence is a fork, more competition.

For those interested in bringing X forward, feel free to join the
mailing list:

https://www.freelists.org/list/xlibre

Git repo:<br>
https://github.com/X11Libre/xserver.git

I'm expecting to be banned from whole freedesktop.org mail system, too.
Excommunication unfortunately had become a common thing in the so-called
"free software" world - GNOME is just one of many examples. So if you
don't hear anymore from me on freedesktop.org lists, you know what's
going on.

Join the xlibre mailing list to stay tuned.

Together, we'll make X great again.


have fun,<br>
--mtx

--<br>
\--- Enrico Weigelt, metux IT consult<br>
Free software and Linux embedded engineering<br>
[info at metux.net](https://lists.x.org/mailman/listinfo/xorg-devel) -- +49-151-27565287

---

Source: [History repeats: Redhat censored me on freedesktop.org - Xlibre fork release coming in few days](https://lists.x.org/archives/xorg-devel/2025-June/059396.html)


## First version of the README of XLibre at Github

The first version of the XLibre-specific [`README.md`](https://github.com/X11Libre/xserver/blob/master/README.md) file was created on June 12, 2025, and updated throughout the day. The version below reflects the status at the end of June 12, 2025.

---

### XLibre Xserver

Xlibre is a fork of the Xorg Xserver, with lots of code cleanups
and enhanced functionality.

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
small furry creature from Alpha Centauri, or just an boring average person.
Anybody who's interested in bringing X forward is welcome.

Together we'll make X great again!

#### Upgrade notice

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

#### Driver repositories

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


#### Contact

|  |  |
| --- | --- |
| Mailing list:                     | https://www.freelists.org/list/xlibre |
| Telegram channel:                 | https://t.me/x11dev |
| Matrix room (mirror of tg group): | https://matrix.to/#/#xlibre:matrix.org |

---

Source: [xserver/README.md at fea8b78 - X11Libre/xserver - Github](https://github.com/X11Libre/xserver/blob/fea8b78358a169238a288952b77e15422837e8a5/README.md)<br>
Also see: [History for README.md - X11Libre/xserver - Github](https://github.com/X11Libre/xserver/commits/master/README.md)
