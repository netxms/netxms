# UltraVNC Screen Blanking in Remote Control

## Overview
Remote Control can ask an UltraVNC server to disable its local keyboard and mouse and blank its monitor, so people at the remote machine can't watch or interfere while an operator works. A new server option, off by default, makes both consoles do this on every connect. Both consoles also get a toggle for doing it mid-session.

The web console gets the feature from the noVNC fork at github.com/alkk/noVNC, which already has the setting and the toolbar button. The desktop console gets it from vernacular-vnc.

**Prerequisite:** vernacular-vnc 1.20 must be released first. Its plan is `docs/plans/20261112-1-ultravnc-detection-and-eof.md` in the vernacular-vnc repo. What it adds:
- UltraVNC detection, exposed as `VernacularClient.isServerInputSupported()`;
- nothing is sent to servers it does not detect;
- one terminal error-listener event per connection, including a clean server disconnect;
- `isRunning()` is already false when that event fires.

## Decisions
- **The option sets the state at connect time. It does not gate the feature.** Name: `Client.RemoteControl.BlankScreenOnConnect`, boolean, default `0`, no restart needed. The toggle is always available on UltraVNC servers.
  - Feature-switch and tri-state (free/locked/forbidden) variants were ruled out. noVNC's `mandatory.json` could express a lock, but nothing needs one yet.
- **Delivery uses the existing `Client.*` configuration hints, so the server needs no code change.**
  - `GetClientConfigurationHints` strips the `Client.` prefix, so the consoles read the hint as `RemoteControl.BlankScreenOnConnect`.
  - A per-user override is a user attribute named `Client.RemoteControl.BlankScreenOnConnect`, with the prefix. Attributes go through the same prefix filter, so an unprefixed attribute is ignored.
  - The console receives hints only in the login response; `NXCSession` has no way to refresh them. A changed value therefore takes effect at the next console login, without a server restart. Picking it up on every VNC connect was ruled out: it would need a new refresh path, and for a setting that rarely changes, logging in again is enough.
- **Only UltraVNC servers get the request, in both consoles.** noVNC recognises UltraVNC by security type 17 and only then shows its button (`capabilities.serverInput`). vernacular 1.20 uses the same rule. Other servers get no type-9 message: the session stays up, the web button is hidden and the desktop action is disabled.
- **The toggle shows the state this connection requested, not what the server actually did.** UltraVNC ignores the request in view-only mode, lets only the viewer that blocked input unblock it, and blanks the monitor only if "Enable Blank Monitor on Viewer Request" is on. The desktop tooltip says this.
- **Web console: replace the bundled noVNC with the fork's master (`ae202d8`) and drop NetXMS's own patch from #3818.**
  - The bundled copy dates from 2024-07, and its only local change is the #3818 relative-WebSocket-path patch in `app/ui.js`.
  - Upstream `connect()` now resolves a host-less `path` against `location.href` and picks `ws`/`wss` itself. That keeps the reverse-proxy prefix, so the patch is no longer needed.
  - Catch: noVNC reads settings in this order: URL query, then the browser's localStorage, then defaults. A `host` saved in localStorage would send the connection down the root-based branch and lose the proxy prefix. So the embedding URL must always pass an empty `host=`.
  - For the same reason `ultravnc_blank_screen` is always passed explicitly as `true` or `false`.
- **The desktop toggle is a check-style action in the view toolbar and menu, next to Connect/Disconnect.** It is disabled (not hidden) when the server isn't UltraVNC. Its icon is rasterised from the fork's `app/images/blankscreen.svg`, so both consoles use the same symbol. The SVG's white stroke is meant for noVNC's dark control bar, so the icon is recoloured to match `connect.png`/`disconnect.png` on the light toolbar.
  - Each connection builds a fresh `VernacularConfig` from the hint. A manual toggle never carries over to the next connection.
- **Going from 1.17 to 1.20 crosses 1.18's breaking change ("Upgrading to 1.18" in the vernacular README).**
  - Tight is enabled by default, and since 1.18 a Tight JPEG sub-rectangle throws "No JPEG decoder configured", which kills the session.
  - The desktop view sets an ImageIO-based `JpegDecoder` on every config. `java.desktop` is already used in the module.
  - It was ruled out to rely on servers not sending JPEG. vernacular sends no quality pseudo-encoding today, but that is an inference about server behaviour, not a guarantee.
  - `handleKeyEvent`, which 1.18 removed, is not used by NetXMS.
- **Tasks 1–2 do not depend on vernacular 1.20.** They can land before the release; only Tasks 3–4 wait for it.
- **The desktop view's connection lifecycle gets fixed in its own commit, before the toggle.** The toggle's enabled state depends on it. The fixes are useful on their own and fail in different ways from the toggle. Existing bugs:
  - The worker waits only for `stopFlag`, so a server disconnect or library error leaves the view showing "connected".
  - A failure starting the forwarder or `start()` skips cleanup.
  - `jobFailureHandler` leaves Connect disabled.
  - `dispose()` only sets `stopFlag`, so a handshake that stalls (no banner, or waiting for auth) blocks forever and never reaches cleanup.
- Admin guide entry for the new parameter: separate doc repo, later.

## Implementation Steps

### Task 1: Server configuration parameter
Layers: DB schema (`sql/setup.in`, nxdbmgr upgrade, `include/netxmsdb.h`).

Description: "Request UltraVNC servers to disable local input and blank the monitor when a remote control session starts. Ignored for other VNC servers."

