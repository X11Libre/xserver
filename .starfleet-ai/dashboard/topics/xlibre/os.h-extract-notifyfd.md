Title: "os.h: notify-fd.h aus os.h extrahieren"
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

**STATUS:**
- `fd_notify.h` existiert bereits in `include/` (nur X_NOTIFY_* constants)
- `SetNotifyFd` / `RemoveNotifyFd` sind noch in `os.h`

**NOCH ZU TUN:**
- `notify-fd.h` erweitern: Function Declarations für SetNotifyFd, RemoveNotifyFd
- `os.h` anpassen: `#include "notify-fd.h"`
- Interne Nutzer migrieren
- Build testen
