Title: "Nachtrag: Implement starfleetctl session transcript command"
Category: starfleet
Kind: "task"
Status: "done"
Assigned-To: "Scotty"
Created: "2026-09-10T08:13:21Z"
Doc-Ref: "—"

Bereits umgesetzt:
-------------------

Add a subcommand to extract Opencode session transcript from SQLite DB and output to file or stdout, similar to web frontend's ocsessions.SessionTranscript

- 2026-09-10T08:19:35Z Enterprise: Implemented 'session transcript <id> [--limit/--offset/--output/--json|--text]' in internal/session/transcript.go, wired into run.go, exported ocsessions.MaxListLimit(). Tests green in clean worktree at HEAD. Committed cf7afb3 + pushed to master.

- 2026-09-10T08:19:41Z Enterprise: completed

Noch nachschärfen:
------------------

* User-Dokumentation aktualisieren
* separater starfleet-skill (von starfleet selbst geliefert) für troubleshooting-aufgaben, der uA. diese neue funktionalität genau beschreibt
* wenn du noch andere sinnvolle Troubleshooting-Tips für Agents hast, können die auch mit in diesen skill rein
* der skill soll genau dann bei Bedarf geladen werden, wenn ein schiff sich um probleme mit anderen schiffen kümmern soll 

- 2026-09-16T10:24:43Z Scotty: completed
