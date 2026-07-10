# Actionable protocol version mismatch dialog

## Overview

When nxmc connects to a server on a different release line, the user sees:

```
Connection Error

Server uses incompatible version of communication protocol
(server=[62, 4, 1, 1, 1, 56, 1, 2] client=[60, 4, 1, 1, 1, 47, 1, 2])
```

Two raw integer arrays. The array positions are named in code (`INDEX_BASE`, `INDEX_ALARMS`, `INDEX_FULL`, ...) but the names never reach the user, and neither do the product versions. Nothing tells the user what to do.

This change replaces that dialog, for this one error code, with a dialog that names both product versions and links to the release directory holding a matching client.

**Domain facts that shape the design** (from project owner):

- Server and client with the same `major.minor` are **always** compatible. A protocol mismatch therefore always means `major.minor` differ, and the fix is always "get a client matching the server".
- All releases are published at `https://netxms.org/download/releases/MAJOR.MINOR/`.
- Running a client **newer** than the server is normal and legitimate, especially for admins managing several servers on different release lines. The dialog must not frame this as a server fault or suggest upgrading the server.

**Key enabler**: `NXCSession.setupSession()` throws the protocol error *before* it reads `VID_SERVER_VERSION` / `VID_SERVER_BUILD` out of the very same response message. The human-readable server version is already in hand and simply discarded.

## Context (from discovery)

Files/components involved:

- `src/client/java/netxms-client/src/main/java/org/netxms/client/NXCSession.java` — `setupSession()` at line 2379 performs the check and throws `new NXCException(RCC.BAD_PROTOCOL, protocolVersion.toString())` at line 2385, after a `logger.warn` with the raw arrays at line 2384. `serverVersion` / `serverBuild` / `serverId` are assigned from `response` a few lines *below* the throw.
- `src/client/java/netxms-client/src/main/java/org/netxms/client/ProtocolVersion.java` — `toString()` produces the raw `server=[...] client=[...]` dump.
- `src/client/java/netxms-client/src/main/java/org/netxms/client/NXCException.java` — `getErrorMessage()` delegates to `RCC.getText(code, lang, additionalInfo)`: one localized skeleton per error code, one `%s`. `RCC_0040` lives in all eight `netxms-client-messages*.properties` files.
- `src/client/nxmc/java/src/main/java/org/netxms/nxmc/base/login/LoginJob.java` — calls `Registry.setSession()` only at line 258, *after* successful login. On protocol failure nxmc has no session to query, so the exception must carry the data.
- `src/client/nxmc/java/src/swt/java/org/netxms/nxmc/Startup.java:608-616` and `src/client/nxmc/java/src/rwt/java/org/netxms/nxmc/Startup.java:467-475` — the two login-failure handlers. Both branch on `RCC.OPERATION_CANCELLED`, but **they are not symmetric**: swt calls `MessageDialog.openError(null, i18n.tr("Connection Error"), ...)`, while rwt calls `loginDialog.setErrorMessage(cause.getLocalizedMessage())` and lets the login form redisplay the text inline.
- `src/client/nxmc/java/src/rwt/java/org/netxms/nxmc/base/login/LoginDialog.java:399-402` — `setErrorMessage()` only stores the string into a field for the *next* `open()`. The login dialog's shell is already disposed when the failure handler runs, so `loginDialog.getShell()` is not a usable parent.

Related patterns found:

