Automatischen Stash erzeugt: 3ff68b3ddc
Aktueller Branch mtx/agent-config ist auf dem neuesten Stand.
Automatischen Stash angewendet.
Title: "Xinerama/PanoramiX refactor — replace proc-vector hooking with a frontend/backend split"
Category: active
Status: "**Idea stage — plan specified by praetor, no branch yet**"
Created-By: ""
Created: ""
Assigned-To: ""
Doc-Ref: "no doc yet; planned as a **fully separate branch** (not created)"
Migrated-From: 8cf8692b9f0057ffe44e793b35bcf7329da83d3f
Tags: "xlibre"
Slug: xinerama-panoramix-refactor-replace-proc-vector-hooking-with

Praetor request, 2026-07-02: successively get rid of the current detour via request-handler proc-vector hooking.
**Plan:** split each affected proc into a **frontend** (owns all protocol handling — request parsing, checks, etc. — and
stays the single `ProcVector` entry) and a **backend** doing the actual execution (e.g. calling into the screen's ops);
one backend for normal/single-screen mode, one for Xinerama mode. Xinerama-init then just flips a flag the frontend
checks to pick which backend to call, instead of swapping `ProcVector` entries wholesale. Praetor has done this split
for some procs before (precedent referenced, not yet located in current in-tree history — worth asking for pointers
before starting). **Current-state survey (2026-07-02, via Explore agent, not manually re-verified line-by-line):**
today's hooking lives in `Xext/panoramiX/panoramiX.c` — a static `SavedProcVector[256]` (line 108) stashes the original
handlers, and `PanoramiXExtensionInit()` (lines 422-570) directly overwrites `ProcVector[X_*]` with Xinerama-aware
wrappers for roughly **50 opcodes** across three groups: window/geometry (CreateWindow, ChangeWindowAttributes,
DestroyWindow, DestroySubwindows, ChangeSaveSet, ReparentWindow, MapWindow, MapSubwindows, UnmapWindow, UnmapSubwindows,
GetGeometry, TranslateCoords, ConfigureWindow, CirculateWindow), pixmap/GC/drawing (CreatePixmap, FreePixmap, CreateGC,
ChangeGC, CopyGC, SetDashes, SetClipRectangles, FreeGC, ClearArea, CopyArea, CopyPlane, PolyPoint, PolyLine,
PolySegment, PolyRectangle, PolyArc, FillPoly, PolyFillRectangle, PolyFillArc, PutImage, GetImage, PolyText8,
PolyText16, ImageText8, ImageText16), and colormap (CreateColormap, FreeColormap, CopyColormapAndFree, InstallColormap,
UninstallColormap, AllocColor, AllocNamedColor, AllocColorCells, AllocColorPlanes, FreeColors, StoreColors,
StoreNamedColor). The wrapper implementations live in `Xext/panoramiX/panoramiXprocs.c`, mostly looping over screens via
the `XINERAMA_FOR_EACH_SCREEN_BACKWARD` macro and calling `SavedProcVector[X_*]` per screen (re-dispatching into the
saved original single-screen proc, not a dedicated backend function). The controlling flag is `noPanoramiXExtension`
(panoramiX.c:68), checked only once at init time to decide whether to install the hooks — not per-request by the
wrappers, which is exactly the indirection this refactor removes. **One partial precedent for a frontend/backend-like
split:** `PanoramiXCreateWindow` (panoramiXprocs.c:158) calls `DoCreateWindowReq()` directly instead of
`SavedProcVector[X_CreateWindow]` — the closest thing today to a dedicated backend entry point, but incidental (dix
already had `DoCreateWindowReq` as an internal helper) rather than a deliberately designed pattern; the other wrappers
have no equivalent. **Next step:** praetor to point at the past procs this split was already done for (as a template),
then start in a fresh branch — no code written yet, this row exists purely so the plan survives between sessions. **See
also** the Parkplatz row "Xinerama-Umbau: interner Proxy-Screen" — a same-day, related-but-distinct praetor idea for the
same subsystem (route default-screen traffic through one hidden internal `ScreenRec` rather than a per-proc
frontend/backend split); the frontend/backend split here could plausibly be a stepping stone toward that (once every
proc explicitly picks a backend via a flag, that flag could later generalize into "does this screen dispatch land on the
real screen or the internal Xinerama-proxy"). Not reconciled — two independent praetor asks to two sessions the same
day, not yet compared against each other by the praetor.
