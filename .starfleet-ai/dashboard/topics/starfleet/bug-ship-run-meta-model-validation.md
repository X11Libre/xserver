---
title: ship-run validation kennt meta-model Strategien nicht (heavy-model != available)
status: done
area: proxy
priority: medium
assignee: ""
tags: [proxy, ship-run, bug]
---

# ship-run model validation schlaegt fuer meta-model-Strategien fehl (FIXED)

Beim Respawn von Barcley (2026-09-16) nach API-Loop:

`session ship-run --model heavy-model` und `--model meta-model/heavy-model`
→ "model \"heavy-model\" not found in provider \"meta-model\" (available: 0 models)"

## Ursache
`validateModelAvailable` (internal/session/launch.go:1143) ruft `cfg.ModelListFor(*prov)` auf.
`ModelListFor` liest nur die statische Provider-Config (models: []-Liste). Die virtuellen
meta-model-Strategien (heavy/cruiser/scout/balanced) existieren nur zur Laufzeit im Proxy
(handleModels), sie werden von der statischen Config-Liste nicht erfasst → 0 Models. Zudem
wurden die Strategien gar nicht in die zur Validierung gebaute Config übernommen.

## Fix (Commit eb5f323, deployed 2026-09-17)
- `validateModelAvailable` übernimmt `Strategies` aus der geladenen model-proxy-Config in die
  Validierungs-Config → `ModelListFor` des virtuellen meta-model-Providers liefert jetzt die
  Strategie-IDs (heavy-model, cruiser, scout, balanced, ...) statt 0.
- Zusätzlich: akzeptiert einen (ggf. provider-präfixlosen) Strategie-ID direkt, falls der
  aufgelöste Provider ein meta-model-Virtueller ist (Netz für Upstream-Listungs-Fehler).

## Im selben Wurf entfernt (Commit c0c5d85)
- Statisches `.starfleet-ai/conf/models.yaml` (alte `models`-Subcommand + `internal/models`)
  → der Model-Katalog kommt jetzt live aus den Proxy-Backends (ProxyModelInfos); `model-proxy
  check` läuft über den Live-Katalog statt über models.yaml (verhindert Drift).

## Verifikation
- `make all` grün (build + vet + fmt + full tests)
- `model-proxy check`: total=158 ok=158 über Live-Katalog
- `starfleetctl models` → "unknown subcommand" (entfernt)
- Deployed via starfleet-bootstrap; web (200), timer worker + model-proxy neu gestartet