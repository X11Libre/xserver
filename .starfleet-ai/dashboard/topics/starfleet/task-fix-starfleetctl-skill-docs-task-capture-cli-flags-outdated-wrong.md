Title: "Fix starfleetctl skill docs: task capture CLI flags outdated/wrong"
Category: starfleet
Kind: "task"
Status: "done"
Assigned-To: "scotty"
Created-By: "Defiant"
Created: "2026-09-14T13:37:15Z"
Doc-Ref: "—"

Defiant hat beim Aufruf von `task capture` mit --priority rumprobiert (flag existiert nicht). Die starfleetctl-Skills (SOP-Fragments unter .claude/skills/starfleet/*, agents.d/starfleet-instructions/*, sop.d/*) dokumentieren vermutlich unvollstaendige oder falsche CLI-Flags fuer `task capture` (z.B. fehlende --slug, --no-push, falsche optionales --priority, unklare --desc vs --title-pflicht).

Scope (starfleetctl-Source, Branch master):
- Pruefe Skill-Fragments/README/Kommentare die task capture flags dokumentieren
- Korrigiere auf Stand von `task capture --help` (Titel ist --title Pflicht, --desc, --slug, --assign [<ship>], --category, --no-push)
- Ggf. CLI-Hilfe itself (cmd/starfleetctl/task.go oder wo `task capture` definiert ist) pruefen ob --help korrekt ist
- Make + tests gruen, Deploy via starfleet-bootstrap, SOP reindex, Zwischenstand melden

- 2026-09-16T10:32:46Z Scotty: completed
