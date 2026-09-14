Title: "starfleet: bug: terminal kann kein paste"
Category: starfleet
Kind: "task"
Status: "assigned"
Assigned-To: "Scotty"
Created-By: "Pasteur"
Created: "2026-08-04T08:13:52Z"
Doc-Ref: "—"

hab eben die URL im browser markiert (und zusätzlich noch mit ctrl+c) kopiert und wollte im termctl terminal (go-x11proto) einfügen: weder mittle click noch shift-insert noch ctrl+v funktionieren.

## Status (2026-08-06, Defiant)

Fix implementiert und auf `wip/term-paste-fix` gepusht (Commit a021749):

- **Ctrl+V / Ctrl+Shift+V / Shift+Insert pasten jetzt die X CLIPBOARD-Selection** in die PTY. Vorher ging Ctrl+V als Steuerbyte 0x16 (SYN) an die Shell und Shift+Insert wurde komplett verworfen (xkInsert hatte kein specialKey-Mapping).
- **Middle-Klick fällt auf CLIPBOARD zurück**, wenn PRIMARY leer ist — ein Browser-Ctrl+C (das nur CLIPBOARD füllt) pastet damit auch per Middle-Klick, wie in modernen Terminals.
- keyboard: `xkInsert`/`KeyInsert`-Mapping ergänzt (plain Insert sendet weiterhin nichts).
- Tests: `TestLookupPasteShortcuts` + erweitertes `TestLookupSpecialKeys` in tk/keyboard; `go build ./...` + `go test ./...` grün.

Offen: visuelle Verifikation mit echter X-Verbindung (termctl + Clipboard-Owner) sowie PR gegen master, falls gewünscht.
