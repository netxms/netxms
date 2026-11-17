# nxdbmgr check: PR #3701 second review fixes

## Overview
- Address the second review of PR #3701 (review 5864605988 on dd0009273d, 2026-11-11, six inline
  findings) for the `nxdbmgr check` referential integrity work
  (`docs/plans/completed/20260924-nxdbmgr-check-integrity.md`, first review round in
  `docs/plans/completed/20261108-nxdbmgr-check-review-fixes.md`).
- Findings 1, 2, 4 and 5 are fixed here; 3 and 6 are answered on the PR without code changes. The
  triage was agreed with Codex on 2026-11-17, before this plan.
- Two backlog items this PR added are resolved and deleted:
  `docs/backlog/nxdbmgr-check-unlinked-node-after-orphan-cleanup.md` (finding 2) and
  `docs/backlog/nxdbmgr-check-raw-sql-stages-bypass-abort.md` (finding 5).

## Context (from discovery)
- `src/server/tools/nxdbmgr/check.cpp`: `s_checkAborted`, `CheckSelect()` / `CheckQuery()`,
  `CollectObjectIdentifiers()`, `GetDataCollectionTargets()`, `CheckMissingObjectProperties()`,
  `s_objectClassTables[]`, `s_containerExtensionTables[]`, `CheckObjectClassCoverage()`,
  `IsDataTableExist()`, `CheckDataTables()`, `CheckEventCodeReferences()`, `s_orphanRelations[]`,
  `OrphanStage`, `CheckOrphanRelation()`, `CheckOrphanRelations()`, `CheckEppRelations()`,
  `s_checkStages[]`, `CheckDatabase()`
- `src/server/tools/libnxdbmgr/confirm.cpp`: `s_yesForAll` / `s_noForAll` persist until
  `ResetBulkYesNo()`; `src/server/tools/libnxdbmgr/check.cpp`: `StartStage()` / `EndStage()` reset
  them, nothing else in the grouped stages does
- `src/db/libnxdb/session.cpp`: `DBGetPerfCounters()` (`failedQueries`, process-wide, counts final
  failures of `DBQueryEx`, `DBSelectEx`, `DBPrepareEx`, `DBExecuteEx`, prepared and unbuffered
  selects); `DBIsTableExist()` calls the driver directly and is not counted; automatic reconnect
  (`m_reconnectEnabled`, default true, `DBEnableReconnect()`) retries a statement on a fresh
  connection that is outside the open transaction, and a successful retry counts no failure
- `src/server/tools/nxdbmgr/CLAUDE.md` documents the `CheckSelect()` / `CheckQuery()` abort rule
- Harness: `tests/nxdbmgr-check/` (fixture rules in `src/server/tools/nxdbmgr/CLAUDE.md`); every run
  is `-f`, so bulk answers and SQL failures are verified in scratch runs, not in the harness

## Development Approach
- **testing approach**: regular; every code task updates harness fixtures where the harness can
  express the case, and records the scratch verification it ran where it cannot
- every task: `make -j && make install` in this worktree (`.claude/worktrees/nxdbmgr-check-build`,
  on branch `nxdbmgr-check-integrity`, configured with `--prefix=/Users/alk/netxms/nxdbmgr-check`),
  then `tests/nxdbmgr-check/run.sh -b /Users/alk/netxms/nxdbmgr-check/bin/nxdbmgr` (SQLite) and
  `PATH=/opt/homebrew/opt/postgresql@18/bin:$PATH tests/nxdbmgr-check/run.sh -d pgsql -U alk -b
  /Users/alk/netxms/nxdbmgr-check/bin/nxdbmgr`; both must pass
- ➕ run with `NETXMS_HOME=/Users/alk/netxms/nxdbmgr-check`: the shell exports `NETXMS_HOME` for
  the local test server, and `nxdbmgr init` then takes that prefix's (newer) schema; the branch's SQL
  is installed with `make -C sql install`. The full `make` stops in `src/server/aitools` (automake
  1.19 configured, 1.20 installed); nxdbmgr's libraries are built per directory (`src/libnetxms`,
  `src/db`, `src/libnxsl`, `src/snmp/libnxsnmp`, `src/agent/libnxagent`, `src/server/libnxsrv`,
  `src/server/tools`)
- the main tree's build and the `~/netxms/master` prefix belong to the local test server and are not
  touched

## Progress Tracking
- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix

## Solution Overview

1. **Bulk "All"/"Skip" leaks across relations** → fix. `ResetBulkYesNo()` before each unit that has
   its own prompt wording inside a multi-unit stage: each relation in `CheckOrphanRelation()`, each
   table in `CheckEppRelations()`, each class table in `CheckObjectClassCoverage()`, each reference
   in `CheckEventCodeReferences()`. "All" still applies to every missing parent of one relation,
   which is what the operator saw when answering.
