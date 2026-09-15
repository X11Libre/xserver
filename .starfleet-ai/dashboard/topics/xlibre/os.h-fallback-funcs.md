Title: "os.h: AC_REPLACE_FUNCS-Fallbacks verschieben"
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
- Fallbacks noch in `os.h`: reallocarray, strlcpy, strlcat, strndup, timingsafe_memcmp
- Implementierungen in `os/` (reallocarray.c, strlcpy.c, strlcat.c, strndup.c, timingsafe_memcmp.c)

**NOCH ZU TUN:**
- Privaten Header `include/os_fallbacks.h` (oder ähnlich) erstellen
- Deklarationen dorthin verschieben
- `os.h` anpassen: nur noch `#include "os_fallbacks.h"` für interne Nutzer
- Interne Nutzer migrieren
- Build testen
