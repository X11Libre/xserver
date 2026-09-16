---
name: starfleet-sessions
description: "Ship session and workspace management — spawning ships, session attach/stop, git worktrees, web console, deployment. Load when launching/administering ships or managing the fleet web console."
---

# starfleet-sessions — ship sessions, worktrees, web console

Spawning and managing ships, isolated worktrees, and the fleet web console.
Comms/task core lives in the **`starfleet`** and **`starfleet-tasks`** skills.

## Ship sessions

| Subcommand | Purpose |
|---|---|
| `run [--flagship\|--name <id>] [--client claude\|opencode]` | Start an AI ship session |
| `session list` | List running detached sessions |
| `session attach <id>` | Attach terminal to a detached session |
| `session stop <id>` | Kill a detached session + release ship name |

Background ships never prompt on their console (see the starfleet-instructions working
practices) — hand them a task via the dashboard/comms, not as extra CLI args.

## Git worktrees

**Rule: Worktrees werden immer über starfleetctl verwaltet, nie direkt git worktree.**

| Subcommand | Purpose |
|---|---|
| `worktree add <repo-path> [name] [--from <ref>] [--branch <existing-branch>]` | Create a per-task git worktree |
| `worktree list [repo-path]` | List worktrees |
| `worktree remove <repo-path> <name> [--force] [--keep-branch]` | Remove a worktree |
| `worktree prune [repo-path]` | Garbage-collect stale worktrees |

**Rationale:** starfleetctl maintains its own worktree registry (`.starfleet-ai/var/worktrees/`), handles concurrent access via flock, and ensures consistent naming/cleanup. Direct `git worktree` bypasses this and causes conflicts.

## Web console & setup

| Subcommand | Purpose |
|---|---|
| `web start/stop/restart/autostart` | Fleet web console (mobile-first) |
| `genesis-init [dir]` | Bootstrap a workspace from nothing |
| `self-install` | Clone/pull + build + symlink starfleetctl |
| `sop install-starfleet` | Install/update SOP fragments and skills |

## Deployment

```bash
# Phase A: genesis (from an existing binary)
starfleetctl genesis-init .

# Phase B: bootstrap (from the committed script)
./starfleet-bootstrap
```

Everything under `.starfleet-ai/` is gitignored. Re-run `./starfleet-bootstrap` anytime to update.
