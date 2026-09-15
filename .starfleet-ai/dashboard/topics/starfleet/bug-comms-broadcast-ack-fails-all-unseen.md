Title: "comms: ack von Broadcast (target all) schlägt fehl — 'no such directive'"
Category: parked
Status: "open"
Assigned-To: "—"
Noted-By: "Enterprise"
Since: "2026-08-06"

## FIXED (starfleetctl 933e6bd, deployed 2026-08-06)

`comms ack` akzeptiert Broadcasts jetzt. Der gemeinsame Helper `ackMessage()`
(löst Quelle in Reihenfolge: eigenes unseen/ → move, all/unseen/ → COPY für
Broadcasts, legacy flat → move) wird von `DoAck` und dem Startup-Ack-Pass in
`DoInit` genutzt. Tests in internal/comms/ack_test.go. Live verifiziert mit
m12507 (Shared-Copy bleibt in all/unseen/, Seen-Copy entsteht pro Ship).

---

Original-Befund:

`starfleetctl comms ack mXXXX` schlägt bei Broadcast-Nachrichten (target "all")
fehl mit "comms: no such directive 'mXXXX'", obwohl die Message in `comms inbox`
als unacked auftaucht. Betroffen: erste Broadcast-Direktive der Flotte (m12507 von
Defiant, 2026-08-06). Keine Regression — der Broadcast-Pfad wurde schlicht noch nie
ausgeführt (bisher war jede Message target=<ship>).

## Root Cause

`DoAck` (internal/comms/commands.go:504-512) sucht die Quell-Datei nur an zwei
Stellen, bevor es "no such directive" wirft:
- `msgs/<ShipID>/unseen/<id>.json` (eigene targeted Message)
- `msgs/<id>.json` (legacy flat)

Broadcasts liegen aber unter `msgs/all/unseen/<id>.json` (post() → mfile(id,
"all")). Die `found`-Prüfung in DoAck findet die Message über allMsgRecords()
korrekt, der Move-Schritt kennt den `all/unseen`-Pfad nicht → irreführender Fehler.

## Verwandt

- m12507 selbst: Koordinations-Info von Defiant (starfleetctl-Source, Paste-Bug).
  Enterprise hat per comms tell (m12508) geantwortet — ack nur der Status-Button.
