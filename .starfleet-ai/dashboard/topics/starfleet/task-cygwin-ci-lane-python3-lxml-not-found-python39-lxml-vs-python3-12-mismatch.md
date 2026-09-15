Title: "cygwin CI lane: python3 lxml not found (python39-lxml vs python3.12 mismatch)"
Category: starfleet
Kind: "task"
Status: "open"
Assigned-To: "—"
Created-By: "Enterprise"
Created: "2026-09-11T14:44:47Z"
Doc-Ref: "—"

xserver-build-cygwin lane fails fleet-wide with 'hw/xwin/glx/meson.build:6:4: ERROR: Problem encountered: python3 lxml module not found' — meson runs /usr/bin/python3.12.exe but the setup installs python39-lxml into Cygwin. Verified identical failure on unrelated PR (glamor-upload-boxes, job 103268716171) and on master — NOT caused by PR #3673's misyncfd backport. Cygwin setup must either install python3.12-lxml or point meson at python3.9. Repro: PR #3673 run 34609502935, job 103296141307.

- 2026-09-11T16:35:42Z Defiant: progress 80% (PR #3692 (wt/fix-cygwin-python312-lxml) verifiziert: python312-lxml 6.1.3-1 wird installiert, xserver-build-cygwin = success (Run 34618298573, Job 103325613372); 'Fail: 0'. Fix greift. Commit hatte keinen Signed-off-by -> per amend ergaenzt (Enrico Weigelt, metux IT consult <info@metux.net>), force-push -> HEAD fb4f07e836, CI neu getriggert. Verbleibend: Check Signed-Off-By-Action bricht selbst mit HttpError 'Resource not accessible by integration' (Token/Permission der Action, nicht PR-seitig). NetBSD-Lane separater Infra-Bug (siehe parked).)
