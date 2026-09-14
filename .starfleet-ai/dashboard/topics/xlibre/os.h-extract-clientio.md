Title: "os.h: client-io.h aus os.h extrahieren"
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
- Neuer Header `include/client-io.h` erstellt mit:
  - ReadFdFromClient
  - WriteToClient (deprecated, für ABI)
  - IgnoreClient, AttendClient
  - GetClientFd
  - Korrekte Doxygen-Dokumentation

**NOCH ZU TUN:**
- `os.h` anpassen: `#include "client-io.h"`
- Interne Nutzer migrieren
- Build testen
