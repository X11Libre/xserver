XLibre Xserver
===============

Xlibre project's fork of the Xorg xserver, with lots of code cleanups
and enhanced functionality.

That fork was necessary since toxic elements within Xorg projects, moles
from certian big corp are boycotting any substantial work on Xorg, in order
to destroy the project, to elimitate competition of their own products.
(classic "embrace, extend, extinguish" tactics)

This is an independent project, not at all affiliated with BigTech or any
of their subsidiaries or tax evasion tools, nor any political activists
groups, state actors, etc. It's explicitly free of any "DEI" or similar
discriminatory policies. Anybody who's treating others nicely is welcomed.


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
  All attempts to get into direct contact have failed. We're trying to work
  around this, but cannot give any guarantees.

* Most xorg drivers should run as-is (need recompile!), with some exceptions.
  See .gitlab-ci.yml for the versions/branches built along w/ Xlibre.


Contact
-------

* Mailing list: https://www.freelists.org/list/xlibre
* Telegram channel: https://t.me/x11dev
* Matrix room: https://matrix.to/#/#xlibre:matrix.org