- **Closest precedent for the new dialog**: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/dialogs/HostCommandBlockedDialog.java` — a JFace `Dialog` with system icon + banner-font header + wrapped prose + SWT `Link` + `ExternalWebBrowser.open()`, laid out with `WidgetHelper.DIALOG_*` constants and localized via `LocalizationHelper.getI18n(Class)`.
- `org.netxms.nxmc.tools.ExternalWebBrowser.open(String)` exists in **both** the swt and rwt source trees under the same package, so code in `main/` calls it unconditionally. Precedent: `base/menus/HelpMenuManager.java:67`.
- `Registry.IS_WEB_CLIENT` — defined separately in the swt and rwt `Registry.java`, referenced freely from `main/` (e.g. `modules/filemanager/views/AgentFileManager.java:504`).
- `org.netxms.base.VersionInfo.version()` lives in `netxms-base`, so it is reachable from both `netxms-client` and nxmc. Used by `AboutDialog.java:121`.

Dependencies identified:

- `netxms-base` → `netxms-client` → nxmc. `VersionInfo` sits at the bottom, so both layers can read the client's own product version.
- No new external dependencies.
- **Maven build order matters.** nxmc is *not* a module of the `src/pom.xml` reactor (`src/pom.xml:9-19`); it resolves `netxms-client` at `${project.version}` from the local `~/.m2` repository (`src/client/nxmc/java/pom.xml:608-610`). The new exception class must be installed into `~/.m2` before any nxmc build, or nxmc compiles against a stale artifact and fails with `cannot find symbol: ProtocolVersionException`.

The typical build for this change:

```bash
# install base + client into ~/.m2 (needed for both web and desktop)
mvn -f src/pom.xml install -pl java-common/netxms-base,client/java/netxms-client

# desktop
mvn -f src/client/nxmc/java/pom.xml package -Pdesktop

# web
mvn -f src/client/nxmc/java/pom.xml package -Pweb
```

## Development Approach

- **testing approach**: Regular (code first, then tests).
- Complete each task fully before moving to the next.
- Make small, focused changes.
- **Test coverage reality for this change** (verified during discovery):
  - `netxms-client` **has** a JUnit 5 harness at `src/client/java/netxms-client/src/test/java/org/netxms/client/`, including `ExceptionTest.java`. Task 2 therefore carries a real unit test.
  - `src/client/nxmc` has **no test directory at all** — there is no harness for SWT/JFace dialogs anywhere in the tree. Tasks 3 and 4 cannot ship unit tests without standing up a UI test harness, which is far outside this change's scope. Those tasks are verified manually per the Post-Completion section, and this is called out explicitly rather than silently skipped.
- **All tests must pass before starting the next task.**
- **Update this plan file when scope changes during implementation.**
- Maintain backward compatibility: `getErrorCode()` still returns `RCC.BAD_PROTOCOL` and `getLocalizedMessage()` still returns the `RCC_0040` text, so no existing consumer of `netxms-client` sees a behavior change.

## Testing Strategy

- **unit tests**: Task 2 (`ProtocolVersionException`) — assert the error code, the preserved `RCC_0040` localized message, and the three new getters. Runs with the existing JUnit 5 setup: `mvn -f src/client/java/netxms-client/pom.xml test`.
- **e2e tests**: the project has no UI-based e2e harness (no Playwright/Cypress/SWTBot). Not applicable.
- **manual verification**: the dialog itself, in both the desktop (SWT) and web (RWT) clients. Procedure in Post-Completion.

## Progress Tracking

- Mark completed items with `[x]` immediately when done.
- Add newly discovered tasks with ➕ prefix.
- Document issues/blockers with ⚠️ prefix.
- Update plan if implementation deviates from original scope.

## Solution Overview

Three moves:

1. **Stop discarding the server version.** Hoist the `serverVersion` / `serverBuild` assignments in `setupSession()` above the protocol check. They are plain field reads from `response` with no side effects.

2. **Carry the data out in a typed exception.** `ProtocolVersionException extends NXCException`, constructed with `RCC.BAD_PROTOCOL` and `additionalInfo = protocolVersion.toString()`. Every existing handler and the `RCC_0040` fallback behave exactly as before; nxmc downcasts to reach the new getters. This is the minimum surface that works: `ProtocolVersion` never escapes `NXCSession` today, and `LoginJob` has not yet published the session when the throw happens.

3. **Show a dialog that says what to do.** `ProtocolVersionMismatchDialog` renders both product versions adjacently and links to `https://netxms.org/download/releases/<major>.<minor>/` derived from the server's version.

