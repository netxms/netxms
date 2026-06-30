# Rule Chains in EPP (NX-2334 / #2540)

## Overview

Add reusable, first-class **rule chains** to the Event Processing Policy (EPP). Today the EPP is a single flat, top-to-bottom list of rules. This change introduces a **main chain** (the entry point) whose rules can route a matching event into one or more **sub-chains**. Sub-chains are first-class objects with their own identity, name, ACL, and version, shown as separate tabs in the editor.

Problems solved / benefits:
- **Granular access control** — service providers (MSPs) can be granted edit/read on individual chains without touching the rest of the policy.
- **UI scaling** — the current single giant rule list is slow to render; per-chain tabs paint only their own rules.
- **Reuse** — a chain can be called from multiple rules / multiple chains.
- **Composition** — a calling rule acts as an upper filter that descends into a sub-chain only when it matches.

The feature is designed so that an upgraded system with no chains behaves **exactly** as before: the existing flat policy becomes the main chain.

## Context (from discovery)

Files / components involved:
- **Server C++**: `src/server/core/epp.cpp` (`EPRule`, `EventProcessingPolicy`), `src/server/include/nms_events.h`. Rule position == `rule_id` (no sequence column); GUID is stable identity; whole policy is DELETE+rewrite on save; `saveWithMerge` (`epp.cpp:2350`) does GUID-based optimistic 3-way merge; `EPRule::processEvent` (`epp.cpp:1189`), `EventProcessingPolicy::processEvent` (`epp.cpp:2673`); `fillMessage` (`epp.cpp:1982`); `loadFromDB`/`saveToDB`; `sendToClient` (`epp.cpp:2683`); `replacePolicy` (`epp.cpp:2700`).
- **NXCP**: `CMD_GET_EPP=0x0014`, `CMD_SAVE_EPP=0x0015`, `CMD_EPP_RECORD=0x0016` (one message per rule). Handlers in `src/server/core/session.cpp`: `getEventProcessingPolicy` (6249), `saveEventProcessingPolicy` (6281), `processEventProcessingPolicyRecord` (6332), `finishEPPSave` (6357). Key VIDs: `VID_RULE_ID=67`, `VID_GUID=222`, `VID_NUM_RULES=74`, `VID_EPP_VERSION=919`, `VID_RULE_VERSION=920`, `VID_BASE_VERSION=921`, plus conflict/deleted/version list bases.
- **DB schema**: `sql/schema.in` — `event_policy` (~1700) + child tables `policy_source_list`, `policy_event_list`, `policy_time_frame_list`, `policy_action_list`, `policy_timer_cancellation_list`, `policy_pstorage_actions`, `policy_cattr_actions` (all key on `rule_id`). `DB_SCHEMA_VERSION_MINOR` lives in `src/server/include/netxmsdb.h` **only** (`dbschema_*.sql` / `dbinit_*.sql` are generated). nxdbmgr upgrade procedure under `src/server/tools/nxdbmgr/`.
- **Java client lib**: `src/client/java/netxms-client/.../events/EventProcessingPolicy.java`, `EventProcessingPolicyRule.java` (fillMessage, from-message ctor, `ruleNumber`, `version`, `modified`, `DeletedRuleInfo`); `NXCSession.java` `getEventProcessingPolicy` (9149) / `saveEventProcessingPolicy` (9176), `EPPSaveResult`/`EPPConflict`.
- **UI (nxmc)**: `src/client/nxmc/.../events/views/EventProcessingPolicyEditor.java` (vertical stack of `RuleEditor` widgets; `insertRule` renumber loops ~897-911; `savePolicy` 469), `widgets/RuleEditor.java`, `propertypages/Rule*`, `dialogs/EPPConflictDialog.java`, `views/helpers/RuleClipboard.java` (in-session only, NOT cross-system).

Related patterns found:
- Stable identity is the **GUID**; `rule_id` is reassigned on every save. Chain references must be keyed by **GUID**, not numeric id.
- Optimistic concurrency: in-memory `m_version` on policy and per-rule; client sends base version, server detects conflicts.
- Alarm access uses a default-on system right plus per-object ACL — a useful precedent for chain access (see open question C).

