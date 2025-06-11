XLibre Xserver
===============

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


Contact
-------

* Mailing list: https://www.freelists.org/list/xlibre
* Telegram channel: https://t.me/x11dev
