---
name: flagship-standing-ships
description: "Flagship (Enterprise) requirements for the standing ships in this workspace — ONLY the flagship needs these; other ships must not load or follow this skill. Use when respawning, monitoring, or delegating to Scotty/Barcley/Galaxy. Not for the starfleetctl repo."
---

# Standing Ships (Flagship-Anweisungen)

Diese Anweisungen gelten **nur für das Flagschiff (Enterprise) und nur für
dieses Workspace** (`mpbt-workspace`). Andere Schiffe müssen sie nicht kennen
oder beachten, und sollen diesen Skill **nicht** laden. Nicht in die
starfleetctl-Repo übernehmen.

## Ständige Schiffe

Folgende drei Schiffe müssen **stets vorhanden** sein (laufend, falls nötig
automatisch wieder hochgezogen):

- **Scotty** — einzuständig für **alle Arbeiten am starfleetctl** (und nur
  dafür; keine xlibre/X11/os.h/desqview/Kernel-Tasks).
- **Barcley** — zuständig für den **Volla-Kernel** (und nur dafür).
- **Galaxy** — für **verschiedene andere größere Aufgaben (on-demand)**.

Alle drei laufen wahlweise mit **Nemotron Ultra** oder **Nemotron 3**
(`nvidia/nemotron-3-ultra-550b-a55b` bzw. das jeweils verfügbare Nemotron-3
Modell), je nachdem was gerade am besten geht (Auslastung/Rate-Limits).

## Neustart bei Absturz / Stop

Wenn eines der drei Schiffe abstürzt oder gestoppt werden muss:

1. Automatisch wieder neu starten (`session ship-run --name <ship> --model <modell> --class worker --launch-type background`).
2. Continuation-Direktive senden, damit es **an seiner Aufgabe weiterarbeitet**.
3. Vorher gestopptes zuständigkeitsfremdes Zuweisungen ggf. korrigieren (siehe
   "Task-Zuständigkeit" oben).

## Eigenständiges Arbeiten + Reports

- Die Schiffe müssen ihre zugewiesenen Tasks **sauber und eigenständig**
  abarbeiten (echte Arbeit, keine Platzhalter; Tools korrekt aufrufen; Tests
  laufen lassen; committen mit Doku).
- **Nach jedem abgearbeiteten Task einen starfleet-Report einstellen**
  (`starfleetctl reports submit`). Das Flagschiff stellt sicher, dass das
  passiert.
- Das Flagschiff **überwacht die Schiffe** (Board, Screens bei Auffälligkeiten)
  und tut das Nötige, damit sie durchgängig arbeiten können (Respawn,
  Continuation, Rate-Limit-Abwarten, Modellwechsel).

## Störungen dokumentieren (Reports)

Bei **jeder Störung** (Absturz, Stop, Loop, Rate-Limit, Blockade) und deren
**Behebung** wird ein starfleet-Report eingestellt (`starfleetctl reports
submit`), der **genau beschreibt**:

- **Was passiert ist** (Symptome: z.B. Repetitions-Loop, deutsches
  Prosa-Text als Bash-Kommando mit exit 127, 429 rate-limit, eingefrorener
  Screen, Kontext-Overflow).
- **Wie das Problem gelöst wurde** (z.B. session stop + respawn mit anderem
  Modell, Continuation-Direktive, Task-Neuzuweisung, auf Rate-Limit warten).
- **Welches warum** / Erkenntnisse.

Diese Reports dienen als **Lektionen für später** — wiederkehrende Muster,
funktionierende Modelle je Aufgabe, do's/don'ts beim Respawn. Wichtige
generalisierbare Erkenntnisse zusätzlich in die `agents.d/local/` knowledge
dump ablegen bzw. als SOP-Fragment promoten, wenn sie sich als stabil
erweisen.