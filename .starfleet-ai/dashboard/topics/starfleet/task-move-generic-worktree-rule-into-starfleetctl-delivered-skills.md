Title: "Move generic worktree rule into starfleetctl-delivered skills"
Category: starfleet
Kind: "task"
Status: "assigned"
Assigned-To: "Scotty"
Created-By: "Enterprise"
Created: "2026-09-11T13:48:28Z"
Doc-Ref: "—"

Workspace commit 7de3c34817 has sharpened the worktree guidance in the workspace-local skill copies (starfleet-sessions SKILL.md, branch-hygiene reference.md, go-x11proto reference.md): ALWAYS use 'starfleetctl worktree add/list/remove/prune', never bare 'git worktree', even on mpbt-cloned sources under _WORK_/. The starfleet-generic part of this (the rule itself + the worktree command table with --from/--branch/--keep-branch semantics) needs to be promoted into the starfleetctl-repo skill sources under fragments/starfleet-skills/starfleet-sessions/SKILL.md (and any other starfleet-* skill that mentions worktrees), then committed in the starfleetctl repo and rolled out via ./starfleet-bootstrap. WORKSPACE-LOCAL parts that must NOT be copied: the go-x11proto re-dock note (project-specific), the branch-hygiene zipper-rebase workflow (project-specific). Reviewed-by Enterprise.

- 2026-09-11T14:25:58Z Enterprise: progress 100% (Enterprise verifiziert: Commit 5aa6283 auf starfleetctl master (Rebase ok), analoger Workspace-Edit 024ec33f88 vorhanden; deployed Skill wird beim nächsten Bootstrap aktualisiert; Optiones-Detail optional nachpflegbar — Frage an Scotty gestellt.)

- 2026-09-11T14:47:21Z Enterprise: progress 100% (Final verifiziert: Scotty pflegte dbcc513 (--from/--branch/--force/--keep-branch/[repo-path]) in fragments/starfleet-skills/starfleet-sessions/SKILL.md nach. Regel + vollständige Optionen doku sind in den delivered skills. Make all grün.)
