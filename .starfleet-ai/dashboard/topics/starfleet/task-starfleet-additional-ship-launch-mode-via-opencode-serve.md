Title: "starfleet: additional ship-launch mode via opencode serve"
Category: starfleet
Kind: "task"
Status: "assigned"
Assigned-To: "Scotty"
Created-By: "McKinley"
Created: "2026-09-07T08:48:29Z"
Doc-Ref: "—"

# 🛰️ Starfleet Task: Implement Dual-Mode Architecture with Unix Domain Sockets (PTY vs. Native API)

## 📋 Context & Objective
We are introducing a new architectural mode for launching client ships. The existing PTY (Terminal) infrastructure must remain fully functional as the default fallback. A new feature flag (`--mode=api` / UI Toggle) will allow launching ships in a **Native API Mode**. 

This mode can also be enabled in web gui ship launch via separate checkbox.
(note that the mode is always per-ship)

When a ship is launched in Native API Mode, `starfleetctl` will bypass raw PTY streaming for this specific instance, initialize a headless OpenCode Server listening on a local **Unix Domain Socket (UDS)**, and swap the frontend terminal (`xterm.js`) with a native web client interface. All existing core features—such as our custom **model-router** and **message injection**—must remain fully operational across both modes.

In contrast to existing PTY/terminal approach (which is read-only, the new client also allows the same user inputs as the classic interactive opencode TUI client, including sending prompts, answering questions, granting permissions, model switch, turn cancellation, etc. It should also present the extra information sent by the model (eg. 2do lists) which opencode TUI client presents on the right bar.

---

## 🛠️ Requirements & Architectural Changes

### 1. Frontend Launch Form & Fleet Flag
* **UI Toggle:** Add a switch/checkbox in the ship launch form: `"Use Native API Mode"`.
* **State Mapping:** When toggled on, pass a flag (e.g., `native_api: true`) within the launch payload to the Go backend.
* **Conditional UI Render:** 
  * If `native_api == false` -> Render the classic `xterm.js` PTY terminal.
  * If `native_api == true` -> Render the new native chat/log client layout (Chat input, SSE log feed, multiplayer view).

### 2. Unix Domain Socket Architecture (No TCP Ports)
To avoid port conflicts with the web-frontend (port `8080`) or other services, ships in API mode must communicate exclusively via Unix Domain Sockets.
* **Storage Schema:** Follow the existing ship metadata convention. Store the socket file inside the ship's specific state directory alongside its PID-files and `termctl` pipes:
  ```path
  .starfleetctl/var/ships/<ship-id>/opencode.sock
  ```
* **Process Spawn:** Launch the headless OpenCode server process passing the socket path (e.g., `opencode serve --socket .starfleetctl/var/ships/<ship-id>/opencode.sock`).
* **Lifecycle & Cleanup:** Ensure that when a ship is stopped or terminated, the corresponding `opencode.sock` file is safely unlinked/deleted from the file system to prevent file accumulation.

### 3. Service Layer Integration (Native Mode Only)
Create a clean abstraction layer inside a new `pkg/opencode` package.
* **Socket Transport:** Implement a custom `http.Transport` utilizing `net.DialContext` mapped to the `"unix"` network type to query the socket file.
* **Session Listing & Interaction:** Interface with `GET http://localhost/api/session` and `POST http://localhost/api/v1/agent/chat` through the custom Unix dialer.
* **Authentication:** Ensure `req.SetBasicAuth("opencode", password)` is applied dynamically per ship instance using the designated fleet password.

### 4. Compatibility with Existing Starfleet Features
* **Model Router Compatibility:** Ensure that when OpenCode queries an LLM, it routes requests through our existing central Starfleet model router without disruption.
* **Message Injection Preservation:** Ensure the message injection mechanism still works. For API-mode ships, intercept incoming agent-to-agent (A2A) messages and inject them directly via `POST http://localhost/api/v1/session/{id}/message` over the Unix socket instead of writing them into a PTY stream.

---

## 🚀 Definition of Done (DoD)
- [ ] The existing PTY mode functions identically to its current state when the API flag is disabled.
- [ ] Launching multiple ships concurrently works flawlessly using separate socket files inside `.starfleetctl/var/ships/`.
- [ ] Stale `.sock` files are automatically cleaned up when a ship process exits.
- [ ] Selecting an API-mode ship in the frontend displays a structured native chat/log UI instead of an empty or broken terminal.
- [ ] Message injection and custom model-routing function correctly in both modes over the Unix socket layer.
- [ ] All code compiles without errors (`go build ./...`) and passes automated tests.


## Further instructions

* do the work in a separate branch (don't merge to master yet!) and separate worktree (use starfleetctl for managing the worktrees)
* clean testing w/o interrupting the current workspace (other work still needs to be done in the same workspace)
* add starfleetctl repo to the "mpbt" solution within this mpbt workspace.
* report to McKinley and add a starfleet report.

- 2026-09-11T15:12:35Z Enterprise: Praetor-Entscheidung (2026-09-11): Direkt-Modus (opencode serve, ohne opencode-Frontend) ist REIN OPTIONAL. Standard bleibt der alte PTY-Modus. Ein explizites Flag beim Ship-Launch (Web + CLI, per-Schiff-Einzelentscheidung) aktiviert ihn. Das Flag wird pro Schiff in den Ship-Metadaten erfaßt, damit Web-Terminal/Screen-Dump/Session-Listing den Modus je Schiff kennen und entsprechend behandeln (API-Mode: keine PTY-Attach, native Chat/Log-UI; Modus sichtbar in Board/Session-Anzeige). Frontend-Umbau inkrementell (Option C: erst Chat/Log-UI für API-mode ships). Weiterleitung an Scotty (m100743).
