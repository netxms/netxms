# Physical placement and chassis controller follow-ups (PR #3633)

## Overview

Two `worth: yes` backlog items left behind by PR #3633, both in the physical placement / chassis
controller code:

- `docs/backlog/chassis-nxcp-controller-id-does-not-rebind.md` — `Chassis::modifyFromMessageInternal`
  stores `VID_CONTROLLER_ID` without scheduling `updateControllerBinding()`. The console masks it by
  always sending the flag mask with the id; nxshell / raw Java API calls leave the chassis linked
  under the old controller (and its inherited access rights) until restart.
- `docs/backlog/placement-patch-commits-stale-snapshot.md` — the WebAPI `modifyFromJSONInternal`
  paths for Node and Chassis stage all six placement fields (plus `m_controllerId` on Chassis), drop
  the property lock for validation, and write the whole snapshot back after relocking. A concurrent
  NXCP edit landing in that window is overwritten by fields the PATCH never mentioned, and an
  `onObjectDelete` clear of `m_physicalContainer` / `m_controllerId` is undone.

The third backlog item (`rack-placement-out-of-range-unvalidated`, `worth: later`) is out of scope:
its strict-vs-lenient decision is still open.

No GitHub issue: commit messages reference PR #3633 as a follow-up.

## Context (from discovery)

- `src/server/core/chassis.cpp:190-211` `Chassis::modifyFromMessageInternal`; the rack-id branch at
  :194-198 already schedules `ThreadPoolExecuteSerialized(g_mainThreadPool, PHYSICAL_BINDING_TASK_KEY,
  this, &Chassis::updateRackBinding)` — the controller branch at :192-193 does not.
- `src/server/core/chassis.cpp:216-274` `Chassis::modifyFromJSONInternal`: `controllerId` staged at
  :219, placement staged at :242-248, committed unconditionally at :255-260, `m_controllerId`
  committed unconditionally at :269.
- `src/server/core/node.cpp:11740-11758` `Node::modifyFromJSONInternal`: same stage / unlock /
  commit-all block.
- `src/server/core/physical_placement.cpp` `ModifyPhysicalPlacementFromJson` writes all six fields
  through `PhysicalPlacementRef`; for keys absent from the document it writes back the staged value.
- #3634 (closed, 469770e366) already serialises rebind jobs under `PHYSICAL_BINDING_TASK_KEY`, so an
  extra queued `updateControllerBinding` is harmless.
- WebAPI requests are serialised on one MHD thread (webapi.cpp:542,550), so the race is only
  REST vs. NXCP / object deletion, never REST vs. REST.
- Java API: `NXCObjectModificationData.setControllerId` → `VID_CONTROLLER_ID` in `modifyObject`
  (NXCSession.java:8251); chassis creation sends `VID_CONTROLLER_ID` (NXCSession.java:7386).
  Integration tests live in `tests/integration/src/test/java/org/netxms/tests/`
  (`EppSourceObjects` shows creating containers/nodes via `createObjectSync`).

## Development Approach

- Complete each task fully before moving to the next; keep changes small and focused.
- Each task removes its backlog item with `git rm` in the same commit.
- Build the server core after each C++ change; the Java integration test for Task 1 runs against the
  local test server (see memory `reference_local_test_server`).
- Commit messages: `... (follow-up to PR #3633)` in place of an issue reference.

## Testing Strategy

- Task 1: Java integration test — create two nodes and a chassis bound under controller A with
  `CHF_BIND_UNDER_CONTROLLER`, modify only `controllerId` to B (no flags in the modification data),
  and assert the chassis parent becomes B and is no longer A (poll with a short timeout; the rebind
  runs on the thread pool). Clean up created objects.
- Task 2: the race window cannot be driven deterministically from a test. Coverage is by review, plus
  a manual/integration sanity check that a PATCH carrying only some placement keys still updates
  those keys and leaves the others unchanged, and that `controllerId` is untouched by a PATCH that
  omits it.

## Progress Tracking

- Mark completed items with `[x]` immediately when done.
- Add newly discovered tasks with ➕ prefix; document blockers with ⚠️ prefix.

## Implementation Steps

### Task 1: Rebind chassis under controller when controller id arrives over NXCP

- [ ] In `Chassis::modifyFromMessageInternal`, schedule
      `ThreadPoolExecuteSerialized(g_mainThreadPool, PHYSICAL_BINDING_TASK_KEY, this, &Chassis::updateControllerBinding)`
      when `VID_CONTROLLER_ID` is present, same shape as the `VID_PHYSICAL_CONTAINER_ID` branch
- [ ] Add an integration test in `tests/integration` that changes only the controller id via the
      Java API and asserts the chassis moves under the new controller
- [ ] Build server core; run the new integration test against the local server
- [ ] `git rm docs/backlog/chassis-nxcp-controller-id-does-not-rebind.md`

### Task 2: Commit only the placement keys the PATCH carried

- [ ] `Node::modifyFromJSONInternal`: after relocking, write back each of `m_physicalContainer`,
      `m_rackPosition`, `m_rackHeight`, `m_rackOrientation`, `m_rackImageFront`, `m_rackImageRear`
      only when the corresponding key (`containerId`, `position`, `height`, `orientation`,
      `imageFront`, `imageRear`) is present in the `physicalPlacement` group
- [ ] `Chassis::modifyFromJSONInternal`: same for `m_rackId` and the rest of the placement group
- [ ] `Chassis::modifyFromJSONInternal`: commit `m_controllerId` only when the document carries
      `controllerId` (fold into the existing `json_object_get(json, "controllerId")` check)
- [ ] Update the "Stage into locals" comments where they now misdescribe the commit
- [ ] Build server core; sanity-check a partial placement PATCH and a PATCH without `controllerId`
      against the local server
- [ ] `git rm docs/backlog/placement-patch-commits-stale-snapshot.md`

Known residual, accepted: validation ran against the staged snapshot, so a PATCH carrying only
`position` is checked against the staged `height`; a concurrent NXCP height change in the window can
leave a combined extent the JSON path would have rejected. The NXCP path validates nothing today
(see the `rack-placement-out-of-range-unvalidated` backlog item), so this adds no new reachable state.
Likewise a PATCH that carries `containerId` for a container deleted inside the window still commits
the dead id; that is the ordinary validate-then-use gap every object-id property has.

## Post-Completion

- Move this plan to `docs/plans/completed/`.

## Tandem

| Task | Writer | Reviewer |
|------|--------|----------|
| 1 | claude | codex |
| 2 | claude | codex |
