Title: "Remove dashboard show + DASHBOARD.md generation and file; fix comments/docs/skills"
Category: starfleet
Kind: "task"
Status: "done"
Assigned-To: "scotty"
Created-By: "Defiant"
Created: "2026-09-14T13:35:38Z"
Doc-Ref: "—"

dashboard-show + DASHBOARD.md sind obsolet: topic list / Task-Dedup lesen bereits live die Topic-Dateien (loadAllTopics / LoadAllTopicsJSON). DASHBOARD.md ist nur noch ein regeneriertes Anzeige-Derivat. Schluessel mit "dashboard topic list" ist nuetzlich (liefert Filter/JSON) - dashboard show ist dagegen reine Optik.

Scope (starfleetctl-Source, Branch master, make+tests gruen, Deployment via ./starfleet-bootstrap):

1. Remove dashboard show subcommand (DoShow in internal/dashboard/commands.go, usage/text in internal/dashboard/run.go).
2. Remove DASHBOARD.md generation + guards: DoReindex (internal/dashboard/topic_commands.go) wird von timer/system.go runReindex + internal/web apiDashboardReindex + web.go:395 aufgerufen. Pruefen ob timer/web-Aufrufer mitentfernt/ggf. weiter auf No-Op umgestellt werden muessen. Entferne DoReindex aus timer-System + Web, oder fuehre ihn auf Topic-File-Index-Funktion zurueck.
3. Remove bootstrap/support: internal/bootstrap/checks.go verifyDashboardMD/fixDashboardMD, internal/dashboard/bootstrap.go EnsureBootstrapped + minimalSkeleton (wenn不再 gebraucht).
4. Remove/lösche .starfleet-ai/var/DASHBOARD.md (gitignored) aus dem Workspace (praetor-staging-mtx-Branch).
5. Fix outdated comments/docs im Starfleet-Source:
   - internal/web/web.go:537+ "task dedup reads the index" Kommentar entfernen/korrigieren
   - internal/dashboard/*.go Header-Kommentare (references to "DASHBOARD.md cross-session index")
   - cmd/starfleetctl/github.go:180/186/196/262/267 (verweist auf "DASHBOARD.md task/roadmap")
   - cmd/starfleetctl/main.go:7 (DASHBOARD.md Row)
   - internal/bootstrap/bootstrap.go + internal/bridged/* + internal/genesis/* + internal/comms/monitor.go + internal/ghpr/ghpr.go
6. Fix starfleet-delivered SOP-Fragments / Skills im Starfleet-Source: .claude/skills/starfleet/*, agents.d/starfleet-instructions/*, sop.d/* Referenzen auf DASHBOARD.md / dashboard show.
7. Workspace: nach Deploy neue starfleet-bootstrap + sop reindex ausfuehren.

Erwartung: make gruen, tests laufen, bootstrap deployt sauber, Zwischenstand an Defiant + Enterprise melden.

- 2026-09-16T10:47:40Z Scotty: completed
