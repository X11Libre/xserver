Title: "go-x11proto: XEmbed tab manager (tabbed-style)"
Category: active
Kind: "task"
Status: "assigned"
Assigned-To: "Galaxy"
Created-By: "Enterprise"
Created: "2026-07-28T15:28:30Z"
Doc-Ref: "—"

Implement generic XEmbed-based tab manager (suckless tabbed analogue). Requires adding XEmbed protocol support to go-x11proto first: _XEMBED_INFO/_XEMBED client messages, reparenting. Each tab = independent terminal process reparented into container window. tk/term itself never multiplexes multiple Terms.

Status: 2026-09-07 (praetor): in separater branch implementiert, aber noch nicht getestet und merged.

- 2026-09-14T16:51:07Z Galaxy: progress 10% (Examining the wip/xembed-tab-manager branch)

- 2026-09-15T02:26:20Z Galaxy: progress 75% (XEmbed tab manager implemented and smoke test passes: 2 _XEMBED_INFO properties verified, XEmbed handshake working. xdotool tab switching test failed but core XEmbed protocol functional.)
