Automatischen Stash erzeugt: d437151ff0
Aktueller Branch mtx/agent-config ist auf dem neuesten Stand.
Automatischen Stash angewendet.
Title: "Model-API Infrastruktur blockiert: NIM saturiert, Zen-Credits leer"
Category: active
Kind: "task"
Status: "assigned"
Assigned-To: "Enterprise"
Created-By: "Enterprise"
Created: "2026-09-09T09:54:47Z"
Doc-Ref: "—"

NVIDIA-NIM-Upstream antwortet massenhaft mit HTTP 429 (Rate-Limit/Saturation); selbst Test-Requests haengen. zen-proxy (OpenCode Zen) liefert CreditsError: Workspace wrk_01KTKTCQ06G0YCTM7VJ6WYPT0Q hat keine Zahlungsmethode, Free-Tier-Credits aufgebraucht. Flagschiff-Status: beide Pfade blockiert. Ship-Sessions von Gowron/Voyager auf blocked gesetzt (per Comms angewiesen). Aktionsbedarf: Entscheidung Praetor - Zahlungsmethode hinterlegen oder NIM-Quota pruefen.

- 2026-09-09T09:55:10Z Enterprise: progress 10% (Diagnose abgeschlossen: (1) NIM-Upstream saturiert (massenhaft 429, Test-Requests haengen). (2) zen-proxy liefert CreditsError - Workspace wrk_01KTKTCQ06G0YCTM7VJ6WYPT0Q ohne Zahlungsmethode, Free-Tier-Credits leer. Beide Pfade blockiert. Modell-Proxy neu gestartet (pid 29730, Header-Stamp aktiv). Voyager/Gowron via Comms auf blocked gesetzt. Blockiert auf Praetor-Entscheidung: Zahlungsmethode hinterlegen oder NIM-Quota pruefen. --status blocked)
