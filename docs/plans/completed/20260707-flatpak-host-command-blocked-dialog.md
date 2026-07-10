# Flatpak Restricted-Sandbox: Modal Dialog for Blocked Host Commands

## Overview
Follow-up to commit `ff6d7dc042` ("feat: detect restricted Flathub sandbox and block host command execution"), whose WIP note reads *"warning message should be shown differently"*.

When nxmc runs inside a restricted Flatpak sandbox (no host command execution permission), attempting to run a **local command** object tool currently surfaces an ugly error:
- Tools **with** `GENERATES_OUTPUT` → the `IOException` from `SandboxHelper.buildHostShellCommand()` is caught in `AbstractObjectToolExecutor.ExecutorJob.run`, stored as `failureReason`, and rendered as a collapsed one-line `"Failed (…)"` string in `ExecutorListLabelProvider`. Newlines are lost, the URL is not clickable, text is not selectable. *(This is the screenshot that motivated the change.)*
- Tools **without** `GENERATES_OUTPUT` → the same exception is caught by the fire-and-forget `Job` and shown as a terse "Cannot start external process" message-area bubble.

**Key insight:** the restricted-sandbox condition is *static and known at startup* (`flatpak && !hostAccessAvailable`, computed once in `SandboxHelper`'s static initializer). It does not need to be discovered by *attempting* execution. So we replace the deep-exception path with a **proactive pre-check** at the true top-level entry point (`ObjectToolExecutor.execute()`), and present a proper **modal dialog** with selectable text, a copyable `flatpak override …` command, and a clickable documentation link.

**Dispatch tree (verified against code — do NOT guard inside `executeLocalCommand()`):** `ObjectToolExecutor.execute()` (`:122`) is the single top-level entry for every object-tool run. Inside its background `Job` it branches by node count and output flag:
- **Multi-node + `GENERATES_OUTPUT`** → `executeOnMultipleNodes()` (`:329`) → `MultiNodeCommandExecutor` → `LocalCommandExecutor` widget → `buildHostShellCommand()` at `src/swt/.../widgets/LocalCommandExecutor.java:137`. The `IOException` becomes `failureReason` (`AbstractObjectToolExecutor:630`) rendered as the collapsed `"Failed (…)"` label by `ExecutorListLabelProvider:75`. **This is the motivating screenshot path — and it never reaches `executeLocalCommand()`.**
- **Single-node, or multi-node without `GENERATES_OUTPUT`** → `executeOnNode()` (`:338`/`:342`, one call per node) → `executeLocalCommand()` → either `LocalCommandResults` view (single-node output) or a fire-and-forget `Job`.

Therefore the guard must live in `execute()`, not `executeLocalCommand()`: guarding `executeLocalCommand()` would (a) miss the multi-node output path entirely and (b) pop the dialog once *per selected node* on the multi-node no-output path.

`buildHostShellCommand()` consequently has **three** call sites, all in `src/swt`/shared: `ObjectToolExecutor.java:696`, `LocalCommandResults.java:213`, `LocalCommandExecutor.java:137`. All three keep the `IOException` as a defensive fallback.

Benefits: no empty results view / console flashes open, no collapsed "Failed (…)" label, and the user gets an actionable, selectable, clickable message.

## Context (from discovery)
- **Files/components involved:**
  - `src/client/nxmc/java/src/main/java/org/netxms/nxmc/tools/SandboxHelper.java` — owns all Flatpak knowledge; static `flatpak`, `hostAccessAvailable`, `applicationId`, private `DOCUMENTATION_URL`, and `buildHostShellCommand()`.
  - `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/ObjectToolExecutor.java` — `execute()` (line 122) is the single top-level entry for all object-tool runs and executes on the UI thread (it opens `InputFieldEntryDialog` via `readInputFields()` before spawning its background `Job` at line 241). This is where the guard belongs. See the verified dispatch tree in the Overview — the branch at `:329` (multi-node output) bypasses `executeLocalCommand()`.
  - `.../modules/objecttools/views/MultiNodeCommandExecutor.java`, `.../views/helpers/ExecutorListLabelProvider.java`, `.../widgets/AbstractObjectToolExecutor.java`, `src/swt/.../widgets/LocalCommandExecutor.java`, `src/swt/.../views/LocalCommandResults.java` — the multi-node / output execution path that produces the current `"Failed (…)"` label.
  - New: `HostCommandBlockedDialog` in a new `modules/objecttools/dialogs` package (shared `src/main`).
- **Related patterns found:**
  - `WidgetHelper.copyToClipboard(String)` exists in **both** `src/swt` and `src/rwt` → clipboard copy is web-safe.
  - `ExternalWebBrowser.open(String)` exists in both platforms and is already used from shared `src/main` (e.g. `AlarmList`, `ObjectsPerspective`, `ObjectToolExecutor`).
  - SWT/JFace `Dialog`, `Link`, read-only `Text` all compile under RWT.
  - `Registry.getMainWindowShell()` is the standard shell source for modal dialogs (already imported in `ObjectToolExecutor`).
- **Dependencies identified:** none new. Pure client-side (nxmc) change.

## Development Approach
- **Testing approach:** Regular (code first). nxmc has **no UI unit-test harness**; the predecessor plan (`docs/plans/completed/20260706-flathub-sandbox-detection.md`) was verified by compiling both build profiles and by manual runtime checks. This plan follows the same discipline: **build must pass for both desktop (SWT) and web (RWT) profiles** after each code task, and behavior is confirmed by manual verification. Do **not** fabricate unit tests for a dialog where the project has no such convention.
- Complete each task fully before moving to the next.
- Make small, focused changes; maintain backward compatibility (no behavior change on Windows or non-Flatpak, since `isHostCommandExecutionBlocked()` is `false` there).
- Keep this plan file in sync if scope changes.

## Testing Strategy
- **Compile gate (required after every code task):** the touched files and the new dialog live in shared `src/main`, so both toolkits must compile.
  - Desktop: `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone`
  - Web: `mvn -f src/client/nxmc/java/pom.xml clean package -Pweb`
  - Prereqs (once): `mvn -f src/java-common/netxms-base/pom.xml install` and `mvn -f src/client/java/netxms-client/pom.xml install`.
- **Manual verification (Post-Completion):** runtime behavior inside/outside a restricted Flatpak sandbox — see Post-Completion section. No automated e2e harness exists for nxmc.

## Progress Tracking
- Mark completed items with `[x]` immediately when done.
- Add newly discovered tasks with ➕ prefix; document blockers with ⚠️ prefix.
- Update plan if implementation deviates from original scope.

## Solution Overview
1. Promote the Flatpak-specific facts needed by the UI (the blocked flag, the override command, the doc URL) to public accessors on `SandboxHelper`, and refactor the existing exception message to reuse the override-command builder so the command string is defined once.
2. Add a self-contained `HostCommandBlockedDialog` (JFace `Dialog`) that presents the situation with selectable prose, a copyable command field, and a clickable doc link.
3. Insert a proactive guard clause at the top of `ObjectToolExecutor.execute()`, gated on `TYPE_LOCAL_COMMAND`, that shows the dialog and returns early when execution is blocked — this fires exactly once and covers all node-count / output-flag combinations.
4. Keep the existing `IOException` throw in `buildHostShellCommand()` as a defensive fallback for any future path that skips the pre-check.

## Technical Details

### `SandboxHelper` API additions
```java
public static boolean isHostCommandExecutionBlocked()   // returns flatpak && !hostAccessAvailable
public static String  getHostAccessOverrideCommand()    // "flatpak override --user --talk-name=org.freedesktop.Flatpak <appId>"
public static String  getDocumentationUrl()             // returns DOCUMENTATION_URL
```
- The `DOCUMENTATION_URL` constant is updated from `https://go.netxms.org/flatpak` to **`https://go.netxms.com/flatpak-sandbox`** (correct host and path).
- `getHostAccessOverrideCommand()` uses the resolved `applicationId`, falling back to the literal `<application-id>` when it is `null`/empty — identical logic to the current inline message.
- `buildHostShellCommand()`'s `IOException` message is refactored to interpolate `getHostAccessOverrideCommand()` and `getDocumentationUrl()` instead of re-templating the command inline. The throw itself stays.

### `HostCommandBlockedDialog`
- Package `org.netxms.nxmc.modules.objecttools.dialogs`, extends `org.eclipse.jface.dialogs.Dialog`, in shared `src/main`.
- `configureShell(Shell)` → title `i18n.tr("Host Command Execution Blocked")`.
- `createDialogArea(Composite)`:
  - Warning icon + bold header label.
  - `SWT.WRAP` prose label explaining the restricted sandbox and what to do.
  - Read-only, selectable command field: `Text` with `SWT.READ_ONLY | SWT.BORDER | SWT.SINGLE`, monospace font, `GridData` with `grabExcessHorizontalSpace = true`; text = `SandboxHelper.getHostAccessOverrideCommand()`.
  - "Copy" `Button` → `WidgetHelper.copyToClipboard(SandboxHelper.getHostAccessOverrideCommand())`.
  - `Link` widget with `<a>` markup; `SelectionListener` → `ExternalWebBrowser.open(SandboxHelper.getDocumentationUrl())`.
- `createButtonsForButtonBar(Composite)` → single Close/OK button (`IDialogConstants.OK_ID` / `CLOSE_LABEL`); no Cancel (informational).
- All strings via `LocalizationHelper.getI18n(HostCommandBlockedDialog.class)`.
- Dispose the monospace `Font` on dialog close (create via `new Font(...)`, dispose in a `DisposeListener` or override `close()`), following existing nxmc font-lifecycle practice.

### Guard clause
At the very top of `ObjectToolExecutor.execute(...)` (line 122), before `readInputFields()` and before the background `Job` is spawned:
```java
if ((tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND) && SandboxHelper.isHostCommandExecutionBlocked())
{
   new HostCommandBlockedDialog(Registry.getMainWindowShell()).open();
   return;
}
```
`execute()` runs on the UI thread (it opens `InputFieldEntryDialog` directly before starting its `Job`), so no `runInUIThread` wrapper is needed. Placing the guard here fires the dialog exactly once regardless of how many nodes are selected, and covers all four combinations (single/multi-node × with/without `GENERATES_OUTPUT`) — including the multi-node output path that goes through `executeOnMultipleNodes()`/`MultiNodeCommandExecutor` rather than `executeLocalCommand()`. Add imports for `SandboxHelper` and `HostCommandBlockedDialog` (`Registry` and `ObjectTool` already imported).

## What Goes Where
- **Implementation Steps** (`[ ]`): all code changes + build gate + doc updates in this repo.
- **Post-Completion** (no checkboxes): manual runtime verification inside/outside a restricted Flatpak sandbox.

## Implementation Steps

### Task 1: Add public accessors to SandboxHelper and de-duplicate the exception message

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/tools/SandboxHelper.java`

- [x] Add `public static boolean isHostCommandExecutionBlocked()` returning `flatpak && !hostAccessAvailable`.
- [x] Add `public static String getHostAccessOverrideCommand()` building `flatpak override --user --talk-name=org.freedesktop.Flatpak <appId>`, reusing the existing null/empty `applicationId` → `<application-id>` fallback.
- [x] Update the `DOCUMENTATION_URL` constant to `https://go.netxms.com/flatpak-sandbox`.
- [x] Add `public static String getDocumentationUrl()` returning `DOCUMENTATION_URL`.
- [x] Refactor `buildHostShellCommand()`'s `IOException` message to interpolate `getHostAccessOverrideCommand()` and `getDocumentationUrl()`; keep the `throw` intact as a defensive fallback.
- [x] Build gate — desktop profile compiles.
- [x] Build gate — web profile compiles.

### Task 2: Create HostCommandBlockedDialog

**Files:**
- Create: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/dialogs/HostCommandBlockedDialog.java`

- [x] Create the `dialogs` package and the `HostCommandBlockedDialog extends Dialog` class with `Shell` constructor.
- [x] Implement `configureShell` (title) and `createDialogArea` (warning icon, bold header, WRAP prose label, read-only selectable monospace command `Text`, "Copy" button, clickable `Link` to docs).
- [x] Wire "Copy" → `WidgetHelper.copyToClipboard(...)` and `Link` selection → `ExternalWebBrowser.open(...)`.
- [x] Implement `createButtonsForButtonBar` with a single Close/OK button. Uses `JFaceResources.getTextFont()` (monospace) and `getBannerFont()` (bold header) — shared managed fonts, so no manual `new Font(...)`/disposal is needed (this is the standard nxmc font-lifecycle practice; deviates from the plan's `new Font(...)` suggestion but is cleaner and toolkit-safe).
- [x] Ensure all referenced classes are imported (no fully-qualified inline names); all user-facing strings via module `I18n`. Close button uses `i18n.tr("Close")` rather than `IDialogConstants.CLOSE_LABEL` (the latter is a non-static instance field under RWT/RAP and fails to compile in shared code).
- [x] Build gate — desktop profile compiles.
- [x] Build gate — web profile compiles.

### Task 3: Add proactive guard clause in ObjectToolExecutor.execute

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/ObjectToolExecutor.java`

- [x] Add the guard at the very top of `execute()` (line 122), gated on `tool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND && SandboxHelper.isHostCommandExecutionBlocked()`, opening `HostCommandBlockedDialog` and returning before `readInputFields()` / the `Job`.
- [x] Add imports for `SandboxHelper` and `HostCommandBlockedDialog` (confirm `Registry` and `ObjectTool` already imported).
- [x] Confirm no `runInUIThread` wrapper is added (`execute()` is already on the UI thread).
- [x] Verify the guard fires once for a multi-node selection (not once per node) and precedes both the `executeOnMultipleNodes` and `executeOnNode` branches.
- [x] Build gate — desktop profile compiles.
- [x] Build gate — web profile compiles.

### Task 4: Verify acceptance criteria
- [x] Verify all four combinations route through the guard when blocked (no view/console opens): single-node output, single-node no-output, **multi-node with `GENERATES_OUTPUT`** (must not open `MultiNodeCommandExecutor`), multi-node without `GENERATES_OUTPUT`. (Code-inspection: guard at `ObjectToolExecutor.execute():126` returns early, before the background `Job` that branches to `executeOnMultipleNodes():337` / `executeOnNode():346,350`, so all four combinations are bypassed for the blocked condition.)
- [x] Verify a multi-node selection shows the dialog exactly **once**, not once per node. (Code-inspection: guard sits at the top of `execute()`, outside the per-node loop; it opens the dialog and returns once regardless of node count.)
- [x] Verify the old `"Failed (…)"` label (`ExecutorListLabelProvider`) and "Cannot start external process" bubble no longer appear for the restricted-sandbox condition. (Code-inspection: the early return prevents reaching `LocalCommandExecutor`/`executeLocalCommand`, so the `IOException`→`failureReason` path is never entered when blocked.)
- [x] Verify no behavior change on Windows / non-Flatpak (guard is `false`; direct execution path unchanged). (Code-inspection: `isHostCommandExecutionBlocked()` returns `flatpak && !hostAccessAvailable` = `false` off Flatpak, so the guard is a no-op and execution proceeds unchanged.)
- [x] Confirm the `IOException` fallback in `buildHostShellCommand()` still exists and its message renders the override command once. (Code-inspection: `SandboxHelper.buildHostShellCommand():215-223` still throws, interpolating `getHostAccessOverrideCommand()`/`getDocumentationUrl()`.)
- [x] Run full build gate one last time: desktop `-Pdesktop -Pstandalone` and web `-Pweb`. (Both profiles built cleanly offline after installing netxms-base and netxms-client.)

### Task 5: [Final] Documentation and plan closeout
- [x] Add a debug tag / doc note only if one was introduced (none expected — skip if N/A). N/A: pure client-side dialog change, no debug tag or doc note introduced.
- [x] Update `docs/plans/completed/20260706-flathub-sandbox-detection.md` follow-up note or cross-reference if helpful. Added a follow-up cross-reference block to the predecessor plan's Overview.
- [x] Move this plan to `docs/plans/completed/`.

## Post-Completion
*Items requiring manual intervention or external systems — informational only.*

**Redirect verification (do before/with merge):**
- Confirm `https://go.netxms.com/flatpak-sandbox` resolves to the intended Flatpak-sandbox documentation (it replaces the predecessor's `https://go.netxms.org/flatpak`). The redirect is owned by the project; ensure it is configured before shipping so the dialog's link does not 404.

**Manual verification:**
- In a **restricted** Flatpak sandbox (Flathub build without `--talk-name=org.freedesktop.Flatpak`): run a local-command object tool of each kind — with and without "generates output", and against **both a single node and multiple nodes** — confirm the modal dialog appears (once, even for multi-node), the command text is selectable, the "Copy" button copies the exact `flatpak override …` line, and the documentation link opens in a browser.
- After running the shown `flatpak override …` command and restarting: confirm local command tools execute normally (dialog no longer appears).
- On a **non-Flatpak** desktop and on **Windows**: confirm local command tools behave exactly as before (no dialog, no regression).
- Web (RWT) build: confirm the module compiles and loads; the dialog is desktop-only in practice but must not break the web client.
