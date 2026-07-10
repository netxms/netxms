# Branding flag for client download link in protocol version mismatch dialog

## Overview

`ProtocolVersionMismatchDialog` (added in commit `1b79172498`) unconditionally renders a link to
`https://netxms.org/download/releases/<major.minor>/` whenever the server-reported version can be parsed.
Branded / OEM builds should not point their users at netxms.org.

This adds a `BrandingProvider` boolean flag, `isClientDownloadLinkEnabled()`, that lets a branding provider
suppress the link. When suppressed the dialog falls through to the prose-only branch that already exists for
the unparseable-version case, so the dialog remains coherent with no new UI and no new translatable strings.

## Context (from discovery)

- Files/components involved:
  - `src/client/nxmc/java/src/main/java/org/netxms/nxmc/BrandingProvider.java`
  - `src/client/nxmc/java/src/main/java/org/netxms/nxmc/BrandingManager.java`
  - `src/client/nxmc/java/src/main/java/org/netxms/nxmc/base/dialogs/ProtocolVersionMismatchDialog.java`
- Related patterns found:
  - Existing flags `isExtendedHelpMenuEnabled()` and `isWelcomePageEnabled()` return `Boolean` — `null` means
    "abstain, let another provider decide". `BrandingManager` iterates `providers`, first non-null wins,
    with a hardcoded default when all abstain.
  - Newer provider methods (`getPerspectiveSwitcherBackground()` etc.) are declared `default` returning `null`,
    so adding them did not break existing implementations. The new flag follows these, not the older abstract ones.
  - `ProtocolVersionMismatchDialog.createDialogArea()` already has an `if (releaseBranch != null) { link } else { prose }`
    structure. The "off" state is the existing `else` branch.
- Dependencies identified:
  - No `BrandingProvider` implementations exist in-tree — they ship in OEM/plugin JARs loaded via `ServiceLoader`.
    Therefore the flag must be a `default` method or those external providers will fail to compile.
  - `BrandingManager.providers` is a static list populated at class init; it is not injectable.

## Development Approach

- **testing approach**: Regular (code first), verified by compile + manual run.
  The nxmc module has **no test source tree and no unit tests**. Adding one would require introducing a
  `src/test` tree, a JUnit dependency in the nxmc pom, and refactoring `BrandingManager.providers` to be
  injectable — all well outside the scope of a three-line behaviour flag. Verification is therefore:
  desktop and web builds compile, and the default (no branding provider) still shows the link.
- complete each task fully before moving to the next
- make small, focused changes
- maintain backward compatibility: external `BrandingProvider` implementations must keep compiling unchanged

## Testing Strategy

- **unit tests**: not applicable — nxmc has no test infrastructure (see Development Approach). Do not add a
  test tree as part of this change.
- **e2e tests**: none in this project.
- **verification**: build both the desktop (`-Pdesktop`) and web (`-Pweb`) profiles, since
  `ProtocolVersionMismatchDialog` is shared code reached from both `Startup` paths. Confirm the link renders by
  default. `ProtocolVersionExceptionTest` in netxms-client is unaffected and must continue to pass.

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope

## Solution Overview

Mirror the established branding-flag idiom exactly:

1. `BrandingProvider` gains `default Boolean isClientDownloadLinkEnabled() { return null; }`.
2. `BrandingManager` gains `public static boolean isClientDownloadLinkEnabled()` — first non-null provider
   answer wins, default `true`.
3. `ProtocolVersionMismatchDialog` guards the link branch on both conditions:
   `if ((releaseBranch != null) && BrandingManager.isClientDownloadLinkEnabled())`.

Key design decisions:

- **`default`, not abstract** — external providers compiled against the current interface keep working, and a
  provider that does not care about this flag needs no code.
- **Reuse the existing `else` branch** rather than adding a new "link suppressed" UI state. The branch needed one
  addition: it originally carried a single generic sentence, because it only ever handled the unparseable-version
  case where no download instruction applies on either platform. Routing the branding-disabled case through it
  dropped the web-only sentence "The deployed web client does not match the server.", which lives solely in the
  link branch's `Registry.IS_WEB_CLIENT` ternary. The `else` branch now carries the same ternary, adding one new
  translatable string.
- **Boolean, not a URL override.** A `getClientDownloadURL()` would need the release branch passed in
  (the URL is version-derived), producing an awkward signature for a need no OEM has stated. Rejected under YAGNI.

## Technical Details

`BrandingProvider.java` — append after `isWelcomePageEnabled()`:

