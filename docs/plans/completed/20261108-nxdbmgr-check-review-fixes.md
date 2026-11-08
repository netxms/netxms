# nxdbmgr check: PR #3701 review fixes

## Overview
- Address the code review on PR #3701 (review comment at commit 3c7df67, seven findings) for the
  `nxdbmgr check` referential integrity work (`docs/plans/completed/20260924-nxdbmgr-check-integrity.md`).
- The branch was rebased onto master 9b26c5f79d before this plan was written. Master dropped
  `object_ai_data` in schema 70.53 (rows migrated to `ai_memory`) and added `CheckTrafficObservers`
  (c484139b0d); the rebase put that stage into `s_checkStages[]` unchanged. Until Task 1 lands the
  harness fails on the `object_ai_data` relation, which is finding 1.

## Context (from discovery)
- `src/server/tools/nxdbmgr/check.cpp`: `s_orphanRelations[]`, `CheckOrphanRelation()`,
  `CheckOrphanRelations()`, `CheckEppRelations()`, `CheckEPP()`, `s_checkStages[]`,
  `s_builtinObjectIds[]`, `CheckDuplicateObjectIds()`, `CheckTrafficObservers()`, `CheckDatabase()`
- `src/server/include/nms_objects.h` (`BUILTIN_OID_*`), `src/server/include/nxsrvapi.h` (included by
  both netxmsd and nxdbmgr through `nxdbmgr.h`)
- `src/server/core/ai_memory.cpp`: `DeleteAIMemoryForScope()` writes `scope_type` as `'0' + scope`,
  so object scope is `'2'` (the review says `'O'`, which is wrong; `MigrateObjectAIData()` in
  `upgrade_v70.cpp` also inserts `'2'`)
- `src/db/libnxdb/session.cpp`: `DBCommit()` decrements the transaction level before calling the
  driver, also on failure
- Harness: `tests/nxdbmgr-check/` (`baseline.sql`, `inject-defects.sql`, `expected-first-run.txt`,
  `unexpected-first-run.txt`, `assert-after-first-run.sql`, `cleanup-report-only.sql`,
  `assert-after-cleanup.sql`), fixture rules in `src/server/tools/nxdbmgr/CLAUDE.md`

## Development Approach
- **testing approach**: regular; every code task adds or updates harness fixtures
- every task: build and install into the test prefix, run `tests/nxdbmgr-check/run.sh -b <nxdbmgr>`
  (SQLite) and `run.sh -d pgsql -U alk -b <nxdbmgr>` (PostgreSQL 18); both must pass
- build: worktree `.claude/worktrees/nxdbmgr-check-build` (detached, configured with
  `--prefix=/Users/alk/netxms/nxdbmgr-check`), `git checkout --detach <sha> && make -j && make install`;
  binary `/Users/alk/netxms/nxdbmgr-check/bin/nxdbmgr`. The main tree's build and the
  `~/netxms/master` prefix belong to the local test server and are not touched.

## Progress Tracking
- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix

## Solution Overview

Finding by finding:

1. **`object_ai_data` relation** → fix. Replace it with `ai_memory.scope_id → object_properties`
   restricted to `scope_type='2'`. `OrphanRelation` gets a trailing `childFilter` member (SQL
   condition on the child table, `nullptr` for none) that `CheckOrphanRelation()` appends to the
   select and to the DELETE/UPDATE fix. Only the object scope is covered; user scope (`'1'`) is
   outside the review and the original plan.
2. **`dashboard_associations.dashboard_id → dashboards`** → fix. Parent becomes `object_properties`
   (any object class is accepted by `NetObj::addDashboard`, the client offers dashboards, dashboard
   templates and network maps). Built-in object skip applies automatically.
3. **Rollback after failed commit** → not fixing. `DBCommit()` decrements `m_transactionLevel`
   before the driver call, so after a failed commit the level is 0 and `DBRollback()` returns false
   without sending anything. On PostgreSQL a failed `COMMIT` ends the transaction server side
   (a commit of an aborted transaction is a rollback), so there `RemoveDatabaseLock()` already runs
   outside any transaction. The PR reply covers only that: the proposed call is ineffective, and the
   PostgreSQL case is fine.
   ⚠️ Codex found the general problem is real on SQLite: a `COMMIT` that hits `SQLITE_BUSY` fails but
   leaves the native transaction open while libnxdb has already dropped the level to 0, so the unlock
   UPDATE stays inside it and is rolled back on close (reproduced). That is libnxdb transaction state,
   outside this plan: `docs/backlog/dbcommit-failure-leaves-native-transaction-open.md`.
