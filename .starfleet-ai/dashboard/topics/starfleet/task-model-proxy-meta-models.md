Title: "Model-Proxy Meta-Models — klassenbasiertes Model-Routing mit Fallback/Round-Robin"
Category: active
Kind: "task"
Status: "done"
Assigned-To: "Scotty"
Created-By: "Defiant"
Created: "2026-09-09T00:00:00Z"
Doc-Ref: "—"
Tags: "starfleet"

# Model-Proxy Meta-Models — klassenbasiertes Model-Routing mit Fallback/Round-Robin

## Status
- **Created:** 2026-09-09
- **Created-By:** Defiant
- **Assigned-To:** —
- **Status:** open
- **Category:** active
- **Kind:** task
- **See-Also:** starfleet/task-agent-templates-schiffsklassen (Schiffsklassen-Templates)

## Zusammenfassung

Der Model-Proxy bekommt eine neue Schicht: **Meta-Models** (Model Strategies).
Ein Meta-Model ist eine benannte Auswahlregel für Modelle, der Agent nie
direkt sieht — er spricht nur das Meta-Model an, der Proxy handelt die
Strategie ab (Fallback, Round-Robin, Weighted, Sticky Selection).

## Design-Entscheidungen (2026-09-09)

### Session-ID
- Client-Session-Id wird IMMER an den tatsächlichen Upstream durchgereicht
- Egal welches Modell tatsächlich angesprochen wird
- Cache-Affinität bleibt erhalten (wie opencode es direkt tun würde)

### Modell-Kompatibilität
- Modelle sind üblicherweise kompatibel (manuell getestet)
- System-prompt/parameters werden durchgereicht

### Monitoring/Heuristiken
- Konfigurierbar: globale Defaults + per-Strategy Override
- Metriken: Latenz, Fehlerrate, Token-Durchsatz, Consecutive Failures

### Context-Window-Management (Kern-Design)
- **Proxy berechnet `min(context_window)` über alle Modelle in der Strategie**
- Dieses Limit wird opencode via **zwei Wege** mitgeteilt:
  1. **Erster Response-Header bei Session-Start** (`X-Context-Limit`)
  2. **Erweitertes `/v1/models` Endpoint** (neues Feld `context_window` pro Modell)
- opencode treated dies als effektives Context-Limit
- Compaction wird IMMER vor Erreichen des kleinsten Modells getriggert
- Kein Abhängig davon welches Modell gerade aktiv ist

---

## Teil 1: Strategie-Definitionen (mit echten Context-Windows)

```yaml
strategies:
  heavy-model:
    description: "Komplexe Aufgaben — erst BigPickle, dann Fallback"
    default-model: big-pickle
    models:
      - id: big-pickle
        provider: nim-proxy
        context-window: 200000    # 200k
        priority: 1
      - id: nvidia/nemotron-3-ultra-550b-a55b
        provider: nim-proxy
        context-window: 1000000   # 1000k = 1M
        priority: 2
    strategy: fallback
    # Effektives Limit: min(200000, 1000000) = 200000

  cruiser-model:
    description: "Allgemeine Arbeit — Round-Robin für Verfügbarkeit"
    models:
      - id: nvidia/nemotron-3-ultra-550b-a55b
        provider: nim-proxy
        context-window: 1000000   # 1000k = 1M
        weight: 1
      - id: nvidia/nemotron-3-nano-30b-a3b
        provider: nim-proxy
        context-window: 16384     # 16k (basierend auf config: nemotron-3-nano-30b-a3b)
    strategy: round-robin
    # Effektives Limit: min(1000000, 16384) = 16384

  scout-model:
    description: "Leichte Aufgaben — schnell, billig"
    models:
      - id: nvidia/nemotron-3-nano-30b-a3b
        provider: nim-proxy
        context-window: 16384     # 16k
    strategy: single
    # Effektives Limit: 16384

  balanced-model:
    description: "60% BigPickle, 40% Nemotron Ultra"
    models:
      - id: big-pickle
        provider: nim-proxy
        context-window: 200000    # 200k
        weight: 3
      - id: nvidia/nemotron-3-ultra-550b-a55b
        provider: nim-proxy
        context-window: 1000000   # 1000k = 1M
        weight: 2
    strategy: weighted
    # Effektives Limit: min(200000, 1000000) = 200000
```

