Title: "xlibre: optionaler VNC-Server als generische Extension"
Category: active
Kind: "task"
Status: "assigned"
Assigned-To: "Scotty"
Created-By: "Yamato"
Created: "2026-07-16T07:17:49Z"
Doc-Ref: "—"
Tags: "xlibre"

Optionaler VNC-Server als generische Extension in allen DDXen implementieren: - X11 Protocol Extension fuer
Configuration (Credentials Management, etc.) - Flexibel genug fuer spaetere weitere Protokolle (z.B. RDP) ohne neuen
Extension-Slot - Generisches Design: Extension verwaltet mehrere Remote-Desktop-Protokolle - VNC als erstes Protokoll,
aber Architektur erlaubt zusaetzliche Protokolle - Configuration via X11 Requests (nicht Kommandozeilen-Optionen) -
Credentials koennen dynamisch geaendert werden
