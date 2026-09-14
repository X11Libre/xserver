Title: "os.h: alloc.h aus os.h extrahieren"
Category: active
Kind: "task"
Status: "open"
Assigned-To: "—"
Created-By: "Saratoga"
Created: "2026-07-29T15:54:48Z"
Doc-Ref: "—"

---

Subtask aus compiler.h/os.h Analyse (siehe dashboard/topics/xlibre/*-analysis.md)

## Arbeitsstand (2026-09-07, Casopeia)

**ERLEDIGT:**
- Neuer Header `include/alloc.h` erstellt mit allen Allokations-Funktionen:
  - XNFalloc, XNFcallocarray, XNFrealloc, XNFreallocarray
  - Xstrdup, XNFstrdup
  - Legacy-Macros (xnfalloc, xnfcalloc, xnfrealloc, xstrdup, xnfstrdup)
  - Deprecated XNFcalloc
  - Korrekte Doxygen-Dokumentation, _X_EXPORT, Attribute

**NOCH ZU TUN:**
- `os.h` anpassen: `#include "alloc.h"` statt Inline-Definitionen
- Interne Nutzer (alloc.c etc.) auf neue Header umstellen
- Meson build anpassen falls nötig
- Build testen

Verwandte Tasks (alle Casopeia):
- xlibre/os.h-extract-clientio → in-progress (client-io.h erstellt)
- xlibre/os.h-extract-logging → in-progress (logging.h erstellt)
- xlibre/os.h-extract-timer → in-progress (timer.h erstellt)
- xlibre/os.h-extract-notifyfd → offen (fd_notify.h existiert schon)
- xlibre/os.h-fallback-funcs → offen
- xlibre/os.h-prune-private → offen

Nächster Schritt nach Wiederaufnahme: os.h aufräumen + Build verifizieren
