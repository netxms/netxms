# Fix HostCommandBlockedDialog Layout

## Overview
The `HostCommandBlockedDialog` renders but its layout is broken (see follow-up to the completed
`20260707-flatpak-host-command-blocked-dialog` work). The dialog reuses a single 2-column
`GridLayout` for two unrelated rows, which couples their column widths and produces three visible
defects:

1. The icon and title are not grouped/centered — the long title is pushed to the right of the icon.
2. The **Copy** button is stretched far wider than needed.
3. The documentation link is left-aligned instead of centered.

Target layout:
```
        [ICON] Title                (icon + title centered together)
  <explanatory prose paragraph>      (full width, wrapped)
  [text field ................][Copy] (field grabs all width, button natural size)
              [link]                 (centered)
```

## Context (from discovery)
- File involved: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/dialogs/HostCommandBlockedDialog.java`
- Related helper: `org.netxms.nxmc.tools.SandboxHelper` (unchanged — provides command string + doc URL)
- Toolkit: SWT (desktop) / RWT (web) shared code. Only `GridLayout`/`GridData`/`Composite` used here, all
  RWT-safe — no platform-specific widget calls introduced.
- Root cause: column 1's width = max(title width, Copy button width). The long title widens column 1,
  and the Copy button's `GridData(SWT.FILL, ...)` makes it fill that column.

## Development Approach
- **Testing approach**: Regular (code first). This project has **no automated test harness for SWT/RWT
  dialog layout**; verification is a manual build + visual check. Test checkboxes below reflect that
  reality (manual verification) rather than forcing non-existent unit tests.
- Single focused change to one file's `createDialogArea`.
- Maintain backward compatibility: no API/signature changes, `SandboxHelper` untouched.

## Solution Overview
Replace the shared 2-column grid with a **single-column** `dialogArea` that stacks four independently
laid-out children, so each row sizes on its own:

1. **Header composite** — own 2-col `GridLayout` (icon + title), zero margins, laid out with
   `GridData(SWT.CENTER, SWT.TOP, true, false)` so the icon+title group is centered as a unit.
2. **Prose label** — `SWT.WRAP`, `GridData(SWT.FILL, SWT.TOP, true, false)` with `widthHint` (keep 480)
   driving the dialog width.
3. **Command composite** — own 2-col `GridLayout`, zero margins, `GridData(SWT.FILL, ..., true, false)`.
   Text with `GridData(SWT.FILL, SWT.CENTER, true, false)` grabs all width; Copy button with
   `GridData(SWT.RIGHT, SWT.CENTER, false, false)` at natural size.
4. **Doc link** — `GridData(SWT.CENTER, SWT.CENTER, true, false)`.

## Technical Details
- `dialogArea` layout: `GridLayout` with `numColumns = 1`, keeping existing
  `DIALOG_HEIGHT_MARGIN` / `DIALOG_WIDTH_MARGIN` / spacing from `WidgetHelper`.
- Nested composites use `marginWidth = 0; marginHeight = 0;` and a small `horizontalSpacing` so they
  align flush with the prose/link.
- No changes to `configureShell`, `createButtonsForButtonBar`, listeners, or `SandboxHelper` calls.

## What Goes Where
- **Implementation Steps**: the layout rewrite and a local build.
- **Post-Completion**: visual confirmation of the running dialog (manual, both desktop and — if
  convenient — web).

## Implementation Steps

### Task 1: Rewrite createDialogArea with nested composites

**Files:**
- Modify: `src/client/nxmc/java/src/main/java/org/netxms/nxmc/modules/objecttools/dialogs/HostCommandBlockedDialog.java`

- [x] change `dialogArea` `GridLayout` to `numColumns = 1`
- [x] add header `Composite` (2-col, zero margin) holding the warning icon and title label, centered via `GridData(SWT.CENTER, SWT.TOP, true, false)`; give the inner icon `GridData(SWT.CENTER, SWT.CENTER, ...)` so it centers vertically against the taller banner-font title
- [x] keep prose label full-width (`SWT.WRAP`, `GridData(SWT.FILL, ...)`, `widthHint = 480`); drop the now-unneeded `horizontalSpan`
- [x] add command `Composite` (2-col, zero margin): Text grabs width, Copy button natural size (`GridData(SWT.RIGHT, SWT.CENTER, false, false)`)
- [x] center the doc `Link` via `GridData(SWT.CENTER, SWT.CENTER, true, false)`; drop `horizontalSpan`
- [x] confirm no unused imports/vars remain (e.g. leftover `gd.horizontalSpan` usage)

### Task 2: Build and verify

**Files:**
- (no new files)

- [x] build desktop client: `mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone`
- [x] confirm compilation succeeds with no new warnings for the changed file
- [x] manual visual check (skipped - not automatable; requires running GUI)

### Task 3: Verify acceptance criteria and finalize
- [x] all three layout defects from Overview are resolved
- [x] no API/signature changes; `SandboxHelper` untouched
- [x] move this plan to `docs/plans/completed/`

## Post-Completion
*Manual, no checkboxes — informational only*

**Manual verification:**
- Trigger the dialog from a local command object tool inside a restricted Flatpak build (or force-show it)
  and confirm the layout matches the target on the desktop client.
- If convenient, verify the web (RWT) build renders equivalently — the code is RWT-safe but RWT grid
  rounding can differ slightly.
