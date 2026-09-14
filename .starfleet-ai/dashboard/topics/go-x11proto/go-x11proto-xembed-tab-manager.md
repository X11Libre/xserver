Title: "go-x11proto: XEmbed tab manager (tabbed-style)"
Category: active
Kind: "task"
Status: "open"
Assigned-To: "—"
Created-By: "Enterprise"
Created: "2026-07-28T15:28:30Z"
Doc-Ref: "—"

Implement generic XEmbed-based tab manager (suckless tabbed analogue). Requires adding XEmbed protocol support to go-x11proto first: _XEMBED_INFO/_XEMBED client messages, reparenting. Each tab = independent terminal process reparented into container window. tk/term itself never multiplexes multiple Terms.

Status: 2026-09-07 (praetor): in separater branch implementiert, aber noch nicht getestet und merged.