2. **Orphan cleanup after the linkage checks** → fix. The three relations whose parent decides
   whether `CheckNodes` sees a node as linked (`container_members.container_id`, `nsmap.subnet_id`,
   `cluster_members.cluster_id`) move into a new `OrphanStage::Membership` group that runs after
   `CheckDuplicateObjectGuids` and before `CheckNodes`. They move, not duplicate: run twice, a row the
   operator declined early would be prompted and counted again at the end. Codex checked every later
   stage: no core stage after that point deletes a legitimate subnet, container or cluster
   (`CheckDuplicateSubnets` is report-only, `CheckComponents` runs for interfaces and network
   services only). Their `zeroAllowed` becomes false: 0 is never a container, subnet or cluster, and
   `CheckNodes` currently counts such a row as linkage. Coverage runs first, so a parent whose
   properties row is recreated is not reported as missing.
3. **Rollback after failed commit** → not fixing, reply on the PR. `DBCommit()` decrements the level
   before calling the driver, so a caller `DBRollback()` returns false without sending anything. The
   underlying problem (native transaction left open after a failed `COMMIT`, reproduced on SQLite)
   is real and lives in libnxdb. The backlog item gets Codex's driver audit: MSSQL, ODBC, DB2 and
   Informix `Commit` turn autocommit on after `SQLEndTran(COMMIT)` whatever its result, so a
   library-level rollback after a failed commit is already too late there.
4. **Double count of objects with an extension table** → fix. The `object_containers` coverage
   query skips a row only when the matching extension row exists (its ID is in one of the
   `s_containerExtensionTables[]` tables); an `object_containers` row of an extension class without
   its extension row stays reported as a container. Excluding by class alone would hide an object
   that has neither row.
5. **Raw SQL stages bypass the abort** → fix with one predicate instead of converting the ~113 call
   sites. `IsCheckAborted()` compares `DBGetPerfCounters().failedQueries` with a baseline taken
   after `DBBegin()` and also reads a sticky flag for failures the counter does not see:
   `DBIsTableExist()` returning `DBIsTableExist_Failure` in `CollectObjectIdentifiers()` and
   `IsDataTableExist()`, `DBGetTableList()` returning `nullptr`, and `CheckModuleSchemas()` returning
   false. Every `!s_checkAborted` guard reads the predicate; `CheckSelect()` / `CheckQuery()` stop
   writing a flag and only return early when the check is already aborted. Automatic reconnect is
   disabled from before `DBBegin()` until the transaction ends and re-enabled before
   `RemoveDatabaseLock()`, so a lost connection fails the statement instead of silently continuing
   outside the transaction. `CheckDataTables()` returns before creating or dropping tables when the
   target collection or a table existence check failed, and its second stage does not start after an
   abort; the optional collected-data stages are guarded one by one.
   ⚠️ Limitation, agreed with Codex: `EndStage()` display is not changed, so a stage that hit a
   failure may still print PASSED. The guarantee is about the final result only: the final "SQL
   error during check, rolling back changes" and "Database check aborted" are authoritative, and
   the PR reply must not claim every failed stage displays ERROR. Changing `EndStage` would need an
   exported predicate in libnxdbmgr, which also serves the upgrade commands.
6. **Grouped orphan query scope and cost** → not fixing, reply on the PR. For a fixed database
   state the DELETE/UPDATE predicate selects exactly the counted rows (`childFilter` is applied to
   both). `software_inventory`, `hardware_inventory` and `downtime_log` have the child key as the
   leading primary key column, `maintenance_journal` has `idx_maintjrn_object_id`. Progress per
   relation is cosmetic.

## Tandem

| Task | Writer | Reviewer |
|------|--------|----------|
| 1 | claude | codex |
| 2 | claude | codex |
| 3 | claude | codex |
| 4 | claude | codex |
| 5 | codex | claude |
| 6 | claude | codex |
| 7 | claude | codex |

## Implementation Steps

