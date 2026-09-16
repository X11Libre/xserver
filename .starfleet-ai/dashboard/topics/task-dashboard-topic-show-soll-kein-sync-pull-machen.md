Title: "Dashboard: topic show soll kein sync/pull machen"
Category: active
Kind: task
Status: "assigned"
Created-By: "Enterprise"
Created: "2026-09-16T10:09:51Z"
Assigned-To: "Scotty"
Doc-Ref: "—"
Slug: task-dashboard-topic-show-soll-kein-sync-pull-machen

DoTopicShow in internal/dashboard/topic_commands.go ruft d.sync(runQuiet) vor dem Lesen des Topic-Files auf, was git pull --rebase --autostash im Dashboard-Repo ausfuehrt. In der Workspace/MPBT-Umgebung ist das unnoetig und fuehrt zu Fehlern (z.B. fatal: Cannot rebase onto multiple branches wenn lokale Commits divergieren). sync-Aufruf aus DoTopicShow entfernen, damit das Lesen einfach nur die lokale Datei ausliest.
