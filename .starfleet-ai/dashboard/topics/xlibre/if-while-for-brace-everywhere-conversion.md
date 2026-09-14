Automatischen Stash erzeugt: a1cfa83d45
Aktueller Branch mtx/agent-config ist auf dem neuesten Stand.
Automatischen Stash angewendet.
Title: "if/while/for brace-everywhere conversion (xserver coding style)"
Category: active
Status: "open"
Assigned-To: "—"
Doc-Ref: "PR #3258 (`os/Xtranssock.c` `set_sun_path()`, master, single commit, build-verified via `meson setup` + `ninja hw/vfb/Xvfb hw/xnest/Xnest`)"
Tags: "xlibre"

Praetor confirmed 2026-07-03: always brace `if`/`while`/`for`/`else` bodies, even single-statement, in **all new/touched
code from now on** (already the rule since PR #3199, now made permanent policy) — **and** convert existing unbraced
bodies successively, in small self-contained batches (one file or a handful of functions per PR, brace-only diffs, no
mixed changes). **First batch (Potemkin):** braced `set_sun_path()`'s 4 previously-unbraced `if`/`else if` bodies,
matching the file's already-dominant same-line brace style; scope deliberately kept to exactly this one function (the
candidate this row itself named earlier), not the whole file, to keep the diff trivially reviewable. Update this row
each time a further batch lands so the initiative doesn't stall silently.
