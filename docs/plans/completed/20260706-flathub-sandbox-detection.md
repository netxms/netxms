# Flathub Restricted Sandbox Detection in nxmc

## Overview

Commit a146aecbcbc added Flatpak support: local command and SSH-in-terminal object tools
are routed to the host via `flatpak-spawn --host`, which requires the
`--talk-name=org.freedesktop.Flatpak` permission in the Flatpak manifest. Flathub rejects
manifests with that permission, so the Flathub build ships without it and `flatpak-spawn`
calls are denied by the portal — tools would silently fail.

Distribution is Flathub-only: the build ships restricted, and users who need host command
execution grant the permission themselves via `flatpak override`. nxmc must detect the
restricted case at runtime and, when the user actually tries to run a local command tool,
show an actionable error: the exact `flatpak override` command to unlock the feature plus
a link to detailed documentation.

Key benefit of the chosen detection method: `/.flatpak-info` reflects *effective*
permissions, so a Flathub user who runs the suggested `flatpak override` command gets
working tools after restart with no code awareness of the override.

## Context (from discovery)

- Files involved: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/tools/SandboxHelper.java` (only code change)
- Call sites (already funnel through `SandboxHelper.buildHostShellCommand()`, no changes needed):
  - `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/ObjectToolExecutor.java:696` — inside `Job.run()` which `throws Exception`; job failure shows `getErrorMessage()` + exception message
  - `src/client/nxmc/java/src/swt/java/org/netxms/nxmc/modules/objecttools/widgets/LocalCommandExecutor.java:135` — exec context already handles `IOException` from `Runtime.exec()`
  - `src/client/nxmc/java/src/swt/java/org/netxms/nxmc/modules/objecttools/views/LocalCommandResults.java:213` — same
- Patterns: i18n via `LocalizationHelper.getI18n(Class)` / `i18n.tr(...)`; 3-space indent; nxmc Java style
- `/.flatpak-info` is an INI-style keyfile present at sandbox root, format stable and documented flatpak behavior

## Development Approach

- **testing approach**: Manual verification (nxmc module has no unit test infrastructure —
  no JUnit dependency, no `src/test` tree; introducing one for a single parser is out of scope,
  agreed during planning)
- complete each task fully before moving to the next
- make small, focused changes
- structure the parser so its logic is easy to inspect and verify by hand
- build must pass (`mvn ... clean package`) before a task is considered done
- **CRITICAL: update this plan file when scope changes during implementation**
- maintain backward compatibility: behavior on non-Flatpak platforms and in a sandbox
  with host access granted must be exactly as today

## Testing Strategy

- **unit tests**: none (see Development Approach — module has no test infra; user opted for manual verification)
- **manual verification**: build the standalone jar and exercise all three environment
  variants (see Task 3): no sandbox, restricted Flatpak, restricted Flatpak with
  `flatpak override` applied

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope

## Solution Overview

All detection and error logic lives in `SandboxHelper`; the three call sites stay untouched.

1. **Detection** (static init): if `/.flatpak-info` exists, parse it once —
   `[Session Bus Policy]` → `org.freedesktop.Flatpak=talk` (or `own`) means host access is
   available; `[Application]` → `name=` gives the Flatpak app ID for the override command.
2. **Error surfacing**: `buildHostShellCommand()` gains `throws IOException` and throws a
   localized, actionable message when running sandboxed without host access. `IOException`
   is what `Runtime.exec()` already throws, so every existing caller handles it through its
   existing error path.
3. **Fail closed**: any parse problem (unreadable file, missing section/key) in a sandbox
   counts as "no host access" — a false negative is recoverable via the error message's
   instructions, while a false positive reproduces the silent-failure bug this fixes.

## Technical Details

- `/.flatpak-info` example content:

  ```ini
  [Application]
  name=org.netxms.Nxmc
  runtime=runtime/org.freedesktop.Platform/x86_64/23.08

  [Session Bus Policy]
  org.freedesktop.Flatpak=talk
  ```

- Parser: manual line loop (~15 lines) — trim lines, skip blanks/comments, track current
  `[section]` header, split `key=value` on first `=`. Extract exactly two values:
  `[Application] name` and `[Session Bus Policy] org.freedesktop.Flatpak`.
  Policy values `talk` and `own` grant access; `see`, `none`, or absence do not.
- New static state in `SandboxHelper` (alongside existing `flatpak` boolean):
  - `hostAccessAvailable` — `true` when not sandboxed, or sandboxed with the talk/own policy
  - `applicationId` — from `name=`; `null`/absent → use `<application-id>` placeholder in the message
- Error message (via `LocalizationHelper.getI18n(SandboxHelper.class)`, obtained lazily in
  the throw path, NOT in static init). `I18n.tr()` substitutes via `MessageFormat`, so use
  positional `{0}`/`{1}` placeholders — named placeholders would appear literally:

  > Cannot execute command on host machine: this Flatpak build runs in a restricted sandbox.
  > To enable host command execution, run:
  > flatpak override --user --talk-name=org.freedesktop.Flatpak {0}
  > and restart the application. See {1} for details.

  passed as `i18n.tr("...", applicationId, DOCUMENTATION_URL)`

- Documentation short URL constant in `SandboxHelper`: `https://go.netxms.org/flatpak`
  (redirect target maintained outside the client; see Post-Completion)

## What Goes Where

