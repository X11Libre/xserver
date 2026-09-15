Title: "os.h: logging.h aus os.h extrahieren"
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
- Neuer Header `include/logging.h` erstellt mit:
  - MessageType enum
  - LogVMessageVerb, LogMessageVerb, LogMessage
  - LogHdrMessageVerb
  - FatalError, ErrorF, AuditF
  - LogPrintMarkers
  - Backwards-compat macros

**NOCH ZU TUN:**
- `os.h` anpassen: `#include "logging.h"`
- Interne Nutzer (log.c etc.) migrieren
- Build testen
