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
- **DB schema**: `sql/schema.in` — `event_policy` (~1700) + 8 child tables `policy_source_list`, `policy_event_list`, `policy_time_frame_list`, `policy_action_list`, `policy_timer_cancellation_list`, `policy_pstorage_actions`, `policy_cattr_actions`, and `alarm_category_map` (an EPP child table despite its name; all key on the rule id). `DB_SCHEMA_VERSION_MINOR` lives in `include/netxmsdb.h` **only** (`dbschema_*.sql` / `dbinit_*.sql` are generated). nxdbmgr upgrade procedure under `src/server/tools/nxdbmgr/`.
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

- **Java integration suite** (primary automated target): there is a live-server EPP integration suite under `tests/integration/src/test/java/org/netxms/tests/` (`EppAlarmTest`, `EppScriptTest`, `EppServerActionTest*`, `EppSeverityCondition`, etc.) that drives EPP through the client against a running server. The **execution engine + loop guard** (Phase 3) is naturally tested here: build a policy with chains via the client → post events → assert alarms and `SYS_EPP_CHAIN_LOOP`. Cover traversal order, return-vs-terminate, diamond-runs-twice, and single-loop-event-on-cycle. Commit to extending this suite rather than a manual matrix.
- **Round-trip tests**: NXCP serialization (fillMessage ↔ from-message) and DB load/save verified by save→reload→compare via the integration client (full-policy, single-chain, per-chain save).
- **Migration test**: run nxdbmgr upgrade against a DB populated by a pre-change server; confirm the flat policy is preserved as the main chain with zero behavioral change. Test on **SQLite, PostgreSQL, and MySQL** at minimum (the PK rebuild differs by dialect — see Task 1).
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

