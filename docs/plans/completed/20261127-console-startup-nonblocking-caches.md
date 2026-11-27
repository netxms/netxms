# Console startup without blocking cache requests (issue #4072)

## Overview

With `Connect.BackgroundObjectSync` enabled, the desktop and web console still freeze after the login dialog
closes, until the background object sync has finished. `Startup` builds `ObjectToolsCache`, `SummaryTablesCache`
and `GraphTemplateCache` on the UI thread. `ObjectToolsCache.reload()` downloads every tool icon synchronously
(`ImageProvider.preloadImageFromServer` → `NXCSession.getImage`), and `getObjectTools`, `listDciSummaryTables` and
`getPredefinedGraphs` are blocking round trips as well. During a background sync each request waits behind object
data already queued on the single connection.

Measured with 7.0.5.8 against a large installation over a slow reverse-SSH link: the main thread spent
1.6 + 3.1 + 7.4 s in `getImage()` for three tool icons. The window appeared 0.3 s after "Background object
synchronization completed". This is a regression from 5da3153e2a (#3241), which moved tool icons from embedded
blobs to image library references.

Goal: the UI thread makes none of these four kinds of request (tool icons, tool list, summary table list, graph
template list) after the background sync starts, every cache is filled when the window opens (saved views and
dashboards keep working), and tool icons arrive in the background.

## Decisions and rationale

- **The three lists are fetched in `LoginJob`, just before `startBackgroundObjectSync`.** `SummaryTablesCache` and
  `GraphTemplateCache` attach first and `ObjectToolsCache` last, so icon requests start only after the last
  synchronous login request and cannot get ahead of any of them. Before the sync starts, each list request is a
  single plain round trip. This applies in normal sync mode too; there it only moves the same calls into the login
  dialog.
  - Ruled out: making the caches load asynchronously with readiness notification. That would change what callers
    can assume after login: `ObjectToolResultView` and `MultiNodeCommandExecutor` restore by tool id and fail with
    "Invalid tool id" on an empty cache, summary table dialogs take a snapshot of the list, and the header menus
    would omit entries. That is a separate, larger change.
  - Ruled out: fixing only the icons. The three list requests would stay on the UI thread and still wait behind the
    sync.
- **A cache failure never fails the login.** `LoginJob` disconnects and rethrows on any exception, and `syncExec`
  passes exceptions from the runnable back to the caller. The fetch and the install therefore keep the current
  catch-and-log behaviour, including per-tool handling while icons are built. A failed fetch leaves an empty cache,
  as today.
- **`ObjectToolsCache` fetches off the UI thread and installs on the UI thread.** Installing means replacing the
  tool map, building icons and registering listeners. On RAP 4.5, `ModalContext` runs the login job inside
  `UISession.exec`: it has UI-session context, but `Display.getCurrent()` is null. The tool-change notification
  thread has no context at all. Image creation therefore stays on the UI thread. Installing runs through
  `display.syncExec` from `LoginJob` (already used there for the 2FA dialogs), and directly when already on the
  owning display thread (RWT pop-outs). Cache locks are never held across `syncExec`.
  - **Tool-change reload:** it keeps one raw thread per notification for the fetch and installs through
    `asyncExec`, skipping a disposed display. Nothing waits on that thread. If notifications arrive close together
    (tool import), an older fetch can be installed after a newer one; the last install wins. That race exists
    today and is accepted.
  - **Other caches:** `SummaryTablesCache` and `GraphTemplateCache` only fetch, build a model and register through
    `Registry.setSingleton(display, …)`, so they are safe on the login thread as they are (`GeoLocationCache`
    already attaches on that thread via `MapTilesHousekeeper`).
- **Icons load asynchronously behind a transparent placeholder** (`SharedIcons.EMPTY`, the user's choice over
  `missing.png` and over showing no image).
  - The install uses `ImageProvider.getNativeImage(guid)`, which returns the cached image or starts the existing
    async load. An icon that is already loaded gets the cache-owned 16×16 copy as today. A pending icon gets the
    placeholder descriptor, and no `Image` is created for it (today `getIconImage` would copy `missing.png` and
    keep it under the GUID).
  - When the icon arrives, `imageUpdated` → `onLibraryImageChange` replaces the descriptor. Load completion and
    install both run on the owning UI thread, so the loaded/pending decision cannot race with a completion.
  - The `ImageProvider` listener is registered during the first install, on the UI thread, before any load starts:
    `updateListeners` is a plain `HashSet` iterated in a UI runnable. `ObjectToolsCache.dispose()` removes it.