### Task 1: Bulk answer per relation, table and class

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`

- [x] `ResetBulkYesNo()` at the start of each unit in `CheckOrphanRelation()`,
      `CheckEppRelations()`, `CheckObjectClassCoverage()`, `CheckEventCodeReferences()`; check the
      other hand-written stages for more than one prompt wording per stage and cover them the same way
- [x] scratch verification on SQLite, setup, scripted input and assertions recorded in the commit
      message body: ACL rows for two missing object IDs followed by another "Object references"
      relation with a defect; answers All, then No; row assertions show both ACL parents' rows
      deleted and the next relation's rows kept (the prompt text is printed for auto-answers too, so
      only database state proves it). Second case: Skip on the first ACL prompt keeps both ACL
      parents' rows and the next relation still asks
- [x] build, harness on SQLite and PostgreSQL green
- [x] ➕ older master stages that mix prompt kinds in one stage (template mapping, asset links,
      IDATA/TDATA, business service tickets) are out of this review's scope:
      `docs/backlog/nxdbmgr-check-bulk-answer-mixed-prompt-stages.md`

### Task 2: Membership parents checked before CheckNodes

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`, `src/server/tools/nxdbmgr/CLAUDE.md` (stage list)
- Modify: `tests/nxdbmgr-check/inject-defects.sql`, `tests/nxdbmgr-check/expected-first-run.txt`,
  `tests/nxdbmgr-check/assert-after-first-run.sql`, `tests/nxdbmgr-check/unexpected-first-run.txt`
  as needed
- Delete: `docs/backlog/nxdbmgr-check-unlinked-node-after-orphan-cleanup.md`

- [x] new `OrphanStage::Membership` group with the three relations, `zeroAllowed` false, run as its
      own stage after `CheckDuplicateObjectGuids` in `s_checkStages[]`
- [x] harness: the review's example, a node without interfaces whose only `nsmap` row points to a
      non-existing subnet; the first run deletes the `nsmap` row and reports the node as unlinked in
      the same run (with `-f` it is deleted), so the final `-E` run stays clean; a
      `container_members` row with `container_id=0` and an `nsmap` row with `subnet_id=0` are deleted
- [x] delete the backlog item
- [x] build, harness on SQLite and PostgreSQL green