- [x] Add the `Client.RemoteControl.BlankScreenOnConnect` row to `sql/setup.in`: `B`, `0`/`0`, visible, no restart.
- [x] Add a `upgrade_v71.cpp` step from 71.10 to 71.11 that creates the parameter via `CreateConfigParam`, plus its upgrade-map entry. Bump `DB_SCHEMA_VERSION_MINOR` to 11. The change is master-only, so there is no stable-branch guard; check that 71.10 is still the head at commit time.
- [x] Verify: `nxdbmgr upgrade` on a 71.10 database and `nxdbmgr init` on a fresh one both end with the parameter present and identical.

### Task 2: Web console: sync noVNC and pass settings
Layers: web UI (rwt resources, rwt `RemoteControlView`).

- [x] `src/client/nxmc/java/src/rwt/resources/vncviewer` matches the fork's master (`ae202d8`) for `vnc.html`, `app/`, `core/` and `vendor/`. Also add `defaults.json` and `mandatory.json`, each containing `{}`. Excluded, as in the 2024 import:
  - `docs/`, `tests/`, `po/`, `utils/` and `snap/`;
  - package files and `.DS_Store`;
  - `app/images/icons/Makefile`, `app/locale/README`, `app/sounds/CREDITS` and `vendor/pako/README.md`.

  `vendor/pako/LICENSE` is kept, because pako is MIT-licensed. Files that no longer exist upstream are removed.
- [x] The #3818 patch is gone, and `app/ui.js` is identical to the fork's.
- [x] The rwt `RemoteControlView` URL carries `host=` (empty) and `ultravnc_blank_screen=true|false` from the hint, defaulting to false.
- [x] Commit message records the fork URL and commit.
- [x] `mvn -f src/client/nxmc/java/pom.xml clean package -Pweb` builds.

### Task 3: Desktop console: vernacular 1.20 and connection lifecycle
Layers: desktop UI (swt `RemoteControlView`, `pom.xml`).

- [x] The `vernacular-vnc` dependency is 1.20, and every config sets an ImageIO `JpegDecoder`.
- [x] The worker ends when the client stops on its own (server EOF or library error), not only on `stopFlag`.
- [x] Cleanup covers the whole connection, from forwarder startup through `start()`. The client and forwarder belong to the attempt, not to the view.
- [x] Disconnect or dispose during a stalled handshake (no banner yet, or waiting for auth) actively aborts the pending connection by closing the forwarder or socket. The SWT thread never blocks on the worker, which may be waiting in the password prompt's `syncExec`.
- [x] Every way a connection ends resets Connect and Disconnect, including the failure handler: auth failure, EOF or error, cancel or dispose, normal disconnect.
- [x] Callbacks queued for a disposed view or an earlier attempt do nothing. That covers the password prompt and the screen, cursor, clipboard and "connected" updates.
- [x] `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone` builds.

### Task 4: Desktop console: blank toggle
Layers: desktop UI (swt `RemoteControlView`, icons).

- [x] Each connection sets `VernacularConfig.setBlankRemoteScreen` from the hint.
- [x] Add a "Blank remote screen" check action in the view toolbar and menu, with a tooltip and an icon (`blank-screen.png` + `@2x`):
  - At the start of every attempt it is disabled and unchecked.
  - After a successful `start()` it is enabled if the server is UltraVNC, and checked if both the hint is on and the server is UltraVNC.
  - Toggling sends the request only while this attempt is live: not stopping, client present and running, server supported.
  - It is disabled immediately on Disconnect and in `dispose()`, and reset on every way a connection ends (Task 3).
- [x] `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone` builds.

## Post-Completion
Commit subjects end with the tracking issue `(issue #NNNN)`. The number is to be supplied before the first commit.

Manual verification, both consoles unless noted:
- Server: the parameter is visible in Server Configuration. A change takes effect after the console logs in again, with no server restart; an already logged-in console keeps the old value. A per-user attribute `Client.RemoteControl.BlankScreenOnConnect` overrides it.
- Windows UltraVNC with "Enable Blank Monitor on Viewer Request" on:
  - With the option on, the monitor blanks on connect and the toggle shows pressed.
  - Toggling releases and re-blanks it.
  - Disconnecting restores the monitor.
- With the option off, the session connects unblanked and the toggle works.
- A non-UltraVNC server (TightVNC or macOS Screen Sharing) with the option on: the session stays up, the web button is hidden and the desktop action is disabled.
- Web console behind a path-rewriting reverse proxy, with a non-empty `host` saved in the viewer's localStorage beforehand, still connects. This is the #3818 regression check.
- Desktop:
  - After an auth failure, a retry works because Connect is re-enabled.
  - Killing the server process resets the view.
  - Reconnecting after a manual toggle that disagrees with the option starts from the option's state.
  - Moving from an UltraVNC node to a non-UltraVNC node disables the action.
  - Disconnect, or closing the view, while the server hasn't sent its banner yet or is waiting for the password: the view resets, the tunnel closes and the UI never freezes.
  - After the 1.17 → 1.20 upgrade, Tight sessions against TightVNC and UltraVNC render correctly, including photo-like content that may produce JPEG sub-rectangles.
- View-only credentials enforced by the server: both consoles still show the toggle, because neither client knows the server's permission in advance. The server ignores the request, and the toggle only shows the requested state. noVNC hides its button only in its own `view_only` mode.
- Second viewer while the first holds the block: the second viewer cannot release it, as expected.

Follow-up: an admin guide entry for `Client.RemoteControl.BlankScreenOnConnect` in the documentation repo.
