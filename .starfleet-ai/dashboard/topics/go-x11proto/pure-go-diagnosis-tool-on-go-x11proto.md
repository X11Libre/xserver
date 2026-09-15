Title: "Pure-Go diagnosis tool (xdpyinfo-like) on go-x11proto"
Category: parked
Status: "assigned"
Assigned-To: "Galaxy"
Since: "2026-07-01"

Idea only, deprioritized. A pure-Go server-inventory dumper 
(setup: vendor/release/proto/byteorder; screens+depths+visuals; extensions+versions; pixmap-formats) 
built on go-x11proto — no libX11/libxcb, so it works in minimal/container/CI envs (where the go-xts socket issue lives).
Would ship with `-json`/`-s` from day one. Revisit if a pure-Go `xdpyinfo` is wanted

- 2026-09-15T02:26:52Z Galaxy: progress 100% (Task reviewed: this is a parked/idea-only task. No implementation needed at this time. Pure-Go xdpyinfo-like tool would be useful for CI environments but not currently prioritized.)