### Task 3: Coverage counts an object with an extension table once

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`
- Modify: harness fixtures as needed

- [x] the `object_containers` coverage query excludes rows whose ID has a row in the extension
      table of its class (`s_containerExtensionTables[]`); other `object_containers` rows unchanged
- [x] harness: a class-32 `object_containers` row without a `racks` row and without properties is
      still reported as a container (the forced harness cannot show the double count: the rack
      repair inserts the properties row before `object_containers` is checked)
- [x] scratch verification on SQLite without `-f`, recorded in the commit message body: a rack with
      `racks` and `object_containers` rows and no properties row, the rack repair declined; exactly
      one coverage finding ("Missing rack object") and no container prompt for that ID (the unfixed
      binary prints one of each)
- [x] build, harness on SQLite and PostgreSQL green
- [x] ➕ assertion 160 ("only one lost_ properties row") excludes the new fixture 999982

### Task 4: One abort predicate for every check stage

**Files:**
- Modify: `src/server/tools/nxdbmgr/check.cpp`, `src/server/tools/nxdbmgr/CLAUDE.md`
- Delete: `docs/backlog/nxdbmgr-check-raw-sql-stages-bypass-abort.md`

- [x] `IsCheckAborted()`: failed-query baseline taken after `DBBegin()` plus a sticky flag; every
      `!s_checkAborted` guard (stage loop, inner loops, `CheckDatabase()` result) uses it;
      `CheckSelect()` / `CheckQuery()` only return early on an aborted check
- [x] sticky flag set on `DBIsTableExist_Failure` in `CollectObjectIdentifiers()` and
      `IsDataTableExist()`, on `DBGetTableList()` returning `nullptr` in `CheckDataTables()`, and on
      `CheckModuleSchemas()` returning false; the existing behaviour of these helpers for other
      nxdbmgr commands is unchanged
- [x] `DBEnableReconnect(g_dbHandle, false)` before `DBBegin()`, restored before
      `RemoveDatabaseLock()` on every exit path
- [x] `CheckDataTables()`: return after target collection and before any table check when the check
      is aborted; the orphaned data tables stage does not start after an abort; each optional
      collected-data stage in `CheckDatabase()` is skipped after an abort; data-tables-only mode
      goes through the same predicate
- [x] `nxdbmgr/CLAUDE.md`: the abort rule describes the predicate, not `CheckSelect()` only
- [x] scratch verification on SQLite, setup and assertions recorded in the commit message body:
      (a) raw-SQL stage failure: `object_properties.is_deleted` renamed on a clean database (the
      first read of it is the raw `CheckNodes` query); the run prints the rollback message and
      "Database check aborted", no stage after `CheckNodes` starts, and neither "Database doesn't
      contain any errors" nor "Database check completed" appears; (b) failed target-class SELECT:
      a class table's `id` column renamed, run with `check-data-tables` (a normal check fails earlier
      in class coverage); query trace (`-t`) and table list show no CREATE or DROP, the orphaned data
      tables stage does not start, the result is aborted
- [x] delete the backlog item
- [x] build, harness on SQLite and PostgreSQL green
- [x] ➕ the abort message is "Error during check, rolling back changes..." (was "SQL error during
      check"): a failed module check is not an SQL error
- [x] ➕ scratch case (b) renames `nodes.id` rather than a class table without rows, so the
      incomplete target list would hit the baseline nodes' existing data tables

### Task 5: DBCommit backlog item gets the driver audit

**Files:**
- Modify: `docs/backlog/dbcommit-failure-leaves-native-transaction-open.md`

- [x] add: MSSQL, ODBC, DB2 and Informix `Commit` enable autocommit after `SQLEndTran(COMMIT)`
      regardless of its result, Oracle keeps its native level after a failed `OCITransCommit`,
      SQLite and MySQL run `COMMIT` as SQL; the fix contract (native rollback before the failed
      outer commit returns, connection treated as unusable when that rollback fails) and that a
      library-only patch does not give uniform recovery

### Task 6: Verify and draft the PR reply

- [x] harness on SQLite and PostgreSQL green at the branch tip
- [x] draft the reply to review 5864605988 (one paragraph per finding, the fixing commit for 1, 2,
      4, 5; the reasons for 3 and 6) into the final report for the user; nothing is posted or pushed
- [x] ➕ revmux round `pr-3701/02-second-review` (comprehensive, 4/4 sources): 1 major (a declined
      IDATA/TDATA create aborted a `-d` run) and 3 minor (redundant guards and a wrong comment in
      `CheckDataTables`, `CheckNodes` missing from the mixed-prompt backlog item, reply overstating
      which loops test the abort), all fixed and squashed into tasks 1, 4 and 6

### Task 7: [Final] Move the plan

- [x] move this plan to `docs/plans/completed/`

## PR reply draft

Not posted. The branch was squashed into one review-fix commit after this was written; the first
line takes that commit's ID when posting.

> Addressed in <review-fix commit>.
>
> 1. Fixed. The bulk answer is reset before each relation, EPP rule table, class table
>    and event code reference, so "All" covers the missing parents of one relation only. Older
>    master stages that ask two different questions in one stage have the same issue, worst in
>    `CheckNodes`, where "All" on a "Link?" prompt also deletes later nodes that cannot be relinked;
>    also template mapping, asset links, IDATA/TDATA and DCI deletion. That is outside this PR and
>    is in `docs/backlog/nxdbmgr-check-bulk-answer-mixed-prompt-stages.md`.
> 2. Fixed. `container_members.container_id`, `nsmap.subnet_id` and
>    `cluster_members.cluster_id` run as their own stage after class coverage and before
>    `CheckNodes`, so your example ends in the same run with the node reported as unlinked. They
>    also no longer accept 0 as "not set". The backlog item is gone.
> 3. Not changed, same reason as last round: `DBCommit()` lowers the transaction level before it
>    calls the driver, so a `DBRollback()` after a failed commit returns false without sending
>    anything. The open native transaction is real, but it is a libnxdb/driver problem: the
>    MSSQL, ODBC, DB2 and Informix drivers turn autocommit back on after a failed
>    `SQLEndTran(COMMIT)` before control returns to the caller. Details in
>    `docs/backlog/dbcommit-failure-leaves-native-transaction-open.md`.
> 4. Fixed. The `object_containers` coverage query skips a row when its extension
>    row (racks, dashboards, ...) exists. A container row of such a class without its extension
>    row is still reported, so an object that has neither row is not hidden.
> 5. Fixed without converting every call site. `IsCheckAborted()` compares libnxdb's
>    failed query counter with its value at `DBBegin()`, plus a flag for failures the counter does
>    not see (table existence checks, table list, module checks). The stage runner, the converted
>    stage loops and the final commit decision use it. An older stage such as `CheckNodes` finishes
>    its current loop after a failure, but the failure is detected without relying on a poisoned
>    transaction: the check takes the rollback path and ends with "Database check aborted"
>    (verified on SQLite and PostgreSQL). Automatic reconnect is off during the check,
>    since a reconnect would continue outside the transaction without counting a failure.
>    `CheckDataTables` stops before any CREATE or DROP once a lookup has failed, and the collected
>    data check (`-d`) skips data tables the operator declined to create instead of failing on them. Stage status lines
>    are not changed: a stage that lost a query may still print PASSED, the final result is what
>    counts.
> 6. Not changed. For a fixed database state the DELETE/UPDATE predicate selects exactly the
>    counted rows (the child filter is applied to both). The child keys are indexed:
>    `software_inventory`, `hardware_inventory` and `downtime_log` have them as the leading
>    primary key column, `maintenance_journal` has `idx_maintjrn_object_id`. The checks still scan
>    the child keys, and the stage progress does not move while one long relation query runs; I
>    left that as it is.
