Title: ""
Category: active
Status: "assigned"
Assigned-To: "Galaxy"

proof-of-life — FreeDOS + DESQview + DESQview/X booting under QEMU, DOS-side TCP/IP (mTCP recommended, works well with
QEMU's emulated ne2000 NIC) + packet driver configured so the X server is reachable over TCP from the host; verify with
a simple `xdpyinfo`-style connection from the host against the VM's IP:0 first. (2) only once (1) works: run our tools,
with realistic expectations — X11R5 predates RandR/XKB/GLX/Present/XInput2/Sync/Xinerama entirely, so most pyxtest cases
will fail at the extension-query step before even starting; the meaningful first test is go-x11proto's base
connection/handshake + simple core-protocol requests (CreateWindow, MapWindow, GetProperty), since the wire format has
barely changed since X11R4. Software is abandonware (DESQview/DESQview/X, Quarterdeck, defunct) — normal retrocomputing
practice to source from vetusware.com/WinWorldPC, but document exact source + version here once downloaded (not done
yet). Coordinate with Potemkin's DASHBOARD.md redesign (m0073, `DASHBOARD-RESTRUCTURE.md`) on where this row lands
if/when the migration actually happens — not yet, as of this entry (still one monolithic file).

**Phase 1 progress log (2026-07-06, hands-on subagent under Pegasus):**

- Host tools confirmed present: `qemu-system-i386` 10.0.7 (Debian), ImageMagick `convert`, `qemu-img`, `xdpyinfo`,
`mkisofs`/`genisoimage`. No `socat`; only traditional `netcat-traditional` (`nc`, no `-U` unix-socket support) — used
QEMU's HMP monitor over **raw TCP** (`-monitor tcp:127.0.0.1:4445,server,nowait`), driven with `printf '<cmd>\n' | nc
-q1 127.0.0.1 4445`. Works fine (HMP over `tcp:` doesn't need telnet negotiation).
- All working files under `_WORK_/desqview-x/{iso,disk,media,screenshots,notes}` (gitignored). Three small throwaway
driver scripts written there (not committed, not under `scripts/`): `shot.sh` (screendump→PPM→PNG via `convert`, for the
Read-tool-driven blind-install loop), `key.sh` (wraps `sendkey`), `type.sh` (ASCII string → sequence of `sendkey` calls,
handles shift-glyphs).
- **FreeDOS source: official `download.freedos.org`, release 1.4, the `FD14-LegacyCD.zip` variant** (aimed at
older/low-spec PCs — the right fit for a 386-class QEMU guest) → extracted `FD14LGCY.iso` (sha256
`e25002b7fcc6ead11567f759c7557d6d6796f3469882f1a68564e0225e734165`).
- VM: `qemu-system-i386 -m 32 -drive file=disk/freedos.qcow2,format=qcow2,if=ide -boot order=c -netdev
user,id=n0,hostfwd=tcp::16000-:6000 -device ne2k_isa,netdev=n0 -vga std -display none -monitor
tcp:127.0.0.1:4445,server,nowait` (400MB qcow2 disk, FAT16). NIC is `ne2k_isa` (ISA NE2000-compatible, default
`iobase=0x300 irq=9`) — chosen per the task brief for its long-established DOS packet-driver support.
- Installed FreeDOS via the screenshot-driven blind loop (dozens of `screendump`+Read+`sendkey` round-trips) —
partitioned, quick-formatted FAT16, "Plain DOS system" package set (skipped games/apps to save the small disk for
DESQview later). **Gotcha:** launched the installer's first boot with `-no-reboot`; FreeDOS's mid-install reboot (after
partitioning) then made **qemu itself exit** instead of resetting — had to relaunch qemu by hand each time a reboot was
needed during install. Removed `-no-reboot` for all later launches.
- Boots to a `C:\>` FreeDOS 1.4 prompt selecting boot-menu option **3 "Load FreeDOS with JEMM386 (Expanded Memory)"** —
confirmed via `mem`: 8192K free EMS, 628K conventional free. DESQview classically needs EMS, so JEMM386 (not the other
UMB-only options) is the right boot choice going into Phase 2.
- **File injection into the guest disk** (needed since there's no browser/downloader in DOS): built a small ISO with
`mkisofs -J -r` (plain ISO9660+Joliet+RockRidge — the `-iso-level 4` variant produced an "ISO-9660:1999 (version 2)"
filesystem that DOS's `UDVD2`/`SHSUCDX` CD stack couldn't read, "drive not ready" in a retry loop) containing
`NE2000.COM` + mTCP. **Hot-inserting** media into an already-running guest's empty `ide1-cd0` via the HMP
`change`/`eject` monitor commands did **not** work either (still "drive not ready" after eject+reinsert+retries) — the
reliable path was to **quit qemu and relaunch with `-cdrom <iso>` present from boot**, so the ATAPI/UDVD2 stack sees
real media from the start. Copied files from `D:` to `C:\NET\` inside the guest, then removed the injection ISO from the
launch line again once copied.
- **mTCP source: official `www.brutman.com`** (note: `www.brutmanlabs.org` — the docs/index host — front-ends the same
content but is itself served by mTCP's own `HTTPServ`, a nice touch; had to use `curl --http1.0` since its minimal HTTP
server replies with a bare `HTTP/0.9`-style response to modern HTTP/1.1 requests otherwise). Downloaded
**`mTCP_2025-01-10.zip`** (the current binary release). Packet driver: **`NE2000.COM` v11.4.3** (Crynwr Software, 1993)
from `crynwr.com/drivers/many-other-drivers.zip`.
- **`C:\NET\MTCP.CFG`**: `PACKETINT 0x60`, `IPADDR 10.0.2.15`, `NETMASK 255.255.255.0`, `GATEWAY 10.0.2.2`, `NAMESERVER
10.0.2.3` (QEMU usermode-networking defaults — guest, gateway/host, and DNS respectively).
- **Phase 1 checkpoint PASSED**: `ne2000.com 0x60 9 0x300` loads clean (prints the guest's MAC `52:54:00:12:34:56`,
matching QEMU's default), then `set MTCPCFG=c:\net\mtcp.cfg` + `mtcp\ping.exe 10.0.2.2` → **4/4 ICMP replies from the
QEMU gateway, ~0.6ms average, 0 lost**. Both lines appended to `C:\FDAUTO.BAT` so networking auto-loads on every boot
from here on.
- **Next up (Phase 2):** source DESQview + DESQview/X media (vetusware.com / WinWorldPC / archive.org per the brief),
install both under this same FreeDOS+EMS setup, configure DESQview/X's own network listener on top of the packet driver,
then the `xdpyinfo` checkpoint from the host via a `hostfwd` rule (already reserved: `hostfwd=tcp::16000-:6000` maps
host `16000` → guest `6000`, the standard X11 TCP port for display `:0`).

- 2026-09-15T02:31:49Z Galaxy: progress 25% (DESQview/X media sourced from WinWorldPC (Quarterdeck DESQView X 2.1, 9 floppy images extracted). FreeDOS VM with mTCP networking already operational (Phase 1 complete). Ready for Phase 2: install DESQview + DESQview/X from floppy images, configure DESQview/X TCP listener, verify with host xdpyinfo on port 16000.)