- **Live refresh covers only the surfaces that keep a tool icon.** `findIcon` has two callers: `ObjectMenuFactory`
  (tool menus) and the overview Commands element. Context menus (object tree, alarm list, summary table widget) use
  `setRemoveAllWhenShown` and pick up the new icon the next time they open.
  - **Header Tools menu:** it is built once per selection and reused on every click. It refreshes tool item images
    from the cache on `SWT.Show`, using the menu-owned `ImageCache`. The listener is added only when the menu was
    built with at least one placeholder, and it removes itself once nothing is pending.
    ⚠️ The plan originally justified this with a per-opening RAP round trip. revmux round 01 showed that claim is
    false: RAP 4.5 subscribes every non-bar menu to Show anyway, and `setVisible` dispatches Show inline. The
    listener is kept for its scope (only menus with placeholders), not for request cost.
    `ObjectsPerspective.updateObjectMenuBar` must not be used for this. It disposes every header
    `ToolItem`, and their dispose hooks dispose the attached menus, so an arriving icon would close whatever header
    menu the user has open.
  - **Commands element:** the listener's lifetime follows the `commandBox` widget, not the element.
    `ObjectOverviewView.updateLayout` calls `element.dispose()` when the selected object does not apply and reuses
    the same element instance later.
  - **Notification:** `ObjectToolsCache` notifies its own listeners after `onLibraryImageChange` has committed the
    descriptor and released the lock. The listener type is the existing `ImageUpdateListener`. Consumers do not
    listen to `ImageProvider` directly, because the order of its `HashSet` listeners relative to the cache's own
    listener is undefined.
- **What happens when an icon cannot be loaded:**
  - **Stale reference at login** (GUID not in the library index, failed fetch, or failed metadata sync): the
    placeholder stays. The old blocking preload could still fetch a GUID the index lacked; that fallback is dropped
    deliberately, since it only matters when the image library sync is already broken.
  - **Image deleted during the session:** the tool ends up with no image (existing `onLibraryImageChange`
    behaviour, unchanged).
  - **Inherited race, recorded and not fixed here:** a load still in flight when the image is deleted or updated
    can put the old bytes back into `ImageProvider`'s caches when it completes (`deleteImage` and `updateImage`
    run outside the completion runnable). It already affects every async image consumer; this change only routes
    tool icons through that path.
- **Fold-in:** `onObjectToolDelete` clears the icons of all tools (`icons.clear()`); the server sends only
  `OBJTOOL_DELETED` on delete, so the other tools lose their icons until the next change. With async icons they
  would not come back until an image changes. Task 1 removes only the deleted tool's icon.
- **Out of scope:**
  - `PreferenceStore.attachSession` → `getUserVariables()` stays a blocking UI-thread call after the sync starts
    (0.2 s in the measured run). The user accepted this residual wait.
  - `WelcomePage.SeenVersions` read on first activation of the main window.
  - RWT pop-outs keep their synchronous image library metadata sync and tool list fetch.
  - `LoginJob`'s blocking preload of object category icons.
  - Header menu refresh after an object tool is edited during a session (unchanged: next selection).
  - Batching of icon refreshes (only a handful happen per session).
  - The per-opening `ImageCache` leak in tool context menus, filed as #4073.

## Discovery notes

- `ImageProvider.createInstance` (in `LoginJob`) runs `syncMetaData()` synchronously, before the object sync
  starts, so the library index is ready for async icon loads.
- `ObjectToolsCache` registers its `ImageProvider` listener through the display-less `ImageProvider.getInstance()`
  and never unregisters it.
- RWT: `ObjectToolsCache` is a per-display singleton (`setDisplaySingleton`). `SummaryTablesCache` and
  `GraphTemplateCache` are per session (`setSingleton`, shared with pop-outs through `Registry.copyToDisplay`).
  Every non-pop-out RWT path, kiosk and cookie auto-login on page reload included, goes through `doLogin` /
  `LoginJob`. Pop-out windows do not, and they need their own `ObjectToolsCache`. Nothing reads the three caches
  between `LoginJob` completion and `openWindows`.

