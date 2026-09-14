Title: "xlibre: LSPs mit einbinden (workspace-schema + ship-config snippets)"
Category: active
Kind: "task"
Status: "assigned"
Assigned-To: "Scotty"
Created-By: "Praetor"
Created: "2026-08-07T11:20:00Z"

## Problem

xlibre braucht hier eine spezielle LSP-Configuration, die das Workspace-Schema
sauber beruecksichtigt:

- **Multiple Clones / Build-Dirs**: das mpbt-workspace haelt pro Release-Line und
  Package einen eigenen Clone + Build-Dir (`_WORK_/<solution>/sources/...`,
  `_WORK_/<solution>/build/...`, per-package install prefixes). Ein LSP muss wissen,
  wo Compile-Commands / Build-Dirs liegen und wie die Includes aufgeloest werden
  (pkg-config/aclocal-Pfade aus den Solution-env in `cf/<solution>/solutions/devuan.yaml`).
- **Per-Ship opencode.json wird bei Launch generiert**: starfleetctl erzeugt beim
  Ship-Start eine projektspezifische `opencode.json` (`generateOpencodeConfig`,
  `var/ships/<id>.opencode.json`). Es muessen daher projektspezifische Snippets
  mit der Schiffskonfiguration durch starfleetctl gemerged werden, damit LSP-/
  Editor-Setup der Ships das Workspace-Schema korrekt mitbekommt.

## Was zu klaeren / bauen

1. LSP-Konfiguration fuer die xserver/driver-Clones (clangd o.ae.) die auf
   `_WORK_/<solution>`-Build-Dirs und die Solution-env zeigt.
2. Wo Snippets leben (per-Solution, per-Project) und wie starfleetctl sie in die
   generierte per-Ship `opencode.json` merged.
3. Zusammenarbeit mit Enterprise (das baut bereits an `generateOpencodeConfig` /
   Provider-Injection auf der gleichen Naht).

## Hints

- Konfigurations-Hook: `generateOpencodeConfig` / `ProviderConfigs` in
  starfleetctl (`internal/session/launch.go`, `occonfig.go`).
- Workspace-Schema-Doku: `cf/<solution>/solutions/devuan.yaml` `env:`, per-package
  install prefixes `_WORK_/<solution>/install`.