### Bekannte Modelle aus opencode config
- **Nemotron 3 Super** (`nvidia/nemotron-3-super-120b-a12b`): 262144 tokens (~262k)
- **Nemotron Ultra** (`nvidia/nemotron-3-ultra-550b-a55b`): 1000000 tokens (1000k = 1M)
- **Big Pickle** (`big-pickle`): 200000 tokens (200k)
- **Nemotron Nano** (`nvidia/nemotron-3-nano-30b-a3b`): 16384 tokens (16k)
- **DeepSeek V4 Flash**: 1000000 tokens (1M)

### Strategien-Typen

| Strategie     | Verhalten                                           |
|---------------|-----------------------------------------------------|
| `single`      | Nur ein Modell, kein Fallback                       |
| `fallback`    | Sequenz nach `priority` (niedriger = zuerst)        |
| `round-robin` | Gleichmäßig wechseln                                |
| `weighted`    | **Sticky selection**: Bei Start ein Modell nach Gewicht wählen, danach bei Fehler/Timeout wechseln |

---

## Teil 2: Context-Window-Management (Detail)

### Problem-Szenario

```
1. Session startet mit Nemotron Ultra (1M Context)
2. Session wächst auf 150k Tokens
3. Nemotron Ultra wird unverfügbar (Quota aufgebraucht)
4. Proxy wechselt auf Big Pickle (200k Context)
5. 150k < 200k → OK, kein Problem
6. Wenn wir stattdessen auf Nemotron Nano (16k) fallen würden:
   150k > 16k → upstream lehnt ab → Compaction auf 16k-Modell → ggf. zu groß
```

### Lösung: Min-Limit von Anfang an

```
1. Strategie "heavy-model" definiert:
   - Big Pickle: 200k
   - Nemotron Ultra: 1M
2. Proxy berechnet: effective_limit = min(200k, 1000k) = 200k
3. Proxy sendet an opencode:
   a. X-Context-Limit: 200000 Header bei Session-Start
   b. Erweiterte /v1/models Response mit context_window: 200000 pro Modell
4. opencode behandelt 200k als hartes Limit
5. Compaction wird bei ~170k getriggert (200k - buffer)
6. Bei Fallback auf Big Pickle: Context passt immer
7. Bei Fallback auf Nemotron Ultra: noch mehr Headroom verfügbar
```

### Vorteile

- **Sicher:** Context ist IMMER kompatibel mit allen Modellen in der Strategie
- **Transparent:** opencode merkt den Wechsel nicht
- **Einfach:** Keine dynamische Limit-Anpassung nötig
- **Compaction funktioniert:** Immer genug Luft für Summary

### Nachteile

- **Verschwendung:** Größere Modelle werden nicht voll ausgenutzt
- **Konservativ:** Nutzer könnte mehr Context haben

### Abwägung

Für unsere Fleet-Nutzung überwiegen die Vorteile:
- Ships arbeiten meistens autonom (Hintergrund)
- Sicherheit > maximale Context-Ausnutzung
- Nutzer kann bei Bedarf manuell mit `--model <specific>` starten (ohne Strategie)

---

## Teil 3: Implementierungsplan

### Phase 1: Model-Proxy Änderungen
1. Context-Windows in Model-Definitionen pflegen
2. Effektives-Limit-Berechnung implementieren (`min(context_window)`)
3. Header-Injection: `X-Context-Limit` in Antworten hinzufügen
4. `/v1/models` Endpoint erweitern: `context_window` Feld pro Modell hinzufügen
5. Provider-Health-Checks implementieren

### Phase 2: Routing-Strategien
1. Strategie-Definitionen (fallback, round-robin, weighted-sticky)
2. Weighted-Round-Robin als Sticky Selection:
   - Bei Session-Start: gewichtete Zufallsauswahl
   - Bei Fehler/Timeout: zum nächsten gewichteten Modell wechseln
   - Cooldown-basiert: nach `cooldown_period` Sekunden automatisch zurück ins Pool
3. Circuit Breaker (Netflix-Style) implementieren
4. Session-Affinität: gleiche Session → gleiches Modell (Cache-Shard)

### Phase 3: Trigger-Regeln & Heuristiken
1. Trigger-Regeln implementieren:
   - rate-limited: skip + 60s cooldown + after 3 retries
   - quota-exhausted: skip + 3600s cooldown + auto-recover
   - timeout: skip + 30s cooldown + max-consecutive: 3
   - error: skip + 10s cooldown
2. Heuristiken (konfigurierbar):
   - Latenz p95 > 30s → Affinität 1 Request brechen
   - Fehlerrate > 20% in 5min → Circuit Breaker OPEN
   - Token-Durchsatz < 10 tokens/s → Modell-Wechsel
   - Consecutive Failures ≥ 3 → Sofortiges Skip

