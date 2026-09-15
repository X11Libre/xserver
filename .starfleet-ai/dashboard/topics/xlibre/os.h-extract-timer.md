Title: "os.h: timer.h aus os.h extrahieren"
Category: active
Kind: "task"
Status: "open"
Assigned-To: "—"
Created-By: "Saratoga"
Created: "2026-07-29T15:54:48Z"
Doc-Ref: "—"

---

Subtask aus compiler.h/os.h Analyse

## Arbeitsstand (2026-09-07, Casopeia)

**ERLEDIGT:**
- Neuer Header `include/timer.h` erstellt mit:
  - OsTimerPtr, OsTimerCallback
  - TimerSet, TimerCancel, TimerFree, TimerForce
  - GetTimeInMillis, GetTimeInMicros
  - AdjustWaitForDelay
  - TimerAbsolute, TimerForceOld flags

**NOCH ZU TUN:**
- `os.h` anpassen: `#include "timer.h"`
- Interne Nutzer migrieren
- Build testen
