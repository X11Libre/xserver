Title: "Vendor `libXfont2` in-tree (like `os/Xtrans*.c` already is), then strip the dead `fc/` backend"
Category: active
Status: "parked"
Assigned-To: "—"
Doc-Ref: "precedent: `cd81370e9d` (\"os: incorporate xtrans xtrans-1.6.0\")"
Tags: "xlibre"

Follows from the XFS investigation above. **Other consumers:** none outside the X-server family — Debian reverse-deps of
`libxfont2` are only
`nxagent`, `xwayland`, `xvfb`, `xserver-xspice`, `xserver-xorg-core`,
`xserver-xephyr`, `x2gokdrive`, `xnest`, `tigervnc-standalone-server`;
the historical other consumer, the `xfs` font-server daemon, no longer exists as a Debian package. Vendoring costs no
one else anything. **Size vs. the xtrans precedent** (that commit: 7 files, 6,969 lines, one shot, then a long tail of
small cleanup commits over months — same pattern to repeat here): libXfont2 is **~4x bigger** — ~26,164 lines under
`src/` (`fc/` 5,320, `bitmap/` 5,836, `fontfile/` 5,086, `FreeType/`-glue 5,388, `builtins/` 1,882, `util/` 2,227,
`stubs/` 425) + ~1,370 lines of public headers. Upstream is autotools-only (no meson.build), same situation xtrans was
in — vendoring means hand-writing the meson `static_library()` wiring, no reusable upstream build file. **New direct
deps `meson.build` would gain** (currently hidden behind `dependency('xfont2')`'s transitive `Requires`, absent from
xserver's own build file today): freetype2, zlib (+optional bz2), and `fontenc` (a separate small lib, ~2,144 lines —
vendorable too or kept external). **The payoff, already proven this session:** `fc/` (the font-server client backend,
5,320 lines ≈ 20% of the codebase) is confirmed dead code for this fork (see the XFS row above) — an immediate,
justified first cut right after vendoring, unlike xtrans's cleanup which was many small incremental cuts over time.
**Real trade-off to weigh:** vendoring means XLibre must track libXfont2 upstream fixes/CVEs manually from then on (same
trade-off already accepted for xtrans). **In-tree consumers to keep in mind when scoping this** (what actually uses
libXfont2's output, i.e. what any refactor must not break): the core-protocol font requests in `dix/dispatch.c`
(`OpenFont`/`CloseFont`/`QueryFont`/`QueryTextExtents`/`ListFonts(WithInfo)`/`PolyText8/16`/`ImageText8/16`), font-based
cursors (`XCreateGlyphCursor` → `AllocGlyphCursor` → `dix/glyphcurs.c`), and the rendering backends that consume the
resulting `FontPtr`/`CharInfoPtr` structs — `mi/mipolytext.c`, `mi/miglblt.c`, `fb/fbglyph.c`,
`glamor/glamor_text.c`+`glamor_font.c`, the `miext/shadow/sh*.c` variants, `exa/exa_accel.c`, `hw/xnest/Font.c`. Startup
wiring: `dix/main.c` → `xfont2_init_glyph_caching()` + `InitFonts()`