### Phase 4: starfleetctl Integration
1. `starfleetctl model-proxy meta-models list/show`
2. `starfleetctl model-proxy sessions list/show`
3. `starfleetctl model-proxy switch <session-id> <strategy> <model>`
4. `starfleetctl model-proxy force <strategy> <model>`
5. Web-API Endpoints:
   - `GET /v1/meta-models` — Liste aller Strategien
   - `GET /v1/meta-models/<strategy>` — Strategie-Details
   - `GET /v1/meta-models/sessions` — Alle Sessions
   - `POST /v1/meta-models/switch` — Manuell Switch
   - `POST /v1/meta-models/force` — Alle Sessions zwingen

### Phase 5: Testing & Dokumentation
1. Unit Tests für alle Komponenten
2. Integration Tests mit echtem opencode
3. **Komplette Dokumentation aktualisieren:**
   - Model-Proxy-Konfiguration erklären
   - starfleetctl Befehle dokumentieren
   - Context-Limit-Header Verhalten erklären
   - Beispiele für Strategien zeigen (heavy-model, cruiser-model, scout-model)
   - Konfigurierbare Optionen erklären (cooldown periods, heuristic thresholds)

---

## Teil 4: Circuit Breaker + Session-Affinität

### Affinität
- Default: gleiche Session → gleiches Modell (Cache-Shard)
- Fallback bricht Affinität temporär
- Session-ID wird konsistent an Upstream durchgereicht

### Circuit Breaker (Netflix-Style)

```
Zustände:
  CLOSED    → normal, Affinität aktiv
  OPEN      → Modell geskippt, Fallback aktiv
  HALF-OPEN → nach Cooldown 1 Request testen

Transition:
  CLOSED  → OPEN:      bei Schwellenwert-Überschreitung
  OPEN    → HALF-OPEN: nach cooldown
  HALF-OPEN → CLOSED:  bei Erfolg
  HALF-OPEN → OPEN:    bei erneutem Fehler
```

### Heuriken (konfigurierbar: global + per Strategy)

| Metrik              | Schwellenwert (Default) | Aktion                    |
|---------------------|-------------------------|---------------------------|
| Latenz p95          | > 30s                   | Affinität 1 Request brechen|
| Fehlerrate          | > 20% in 5min          | Circuit Breaker OPEN      |
| Token-Durchsatz     | < 10 tokens/s          | Modell-Wechsel            |
| Consecutive Failures| ≥ 3                     | Sofortiges Skip           |

---

## Teil 5: Trigger-Regeln

```yaml
triggers:
  rate-limited:
    action: skip
    cooldown: 60s
    after: 3 retries

  quota-exhausted:
    action: skip
    cooldown: 3600s
    auto-recover: true

  timeout:
    action: skip
    cooldown: 30s
    max-consecutive: 3

  error:
    action: skip
    cooldown: 10s
```

---

## Teil 6: Provider-Health

```yaml
providers:
  nim-proxy:
    base-url: http://127.0.0.1:8443/v1
    health-check:
      interval: 30s
      timeout: 5s
      endpoint: /v1/models

  zen-proxy:
    base-url: http://127.0.0.1:8443/v1
    health-check:
      interval: 60s
```

---

## Teil 7: Routing-Tabelle

```yaml
routing:
  heavy-model: heavy-model
  cruiser-model: cruiser-model
  scout-model: scout-model
  balanced-model: balanced-model
```

---

## Offene Punkte

1. **Weighted-Round-Robin Sticky Selection Cooldown**: 
   - **Preset**: 300 Sekunden (5 Minuten) 
   - **Konfigurierbar**: pro Strategie über `cooldown_period` Parameter

2. **User-Dokumentation**: 
   - Vollständige Dokumentation wird erstellt
   - Beispiele: heavy-model, cruiser-model, scout-model
   - Alle Konfigurationsoptionen erklärt
   - starfleetctl Befehle dokumentiert

Die beiden Topics sind jetzt vollständig spezifiziert und bereit für die Implementierung:
- `starfleet/task-agent-templates-schiffsklassen` – Schiffs-Klassen-Templates + Enterprise-Koordination
- `starfleet/task-model-proxy-meta-models` – Model-Routing mit Context-Sicherheit, starfleetctl-Integration, forced switching, session-affinity, circuit breaker, heuristiken, kompletter Dokumentationsplan

- 2026-09-16T10:05:09Z Scotty: completed

- 2026-09-16T10:25:34Z Enterprise: completed