### Key design decisions and rationale

- **No older/newer verdict is computed.** Earlier drafts compared the protocol arrays componentwise to say "your client is too old". Once "install a client matching the server" became the advice in *every* direction, the verdict earned nothing: the two version numbers sit one above the other and the direction is self-evident. This deletes the componentwise comparison, the mixed-direction edge case, and any new methods on `ProtocolVersion`. **Do not add comparison helpers.**
- **The link target is the server's `major.minor` in both directions**, because in both directions the resolution is "get a client that matches this server". Only the surrounding sentence varies.
- **Title is "Incompatible Client Version", not "Connection Error"** — the connection succeeded; the versions disagree.
- **No "connect anyway" button.** `ignoreProtocolVersion` stays a deliberate command-line/properties flag. Confirmed with the project owner: surfacing it as one click invites people to run genuinely incompatible clients.
- **Raw protocol arrays stay out of the dialog.** They are already written to the log at `NXCSession.java:2384`, which is where support wants them.
- **One dialog class, one conditional sentence** for web vs desktop, rather than two dialogs.

## Technical Details

### Dialog content

```
Incompatible Client Version

  (!)  This management client cannot connect to this server

       Server:  5.3.1 (build 5.3.1-2043)
       Client:  5.1.4

       Server and client must have the same major and minor version.
       Download a matching client from netxms.org/download/releases/5.3.

                                                              [ OK ]
```

Closing sentence branches on `Registry.IS_WEB_CLIENT` — **the link is present in both cases**:

| Client | Sentence |
|---|---|
| desktop | `Download a matching client from <a>netxms.org/download/releases/5.3</a>.` |
| web | `The deployed web client does not match the server. Deploy a matching web client from <a>netxms.org/download/releases/5.3</a>.` |

Rationale for the web wording: the web client is served by the server, so the reader cannot "install" anything locally — but they are typically the admin, so "contact your administrator" is a dead end. The matching web client ships from the same release directory, so the link stays useful; only the verb changes.

### Version parsing

- Release URL is built from the **server's** version, matched against `^(\d+)\.(\d+)`; groups 1 and 2 form the path segment.
- `serverBuild` null or empty → omit the `(build ...)` suffix entirely. Never render `build null`.
- `serverVersion` not matching `^(\d+)\.(\d+)` (unexpected/dev build string) → keep the "must have the same major and minor version" sentence and **omit the link sentence** rather than emit a broken URL.

### Processing flow

```
NXCSession.setupSession()
  ├─ read serverVersion, serverBuild from response      <- hoisted above the check
  ├─ protocolVersion = new ProtocolVersion(response)
  └─ if mismatch:
       logger.warn("Connection failed (server=[..] client=[..])")   <- unchanged
       throw new ProtocolVersionException(protocolVersion, serverVersion, serverBuild)
                                    │
                                    │ errorCode == RCC.BAD_PROTOCOL
                                    │ additionalInfo == protocolVersion.toString()
                                    ▼
LoginJob  ──(job failure)──▶  Startup
                                    │
                    cause instanceof ProtocolVersionException ?
                          │
              yes ────────┴──────── no
               │                     │
   new ProtocolVersionMismatchDialog(null, e).open()
               │                     ├─ swt: MessageDialog.openError(null, "Connection Error", msg)
               │                     └─ rwt: loginDialog.setErrorMessage(msg)
               │
        same call in both variants (parent shell is null in both)
```

The `yes` branch is identical in swt and rwt; the `no` branch is the pre-existing, already-divergent code and is left alone.

## What Goes Where

- **Implementation Steps** (`[ ]` checkboxes): code changes and the one unit test that the existing harness supports.
- **Post-Completion** (no checkboxes): manual dialog verification in desktop and web clients, which requires a running server on a different release line.