Per-chain optimistic-concurrency **version is in-memory only**, held on the chain object in `EventProcessingPolicy` — never a DB column. This mirrors the existing EPP `m_version` (`nms_events.h`: "in-memory only, not persisted"). No `flags` column — there is no consumer for one (the main chain's undeletable status is implied by `chain_id == 0`).

`policy_chain_call_list`:
- `chain_id` integer — owning chain of the calling rule
- `rule_id` integer — position of the calling rule within that chain
- `target_chain_guid` (uuid) — chain entered when the rule matches
- `sequence` integer — order in which a rule's called chains run
- PK `(chain_id, rule_id, sequence)`

`policy_chain_acl`:
- `chain_id` integer
- `user_id` integer — user or group id
- `access_rights` integer — bitmask: **Read** and **Edit** only (no per-chain "create"; chain creation requires the global EPP edit right — see Resolved decisions)
- PK `(chain_id, user_id)`

`event_policy` and all 8 child tables (`policy_source_list`, `policy_event_list`, `policy_time_frame_list`, `policy_action_list`, `policy_timer_cancellation_list`, `policy_pstorage_actions`, `policy_cattr_actions`, `alarm_category_map`):
- add `chain_id` integer (default 0); keys become `(chain_id, rule_id, …)`; `alarm_category_map.alarm_id` is renamed to `rule_id` since it always held the rule id.

### New protocol constants

- `VID_CHAIN_ID` — chain a rule belongs to / save scope.
- `VID_CHAIN_CALL_COUNT` + `VID_CHAIN_CALL_LIST_BASE` (stride for: target chain guid, sequence) — the per-rule chain-call block.
- Chains-registry header on full load: chain count + per-chain (guid, name, current in-memory version, caller's effective rights). The version is transmitted so the client has a base version for conflict detection on save — it is not stored in the DB.
- **One chain per save request**: `CMD_SAVE_EPP` carries `VID_CHAIN_ID` (mandatory; a request without it or with an unknown chain is rejected with `RCC_INVALID_ARGUMENT` before any rule is uploaded) and that chain's base version in `VID_BASE_VERSION`. There is no whole-policy save; a client saves each modified chain separately, so a save is either fully applied or fully rejected with conflicts.
- **Chain management transport (decided): dedicated commands** `CMD_CREATE_EPP_CHAIN`, `CMD_MODIFY_EPP_CHAIN` (rename / edit ACL), `CMD_DELETE_EPP_CHAIN` and `CMD_GET_EPP_CHAIN_CALLERS`, separate from the rule-save path. Keeps chain-metadata edits off the heavy per-rule save and gives a clean place for ACL changes.
- New system event `SYS_EPP_CHAIN_LOOP` (chain loop detected), posted by the calling rule with parameters calling-chain name, target-chain name, calling rule number, and calling rule GUID. It is posted at most once per 24 hours per chain call: the report time lives on the chain call object in memory only, so it resets when the policy is saved and after a server restart. The `EVENT_EPP_CHAIN_LOOP` numeric code constant lives in **`include/nxevent.h`**; the event template seed goes in **`sql/events.in`**.

### Processing flow (per rule, within a chain)

1. Evaluate filter (sources, events, severity, time frames, script) as today.
2. If matched, apply own effects (actions, alarm, pstorage, custom attrs) as today.
3. For each chain call in `sequence` order:
   - If `target_chain_guid` ∈ `visited` → loop detected: skip, log, fire `SYS_EPP_CHAIN_LOOP` (at most once per 24 hours per chain call).
   - Else add to `visited`, recurse into the target chain, remove from `visited` on return.
4. After calls return, the **calling rule's stop-processing flag** decides: set → halt all processing; unset → continue to next rule in this chain.

Resolved (question A): **stop-processing inside a sub-chain halts the entire event's processing**, preserving today's semantics; "return to caller" happens only when nothing in the sub-chain stopped. A chain-local exit ("exit chain" flag that returns to the caller without halting) may be added later as a separate flag — **out of scope** for this plan.

### Access control model (resolved)

- Per-chain rights are **Read** and **Edit** only.
- The **existing global EPP edit system right** is the umbrella: a user with it has full access to **every** chain, plus chain create / rename / delete / management. There is no separate "access all chains" right.
- A user may access a chain if they have **either** the global EPP edit right **or** chain-level access on that chain.
- **Creating** a chain requires the global EPP edit right — so a creator inherently has access to every chain, including the new one (no special auto-grant needed).
- A **non-global** user:
  - can **view/read** chains they have chain-level **Read** on;
  - can **edit** rules in chains they have chain-level **Edit** on;
  - can add a **"call chain X"** action only when they have **Edit** on the containing chain **and Read** on target chain X.

## What Goes Where

- **Implementation Steps** (`[ ]`): schema, server core, execution engine, protocol, access control, Java client, UI, export/import — all achievable in this repo.
- **Post-Completion** (no checkboxes): manual UI verification and manual protocol/migration round-trips.

## Implementation Steps

### Task 1: Database schema + migration

**Files:**
- Modify: `sql/schema.in`
- Modify: `sql/events.in` (add the `SYS_EPP_CHAIN_LOOP` template — generated into `dbinit_*.sql`)
- Modify: `include/nxevent.h` (add the `EVENT_EPP_CHAIN_LOOP` numeric code constant)
- Modify: `include/netxmsdb.h` (bump `DB_SCHEMA_VERSION_MINOR`)
- Create/Modify: nxdbmgr upgrade procedure under `src/server/tools/nxdbmgr/` (the current-version upgrade source file)

- [x] Add `event_policy_chain`, `policy_chain_call_list`, `policy_chain_acl` table definitions to `sql/schema.in`. (junction call-order column named `sequence_number`, not `sequence`, to avoid the SQL reserved word)
- [x] Add `chain_id` column to `event_policy` and **all 8 child tables** (`policy_source_list`, `policy_event_list`, `policy_time_frame_list`, `policy_action_list`, `policy_timer_cancellation_list`, `policy_pstorage_actions`, `policy_cattr_actions`, and `alarm_category_map`, whose `alarm_id` column is renamed to `rule_id`) in `sql/schema.in`; PKs now `(chain_id, rule_id, …)`.
- [x] Add the reserved main chain (`chain_id = 0`, GUID `a7f4c1e2-3b8d-4f6a-9c5e-1d2b3a4c5e6f`) to the schema seed (`sql/policy.in`); all 123 default-policy seed inserts updated with explicit `chain_id=0` (no DEFAULT, per project convention).
- [x] Add `EVENT_EPP_CHAIN_LOOP` (code 173; 172 was taken by `EVENT_HA_NODE_ACTIVATED` after rebase onto schema v70) to `include/nxevent.h` and the `SYS_EPP_CHAIN_LOOP` template (GUID `c3e1b9d4-6f2a-4b8c-a7e5-9d0f1a2b3c4d`) to `sql/events.in`.
- [x] Write the nxdbmgr upgrade procedure (`H_UpgradeFromV5` in `upgrade_v70.cpp` — moved from `upgrade_v62.cpp` when master jumped to schema major version 70): create the three new tables, insert the main-chain row, add `chain_id` to `event_policy` + all 8 child tables, backfill to 0, `CreateEventTemplate` for the new event.
- [x] **Cross-dialect PK rebuild**: handled by the portable `DBDropPrimaryKey`/`DBAddPrimaryKey` framework helpers (which already implement the SQLite create/copy/drop/rename internally) — no per-dialect code needed. Finding #4 is a non-issue.
- [x] Bump `DB_SCHEMA_VERSION_MINOR` to 36 (70.35 → 70.36) in `include/netxmsdb.h`.
- [x] Verify upgrade on **SQLite** (live round-trip, 2026-07-07): built a 70.5 database from master's generated `dbinit_sqlite.sql`, ran the new `nxdbmgr upgrade` → 70.6 succeeded (constants since renumbered to 70.36 / event 186 after rebase onto master); `nxdbmgr check` clean; all 55 stock rules backfilled to `chain_id=0`; main-chain seed, event 173, and the three chain tables present; migrated schema matches fresh 70.6 init in columns, types, and PK order (`chain_id` first) for all 12 affected tables — only physical column position differs, as expected for `ALTER TABLE ADD`. SQLite exercises the hardest PK-rebuild path (full table rebuild).
- [ ] ⚠️ Verify upgrade on **PostgreSQL and MySQL**: not yet run — needs live database servers; the dialect-specific part (`DBDropPrimaryKey`/`DBAddPrimaryKey`) uses plain `ALTER TABLE` there, lower risk than the verified SQLite rebuild.

### Task 2: Server core data model — `event_policy_chain` + `EPRule` chain membership

**Files:**
- Modify: `src/server/include/nms_events.h`
- Modify: `src/server/core/epp.cpp`

- [x] Add an `EPRule` member for `chain_id` (`m_chainId`) and a chain-call list (`StructArray<uuid> m_chainCalls`, order = sequence); initialize in all 5 constructors (id, ConfigEntry, json_t, DB_RESULT, NXCPMessage). Accessors get/setChainId, chain-call get/add.
- [x] Add an `EventPolicyChain` class (id, guid, name, description, ACL as `StructArray<ACL_ELEMENT>`, **in-memory** `version` not persisted, own `SharedObjectArray<EPRule>` rules). **Container (settled):** main chain (`chain_id 0`) stays as `m_rules`; sub-chains in `SharedHashMap<uint32_t, EventPolicyChain> m_chains` keyed by `chain_id`. **Scope guard honored:** `processEvent`/`fillMessage`/`EPRule(NXCPMessage)`/`saveWithMerge`/export unchanged — behavior-preserving DB round-trip only.
- [x] Update `EPRule::loadFromDB` / `saveToDB` to read/write `chain_id` (every child-table query scoped by `chain_id AND rule_id`; `chain_id` appended to each INSERT) and the `policy_chain_call_list` rows.
- [x] Update `EventProcessingPolicy::loadFromDB` to load the chain registry + per-chain ACL, then rules `ORDER BY chain_id,rule_id`, routing chain 0 → `m_rules`, others → their sub-chain. The rule SELECT lists `chain_id` first; `EPRule(DB_RESULT)` column reads shifted +1 to match.
- [x] `EventProcessingPolicy::saveToDB`: full-save now also writes `event_policy_chain` (deleting `WHERE chain_id<>0` to keep the seeded main row), `policy_chain_acl`, and each sub-chain's rules + chain-calls. ⚠️ The **chain-scoped** single-chain save variant (`DELETE … WHERE chain_id=?`) is deferred to Task 4 (per-chain save protocol) — Task 2 keeps the full-rewrite path, which preserves sub-chains since `m_chains` is independent of the merge.
- [x] `replacePolicy`: sets `chainId=0` + per-chain `rule_id` on main-chain rules; leaves `m_chains` intact (the merge/replace path is main-chain-only until Task 4).
- Verified: `epp.lo`, `session.lo`, `events.lo`, `alarm.lo`, and all of `aitools` compile clean; header changes additive (no consumer breakage). ⚠️ Pre-existing unrelated bug left untouched (out of scope): `alarm_category_map` INSERT binds `DB_SQLTYPE_INTEGER && success` as the type arg in `EPRule::saveToDB`.
- [ ] Verify load/save round-trip preserves chains and calls (integration suite).

### Task 3: Execution engine + loop event

**Files:**
- Modify: `src/server/core/epp.cpp`
- Create/Modify: `tests/integration/src/test/java/org/netxms/tests/EppChainTest.java` (extend the existing `Epp*Test` integration suite)

- [x] Rework `EventProcessingPolicy::processEvent` to evaluate the main chain and recurse into called chains, threading a `visited` GUID set (active call stack, remove-on-exit) — `EventProcessingContext` (policy + current chain name + chain GUID stack) threaded through `processRuleList` / `enterChain`.
- [x] In `EPRule::processEvent`, after effects, iterate chain calls in `sequence` order and recurse (via `EventProcessingPolicy::enterChain`; `m_chainCalls` is already loaded in sequence order).
- [x] Implement the loop guard: on re-entry of a GUID already in `visited`, skip + `nxlog_debug_tag` + `EventBuilder(EVENT_EPP_CHAIN_LOOP, g_dwMgmtNode)` with `callingChainName` / `targetChainName` parameters.
- [x] Preserve stop-processing semantics: the calling rule's flag decides return vs halt; a stop inside a sub-chain halts globally (resolved decision A) — `EPRule::processEvent` returns true if any called chain stopped, else falls through to its own `RF_STOP_PROCESSING`.
- [x] Add a debug tag entry to `doc/internal/debug_tags.txt` if a new tag is introduced — not needed, reuses existing `event.policy` tag.
- [ ] ⚠️ Extend the Java integration suite (`EppChainTest`): build chained policies via the client and assert traversal order, return-vs-terminate, diamond-reuse-runs-twice, and exactly one `SYS_EPP_CHAIN_LOOP` on a cycle. **Blocked**: building chained policies via the client requires NXCP chain support (Task 4) and Java client support (Task 6); write this test as part of Task 6 verification.

### Task 4: NXCP protocol — chain-aware load and per-chain save/merge

**Files:**
- Modify: `src/server/core/epp.cpp` (`fillMessage`, `EPRule(NXCPMessage)`, `sendToClient`, `saveWithMerge`)
- Modify: `src/server/core/session.cpp` (`getEventProcessingPolicy`, `saveEventProcessingPolicy`, `processEventProcessingPolicyRecord`, `finishEPPSave`)
- Modify: `include/nms_cscp.h` (new `VID_*`, `CMD_CREATE_EPP_CHAIN`, `CMD_MODIFY_EPP_CHAIN`, `CMD_DELETE_EPP_CHAIN`, `CMD_GET_EPP_CHAIN_CALLERS`)
- Modify: `src/java-common/netxms-base/src/main/java/org/netxms/base/NXCPCodes.java`
- Modify: `src/libnetxms/nxcp.cpp` (`NXCPMessageCodeName()`)

- [x] Define `VID_CHAIN_ID` (1032), `VID_CHAIN_CALL_COUNT` (1033), `VID_NUM_CHAINS` (1034), `VID_CHAIN_GUID` (1035), `VID_CHAIN_LIST_BASE` (0x48000000), `VID_CHAIN_CALL_LIST_BASE` (0x79000000), and `CMD_MODIFY_EPP_CHAIN` (0x022B) in `include/nms_cscp.h`. Wire-format decision: the chain-call block is **ordered GUIDs, stride 1** (sequence implied by order — sequence_number is only an ordering key).
- [x] Mirror new `CMD_*`/`VID_*` constants into `NXCPCodes.java`; add the chain management commands to `NXCPMessageCodeName()`.
- [x] Extend `EPRule::fillMessage` and the `EPRule(NXCPMessage)` ctor to carry `chain_id` + chain-call block.
- [x] Extend load path: `fillLoadResponse` emits the chains-registry header (`VID_NUM_CHAINS` + `VID_CHAIN_LIST_BASE` stride 10: id, guid, name, description, version; rights slot at +5 reserved for Task 5) and the all-chains rule total; `sendToClient` streams all chains' rules. Registry filtering to readable chains is Task 5.
- [x] Add optional `VID_CHAIN_ID` scope to `getEventProcessingPolicy` (full vs single-chain load; unknown chain → `RCC_INVALID_ARGUMENT`).
- [x] **Decompose `saveWithMerge` per chain** — `chainId` param added; serverRuleMap, version compare, container rebuild, and predecessor-insertion all scoped to the chain (main chain = `m_rules`/`m_version`, sub-chain = `EventPolicyChain` rules/version); applied rules get `setChainId`; persists via new chain-scoped `saveChainToDB` (deletes only the chain's rows — this also closes the single-chain saveToDB item deferred from Task 2).
- [x] `saveEventProcessingPolicy` / `finishEPPSave` save exactly one chain: mandatory `VID_CHAIN_ID` (missing or unknown chain → `RCC_INVALID_ARGUMENT` before upload) + `VID_BASE_VERSION`; uploaded rules whose `chain_id` differs from the request → `RCC_INVALID_ARGUMENT`; one `saveWithMerge` call. Response carries the chain's new version in `VID_EPP_VERSION` (current server version on conflict) and rule versions of that chain only.
  - ➕ Note for Task 6: moving a rule between chains works via the per-chain fast path; under a concurrent-edit merge the client must delete it in the old chain and re-add with version 0 in the new one.
- [x] Implement chain management commands (`ClientSession::createEppChain`, `modifyEppChain`, `deleteEppChain`, `getEppChainCallers`): `CMD_CREATE_EPP_CHAIN` returns new `VID_CHAIN_ID` + `VID_CHAIN_GUID`; `CMD_MODIFY_EPP_CHAIN` refuses chain 0; ACL block reuses `VID_ACL_SIZE`/`VID_ACL_USER_BASE`/`VID_ACL_RIGHTS_BASE` (present = replace, absent = keep); `CMD_DELETE_EPP_CHAIN` refuses chain 0, removes the chain's rules and every call to the chain from other rules (`policy_chain_call_list` rows by target GUID plus in-memory), bumps the version of each chain that lost a call and returns those versions (`VID_NUM_CHAINS` + `VID_CHAIN_LIST_BASE`, stride 2); `CMD_GET_EPP_CHAIN_CALLERS` lists rules calling a chain (`VID_CHAIN_CALL_COUNT` + `VID_CHAIN_CALL_LIST_BASE`, stride 10) so the client can confirm before deleting. Gated by global `SYSTEM_ACCESS_EPP`. Database work runs outside the policy lock.
- [ ] ⚠️ Verify full load, single-chain load, whole-policy save, per-chain save, and concurrent-edit conflict on one chain (integration suite). **Blocked**: needs Java client chain support (Task 6); verify as part of Task 6.

### Task 5: Access control enforcement

**Files:**
- Modify: `src/server/core/epp.cpp` / `src/server/core/session.cpp`
- Modify: `src/server/core/` user-rights helpers as needed

- [x] Load `policy_chain_acl` into the chain registry (done in Task 2); compute a caller's effective rights per chain — `EPP_CHAIN_ACCESS_READ`/`EPP_CHAIN_ACCESS_EDIT` in `include/nxcldefs.h`, `EventPolicyChain::getUserRights` (explicit user entry wins, else OR of group entries via `CheckUserMembership`), `EventProcessingPolicy::getEffectiveChainRights[ByGuid]`. The global EPP right grants Read+Edit on all chains and is checked by callers, not folded into these helpers.
- [x] Gate load: `fillLoadResponse`/`sendToClient` take (userId, globalRights); registry and rule stream filtered to chains with Read; effective rights sent at registry slot +5; a user without the global right can open the EPP if they can read at least one chain; the main chain (rules and single-chain load) requires the global right. Chain create/modify/delete stays gated by the global right (Task 4).
- [x] Enforce per-chain save scope: `saveEventProcessingPolicy` requires the global right or chain-level Edit on **every** chain in the save scope (main chain in scope → global right only); denied saves are rejected before rule upload starts.
- [x] Enforce the call-chain constraint in `finishEPPSave`: without the global right, every uploaded rule's chain-call target must be a chain the user has Read on (Edit on the containing chain is already guaranteed by the save-scope gate).
- [x] Add chain-ACL (Read/Edit) editing to the chain-management command (done in Task 4 via `VID_ACL_SIZE`/`VID_ACL_USER_BASE`/`VID_ACL_RIGHTS_BASE`).
- [ ] ⚠️ Manually verify a chain-scoped (non-admin) user can load+save only permitted chains, cannot see others, and cannot create chains. **Blocked**: needs client chain support (Tasks 6-7); verify then.

### Task 6: Java client library

**Files:**
- Modify: `src/client/java/netxms-client/.../events/EventProcessingPolicy.java`
- Modify: `src/client/java/netxms-client/.../events/EventProcessingPolicyRule.java`
- Create: `src/client/java/netxms-client/.../events/EventProcessingPolicyChain.java`
- Modify: `src/client/java/netxms-client/.../NXCSession.java`

- [x] Add `EventProcessingPolicyChain` (id, guid, name, description, version, effective rights + `ACCESS_READ`/`ACCESS_EDIT` constants) and a chains list on `EventProcessingPolicy` (`addChain`/`getChains`/`findChain(int|UUID)`, plus `getRules(chainId)` view).
- [x] Add `chainId` + chain-call list fields to `EventProcessingPolicyRule`; `fillMessage`, from-message ctor, and copy ctor updated; setters mark the rule modified. `DeletedRuleInfo` now carries the chain id so single-chain saves send/clear only that chain's deletions.
- [x] Update `getEventProcessingPolicy` to read the registry header; added `getEventProcessingPolicy(Integer chainId)` single-chain load.
- [x] Update `saveEventProcessingPolicy(epp, chainId)`: sends that chain's rules, deleted-rule info, and base version; the no-argument overload saves the main chain. Response updates the saved chain's version and rule versions. (`EPPConflict` unchanged — conflicts map to chains via rule GUID.)
- [x] Add client methods for chain management: `createEppChain(name, description, acl)`, `modifyEppChain(chainId, name, description, acl)` (null ACL = keep), `deleteEppChain(chainId)`.
- [x] Build `netxms-base` and `netxms-client` (`mvn ... install`); `mvn clean` run afterward (per preferences).
- [x] ➕ `EppChainTest` integration suite written (deferred here from Task 3): traversal order + return-to-caller, stop-in-sub-chain halts globally, diamond-reuse-runs-twice, loop detection with `SYS_EPP_CHAIN_LOOP` reported once per calling rule (alarm repeat count stays 1 across two events), and chain create/load/modify/callers/delete round-trip including call removal. Rules trace execution via `ReadPersistentStorage`/`WritePersistentStorage` action scripts. Compiles; **run against a live server pending** (also covers the blocked verify items of Tasks 3-5).

### Task 7: UI — tabbed editor + chain management

**Files:**
- Modify: `src/client/nxmc/.../events/views/EventProcessingPolicyEditor.java`
- Modify: `src/client/nxmc/.../events/widgets/RuleEditor.java`
- Create: chain-management dialogs / property pages under `src/client/nxmc/.../events/`
- Modify: `src/client/nxmc/.../events/dialogs/EPPConflictDialog.java`

- [x] Convert the editor to a tabbed layout (`CTabFolder`): one tab per readable chain, main chain first (shown only to users with the global EPP right), sub-chains sorted by name; per-tab `ChainTab` holds its own scroller, `RuleEditor` stack, selection, and last-selected index.
- [x] Render a rule's chain calls in the action area ("Continue processing in chains:"); double-click on a chain name jumps to its tab; chain tab tooltip shows description + "called by N rule(s)". Added `RuleChainCalls` property page (ordered list with Add/Delete/Up/Down; add offers readable chains excluding the rule's own chain and duplicates). ➕ Chain ACLs are now sent to global-right callers on full load (`VID_CHAIN_ACL_COUNT` 1037 / `VID_CHAIN_ACL_LIST_BASE` 0x47000000, stride 3) so the ACL editor has data to edit.
- [x] Per-chain rule numbering; insert/delete/cut/paste/move renumber loops all chain-scoped; flat policy list positions derived via per-chain insert-index mapping; paste retargets rules to the active chain.
- [x] Chain management UI: `EppChainPropertiesDialog` (name, description, ACL with Read/Edit rights per user/group) used for create and properties; delete confirms and removes the chain's rules. Read-only tabs: structural actions (add/insert/cut/paste/delete) disabled; a rule's property dialog can still be opened but a save is rejected server-side. All chain management gated by the global EPP right.
- [x] `savePolicy` saves dirty chains one at a time via per-chain save; conflicts stop the loop and open the conflict dialog.
- [x] `EPPConflictDialog`: added Chain column; rule numbers are now positions within the rule's chain.
- [x] Desktop (`-Pdesktop`) and web (`-Pweb`) profiles compile. ⚠️ Manual SWT/RWT rendering verification still pending (needs running server + console session).

### Task 8: Export / import

**Files:**
- Modify: `src/server/core/epp.cpp` (`createExportRecord`, `toJson`, import path)
- Modify: corresponding import/export handling (config export module)

- [x] Include chain data in exports: `EPRule::toJson` gains `chainId` + `chainCalls` (GUID array); export records for sub-chain rules carry the owning chain **by GUID** (`chain` field, added in `exportRule` — numeric chain ids are local to a server); the export container gains a `chains` registry (guid, name, description) covering every chain referenced by the exported rules as owner or call target (`exportChains`); `exportRule`/`exportRuleOrdering` now cover sub-chain rules. `EventProcessingPolicy::toJson` (audit) includes the chain registry and sub-chain rules.
- [x] Import: registry entries are imported first (`importChain` — existing GUID reuses the chain, otherwise it is created); a rule's owning chain is resolved by GUID (unknown chain → warning + import into the main chain); unresolved chain-call targets produce a warning and are kept (skipped at runtime until such a chain exists). `importRule` is chain-aware (find/replace/ordering scoped to the owning chain's rule list).
- [x] ➕ `getRuleDetails` / `getRuleAsJson` now find rules in any chain via shared `findRuleByGuid` (AI explain and JSON access for sub-chain rules).
- [ ] ⚠️ Manually verify export→import on a second system preserves chains and reports missing targets (needs two live servers).

### Task 9: Verify acceptance criteria

- [ ] ⚠️ Upgraded system with only the main chain behaves identically to pre-change (no regression). SQLite migration round-trip verified (Task 1); behavioral no-regression check needs a live server run of the existing `Epp*Test` suite.
- [ ] ⚠️ A rule can enter multiple chains in order; return-vs-terminate honors the calling rule's stop flag. Covered by `EppChainTest.testTraversalAndReturn` / `testStopInSubChain` — needs a live server run.
- [ ] ⚠️ Loop on the active call stack is detected, skipped, and reported by `SYS_EPP_CHAIN_LOOP` once per calling rule within 24 hours; diamond reuse runs the shared chain twice. Covered by `EppChainTest.testLoopDetection` / `testDiamondReuse` — needs a live server run.
- [ ] ⚠️ Per-chain load/save works; a chain-scoped user is correctly limited. Load/save/management covered by `EppChainTest.testChainManagement`; chain-scoped user limits are a manual check (Task 5 note) — needs a live server run.
- [x] Run full C++ test suite: `./tests/suite/netxms-test-suite` — all tests pass, zero failures (2026-07-07).

### Task 10: Documentation + cleanup

- [x] Update `doc/internal/debug_tags.txt` if a new tag was added — no new tag; execution engine reuses `event.policy`.
- [x] Note the new tables in any data-dictionary source if maintained in-repo — the data dictionary is maintained externally (netxms.org); event 173 falls within the documented 0-499 system range in `doc/internal/event_code_ranges.txt`.
- [x] Update relevant component CLAUDE.md notes if new patterns were introduced — no new patterns; existing conventions (per-chain container, `nx_swprintf`, ACL pattern) were followed.
- [ ] Move this plan to `docs/plans/completed/` when done — pending the live-server verification runs above.

### Task 11: Review follow-ups ➕

- [x] ➕ Loop reporting: `SYS_EPP_CHAIN_LOOP` is posted by the calling rule (calling chain, target chain, rule number, rule GUID) at most once per 24 hours per chain call (`EPRuleChainCall::lastLoopReport`, in-memory only).
- [x] ➕ One chain per save request; `EppChainTest` saves each chain separately (see Task 4 / Task 6).
- [x] ➕ Chain create/modify/delete do database work outside the policy lock. Delete removes calls to the chain from every rule and bumps the version of each chain that lost a call; nxmc lists the calling rules (`CMD_GET_EPP_CHAIN_CALLERS`) in the delete confirmation.
- [x] ➕ Rule diagnostics carry the owning chain name: script names `EPP::Filter::<chain>::<n>` / `EPP::Action::<chain>::<n>`, script error reports `EPP::<chain>::<n>`, log messages "rule #n in chain X". `EPRule::m_chainName` is set on load, save, import, and chain rename. Event processing metadata records `chainId` per matched rule and `chain-call` / `chain-call-loop` effects; rule details for the AI explanation include `chainName` and `calledChains`.
- [x] ➕ AI tools work on any chain: `replaceAllRules(chainId, …)` replaces and saves one chain against that chain's version; rule tools find a rule by GUID in whichever chain holds it, `create-epp-rule` takes `chain` (name or GUID), rules accept `chain_calls` by name or GUID; policy JSON carries `version` per chain.
- [x] ➕ Session notifications: `NX_NOTIFY_EPP_RULES_CHANGED` (data = chain id) after every rule save of a chain (NXCP, AI tools, WebAPI, import, call removal on chain delete); `NX_NOTIFY_EPP_CHAIN_UPDATED` / `NX_NOTIFY_EPP_CHAIN_DELETED` (data = chain id) on chain create/modify/import/delete. Sent from the policy methods, so every entry point is covered. nxmc reloads a chain without local changes in place (`getEventProcessingPolicy(chainId)` + `EventProcessingPolicy.replaceChainRules`), warns when the changed chain has unsaved edits (merge happens on save), and reloads the policy or warns on chain registry changes; notifications caused by the editor's own round-trips are ignored (`localChangesInProgress`). Import bumps the version of every chain it changes.
- [x] ➕ WebAPI chains: `GET/POST /v1/event-processing-policy/chains`, `GET/PUT/DELETE .../chains/{chain-id}` (numeric id or GUID; PUT = name/description/access list), `PUT .../chains/{chain-id}/rules` (per-chain rule replacement with version check), `GET .../chains/{chain-id}/callers`. `PUT /v1/event-processing-policy` replaces the main chain only. Chain JSON (`EventPolicyChain::toJson`) carries `accessList`; `UpdateEventProcessingPolicyFromJson` takes a chain id. Documented in `openapi.yaml` (chain schemas, `chainId` / `chainCalls` on rules). All chain endpoints require the global EPP right.
- [x] ➕ Export/import: rule selection in nxmc (`RuleSelectionDialog`, export file builder) shows the owning chain, sorts main chain first, and filters by chain name; import with "replace existing rules" also updates name and description of an existing chain with the same GUID. Chain ACLs are not exported (user ids are local to a server).
- [x] ➕ Chain identity: chains are linked and addressed by numeric `chain_id` everywhere inside the server, in NXCP, in the Java client, in the AI tools, and in WebAPI (`policy_chain_call_list.target_chain_id`, `EPRuleChainCall::targetChainId`, `EventProcessingPolicyRule.chainCalls` as `List<Integer>`); the chain GUID is used only in configuration export files, where import maps it back to a local id (calls to chains unknown on the importing server are dropped with a warning). The main chain is an `EventPolicyChain` with id 0 in the server's chain map, so every policy method works on one uniform collection; its name is never displayed from the server data - nxmc localizes "Main" by id 0, and the callers list carries only chain ids.
- [x] ➕ Editor ergonomics: rule editors label chain calls "Call chains"; the rule picker for export is a tree grouped by chain (collapsible, selecting a chain selects its rules, filter by rule name or chain name); the chain tab folder is styled like the object view tabs, with a chain toolbar (new, properties, delete) to its left; a chain with unsaved changes shows its tab as "*name" in italics; "Save" (Ctrl+S) saves the active chain and is enabled only when it has changes, "Save all" saves every changed chain one at a time and stops at the first conflict; tab content is re-laid out on tab activation.
- [x] ➕ Validation and tests: a rule save over NXCP is rejected with `RCC_INVALID_ARGUMENT` when a rule calls a chain that does not exist (the target may have been deleted between load and save); chain names need not be unique; empty chain name, rename or delete of the main chain and operations on an unknown chain return `RCC_INVALID_ARGUMENT`. Integration tests: `EppConflictTest` (every branch of `saveWithMerge`), `EppChainAccessTest` (registry filtering, per-chain load/save scope, chain-call read constraint, management gated by the global right, ACL changes, group rights with explicit user entry taking precedence), `EppChainExportImportTest` (export format with chain GUIDs and chain registry, import recreating chains, reuse of existing chains with and without replace, unknown owning chain and unknown call target), `EppChainTest.testFullLoadAndMultiCallerDelete` and `testChainManagementValidation`.
- [x] ➕ Per-chain access rights apply to every entry point, not only NXCP: `EventProcessingPolicy::getUserRightsOnChain`, `hasAnyChainAccess`, `checkChainEditAccess` and `checkChainCallAccess` are the single implementation of the rules (global EPP right grants everything; otherwise chain ACL read/edit, main chain requires the global right, a rule may only call readable chains, calls to unknown chains are rejected), used by `ClientSession::saveEventProcessingPolicy` / `finishEPPSave`, by `UpdateEventProcessingPolicyFromJson` (WebAPI `PUT .../chains/{id}/rules` → 403) and by the AI tools. JSON views are user-filtered: `toJson(userId)`, `getChainsAsJson(userId)`, `getChainAsJson(chainId, userId, &rcc)`, `getRuleAsJson(guid, userId, &rcc)` return only readable chains, add `effectiveRights` and strip `accessList` for users without the global right. Chain create / modify / delete and the callers list stay gated by the global right.
- [x] ➕ Per-chain loading: `CMD_GET_EPP` without a chain id returns only the chain registry (id, GUID, name, description, version, rights, rule count, caller count, ACLs for global users); with a chain id it returns that chain's registry entry and rules. `NXCSession.getEventProcessingPolicyChains()` returns the registry, `getEventProcessingPolicyChain(id)` a loaded chain, `saveEventProcessingPolicy(chain)` saves it; nxmc loads a tab's rules on first activation and reloads single chains on notifications. WebAPI has no whole-policy endpoint: `GET /chains` is the registry, `GET /chains/{id}` a chain with rules (id 0 = main), `PUT /chains/{id}/rules` replaces rules of any chain, `PUT`/`DELETE /chains/{id}` refuse id 0 with 405.

## Resolved design decisions (from issue discussion)

A. **Stop-processing halts the entire event's processing** (global), inside sub-chains too. A separate "exit chain" flag (return to caller without halting) may be added later — out of scope here.

B. Per-chain rights are **Read + Edit** only. Creating a new chain requires the **global EPP edit** right (no per-chain "create").

C. No new "access all chains" right — the **existing global EPP edit** right is the umbrella. A user can access a chain if they have the global EPP edit right **or** chain-level access.

D. Creators always have access automatically: per B only a global-EPP-edit user can create a chain, and per C that right already grants access to all chains.

E. A non-global user can **view/read** chains they have Read on, and add a **"call chain X"** action — where X is a chain they have Read on — inside a chain they have Edit on. They cannot create chains.

## Post-Completion

*Items requiring manual intervention or external systems — informational only.*

**Manual verification:**
- nxmc desktop (SWT) and web (RWT): tab rendering, chain-call navigation, "used by N", read-only chains, per-chain save, conflict dialog.
- NXCP round-trips: full load, single-chain load, per-chain save, chain create/rename/delete with call removal.
- Migration: upgrade a populated pre-change DB and confirm zero behavioral change.
- Export/import across two systems, including the missing-target-chain warning path.
- Loop behavior under load: confirm `SYS_EPP_CHAIN_LOOP` is posted once per calling rule per 24 hours while the loop stays in place.

**External / follow-up:**
- Documentation site (admin guide / data dictionary) updates outside this repo, if applicable.
- Possible future enhancement: a separate "exit chain" flag for chain-local exit (resolved question A).