- **Implementation Steps** (`[ ]` checkboxes): code changes in this repo, build verification
- **Post-Completion** (no checkboxes): Flathub manifest publishing, redirect URL setup,
  documentation page, live Flatpak testing on a Linux host

## Implementation Steps

### Task 1: Parse /.flatpak-info in SandboxHelper static initialization

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/tools/SandboxHelper.java`

- [x] replace single-boolean static init with one-time parse: keep `flatpak` (file exists), add `hostAccessAvailable` and `applicationId` statics
- [x] implement keyfile parse loop in a package-private static method taking a `File` (static init passes `new File("/.flatpak-info")`; allows fixture-based manual verification without source edits): section tracking, `key=value` split on first `=`, extract `[Application] name` and `[Session Bus Policy] org.freedesktop.Flatpak`
- [x] treat policy value `talk` or `own` as host access available; anything else (including missing section/key) as unavailable; not sandboxed → available
- [x] fail closed on `IOException`/`SecurityException` during read: sandboxed without confirmed permission → `hostAccessAvailable = false`
- [x] verify by inspection against the documented `/.flatpak-info` format (with permission, without permission, file absent)
- [x] build: `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone` — must pass before task 2

### Task 2: Throw actionable IOException from buildHostShellCommand()

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/tools/SandboxHelper.java`

- [x] add `throws IOException` to `buildHostShellCommand()`; throw when `flatpak && !hostAccessAvailable`
- [x] compose localized message via `LocalizationHelper.getI18n(SandboxHelper.class)` (lazily, in the throw path): restricted-sandbox explanation + `flatpak override --user --talk-name=org.freedesktop.Flatpak <appId>` + doc link
- [x] add doc URL constant `https://go.netxms.org/flatpak`; use `<application-id>` placeholder when app ID could not be read
- [x] update `buildHostShellCommand()` javadoc: describe the restricted-Flathub behavior and the thrown exception
- [x] confirm all three call sites compile unchanged and surface the exception through their existing error paths (Job error message / exec IOException handling)
- [x] build: `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone` — must pass before task 3

### Task 3: Manual verification of all environment variants

- [x] no `/.flatpak-info` (regular desktop run): local command object tool executes exactly as before — verified by inspection (static init sets `hostAccessAvailable=true`, no throw) and fixture harness (absent file → fail-closed path unused)
- [x] simulated restricted sandbox (call the package-private parse method with a fixture file): `parseFlatpakInfo` returns `hostAccessAvailable=false` and correct app ID (`org.netxms.Nxmc`); `buildHostShellCommand()` throw path composes the error with app ID, override command, and doc URL
- [x] simulated unrestricted sandbox (fixture with `org.freedesktop.Flatpak=talk`): `parseFlatpakInfo` reports host access available (also verified `own`); in real Flatpak the command is routed via `flatpak-spawn --host`, no error
- [x] error rendering checked in both paths: Job message area (`ObjectToolExecutor`) and output view (`LocalCommandResults`) — manual UI test (skipped: requires running SWT client on a Flatpak host, not automatable; exception surfaces through existing `IOException` handling)
- [x] remove any temporary test scaffolding; final build passes (`mvn ... clean package -Pdesktop -Pstandalone` → `nxmc-7.0-SNAPSHOT-standalone.jar`; fixture harness lived only under `/tmp`, removed)

### Task 4: Verify acceptance criteria

- [x] no build-time flags or build variants: single Flathub build, behavior driven entirely by effective sandbox permissions — verified by inspection (runtime `/.flatpak-info` parse, no Maven profile or flag gates behavior)
- [x] `flatpak override` route works conceptually: detection reads effective policy, so override + restart flips behavior without code involvement — verified (`parseFlatpakInfo` reads `[Session Bus Policy] org.freedesktop.Flatpak`, which reflects effective post-override permissions)
- [x] edge cases handled: unreadable file, missing section, missing key, missing app ID — verified in `parseFlatpakInfo` (catch `IOException`/`SecurityException` → fail closed; null section / unmatched key → `hostAccess` stays false; null app ID → `<application-id>` placeholder)
- [x] full client build passes: `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone` — BUILD SUCCESS, `nxmc-7.0-SNAPSHOT-standalone.jar` produced

### Task 5: [Final] Update documentation

- [x] verify javadoc in `SandboxHelper` describes the restricted-by-default Flathub sandbox, the override path, and the fail-closed rule — verified: `parseFlatpakInfo` javadoc documents fail-closed on missing section/key/read error; `buildHostShellCommand` javadoc documents restricted Flathub sandbox and `flatpak override` path
- [x] update CLAUDE.md only if a new reusable pattern emerged (likely not — scope is one class) — no update needed; scope is a single class, no reusable pattern
- [x] move this plan to `docs/plans/completed/`

## Post-Completion

*Items requiring manual intervention or external systems — informational only*

**External system updates:**
- set up the `https://go.netxms.org/flatpak` redirect before the release that ships this change
- write the documentation page it points to: the `flatpak override` command with explanation, why the Flathub build is restricted by default, and the security implications of granting host access
- publish the Flathub manifest (without `--talk-name=org.freedesktop.Flatpak`) once this change is in a released build

**Manual verification:**
- end-to-end test on a real Linux host with the Flathub-style manifest, including the `flatpak override --user --talk-name=org.freedesktop.Flatpak` + restart path
