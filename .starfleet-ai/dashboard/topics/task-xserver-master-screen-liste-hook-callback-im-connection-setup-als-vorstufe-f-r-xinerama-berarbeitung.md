Title: "Xserver master: Screen-Liste-Hook (Callback) im Connection-Setup als Vorstufe für Xinerama-Überarbeitung"
Category: active
Kind: "task"
Status: "done"
Assigned-To: "Saratoga"
Created-By: "Enterprise"
Created: "2026-09-15T18:30:19Z"
Doc-Ref: "—"

Design-Festlegung von Praetor (2026-09-15):

Vorstufe für die geplante Xinerama-Überarbeitung: ein Callback/Hook im Connection-Setup, der es ermöglicht, die an den Client zurückgelieferte Screen-Liste zu manipulieren.

Gedachter Ablauf:
1. Array mit ScreenPtr's füllen (statt direkt aus globalem Screen-Array zu lesen)
2. Dieses Array an den Hook übergeben
3. Anschließend aus dem (ggf. modifizierten) Array den Reply-Block bauen

Betroffene Stelle: Xext/pseudoramiX/pseudoramiX.c:ProcPseudoramiXQueryScreens() (Z.299) — baut Reply direkt aus globalem pseudoramiXScreens[i]. Ähnliche Muster: Xext/randr/rrxinerama.c.

Offen: Hook-Signatur, Registrierungsmechanismus (Server-Funktionszeiger wie miScreenIsProtected?), Scope (nur QueryScreens oder auch andere Screen-Listen), Default ohne Registrierung (identische Abbildung).

Status: Planung — noch nicht umgesetzt. Falls zugewiesen: eigene Analyse + konkreter Design-Vorschlag vor Implementierung.

- 2026-09-15T18:35:22Z Enterprise: progress 15% (DESIGN-VERFEINERUNG (Praetor, 2026-09-15): 
- Reiner Puffer-Ansatz: globales Screen-Array nach dem Kopieren als temporäres Array dem Callback geben; Slots für virtuell-nicht-vorhandene Screens mit NULL belegt.
- Callback-Signatur: bekommt Pointer auf das temporäre Array + Commit-COUNT + ClientPtr (Client-Kontext unbedingt). Callback kann das Array frei manipulieren.
- Nächster Zug (folgender Task, nicht hier): XINERAMA registriert sich an diesem Hook und macht nötige Screens-Rewrites darüber.
- Langfristig (explizit NICHT in diesem Task): Xinerama registriert eine komplett EIGENE Screen-Struktur, statt an den existierenden Screens herumzufummeln.
TODO Design-Ausarbeitung: genaue Hook-Stelle (Funktion hinter ProcPseudoramiXQueryScreens), Registrierungsmechanismus (globaler Funktionszeiger), Semantik NULL-Slots im Reply-Builder.)