4. **`s_builtinObjectIds` mirror** → fix. Move the `BUILTIN_OID_*` block (1..9, without
   `BUILTIN_OID_CONTEXT_OBJECT`) from `nms_objects.h` to `nxsrvapi.h` and build
   `s_builtinObjectIds[]` from the macros. A range check (`id < 100`, the initial free object ID in
   `id.cpp`) was considered and rejected: ID 8 is not a built-in object and the harness relies on
   an ACL row for object 8 being reported.
5. **CheckEPP before the EPP orphan stages** → fix by running the `OrphanStage::EventPolicy` group
   and `CheckEppRelations` immediately before `CheckEPP`. The other orphan groups stay last: they
   need the deletions done by earlier stages (DCIs, objects) to be visible. `CheckOrphanRelations()`
   becomes per-group so the stage table can name each group.
6. **Wrapper functions** → fix. Capture-less lambdas in `s_checkStages[]` for
   `CheckComponents(...)` and the per-group orphan stages from finding 5.
7. **`CheckDuplicateObjectIds` per-duplicate queries** → fix. One query: class table union with
   table names, left join `object_containers` for the class, filtered by the duplicate ID subquery,
   ordered by ID; grouping done while iterating the result.

From the rebase:

8. **`CheckTrafficObservers` (from master)** → bring under the branch's mechanisms. Its two
   "object properties" sub-stages duplicate `CheckObjectClassCoverage` (both tables are in
   `s_objectClassTables[]`) and go. The plain references become `s_orphanRelations[]` entries:
   `traffic_observers.node_id → nodes` (reset, zero allowed), `observation_point_hosts.point_id →
   observation_points` (delete), `observation_point_hosts.node_id → nodes` (reset, zero allowed).
   The observation point owner check deletes an object (`observation_points` row plus its
   `object_properties` row), which an orphan relation cannot do, so it stays hand-written on
   `CheckSelect()` / `CheckQuery()` so a failure aborts the check.

## Tandem

| Task | Writer | Reviewer |
|------|--------|----------|
| 1 | claude | codex |
| 2 | claude | codex |
| 3 | claude | codex |
| 4 | codex | claude |
| 5 | codex | claude |
| 6 | codex | claude |
| 7 | claude | codex |
| 8 | claude | codex |

## Implementation Steps

### Task 1: Replace the object_ai_data relation with ai_memory object scope

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/inject-defects.sql`, `tests/nxdbmgr-check/expected-first-run.txt`,
  `tests/nxdbmgr-check/assert-after-first-run.sql`, `tests/nxdbmgr-check/baseline.sql`

- [x] add `childFilter` to `OrphanRelation`; `CheckOrphanRelation()` appends it to the select
      `WHERE` and to the delete/reset fix
- [x] replace the `object_ai_data` entry with `ai_memory.scope_id → object_properties`,
      `scope_type='2'`, delete rows
- [x] harness: injected object-scope `ai_memory` row for a missing object (deleted after first run),
      plus "must not be reported" rows: a user-scope (`'1'`) and an environment-scope (`'0'`) row
      with the same missing `scope_id` as the deleted object-scope row (so a DELETE without
      `childFilter` is caught), and an object-scope row for a baseline object
- [x] build, harness on SQLite and PostgreSQL green
- [x] ➕ fixture maintenance after the rebase: `event_policy` INSERTs get master's NOT NULL
      `alarm_flap_count` / `alarm_flap_window`
- [x] ➕ master's `CheckTrafficObservers` deleted the injected host row 999148 before the orphan
      relation saw it. Its host point check is removed and the relation
      `observation_point_hosts.point_id` now has parent `observation_points.id` with zero not
      allowed (it accepted any `object_properties` row before, agreed with Codex); fixtures: host
      rows naming node 990002 and point 0 are deleted, baseline observer 990093 / point 990094 with
      a host row are not reported

### Task 2: dashboard_associations.dashboard_id parent is any object

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: `tests/nxdbmgr-check/baseline.sql`, `tests/nxdbmgr-check/assert-after-first-run.sql`,
  `tests/nxdbmgr-check/unexpected-first-run.txt` as needed

- [x] change the `dashboard_associations.dashboard_id` entry's parent to `object_properties`
      (parent name "dashboard or network map" in the prompt)
- [x] harness: baseline association of a node with a network map (and a dashboard template) that
      must survive both runs and produce no prompt; existing injected defect (dashboard 999242)
      still deleted
- [x] build, harness on SQLite and PostgreSQL green

### Task 3: Event processing policy orphan stages run before CheckEPP

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: harness files as needed

- [x] split `CheckOrphanRelations()` into a per-group stage function; `s_checkStages[]` runs the
      `EventPolicy` group and `CheckEppRelations` right before `CheckEPP`, the remaining groups
      keep their place at the end
- [x] harness: a `policy_source_list` row with an invalid object in a non-existing chain (the
      review's example) and a `policy_event_list` row with an invalid event in a non-existing rule;
      expected output contains the orphan prompts, unexpected output contains CheckEPP's
      "last source inclusion" / "last event" messages for them; the first run ends with
      "All errors in database was fixed" for these rows (verify with the log, the existing run
      has report-only findings so the summary line itself is not asserted)
- [x] build, harness on SQLite and PostgreSQL green
- [x] ➕ the per-group stages in `s_checkStages[]` are capture-less lambdas already (no new wrapper
      functions to remove in Task 5); fixtures use a source row of missing chain 999306 and an event
      row of missing rule 999308, each the only entry of its rule

### Task 4: Built-in object IDs from a shared header

**Files:**
- Modify: `src/server/include/nms_objects.h`, `src/server/include/nxsrvapi.h`
- Modify: `src/server/tools/nxdbmgr/check.cpp`

- [x] move `BUILTIN_OID_NETWORK` .. `BUILTIN_OID_BUSINESSSERVICEROOT` to `nxsrvapi.h`
      (`BUILTIN_OID_CONTEXT_OBJECT` stays in `nms_objects.h`)
- [x] `s_builtinObjectIds[]` built from the macros; comment about the mirror removed
- [x] build netxmsd and nxdbmgr (both include the header), harness on SQLite and PostgreSQL green
      (existing built-in fixtures, rules 990087..990089 and ACL rows for objects 3, 4, 8, cover it)

### Task 5: Lambdas instead of stage wrapper functions

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`