## Implementation Steps

### Task 1: Hoist server version reads above the protocol check

**Files:**
- Modify: `src/client/java/netxms-client/src/main/java/org/netxms/client/NXCSession.java`

- [x] in `setupSession()`, move the `serverVersion = response.getFieldAsString(NXCPCodes.VID_SERVER_VERSION);` and `serverBuild = response.getFieldAsString(NXCPCodes.VID_SERVER_BUILD);` assignments from below the `if (!ignoreProtocolVersion)` block to immediately after `protocolVersion = new ProtocolVersion(response);`
- [x] verify no duplicate assignment remains further down the method
- [x] leave `serverId`, `serverTimeZone`, `serverTime`, `serverTimeRecvTime`, `serverChallenge` where they are — only the two version fields move
- [x] leave the `logger.warn` at the throw site untouched (raw arrays must keep reaching the log)
- [x] build the client library: `mvn -f src/client/java/netxms-client/pom.xml compile`
- [x] run existing tests — must pass before task 2: `mvn -f src/client/java/netxms-client/pom.xml test`

### Task 2: Add ProtocolVersionException

**Files:**
- Create: `src/client/java/netxms-client/src/main/java/org/netxms/client/ProtocolVersionException.java`
- Modify: `src/client/java/netxms-client/src/main/java/org/netxms/client/NXCSession.java`
- Create: `src/client/java/netxms-client/src/test/java/org/netxms/client/ProtocolVersionExceptionTest.java`

- [x] create `public class ProtocolVersionException extends NXCException` with the standard NetXMS GPL header and a `serialVersionUID`
- [x] constructor takes `ProtocolVersion protocolVersion, String serverVersion, String serverBuild`; calls `super(RCC.BAD_PROTOCOL, protocolVersion.toString())` so `getErrorCode()` and `getLocalizedMessage()` are byte-for-byte identical to today
- [x] add `getProtocolVersion()`, `getServerVersion()`, `getServerBuild()`
- [x] in `NXCSession.setupSession()`, replace `throw new NXCException(RCC.BAD_PROTOCOL, protocolVersion.toString())` with `throw new ProtocolVersionException(protocolVersion, serverVersion, serverBuild)`
- [x] write test asserting `getErrorCode() == RCC.BAD_PROTOCOL` and that the three getters round-trip their constructor arguments
- [x] write test asserting `getLocalizedMessage()` still yields the `RCC_0040` text with the raw-array `%s` substituted (guards the third-party-consumer contract). Borrow only the `Locale.setDefault()` handling from `ExceptionTest.java` — **that test asserts nothing, it just prints to stdout**, so it is not an assertion template. Write real `assertEquals` / `assertTrue`.
- [x] write test for the null-`serverBuild` case — the exception must accept it without throwing
- [x] run tests — must pass before task 3: `mvn -f src/client/java/netxms-client/pom.xml test`
- [x] **install the artifacts so nxmc can see the new class** (nxmc is outside the `src/pom.xml` reactor and resolves `netxms-client` from `~/.m2`; skipping this makes Task 3 fail with `cannot find symbol: ProtocolVersionException`):
      `mvn -f src/pom.xml install -pl java-common/netxms-base,client/java/netxms-client`

### Task 3: Add ProtocolVersionMismatchDialog

