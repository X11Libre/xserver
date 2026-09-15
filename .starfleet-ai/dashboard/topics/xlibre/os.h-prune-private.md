Title: "os.h: Private Symbole aus os.h entfernen"
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
Alle 6 neuen public Headers erstellt:
- alloc.h ✓
- client-io.h ✓
- logging.h ✓
- timer.h ✓
- notify-fd.h (teilweise, erweitert werden) → in Arbeit
- os_fallbacks.h (für AC_REPLACE_FUNCS) → offen

**NOCH ZU TUN:**
1. notify-fd.h erweitern (SetNotifyFd, RemoveNotifyFd)
2. os_fallbacks.h erstellen
3. os.h aufräumen: nur noch Includes der neuen Headers + public-only Symbole
4. Private Symbole in private Header(s) verschieben (os_priv.h oder ähnlich)
5. Interne Nutzer migrieren
6. Meson build anpassen
7. Build testen

Nach Wiederaufnahme: os.h finalisieren + Build verifizieren
