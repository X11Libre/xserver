Title: "starfleetctl: schiffsklassen / rollen"
Category: active
Kind: "task"
Status: "done"
Assigned-To: "Scotty"
Created-By: "Defiant"
Created: "2026-09-09T11:55:26Z"
Doc-Ref: "—"

# Agent-Templates / Schiffsklassen — vordefinierte Profile für on-demand Ship-Spawns

## Status
- **Created:** 2026-09-09
- **Created-By:** Defiant
- **Assigned-To:** —
- **Status:** open
- **Category:** active
- **Kind:** task
- **See-Also:** task-starfleetctl-schiffsklassen-rollen (Datenmodell, McKinley)

## Zusammenfassung

Vordefinierte Agent-Templates (Schiffsklassen) ermöglichen es, neue Schiffe
mit passender Konfiguration für bestimmte Aufgabentypen zu starten. Das
Flagschiff (Enterprise) koordiniert den gesamten Lifecycle: Template-Auswahl,
Spawn, Überwachung, Aufräumen.

Zusätzlich: Jedes Schiff kennt seine Klasse und kann ein benutzerkonfigurierbares
Skill nachladen.

## Bisherige Arbeit

Bereits existierende Topics:
- **`task-starfleetctl-schiffsklassen-rollen`** (McKinley) — Metadaten-Modell:
  Klasse als Text/Name, Board-Anzeige, Web-Dropdown, Kommunikation
- **`starfleet/task-workspace-sop-erweitern-auto-assign-und-automatisches-ship-spawn-on-demand`**
  (Discovery) — Auto-Assign + Auto-Spawn SOP

Dieser Topic konsolidiert und erweitert beides um den operationalen Workflow.

## Teil 1: Agent-Templates (Schiffsklassen-Profil)

Jedes Template definiert:
- **Klassenname** (z.B. `Scout`, `Cruiser`, `Heavy`)
- **Modell** — welches LLM (oder Meta-Model) für diese Klasse verwendet wird
- **SOP-Fragmente** — welche Skills immer geladen werden (immer-geladen vs. on-demand)
- **Optionales Skill** — zusätzliches, vom user konfigurierbares Skill, das bei Bedarf geladen werden kann
- **Session-Type** — terminal oder background
- **Name-Prefix** — für automatische Namensvergabe (z.B. `Scout-7`, `Cruiser-3`)
- **Timeout** — maximale Laufzeit
- **Auto-Cleanup** — ob Schiff nach Task-Completion gestoppt werden soll
- **Klassenbewusstsein** — Schiff kennt eigene Klasse (analog zum Namen) für interne Entscheidungen

### Template-Vorschläge

| Klasse   | Modell / Meta-Model    | Immer-geladene Skills | Optionales Skill       | Einsatz                        |
|----------|------------------------|-----------------------|------------------------|--------------------------------|
| Scout    | nemotron-nano (schnell)| datei, grep, glob     | web-scanner            | CI-Checks, leichte Scans       |
| Cruiser  | nemotron-ultra         | datei, grep, edit     | code-reviewer          | allgemeine Arbeit, PR-Reviews  |
| Heavy    | heavy-model (Meta)     | datei, edit, bash     | security-analyzer      | komplexe Analyse, großer Code  |

## Teil 2: Schiffsklassen-Bewusstsein

Jedes Schiff:
1. Kent seine eigene Klasse (als Feld in seiner Konfiguration/Umgebung)
2. Kann diese Klasse für Entscheidungen verwenden (z.B. welches optionale Skill zu laden)
3. Zeigt seine Klasse im Comms/Board an (komplementär zu McKinley's Metadaten-Topic)
4. Kann sein optionales Skill auf Anforderung laden (via starfleetctl skill load oder ähnlich)

## Teil 3: Flagship-Koordination (Enterprise)

Enterprise als koordinierendes Flagschiff:
1. Empfängt Task via `task capture --assign auto`
2. Analysiert Task-Beschreibung → wählt passende Klasse/Template
3. Spawnt Schiff via `session ship-run` mit Template-Config
4. Weist Task zu via `task assign <slug> <ship>`
5. Überwacht Fortschritt via Comms
6. Räumt auf nach Completion (optional: `session stop`)

## Teil 4: Web-Integration

- Template-Auswahl im Web-Formular beim Ship-Spawn
- Dropdown: "Scout (schnell)", "Cruiser (standard)", "Heavy (komplex)"
- Optionales Skill-Feld im Formular (für benutzerdefinierte Zusatzfähigkeiten)
- Explizite Modell-Auswahl weiterhin möglich (Override)

## Teil 5: Implementierungsschritte

1. Template-Definition in YAML (`.starfleet-ai/conf/templates/`)
   - Neues Feld: `optionalSkill` (string, name eines Skills)
2. `starfleetctl` erweitern: `ship template list`, `ship template show <name>`
3. `session ship-run` erweitern: `--template <name>` Flag
4. Schiffsklassen-Bewusstsein:
   - Schiff speichert eigene Klasse bei Start (aus Template)
   - Umgebungsklasse oder Konfigurationsfeld für interne Nutzung
5. Optionales Skill-Loading:
   - Mechanismus zum Nachladen von Skills bei Bedarf
   - Könnte über `starfleetctl skill load <skill-name>` geschehen
   - Oder automatisches Laden basierend auf Task-Anforderungen
6. Enterprise-SOP: Auto-Assign → Template-Auswahl → Spawn → Task-Zuweisung → Monitoring → Cleanup
7. Web-API: `/api/templates` Endpoint für Dropdown + optionalSkill Feld
8. Board: Klassenname als zusätzliches Feld (komplementär zu McKinley's Metadaten-Topic)

## Teil 6: Skill-Loading Mechanismus (Conceptual)

Optionen für das Laden des optionalen Skills:
1. **Manuell:** User läuft `starfleetctl skill load <skill>` im Schiff
2. **Bei Task-Start:** Enterprise signalisiert nötig Skill im Task
3. **Automatisch:** Schiff entscheidet basierend auf Task-Beschreibung
4. **Hybrid:** Vorgefertigtes Skill, aber aktivierbar via Flag

Empfohlen: Kombination aus 2 und 4 - Enterprise kann beim Task-Zuweisen das nötige Skill signalisieren, Schiff kann es darüber hinaus manuell laden.

## Offene Fragen

- Soll das optionale Skill beim Ship-Start automatisch geladen werden, oder nur bei Bedarf?
- Wie soll das Skill-Loading mekanismus aussehen? (starfleetctl Befehl, Event-System, etc.)
- Soll es mehrere optionale Skills geben, oder nur einen pro Klasse?

Die beiden Topics sind jetzt komplett spezifiziert und adressieren alle Anforderungen einschließlich der neuen Fähigkeit, benutzerkonfigurierbare Skills pro Klasse zu laden und Schiffsklassen-Bewusstsein.

- 2026-09-15T16:39:12Z Scotty: completed