Dependencies identified:
- New `CMD_*` / `VID_*` constants must be mirrored into `NXCPCodes.java` and `NXCPMessageCodeName()` in `src/libnetxms/nxcp.cpp` (per CLAUDE.md).
- New system event `SYS_EPP_CHAIN_LOOP` must be added to the event template / `dbinit` events.

## Development Approach

- **Testing approach**: Regular (code first, then tests). This is a C/C++ + Java codebase; automated coverage centers on the C++ test suite (`tests/suite/netxms-test-suite`) plus manual UI verification. Add/extend unit tests where the existing suite has hooks (notably the execution engine and loop guard); UI and protocol round-trips are verified manually and noted under Post-Completion.
- Complete each task fully before moving to the next; keep changes small and focused.
- After a rename or API change, run a project-wide grep for the old name before building (per CLAUDE.md).
- Maintain backward compatibility: an upgraded system with only the main chain must behave identically to today.

## Testing Strategy

- **Unit tests**: the **execution engine + loop guard** (Phase 3) is the highest-value target for automated tests — construct an in-memory policy with chains and assert traversal order, return-vs-terminate behavior, diamond reuse runs twice, and loop detection fires once. Add to the existing server test suite if a harness for `EventProcessingPolicy` is feasible; otherwise document a manual matrix.
- **Round-trip tests**: NXCP serialization (fillMessage ↔ from-message) and DB load/save are verified by save→reload→compare in a running server (manual / scripted).
- **Migration test**: run nxdbmgr upgrade against a DB populated by a pre-change server; confirm the flat policy is preserved as the main chain with zero behavioral change.
- **No project e2e UI harness** exists for nxmc; UI behavior (tabs, chain-call navigation, read-only chains, per-chain save) is verified manually — see Post-Completion.

## Progress Tracking

- Mark completed items with `[x]` immediately when done.
- Add newly discovered tasks with ➕ prefix; document blockers with ⚠️ prefix.
- Keep this plan in sync with actual work; update if scope changes.

## Solution Overview

Data model (see Technical Details for columns):
- New **`event_policy_chain`** registry — one row per chain; reserved `chain_id = 0` is the undeletable main chain and the entry point for event processing.
- New **`policy_chain_call_list`** many-to-many junction — maps a calling rule `(chain_id, rule_id)` to one or more `target_chain_guid` with a `sequence` order.
- New **`policy_chain_acl`** — per-chain `(user/group id → rights)`.
- **`event_policy`** and **every child table** gain a `chain_id` column; `rule_id` becomes position **within a chain**, so each chain can be saved independently.

Execution: `EventProcessingPolicy::processEvent` evaluates the main chain; a matching rule applies its own effects and then enters its called chains in `sequence` order, recursively. A `visited` set of chain GUIDs tracks the **active call stack** (remove-on-exit) for the loop guard. The **calling rule's stop-processing flag** decides whether processing returns to the caller or halts.

Protocol: `CMD_GET_EPP` / `CMD_SAVE_EPP` gain an optional `VID_CHAIN_ID` scope. Whole-policy save still works (admin); per-chain save rewrites only that chain and runs the GUID merge per chain. The chains registry (filtered to readable chains) is sent as a header on full load.

## Technical Details

### New columns / tables

`event_policy_chain`:
- `chain_id` integer PK
- `chain_guid` (uuid) — stable identity for merge / export / reuse
- `name` varchar, `description` varchar
- `flags` integer
- `version` integer — per-chain optimistic-concurrency version

`policy_chain_call_list`:
- `chain_id` integer — owning chain of the calling rule
- `rule_id` integer — position of the calling rule within that chain
- `target_chain_guid` (uuid) — chain entered when the rule matches
- `sequence` integer — order in which a rule's called chains run
- PK `(chain_id, rule_id, sequence)`

`policy_chain_acl`:
- `chain_id` integer
- `user_id` integer — user or group id
- `access_rights` integer — bitmask (read / edit / [create]; see open question B)
- PK `(chain_id, user_id)`

`event_policy` and all child tables (`policy_source_list`, `policy_event_list`, `policy_time_frame_list`, `policy_action_list`, `policy_timer_cancellation_list`, `policy_pstorage_actions`, `policy_cattr_actions`):
- add `chain_id` integer (default 0); keys become `(chain_id, rule_id, …)`.

### New protocol constants

