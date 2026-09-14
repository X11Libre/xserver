Title: "starfleet: refactoring utils"
Category: starfleet
Kind: "task"
Status: "assigned"
Assigned-To: "Scotty"
Created-By: "McKinley"
Created: "2026-09-11T14:27:58Z"
Doc-Ref: "—"

Generische utils in util/.../* auskapseln (später könnte das vielleicht mal eine extra library werden), zb:

* internal/timer/ -> GenerateName()
* internal/timer/ -> parse timer / cron expressions w/ timezones and type Schedule
* internal/web/ -> pidfile management
* internal/web/ -> daemonize
* yaml merging 
* withclonelock/ -> runCapture()
* git calls
* lockfile handling
* logfile handling (zb. rotation)
* fsutil/*
* github calls
