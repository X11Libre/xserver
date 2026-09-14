Automatischen Stash erzeugt: cc3829ac4b
Aktueller Branch mtx/agent-config ist auf dem neuesten Stand.
Automatischen Stash angewendet.
Title: "Driver repos — cross-repo PR/issue dashboard (xserver issue #3280)"
Category: active
Status: "Built + live, 2026-07-07 (Agamemnon) — needs periodic refresh"
Created-By: ""
Created: ""
Assigned-To: ""
Doc-Ref: "https://github.com/X11Libre/xserver/issues/3280"
Tags: "xlibre"
Slug: driver-repos-cross-repo-pr-issue-dashboard

**Why:** the ~63 `xf86-input-*`/`xf86-video-*` driver forks under the `X11Libre` org each have their
own PRs/issues, but GitHub gives no view that spans all of them at once — a driver-repo PR (like
the ones opened for the m0126/m0130 alloc-fail/UAF sweep) is easy to lose track of from the
`xserver` side. Requested by the praetor 2026-07-07.

**What's built:** `scripts/driver-tracker` — discovers every `xf86-input-*`/`xf86-video-*` repo in
the org dynamically (`gh repo list`, name-prefix filter, no hardcoded list to maintain), lists each
repo's open PRs (with draft/`bot-review-*` label markers) and open issues, skips repos with nothing
open, and either prints the report (`driver-tracker`) or pushes it straight into a tracking issue
(`driver-tracker --update <issue#>`, via `gh issue edit --body-file` — plain `gh issue edit` works
fine here, unlike `gh pr edit`'s "Projects classic deprecation" breakage). Takes ~1 minute for the
full sweep (one `gh pr list` + one `gh issue list` per repo).

**The tracking issue:** [`X11Libre/xserver#3280`](https://github.com/X11Libre/xserver/issues/3280)
— created + first populated 2026-07-07. Its body is fully regenerated each run (not appended to),
so re-running produces a no-op diff when nothing changed on any driver repo.

**Refresh cadence — needs a human/ship to actually run it, no cron wired up yet.** This is a
plain `scripts/*` tool, not an automated job: to refresh, run
`scripts/driver-tracker --update 3280` from the workspace root. Treat this like the existing
"PR review backlog" / "xorg-upstream tracking" rows — **ongoing, recurring**, any free ship can
pick it up per the m0116/m0121 autonomy policy (no need to wait for a directive). A follow-up idea
(not built): wire this into a real cron/schedule so it self-refreshes instead of depending on a
ship noticing — flagged here rather than built speculatively, since the manual command already
satisfies the immediate ask.