```java
   /**
    * Control if link to client download page is shown in protocol version mismatch dialog.
    *
    * @return true if download link is enabled, false if disabled, or null to leave decision to other providers
    */
   default Boolean isClientDownloadLinkEnabled()
   {
      return null;
   }
```

`BrandingManager.java` — append after `isWelcomePageEnabled()`:

```java
   /**
    * Control if link to client download page is shown in protocol version mismatch dialog.
    *
    * @return true if download link is enabled
    */
   public static boolean isClientDownloadLinkEnabled()
   {
      for(BrandingProvider p : providers)
      {
         Boolean enabled = p.isClientDownloadLinkEnabled();
         if (enabled != null)
            return enabled;
      }
      return true;
   }
```

`ProtocolVersionMismatchDialog.java` — single condition change in `createDialogArea()`:

```java
      String releaseBranch = getReleaseBranch(serverVersion);
      if ((releaseBranch != null) && BrandingManager.isClientDownloadLinkEnabled())
```

plus `import org.netxms.nxmc.BrandingManager;` (project rule: no inline fully-qualified names).

Processing flow when the flag is `false`: `getReleaseBranch()` is still evaluated (harmless), the guard fails,
control enters the existing `else`, which creates the wrapped 480px-wide prose `Label`. `getReleaseUrl()` and the
`Link` widget are never constructed. No unused-method warning — `getReleaseUrl()` is still called from the taken branch.

## What Goes Where

- **Implementation Steps**: the three source edits plus build verification.
- **Post-Completion**: manual dialog inspection, and notifying OEM branding-provider maintainers that the new
  opt-out exists (their code needs no change to keep current behaviour).

## Implementation Steps

### Task 1: Add isClientDownloadLinkEnabled() to BrandingProvider

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/BrandingProvider.java`

- [x] append `default Boolean isClientDownloadLinkEnabled()` returning `null`, after `isWelcomePageEnabled()`
- [x] add javadoc matching the wording style of the neighbouring flag methods
- [x] confirm it is declared `default` (not abstract) so external providers still compile
- [x] no tests — nxmc has no test tree (see Testing Strategy)

### Task 2: Add resolver to BrandingManager

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/BrandingManager.java`

- [x] append `public static boolean isClientDownloadLinkEnabled()` after `isWelcomePageEnabled()`
- [x] iterate `providers`, return first non-null answer, default `true`
- [x] add javadoc matching the neighbouring resolvers
- [x] no tests — `providers` is a static ServiceLoader list, not injectable (see Testing Strategy)

### Task 3: Gate the link in ProtocolVersionMismatchDialog

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/base/dialogs/ProtocolVersionMismatchDialog.java`

- [x] add `import org.netxms.nxmc.BrandingManager;`
- [x] change the guard to `if ((releaseBranch != null) && BrandingManager.isClientDownloadLinkEnabled())`
- [x] leave the `else` prose branch, `getReleaseBranch()`, and `getReleaseUrl()` untouched

### Task 4: Verify acceptance criteria
- [x] verify default behaviour unchanged: with no branding provider the link renders as before
- [x] verify the suppressed path is the existing prose branch (read the `else` block, confirm width hint 480 applies)
- [x] build desktop client: `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone`
- [x] build web client: `mvn -f src/client/nxmc/java/pom.xml clean package -Pweb` (shared dialog code must compile for RWT)
- [x] run netxms-client tests: `mvn -f src/client/java/netxms-client/pom.xml test` — `ProtocolVersionExceptionTest` must still pass
- [x] grep for `isClientDownloadLinkEnabled` project-wide to confirm interface, resolver, and call site all line up

### Task 5: [Final] Update documentation
- [x] no README/CLAUDE.md change expected — this follows an existing documented pattern, adds no new one
- [x] move this plan to `docs/plans/completed/` (`mkdir -p docs/plans/completed` first)

## Post-Completion

*Items requiring manual intervention or external systems — no checkboxes, informational only*

**Manual verification:**
- Point a current-version client at a server with a different major/minor version and confirm the dialog shows
  the link. Temporarily returning `false` from a stub provider is the only way to see the suppressed state,
  since no in-tree provider exists.

**External system updates:**
- OEM / branded builds that ship a `BrandingProvider` need no change to preserve current behaviour (the `default`
  returns `null` → link stays enabled). Those wanting the link hidden override `isClientDownloadLinkEnabled()`
  to return `false`. Worth a note to whoever maintains the branding plugins.