**Files:**
- Create: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/base/dialogs/ProtocolVersionMismatchDialog.java`

- [x] create the dialog `extends Dialog`, mirroring `HostCommandBlockedDialog` structure: `configureShell()` sets title `i18n.tr("Incompatible Client Version")`; `createDialogArea()` builds icon + header + prose; `createButtonsForButtonBar()` creates a single OK button
- [x] constructor takes `Shell parentShell` and the `ProtocolVersionException` (or its `serverVersion` / `serverBuild` strings)
- [x] header label uses `SWT.ICON_ERROR` system image and `JFaceResources.getBannerFont()`, text `i18n.tr("This management client cannot connect to this server")`
- [x] render the two versions adjacently — server as `<version> (build <build>)`, client as `VersionInfo.version()`; omit the `(build ...)` suffix when `serverBuild` is null or empty
- [x] add private static helper parsing `^(\d+)\.(\d+)` out of the server version and returning `https://netxms.org/download/releases/<major>.<minor>/`, or `null` when it does not match
      ➕ split into `getReleaseBranch()` (parse, returns `null` on mismatch) and `getReleaseUrl()` (build URL) — the `major.minor` string is also needed for the link caption, so parsing it twice would be wasteful
- [x] add the closing `Link` widget, branching the sentence on `Registry.IS_WEB_CLIENT`; wire `SelectionAdapter` to `ExternalWebBrowser.open(url)`
      ➕ release branch passed as an `i18n.tr()` `{0}` placeholder rather than concatenated, so translators get one whole sentence
- [x] when the helper returns `null`, render the "must have the same major and minor version" sentence as a plain `Label` and skip the `Link` entirely
- [x] add explicit `import` for every referenced class — no fully-qualified inline names (project CLAUDE.md)
- [x] use 3-space indent, opening brace on new line, all user-facing strings through `i18n.tr()`
- [x] ⚠️ no unit test: `src/client/nxmc` has no test harness (verified — no test directory exists in the tree). Covered by manual verification in Post-Completion.
- [x] confirm Task 2's `mvn -f src/pom.xml install -pl java-common/netxms-base,client/java/netxms-client` has been run — nxmc resolves both from `~/.m2`, not from the reactor
- [x] build both client variants to confirm it compiles under SWT and RWT — must pass before task 4:
      `mvn -f src/client/nxmc/java/pom.xml package -Pdesktop -Pstandalone` and `mvn -f src/client/nxmc/java/pom.xml package -Pweb`

### Task 4: Wire the dialog into both Startup variants

**Files:**
- Modify: `src/client/nxmc/java/src/swt/java/org/netxms/nxmc/Startup.java`
- Modify: `src/client/nxmc/java/src/rwt/java/org/netxms/nxmc/Startup.java`

**The two handlers are NOT symmetric** — do not search for `MessageDialog.openError` in the rwt file, it is not there:

| | swt `Startup.java:608-616` | rwt `Startup.java:467-475` |
|---|---|---|
| existing failure display | `MessageDialog.openError(null, "Connection Error", msg)` | `loginDialog.setErrorMessage(msg)` |
| where the new branch goes | before that `MessageDialog.openError` call | before that `setErrorMessage` call |

- [x] in swt `Startup.java`, inside the existing `if (!(cause instanceof NXCException) || errorCode != RCC.OPERATION_CANCELLED)` block, branch: if `cause instanceof ProtocolVersionException`, open `ProtocolVersionMismatchDialog`; else fall through to the existing `MessageDialog.openError(...)`
- [x] in rwt `Startup.java`, apply the same branch, but the fall-through is `loginDialog.setErrorMessage(cause.getLocalizedMessage())` — **both files, neither silently skipped** (project CLAUDE.md implementation discipline)
- [x] pass `null` as the dialog's parent shell in **both** variants. Do not use `loginDialog.getShell()` in rwt: `setErrorMessage()` (`rwt/.../LoginDialog.java:399-402`) merely stores the string for the next `open()`, meaning the login dialog's shell is already disposed at this point. swt already passes `null` to `MessageDialog.openError`.
- [x] rwt only: after the modal dialog closes, the `while(!success)` loop reopens the login dialog. Confirm this is acceptable (the user can cancel out); do **not** additionally call `setErrorMessage` for the protocol case, since the modal already carried the message
- [x] preserve the existing `RCC.OPERATION_CANCELLED` suppression in both; the new branch is a sibling, not a replacement
- [x] preserve the existing `logger.error("Login job failed", cause)` call in both
- [x] add the `ProtocolVersionMismatchDialog` and `ProtocolVersionException` imports to both files
- [x] ⚠️ no unit test: same harness gap as Task 3.
- [x] grep the tree for any other handler of `RCC.BAD_PROTOCOL` that should be updated: `grep -rn "BAD_PROTOCOL" src/` — expected in-scope hit is only `NXCSession.java`; `libnxclient/session.cpp:233,259` and `mobile-agent/.../Session.java:374` are out of scope
- [x] build both variants — must pass before task 5

