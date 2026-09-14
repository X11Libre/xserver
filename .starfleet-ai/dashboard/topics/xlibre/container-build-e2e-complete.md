Automatischen Stash erzeugt: 82643f8495
Aktueller Branch mtx/agent-config ist auf dem neuesten Stand.
Automatischen Stash angewendet.
Title: ""
Category: xlibre
Status: "open"
Assigned-To: "—"
Created-By: ""
Created: ""
Doc-Ref: ""

## Zusammenfassung

Container-Build fuer xserver-container (Devuan daedalus) komplett erfolgreich:
**60/60 Pakete gebaut, 0 Fehler, EXIT 0**. Zusaetzlich alle ~51 Treiber auf die
`action-build-driver@v0.4.0`-Pipeline hochgezogen (inkl. FreeBSD `bzip2.pc`-Fix) und
Plattform-Abdeckung validiert.

## Was gemacht wurde

### os-installed pkg-config-Modellierung
- 33 Pakete in `cf/xserver-master/packages/os-installed/` mit `pkg-config:` ergaenzt/erstellt
- 23 neue Pakete erstellt (xfont2, fontenc, gl, egl, gbm, epoxy, xcb-*, xkbfile, libbsd, pixman, xshmfence, libevdev, mtdev, libinput, xrandr, xinerama, spice-protocol)
- `SysPackage()` in `syspackage.go` fuehrt `pkg-config --modversion` je Paket aus (im Container)

### asprintf/_GNU_SOURCE-Bug (6 Treiber)
- GCC14 + `-Werror` → `implicit declaration of function 'asprintf'`
- Fix: `AC_USE_SYSTEM_EXTENSIONS` in `configure.ac`
- Betroffen: xf86-input-mouse, xf86-video-mach64, xf86-video-ati, xf86-video-amdgpu, xf86-video-nouveau, xf86-video-qxl
- Submittet als PRs: mouse#15, mach64#10, ati#33, amdgpu#66, nouveau#20, qxl#19

### action-build-driver v0.4.0 (FreeBSD bzip2-Fix)
- FreeBSD `bzip2`-Paket hat kein `bzip2.pc` → freetype2 `Requires.private` bricht
  xfont2-pkgconf. Fix: `bzip2.pc` synthetisieren (action-build-driver PR #4 → master `7a0a5cf`, Tag **v0.4.0**).
- Alle ~51 Treiber auf `@v0.4.0`-Bump-PRs: 6 asprintf-Treiber (mouse#16 mach64#11 ati#34
  amdgpu#67 nouveau#21 qxl#20) + 45 weitere via xx-make-pr (jeweils assignee + Review-Team X11Libre/dev).
- Bump-Skript: `_WORK_/bump-ci-v0.4.0.sh`.

## CI-Status der 6 Bump-PRs
- **FreeBSD-Lanes: alle gruen** (mouse/mach64/qxl/amdgpu/ati freebsd 3/3 SUCCESS; nouveau keine freebsd-Lane)
- `ati dragonfly (xlibre-xserver-25.1.0)`: reproduzierbare VM-Boot-**Flake** (nicht blockierend,
  auch in asprintf-PR #33 mal fail/pass; nicht vom Bump verursacht)

## Plattform-Validierung
- Korrekte dokumentierte Ausnahmen: wacom/freedreno/omap/v4l (Linux-only), nouveau (kein BSD), geode (32bit, kein Dragonfly), neomagic/nested (dragonfly nur master/25.1)
- **Luecke: xf86-video-sisusb** — baut nur ubuntu, keine BSD-Lanes, kein Ausnahme-Kommentar, alte `target-ubuntu`-Struktur → eigener PR fuer 4-Lane-Migration empfohlen

## Release-/Tag-Empfehlung (an Praetor)
- Prio: 6 asprintf-Treiber (echte Build-Fixes) → neuer Tag (25.x-Level hoch)
- Aktiv gepflegt (intel/evdev/libinput/synaptics/vesa/vmware/vbox/ast/mga/savage/...) → neuer Tag nach Bump-Merge
- Obskure Alttreiber (apm/ark/chips/cirrus/i128/i740/neomagic/suncg*/suntcx/sunleo/sunffb/trident/voodoo/xgi/sisusb) → nur Merge

## Ergebnis
- 60 Tarballs erzeugt; alle Treiber auf v0.4.0-Pipeline; WARTET auf Praetor-Entscheidung bzgl. Merge + Release-Tags.
