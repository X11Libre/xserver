Title: "NetBSD-Lane PR #3691: red — retries/exhausted"
Category: xlibre
Kind: "task"
Status: "assigned"
Assigned-To: "galaxy"
Created-By: "Enterprise"
Created: "2026-09-15T11:46:59Z"
Doc-Ref: "—"

PR https://github.com/X11Libre/xserver/pull/3691 — Workflow Run https://github.com/X11Libre/xserver/actions/runs/34956549682, Job xserver-build-netbsd (job/104363229913).

Beobachtung: NetBSD-Job war bei Check bereits im Retry-Loop — 'run in netbsd VM' attempt 1 + 2 fehlgeschlagen, attempt 3 (letzter) in_progress/in_progress-Retry. Andere BSD-Lanes (openbsd/dragonflybsd/freebsd) waren success.

Fokus (ci-platform-Skill):
- NetBSD-Lane = netbsd-pkgsrc-mirror release in X11Libre/xserver (dodged ftp.netbsd.org flakes). Verifizieren, dass der Mirror populated ist (ein workflow_dispatch auf netbsd-pkg-mirror.yml kann nötig sein, falls leer/stale). install-pkg.sh versucht erst Mirror, fällt auf offiziell zurück.
- Wenn der Build selbst im VM nicht durchläuft: echten Compile-Fehler aus 'run in netbsd VM' (attempt 3) Job-Log ziehen und fixen (branch: submit/*, eigener isolierter Clone).
- Ergebnis: Fix committen/pushen, CI grün bekommen, danach Starfleet-Report + Status an Enterprise.

- 2026-09-15T12:10:15Z Galaxy: progress 25% (Triggered full workflow re-run (run 34956549682) for PR #3691. NetBSD VM was timing out on boot (attempt 3: boot timeout after 100 retries, SSH not accessible). This appears to be a transient infrastructure/VM boot issue rather than a build error. Re-run should get fresh runner.)