- `VID_CHAIN_ID` — chain a rule belongs to / save scope.
- `VID_CHAIN_CALL_COUNT` + `VID_CHAIN_CALL_LIST_BASE` (stride for: target chain guid, sequence) — the per-rule chain-call block.
- Chains-registry header on full load: chain count + per-chain (guid, name, flags, version, caller's effective rights).
- Optional `CMD_MODIFY_EPP_CHAIN` (and/or delete) for chain create/rename/delete/ACL edits, **or** fold chain metadata changes into the save path.
- New system event `SYS_EPP_CHAIN_LOOP` (chain loop detected), with parameters for calling-chain and target-chain names; alarm key derived from both names for dedup.

### Processing flow (per rule, within a chain)

1. Evaluate filter (sources, events, severity, time frames, script) as today.
2. If matched, apply own effects (actions, alarm, pstorage, custom attrs) as today.
3. For each chain call in `sequence` order:
   - If `target_chain_guid` ∈ `visited` → loop detected: skip, log, fire `SYS_EPP_CHAIN_LOOP` (deduped). 
   - Else add to `visited`, recurse into the target chain, remove from `visited` on return.
4. After calls return, the **calling rule's stop-processing flag** decides: set → halt all processing; unset → continue to next rule in this chain.

Baseline assumption (see open question A): **stop-processing inside a sub-chain halts the entire event's processing**, preserving today's semantics; "return to caller" happens only when nothing in the sub-chain stopped.

## What Goes Where

- **Implementation Steps** (`[ ]`): schema, server core, execution engine, protocol, access control, Java client, UI, export/import — all achievable in this repo.
- **Post-Completion** (no checkboxes): manual UI verification, manual protocol/migration round-trips, and the open design questions that must be settled on the issue before/*during* coding.

## Implementation Steps

### Task 1: Database schema + migration

**Files:**
- Modify: `sql/schema.in`
- Modify: `src/server/include/netxmsdb.h` (bump `DB_SCHEMA_VERSION_MINOR`)
- Create/Modify: nxdbmgr upgrade procedure under `src/server/tools/nxdbmgr/` (the current-version upgrade source file)
- Modify: events seed in `sql/` (dbinit events) to add `SYS_EPP_CHAIN_LOOP`

- [ ] Add `event_policy_chain`, `policy_chain_call_list`, `policy_chain_acl` table definitions to `sql/schema.in`.
- [ ] Add `chain_id` column to `event_policy` and every child table in `sql/schema.in`; update PKs/indexes to include `chain_id`.
- [ ] Add the reserved main chain (`chain_id = 0`, well-known GUID) to the schema seed.
- [ ] Add `SYS_EPP_CHAIN_LOOP` system event template to the events seed.
- [ ] Write the nxdbmgr upgrade procedure: create new tables, create main-chain row, add `chain_id` (default 0) to existing tables, backfill, add the event.
- [ ] Bump `DB_SCHEMA_VERSION_MINOR` in `netxmsdb.h`.
- [ ] Manually verify upgrade on a pre-change DB: flat policy preserved as main chain (round-trip).

### Task 2: Server core data model — `event_policy_chain` + `EPRule` chain membership

**Files:**
- Modify: `src/server/include/nms_events.h`
- Modify: `src/server/core/epp.cpp`

- [ ] Add an `EPRule` member for `chain_id` and a chain-call list (target GUID + sequence pairs); initialize in all constructors (id, ConfigEntry, json_t, DB_RESULT, NXCPMessage).
- [ ] Add an `EventPolicyChain` representation (id, guid, name, description, flags, version) and hold a registry of chains on `EventProcessingPolicy` (keep `m_rules` but group/scope by chain).
- [ ] Update `EPRule::loadFromDB` / `saveToDB` to read/write `chain_id` and the `policy_chain_call_list` rows.
- [ ] Update `EventProcessingPolicy::loadFromDB` to load the chains registry and load rules ordered by `(chain_id, rule_id)`.
- [ ] Update `EventProcessingPolicy::saveToDB` / `replacePolicy` to write chains and per-chain rule numbering.
- [ ] Manually verify load/save round-trip preserves chains and calls.

### Task 3: Execution engine + loop event

**Files:**
- Modify: `src/server/core/epp.cpp`
- Possibly Modify: test harness under `tests/` if an `EventProcessingPolicy` hook exists

- [ ] Rework `EventProcessingPolicy::processEvent` to evaluate the main chain and recurse into called chains, threading a `visited` GUID set (active call stack, remove-on-exit).
- [ ] In `EPRule::processEvent`, after effects, iterate chain calls in `sequence` order and recurse.
- [ ] Implement the loop guard: on re-entry of a GUID already in `visited`, skip + `nxlog_debug_tag` + post `SYS_EPP_CHAIN_LOOP` with calling/target chain names and a dedup alarm key.
- [ ] Preserve stop-processing semantics: the calling rule's flag decides return vs halt; a stop inside a sub-chain halts globally (baseline — see open question A).
- [ ] Add a debug tag entry to `doc/internal/debug_tags.txt` if a new tag is introduced.
- [ ] Write/extend unit tests for traversal order, return-vs-terminate, diamond-reuse-runs-twice, and single loop-event-on-cycle (or document a manual matrix if no harness).

### Task 4: NXCP protocol — chain-aware load and per-chain save/merge

**Files:**
- Modify: `src/server/core/epp.cpp` (`fillMessage`, `EPRule(NXCPMessage)`, `sendToClient`, `saveWithMerge`)
- Modify: `src/server/core/session.cpp` (`getEventProcessingPolicy`, `saveEventProcessingPolicy`, `processEventProcessingPolicyRecord`, `finishEPPSave`)
- Modify: `src/server/include/nxcpcodes.h` (new `VID_*`, optional `CMD_MODIFY_EPP_CHAIN`)
- Modify: `src/java-common/netxms-base/src/main/java/org/netxms/base/NXCPCodes.java`
- Modify: `src/libnetxms/nxcp.cpp` (`NXCPMessageCodeName()`)

- [ ] Define `VID_CHAIN_ID`, `VID_CHAIN_CALL_COUNT`, `VID_CHAIN_CALL_LIST_BASE`, registry header VIDs, and any new `CMD_*`.
- [ ] Mirror new `CMD_*`/constants into `NXCPCodes.java`; add `CMD_*` names to `NXCPMessageCodeName()`.
- [ ] Extend `EPRule::fillMessage` and the `EPRule(NXCPMessage)` ctor to carry `chain_id` + chain-call block.
- [ ] Extend `sendToClient` to emit the chains-registry header (filtered to readable chains) before rule records.
- [ ] Add optional `VID_CHAIN_ID` scope to `getEventProcessingPolicy` (full vs single-chain load) and to the save path; run `saveWithMerge` per chain with per-chain base version / conflict tracking / version bump.
- [ ] Implement chain create/rename/delete handling (new command or save-path extension).
- [ ] Manually verify full load, single-chain load, whole-policy save, and per-chain save round-trips.

### Task 5: Access control enforcement

**Files:**
- Modify: `src/server/core/epp.cpp` / `src/server/core/session.cpp`
- Modify: `src/server/core/` user-rights helpers as needed

- [ ] Load `policy_chain_acl` into the chain registry; compute a caller's effective rights per chain.
- [ ] Gate full vs filtered registry on load by per-chain read rights; gate main chain + chain management by the existing EPP system right.
- [ ] Enforce per-chain scope on every per-chain save: reject/ignore rules outside the chains the user may edit.
- [ ] Add chain-ACL read/modify handling to the chain-management command.
- [ ] Manually verify a chain-scoped (non-admin) user can load+save only permitted chains and cannot see others.
- [ ] NOTE: exact right set and the "access all chains" default-on right are **open questions B/C** — implement behind the chosen model once settled.

### Task 6: Java client library

**Files:**
- Modify: `src/client/java/netxms-client/.../events/EventProcessingPolicy.java`
- Modify: `src/client/java/netxms-client/.../events/EventProcessingPolicyRule.java`
- Create: `src/client/java/netxms-client/.../events/EventProcessingPolicyChain.java`
- Modify: `src/client/java/netxms-client/.../NXCSession.java`

- [ ] Add `EventProcessingPolicyChain` (guid, name, description, flags, version, effective rights) and a chains list on `EventProcessingPolicy`.
- [ ] Add `chainId` + chain-call list fields to `EventProcessingPolicyRule`; update `fillMessage`, the from-message ctor, and copy ctor.
- [ ] Update `getEventProcessingPolicy` to read the registry header and tag rules by chain; add an optional single-chain load.
- [ ] Update `saveEventProcessingPolicy` to support whole-policy and per-chain save (with `VID_CHAIN_ID` scope and per-chain base version); extend `EPPSaveResult`/`EPPConflict` for per-chain results.
- [ ] Add client methods for chain create/rename/delete/ACL.
- [ ] Build `netxms-base` and `netxms-client` (`mvn ... install`); run `mvn clean` afterward (per preferences).

### Task 7: UI — tabbed editor + chain management

**Files:**
- Modify: `src/client/nxmc/.../events/views/EventProcessingPolicyEditor.java`
- Modify: `src/client/nxmc/.../events/widgets/RuleEditor.java`
- Create: chain-management dialogs / property pages under `src/client/nxmc/.../events/`
- Modify: `src/client/nxmc/.../events/dialogs/EPPConflictDialog.java`

- [ ] Convert the editor to a tabbed layout: one tab per readable chain, main chain first; render only that chain's `RuleEditor` stack.
- [ ] Render a rule's chain calls as chips/links that jump to the target chain's tab; show "used by N rules" on reused chains.
- [ ] Make per-chain rule numbering and the `insertRule`/delete renumber loops chain-scoped.
- [ ] Add chain management UI: create/rename/delete chains and edit a chain's ACL; make read-only chains' tabs read-only.
- [ ] Wire `savePolicy` to per-chain save (save the active/dirty chain) and whole-policy save (admin).
- [ ] Update `EPPConflictDialog` for per-chain conflict reporting.
- [ ] Build desktop + web clients; verify both SWT and RWT render the tabs (manual).

### Task 8: Export / import

**Files:**
- Modify: `src/server/core/epp.cpp` (`createExportRecord`, `toJson`, import path)
- Modify: corresponding import/export handling (config export module)

- [ ] Include chain registry + per-rule chain membership and chain-call (by GUID) in `createExportRecord` / `toJson`.
- [ ] On import, resolve chain references by GUID; warn (and leave the call unresolved) when a target chain is missing.
- [ ] Manually verify export→import on a second system preserves chains and reports missing targets.

### Task 9: Verify acceptance criteria

- [ ] Upgraded system with only the main chain behaves identically to pre-change (no regression).
- [ ] A rule can enter multiple chains in order; return-vs-terminate honors the calling rule's stop flag.
- [ ] Loop on the active call stack is detected, skipped, and fires exactly one (deduped) `SYS_EPP_CHAIN_LOOP`; diamond reuse runs the shared chain twice.
- [ ] Per-chain load/save works; a chain-scoped user is correctly limited.
- [ ] Run full C++ test suite: `./tests/suite/netxms-test-suite`.

### Task 10: Documentation + cleanup

- [ ] Update `doc/internal/debug_tags.txt` if a new tag was added.
- [ ] Note the new tables in any data-dictionary source if maintained in-repo.
- [ ] Update relevant component CLAUDE.md notes if new patterns were introduced.
- [ ] Move this plan to `docs/plans/completed/` when done.

## Open questions for discussion

*To be posted on the issue; settle before/while implementing access control and the execution engine.*

A. Does stop-processing **inside a sub-chain** halt the entire event's processing, or only end that sub-chain and return to the caller? (Plan assumes global halt as the baseline but flags it.)

B. Exact access-right set: read / edit / create — is "create" separable from "edit"?

C. Should there be a common **"access all EPP chains"** system right, default-ON (mirroring alarm access), so per-chain ACLs only apply for deployments that opt in?

D. Is the chain **creator** auto-granted access to chains they create?

E. Can a chain-scoped (non-admin) user **create new chains**, or only edit chains an admin pre-provisioned?

## Post-Completion

*Items requiring manual intervention or external systems — informational only.*

**Manual verification:**
- nxmc desktop (SWT) and web (RWT): tab rendering, chain-call navigation, "used by N", read-only chains, per-chain save, conflict dialog.
- NXCP round-trips: full load, single-chain load, whole-policy save, per-chain save, chain create/rename/delete.
- Migration: upgrade a populated pre-change DB and confirm zero behavioral change.
- Export/import across two systems, including the missing-target-chain warning path.
- Loop behavior under load: confirm `SYS_EPP_CHAIN_LOOP` deduplicates rather than storming.

**External / follow-up:**
- Settle open questions A–E on the issue; revise access-control and execution behavior to match.
- Documentation site (admin guide / data dictionary) updates outside this repo, if applicable.