## Tandem

| Task | Writer | Reviewer |
|---|---|---|
| 1 | claude | codex |
| 2 | codex | claude |
| 3 | claude | codex |

## Implementation Steps

### Task 1: ObjectToolsCache fetches off the UI thread and loads icons in the background

Layer: console UI (shared `main` sources).

- [x] `reload()` is split into a fetch on the caller's thread and an install on the owning display thread. The
      tool-change reload uses the same split, through `asyncExec` with a disposed-display check.
- [x] The `ImageProvider` listener is registered during the first install, through the display the cache owns.
      `dispose()` removes it.
- [x] Pending icons get the `SharedIcons.EMPTY` descriptor and are loaded through `getNativeImage`. Loaded icons
      keep the current cache-owned copy. `preloadImageFromServer` is no longer called from the cache.
- [x] Fetch and install failures are logged and never propagate.
- [x] Deleting a tool removes only that tool's icon.
- [x] Desktop and web builds pass (commands under Verification).

### Task 2: Caches are filled in LoginJob before the background sync

Layer: console UI (shared `LoginJob`, SWT `Startup`, RWT `Startup`).

- [x] `LoginJob` attaches `SummaryTablesCache`, `GraphTemplateCache` and then `ObjectToolsCache` under a progress
      step, after all other synchronous login requests and immediately before `startBackgroundObjectSync`. This
      happens in both sync modes.
- [x] SWT `Startup` no longer attaches these three caches.
- [x] RWT `Startup` attaches `ObjectToolsCache` only for pop-out windows. The other two caches are no longer
      attached there.
- [x] Desktop and web builds pass.

### Task 3: Tool icons already on screen pick up the real icon

Layer: console UI (shared `main` sources).

- [x] `ObjectToolsCache` accepts `ImageUpdateListener`s. It notifies them on the UI thread after
      `onLibraryImageChange` has committed and released the lock, only when a tool uses the changed image, and
      iterates over a snapshot so listeners can unregister during the notification.
- [x] The Commands element registers when `createClientArea` creates its `commandBox`, unregisters when that widget
      is disposed, and rebuilds only while the box is alive.
- [x] `ObjectMenuFactory` refreshes tool item images on `SWT.Show` of a tools menu and its submenus. The listener is
      added only when the menu was built with at least one placeholder.
- [x] Desktop and web builds pass.

## Verification

There are no automated console UI tests, so each task is gated on both builds compiling (after installing
`netxms-base` and `netxms-client` as described in CLAUDE.md):

```
mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone
mvn -f src/client/nxmc/java/pom.xml clean package -Pweb
```

The change is verified by hand after task 3:

1. Desktop, background sync on, a slow link to a large server, JFR recording: the `main` thread shows no
   `getImage`, `getObjectTools`, `listDciSummaryTables` or `getPredefinedGraphs` waits after "Login job completed".
   The `getUserVariables` wait may remain (out of scope). Tool icons are eventually replaced by the real images.
   The opening time is not fixed, because icons can arrive after the sync finishes.
2. Desktop, normal sync: startup works as before, and real tool icons appear without reselecting.
3. Select a node before the icons arrive: the header Tools menu shows real icons on its next opening, and the
   Commands element updates without reselecting. An open header menu is not closed when an icon arrives.
4. Desktop restart with a saved object tool result view: it restores without "Invalid tool id".
5. Web client: log in and check the header Tools menu icons. Pop out an Overview of a node that has a visible
   command, and check the Commands icons in the pop-out.
6. A tool whose icon GUID is not in the image library at login keeps the placeholder. Deleting a tool's icon image
   during the session leaves that tool without an image. Neither case produces errors.
7. Edit an object tool while the console is running: the reload installs on the UI thread, and the context menu
   shows the change. Deleting one tool leaves the other tools' icons in place.

## Post-Completion

- Cherry-pick the commits to `stable-7.0` (milestone 7.0.6). Check first that the touched files have not diverged
  there.
- Close #4072 once it is on both branches. The milestone is already set.