- [x] remove `CheckInterfaces()` / `CheckNetworkServices()`, use capture-less lambdas in
      `s_checkStages[]`; same for any per-group orphan wrappers Task 3 introduced
- [x] build, harness on SQLite and PostgreSQL green (behaviour unchanged, stage output identical)

### Task 6: Duplicate object IDs in one query

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`

- [x] rewrite `CheckDuplicateObjectIds()` as one query (union with table names, left join
      `object_containers`, `WHERE u.id IN (... HAVING count(*) > 1) ORDER BY u.id`), grouping rows
      per ID in the loop; reported messages unchanged
- [x] build, harness on SQLite and PostgreSQL green (existing duplicate ID and container extension
      fixtures cover reported and not-reported cases)
- [x] ➕ preserve diagnostic table order with an ordinal in the named union; extra SQLite scratch
      validation covers a duplicate without a container row and a three-table duplicate alongside
      a valid dashboard/container extension pair, then verifies a clean run after removing them

### Task 7: Traffic observer checks under the generic mechanisms

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: harness files

- [x] drop the two object properties sub-stages from `CheckTrafficObservers()`
- [x] add `s_orphanRelations[]` entries for `traffic_observers.node_id` and
      `observation_point_hosts.node_id`; remove the hand-written versions
      (`observation_point_hosts.point_id` was corrected and its hand-written check removed in Task 1)
- [x] ➕ rebase integration: `device_configs.node_id` and `agent_config_cache.node_id` → `nodes`
      (delete rows; both are node-owned in `Node::deleteFromDatabase`), with injected defects,
      preserved rows for a valid node, output fragments and state assertions
- [x] observation point owner check on `CheckSelect()` / `CheckQuery()`; host rows of a deleted
      point are left to the orphan relation
- [x] harness: one injected defect per reference (observer with missing analyzer node, point with
      missing observer, host row with missing node), using baseline observer 990093 and point 990094
      from Task 1 as the must-not-be-reported side, state assertions after the first run
- [x] build, harness on SQLite and PostgreSQL green
- [x] ➕ the remaining hand-written stage is renamed `CheckObservationPointOwners()` (one LEFT JOIN
      query instead of a per-row lookup); the four new relations use `nodes` as parent, like master's
      traffic observer checks, not `object_properties` like the older node-owned relations;
      `device_configs` / `agent_config_cache` do not treat zero as "not set"

### Task 8: Close the plan

**Files:**
- Move: `docs/plans/20261108-nxdbmgr-check-review-fixes.md` → `docs/plans/completed/`

- [x] move this plan to `docs/plans/completed/`

## Branch review (revmux)
- round `pr-3701/01-initial`, profile comprehensive, 4/4 sources: three minor findings, two fixed
  (unexpected event-code fragments could never match; `run_check` ignored "SQL query failed"),
  folded into the first feature commit; one backlogged
  (`docs/backlog/nxdbmgr-check-unlinked-node-after-orphan-cleanup.md`); the pre-existing raw SQL
  stages backlogged (`docs/backlog/nxdbmgr-check-raw-sql-stages-bypass-abort.md`). Triage agreed
  with Codex.

## Post-Completion
- reply on PR #3701 to each finding (3 with the reasoning above), update the PR description for
  the rebase, the ai_memory relation and the traffic observer stage; force-push the rebased branch
  (Alex pushes)
