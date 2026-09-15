Title: "Xserver master: Screen-Liste-Hook (Callback) im Connection-Setup als Vorstufe für Xinerama-Überarbeitung"
Category: active
Kind: task
Status: "assigned"
Created-By: "Enterprise"
Created: "2026-09-15T18:30:19Z"
Assigned-To: "enterprise"
Doc-Ref: "—"
Slug: task-xserver-master-screen-liste-hook-callback-im-connection-setup-als-vorstufe-f-r-xinerama-berarbeitung

Design-Festlegung von Praetor (2026-09-15):

Vorstufe für die geplante Xinerama-Überarbeitung: ein Callback/Hook im Connection-Setup, der es ermöglicht, die an den Client zurückgelieferte Screen-Liste zu manipulieren.

Gedachter Ablauf:
1. Array mit ScreenPtr's füllen (statt direkt aus globalem Screen-Array zu lesen)
2. Dieses Array an den Hook übergeben
3. Anschließend aus dem (ggf. modifizierten) Array den Reply-Block bauen

Betroffene Stelle: Xext/pseudoramiX/pseudoramiX.c:ProcPseudoramiXQueryScreens() (Z.299) — baut Reply direkt aus globalem pseudoramiXScreens[i]. Ähnliche Muster: Xext/randr/rrxinerama.c.

Offen: Hook-Signatur, Registrierungsmechanismus (Server-Funktionszeiger wie miScreenIsProtected?), Scope (nur QueryScreens oder auch andere Screen-Listen), Default ohne Registrierung (identische Abbildung).

Status: Planung — noch nicht umgesetzt. Falls zugewiesen: eigene Analyse + konkreter Design-Vorschlag vor Implementierung.