### Task 5: Verify acceptance criteria

- [x] verify all requirements from Overview are implemented
- [x] verify edge cases are handled: null/empty `serverBuild`; `serverVersion` not matching `^(\d+)\.(\d+)`
      ➕ `serverVersion` itself is never null: `NXCPMessage.getFieldAsString()` returns `""` for a missing field, so no null guard is needed in `formatServerVersion()`
- [x] verify no bypass/"connect anyway" button was introduced anywhere — the only `ignoreProtocolVersion` references are the pre-existing command-line/properties reads in both `Startup.java` files, and the only "connect anyway" string is the unrelated encryption prompt at swt `Startup.java:663`
- [x] verify no comparison helpers were added to `ProtocolVersion` (scope guard — the older/newer verdict was deliberately dropped) — `git diff master...HEAD` on that file is empty
- [x] verify the eight `netxms-client-messages*.properties` files are untouched: `git status src/client/java/netxms-client/src/main/resources/`
- [x] verify `src/client/libnxclient/main.cpp` and `src/mobile-agent/` are untouched
- [x] run full client library test suite: `mvn -f src/client/java/netxms-client/pom.xml test` — 23 tests, 0 failures
- [x] build desktop and web clients clean — both `BUILD SUCCESS`

### Task 6: [Final] Update documentation

- [x] update `CLAUDE.md` only if a genuinely new pattern emerged — none did; `ProtocolVersionMismatchDialog` reuses the `HostCommandBlockedDialog` shape (JFace `Dialog`, banner-font header, `Link` + `ExternalWebBrowser.open()`), and no CLAUDE.md documents individual dialog classes. No edit made.
- [x] no README changes expected — confirmed; nothing in the tree documents the client login failure path
- [x] move this plan to `docs/plans/completed/`

## Post-Completion

*Items requiring manual intervention or external systems — no checkboxes, informational only*

**Manual verification** (required — there is no automated coverage for the dialog):

- Force a mismatch: either point a current nxmc at an older `netxmsd`, or temporarily bump `ProtocolVersion.BASE` in a local client build.
- **Desktop (SWT)**: confirm the dialog renders, shows the correct server and client versions, the `(build ...)` suffix appears, and the link opens the right release directory in the system browser.
- **Web (RWT)**: confirm the same, with the web-specific sentence. The SWT `Link` widget behaves differently under RWT, so this must be checked separately rather than assumed from the desktop run.
- Confirm the raw-array `logger.warn` line at `NXCSession.java:2384` is unchanged and still present.
- Confirm the `(build ...)` suffix is omitted, not rendered as `build null`, when the server sends no build tag.
- Confirm a third-party consumer of `netxms-client` (anything catching `NXCException` and calling `getLocalizedMessage()`) still gets the `RCC_0040` text.

**Explicitly out of scope** (do not touch):

- `RCC_0040` text in any of the eight `netxms-client-messages*.properties` files — still the fallback for third-party `netxms-client` consumers.
- `src/client/libnxclient/main.cpp:127` — separate C++ client, has no dialog.
- `src/mobile-agent/java/src/main/java/org/netxms/mobile/agent/MobileAgentException.java`.

**Translations**:

- The new nxmc strings enter the `.po` catalogs through the normal xgettext extraction pass; no manual `.properties` editing.
