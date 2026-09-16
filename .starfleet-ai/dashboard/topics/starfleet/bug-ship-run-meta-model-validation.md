---
title: ship-run validation kennt meta-model Strategien nicht (heavy-model != available)
status: open
area: proxy
priority: medium
assignee: ""
tags: [proxy, ship-run, bug]
---

# ship-run model validation schlaegt fuer meta-model-Strategien fehl

Beim Respawn von Barcley (2026-09-16) nach API-Loop:

`session ship-run --model heavy-model` und `--model meta-model/heavy-model`
→ "model \"heavy-model\" not found in provider \"meta-model\" (available: 0 models)"

## Ursache
`validateModelAvailable` (internal/session/launch.go:1143) ruft `cfg.ModelListFor(*prov)` auf.
`ModelListFor` liest nur die statische Provider-Config (models: []-Liste). Die virtuellen
meta-model-Strategien (heavy/cruiser/scout/balanced) existieren nur zur Laufzeit im Proxy
(handleModels), sie werden von der statischen Config-Liste nicht erfasst → 0 Models.

## Konsequenz
Spawns mit `--model heavy-model` (bzw. meta-model/...) werden abgelehnt; schwere Rebase-Tasks
muessen auf einzelne Real-Modelle (z.B. nemotron-3-nano-omni-30b) ausweichen statt die
Fallback-Strategie zu nutzen.

## Fix-Idee
`ModelListFor` / Validation fuer den meta-model-Provider: die Strategie-IDs aus der
Provider-Config (type: meta-model → die unter `strategies:` definierten IDs) als Modelle
auflisten, oder Validation fuer Virtuals aus der config direkt ziehen (nicht HTTP-querien).

## Workaround (aktuell)
`session ship-run --model nvidia/nemotron-3-nano-omni-30b-a3b-reasoning`
