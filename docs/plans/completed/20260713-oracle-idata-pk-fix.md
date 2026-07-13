# Fix Oracle idata table creation typo (issue #3411)

## Overview

On Oracle, creating any new data collection target fails with `ORA-00904: "TDATA_TIMESTAMP": invalid identifier`.

Two 6.0 upgrade procedures write the `IDataTableCreationCommand` row in `metadata` with a typo in the primary key — `PRIMARY KEY(item_id,tdata_timestamp)` instead of `idata_timestamp`:

- `src/server/tools/nxdbmgr/upgrade_v60.cpp:151` — `H_UpgradeFromV30` (60.30 → 60.31)
- `src/server/tools/nxdbmgr/upgrade_v60.cpp:1471` — `H_UpgradeFromV13` (60.13 → 60.14, Oracle-only block)

6.0 and 6.1 read that template from `metadata` at runtime (`src/server/core/objects.cpp:270`) and format the object ID into it, so every Oracle database that passed through those upgrades is poisoned. Fresh installs are unaffected — `sql/metadata.in` has the correct primary key.

Secondary damage: `objects.cpp` ignores the `DBQuery()` result, so the object is created while `CREATE TABLE` silently fails. Affected objects exist with no `idata_<id>` table and will not store data even after the template is corrected — the missing tables must be created as part of the repair.

6.2 and master are not affected going forward: per-object DDL is generated in code (`BuildIDataCreationQuery()` in `src/server/include/dci_table_creation.h:48`) and upgrade 62.7 (`upgrade_v62.cpp:1023`) deletes the metadata rows.

## Context (from discovery)

- Poisoned metadata row is written by: `upgrade_v60.cpp:151`, `upgrade_v60.cpp:1471`
- Runtime consumer (6.0/6.1): `src/server/core/objects.cpp:270` (`MetaDataReadStr("IDataTableCreationCommand")`)
- Helpers available on all four branches (`src/server/tools/nxdbmgr/nxdbmgr.h`): `GetDataCollectionTargets()`, `IsDataTableExist()` — the latter is **not** used by this fix, see "Why the existence probe tests `== DBIsTableExist_Found`"
- `CreateIDataTable()` is **not** used by this fix, and is a trap: the name means metadata-driven on 6.0/6.1 (`check.cpp:825`) but code-generated on 6.2/master (`check.cpp:828`, issue #3204), which is why 6.2/master keep a frozen `CreateIDataTable_V60()` (`upgrade_v60.cpp:1680`) for historical v60 paths. See Technical Details.
- Mode guard used by `CheckDataTables()` in `check.cpp`: `DBMgrMetaDataReadInt32(L"SingeTablePerfData", 0)` — that misspelling is the actual metadata key name
- Close precedent for the procedure shape: `H_UpgradeFromV35` in `stable-6.1`'s `upgrade_v61.cpp` (fix a metadata template, then backfill existing per-object tables)
- Level-guard convention and chain-extension mechanics: `netxms-db-backport` skill
- `DBMgrMetaDataReadStr()` has no cache (`libnxdbmgr/config.cpp:28`), so `CreateIDataTable()` sees the corrected template within the same upgrade transaction

Branch state (verified 2026-07-13):

| Branch | Worktree | Schema | Next free minor |
|---|---|---|---|
| stable-6.0 | `../netxms-6.0` | 60.37 | **60.38** (origin) |
| stable-6.1 | `../netxms-6.1` | 61.36 | **61.37** |
| stable-6.2 | `../netxms-6.2` | 62.32 | not used |
| master | `.` | 70.8 | not used |

## Solution Overview

Origin branch is `stable-6.0`, so the change is keyed `(60, 38)` everywhere, per the `netxms-db-backport` convention.

1. **stable-6.0** — plain procedure `60.37 → 60.38` carrying the repair.
2. **stable-6.1** — replicate the v60 chain extension (so a database fixed on 6.0 at 60.38 can still upgrade to 6.1), plus its own procedure `61.36 → 61.37` with the repair wrapped in `GetSchemaLevelForMajorVersion(60) < 38`, since a database already on 61.x never re-runs v60 procedures.
3. **stable-6.2 and master** — chain extensions only, for both v60 and v61. This is mandatory plumbing: without it, `nxdbmgr` on 6.2/7.0 cannot upgrade a database sitting at 60.38 or 61.37 ("Unable to find upgrade procedure for version X.Y"). The repair procedures come along with those extensions and run for anything arriving from the 6.0/6.1 chain.
4. **All four branches** — correct the two broken literals in `upgrade_v60.cpp`, so a pre-6.0 database upgrading today is not poisoned in transit.

**Deliberate deviation from the `netxms-db-backport` recipe** (user decision): step 2(b) of that skill says every newer branch also gets a level-guarded procedure in its own current-major chain. We are **not** adding one to 6.2 or master. Consequence, accepted: a database poisoned on 6.0/6.1 that has *already* migrated past the v61 map keeps its missing `idata_` tables and is not repaired by any upgrade — `nxdbmgr check` detects them and offers to create them (`CheckDataTables()` → `CreateIDataTable()`, `check.cpp:1399`). Every other upgrade path is still covered, because the chain extensions carry the repair for anything arriving from the 6.0/6.1 chain.

Note for reviewers of the commits: this is an asymmetry with recent precedent — the analogous 61.36 index fix *did* get a 62-chain guarded procedure (`master/upgrade_v62.cpp:1096`, `if (GetSchemaLevelForMajorVersion(61) < 36)`). The difference is deliberate: there, the affected databases could only be repaired by upgrade; here, `nxdbmgr check` already repairs them, and new object creation on 62.x is not broken (the row is gone, DDL is code-generated). If the decision is ever revisited, the guarded procedure must take **62.34** — master's v62 map already reserves 62.33 (`upgrade_v62.cpp:44`, jump entry `{33, 70, 0}`) — and needs the extra fresh-install guard described under Coverage.

Version bumps follow the skill's `netxmsdb.h` rule: bump `DB_SCHEMA_VERSION_MINOR` only where the branch extends **its own** major's chain — 6.0 (→38) and 6.1 (→37). 6.2 and master only extend *older* majors' chains, so their `netxmsdb.h` is untouched. The bumps are load-bearing, not bookkeeping: netxmsd refuses to start unless the database version exactly equals the compiled-in `MAJOR.MINOR`, and the current-major driver loop is bounded by `DB_SCHEMA_VERSION_V<M>_MINOR` — omit the bump and the new procedure compiles, never runs, and `nxdbmgr upgrade` still reports success.

Out of scope, flagged only (see Post-Completion): `objects.cpp` swallowing the DDL failure, and `upgrade_v62.cpp:56` on master calling `SetMinorSchemaVersion(6)` where 32 is meant.

## Technical Details

The repair body is **byte-identical on every branch** (per skill: copy verbatim, do not "improve" one copy). Only the wrapper differs — plain on 6.0's v60 chain, level-guarded on the v61 chains.

```cpp
   // Earlier 6.0 upgrade procedures wrote Oracle idata table creation command with a typo in
   // primary key definition ("tdata_timestamp" instead of "idata_timestamp"), so idata tables
   // could not be created for data collection targets added after that upgrade (issue #3411).
   // DDL is issued directly instead of via CreateIDataTable() - on 6.2 and later that function
   // generates the statement in code (issue #3204), and historical upgrade paths must keep the
   // metadata-driven schema of their era.
   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      static const wchar_t *idataCreationCommand =
            L"CREATE TABLE idata_%d (item_id integer not null,idata_timestamp number(20) not null,idata_value varchar2(255 char) null,raw_value varchar2(255 char) null, PRIMARY KEY(item_id,idata_timestamp))";

      CHK_EXEC(DBMgrMetaDataWriteStr(L"IDataTableCreationCommand", idataCreationCommand));

      if (!DBMgrMetaDataReadInt32(L"SingeTablePerfData", 0))
      {
         IntegerArray<uint32_t> targets = GetDataCollectionTargets();
         for(int i = 0; i < targets.size(); i++)
         {
            uint32_t objectId = targets.get(i);
            wchar_t table[64];
            nx_swprintf(table, 64, L"idata_%u", objectId);

            // Only a definite "table found" answer may skip creation - a failed existence
            // check must not silently leave a missing table behind
            if (DBIsTableExist(g_dbHandle, table) == DBIsTableExist_Found)
               continue;

            wchar_t query[256];
            nx_swprintf(query, 256, idataCreationCommand, objectId);
            WriteToTerminalEx(L"Creating missing table \x1b[1m%s\x1b[0m...\n", table);
            CHK_EXEC(SQLQuery(query));
         }
      }
   }
```

**Why the metadata write sits outside the `SingeTablePerfData` guard.** Only the table backfill is meaningless in single-table mode; the poisoned `IDataTableCreationCommand` row is still there and still wrong. Leaving the write under the mode guard would strand the bad row permanently, since the procedure raises the schema level and never runs again.

**Why the existence probe tests `== DBIsTableExist_Found` rather than calling `IsDataTableExist()`.** `IsDataTableExist()` (`check.cpp:1418`) is fail-open: it returns `rc != DBIsTableExist_NotFound`, so a *failed* probe (`DBIsTableExist_Failure`, which the Oracle driver returns whenever its `user_tables` query errors) reads as "exists" and the table is silently skipped. In a repair whose entire job is to not skip, that would reintroduce the exact silent-failure mode this issue is about. Testing for `DBIsTableExist_Found` makes a failed probe fall through to `CREATE TABLE`, which either succeeds or fails loudly. Same construct as the `H_UpgradeFromV35` precedent.

**Why the DDL is issued directly instead of calling `CreateIDataTable()`** — this is the crux of the design, do not "simplify" it back:

- `CreateIDataTable()` means two different things depending on branch: metadata-driven on 6.0/6.1 (`check.cpp:825`), code-generated from `BuildIDataCreationQuery()` on 6.2/master (`check.cpp:828`, issue #3204).
- 6.2 and master deliberately keep a **frozen** metadata-driven copy for exactly this reason — `CreateIDataTable_V60()` at `upgrade_v60.cpp:1680`, whose comment states that historical upgrade paths must not use the reimplemented global, to avoid schema drift. Calling the global from a new v60-chain procedure would violate that directive; calling the frozen static would need a forward declaration (it is defined ~1650 lines below the top of the file, where new procedures go) and would make the body differ per branch.
- The repair is Oracle-only, so the DDL is a fixed literal — the same one being written into `metadata`. Issuing it directly uses one string for both jobs, keeps the body genuinely identical on all four branches, and depends on no helper whose meaning varies by branch.
- Nothing is lost: on Oracle there are no `IDataIndexCreationCommand_*` rows for the helper to apply (60.31 deleted them; 61.36 re-added slot 0 for MySQL/PGSQL/MSSQL only), so the helper's index loop would be a no-op.

Other notes:

- The `DBIsTableExist()` skip makes the body idempotent and a clean no-op on healthy databases (including a fresh 6.1 Oracle install, which does enter the guarded 61.37 body — see Coverage below).
- The metadata write is correct on every branch. On 6.0/6.1 it fixes the template the running server reads; on 6.2/master these procedures only execute while walking the 60/61 chain, and 62.7 (`upgrade_v62.cpp:1023`) deletes the row later in the same run.
- The literal keeps `%d`, byte-for-byte identical to the fresh-install baseline at `sql/metadata.in:20`, so an upgraded database ends up with exactly the template a fresh one has. Formatting a `uint32_t` object ID through `%d` is what the existing `CreateIDataTable()` already does, and object IDs never reach 2^31.
- `sql/*.in` baselines need no change — fresh installs were never affected.

Chain layout after the change:

```
stable-6.0  v60 map:  { 37, 60, 38, H_UpgradeFromV37 }              <- new, plain

stable-6.1  v60 map:  { 38, 61, 0,  H_UpgradeFromV38 }              <- jump, renamed from V37
                      { 37, 60, 38, H_UpgradeFromV37 }              <- new, plain
            v61 map:  { 36, 61, 37, H_UpgradeFromV36 }              <- new, level-guarded

6.2/master  v60 map:  { 38, 61, 0,  H_UpgradeFromV38 }              <- jump, renamed from V37
                      { 37, 60, 38, H_UpgradeFromV37 }              <- new, plain
            v61 map:  { 37, 62, 0,  H_UpgradeFromV37 }              <- jump, renamed from V36
                      { 36, 61, 37, H_UpgradeFromV36 }              <- new, level-guarded
```

Coverage by upgrade path:

- Database below 60.38 → repaired by the plain v60 procedure. `SetMajorSchemaVersion()` then records `SchemaVersionLevel.60 = 38` at the jump, so the 61.37 guard (`< 38`) is false and the repair does not run twice.
- Database on 61.x (the reporting customer, at 61.36) → repaired by the guarded 61.37 procedure.
- Database already on 62.x/70.x → not repaired by upgrade (accepted deviation); `nxdbmgr check` creates the missing tables.
- Fresh 6.1 install (Oracle) → has no `SchemaVersionLevel.60` row, so `GetSchemaLevelForMajorVersion(60)` returns `-1` and the guarded 61.37 body **does** run. Harmless no-op: the template is already correct and every `idata_` table exists, so `DBIsTableExist()` skips them all. This is the path that exercises idempotency — it must be tested (Task 6).
- Fresh 6.2/7.0 install → never enters the v61 chain at all, so the guard is never evaluated. (Note: it would *not* be saved by `GetSchemaLevelForMajorVersion()` returning `INT_MAX` — that only happens when a marker for a newer major exists, and a fresh install has no `SchemaVersionLevel.*` rows at all. This matters if the deviation is ever reverted: a 62.x guarded procedure keyed `(60, 38)` would evaluate `-1 < 38` → true on a fresh 6.2 Oracle install and re-insert the metadata row that 62.7 deleted. Any such procedure needs an additional guard.)

**Ordering invariant.** The v61 chains already carry three procedures guarded on major 60 at lower levels — `< 35`, `< 36`, `< 37` (`stable-6.1/upgrade_v61.cpp:818, 739, 676`). Our procedure raises `SchemaVersionLevel.60` to 38, above all of them, so it is only safe because it sits at the **chain tip** (61.36 → 61.37) and therefore runs last. It must never be placed lower in the chain, or those three earlier repairs would be silently skipped for a database that jumped at 60.x < 35.

## Development Approach

- **testing approach**: Regular. There is no unit-test harness for `nxdbmgr` upgrade procedures in this repo, so "tests" for each task means: the branch's `nxdbmgr` compiles, and the version/map/`SetMinorSchemaVersion` consistency checks from the `netxms-db-backport` checklist pass. Behavioural verification is a scratch-database upgrade run (see Task 6) and is the real acceptance gate.
- One commit per branch; stable branches are never merged into each other or into master.
- Complete and verify each branch before starting the next.

## Implementation Steps

### Task 1: stable-6.0 — repair procedure (origin, 60.37 → 60.38)

**Files:**
- Modify: `../netxms-6.0/src/server/tools/nxdbmgr/upgrade_v60.cpp`
- Modify: `../netxms-6.0/include/netxmsdb.h`

- [x] correct the broken literal at `upgrade_v60.cpp:142` — change **only** the `tdata_timestamp` token to `idata_timestamp`, leaving the rest of the statement byte-for-byte intact
- [x] correct the broken literal at `upgrade_v60.cpp:1462` (same; note this procedure also `ALTER`s existing tables, so no incidental reformatting)
- [x] add `H_UpgradeFromV37` at the top of the file with the repair body from Technical Details (issues the `CREATE TABLE` directly — do **not** call `CreateIDataTable()`), ending in `CHK_EXEC(SetMinorSchemaVersion(38));`
- [x] add map entry `{ 37, 60, 38, H_UpgradeFromV37 },` at the top of `s_dbUpgradeMap[]`
- [x] bump `DB_SCHEMA_VERSION_MINOR` to `38` in `include/netxmsdb.h`
- [x] verify each `Upgrade from X.Y to X.Z` comment matches its `SetMinorSchemaVersion(Z)` in this file
- [x] build: `make -C src/server/tools/nxdbmgr` in the 6.0 worktree — must pass before Task 2

### Task 2: stable-6.1 — v60 chain extension

**Files:**
- Modify: `../netxms-6.1/src/server/tools/nxdbmgr/upgrade_v60.cpp`

- [x] correct both broken literals (`:151`, `:1471`), identical to Task 1
- [x] rename the existing jump procedure `H_UpgradeFromV37` (body `SetMajorSchemaVersion(61, 0)`) to `H_UpgradeFromV38`, updating its comment to `Upgrade from 60.38 to 61.0`
- [x] add `H_UpgradeFromV37` (60.37 → 60.38) with the repair body copied verbatim from stable-6.0
- [x] update map: `{ 38, 61, 0, H_UpgradeFromV38 },` above `{ 37, 60, 38, H_UpgradeFromV37 },`
- [x] verify the repair body is byte-identical to stable-6.0's (`diff` the procedure)
- [x] build: `make -C src/server/tools/nxdbmgr` in the 6.1 worktree — must pass before Task 3

### Task 3: stable-6.1 — level-guarded procedure (61.36 → 61.37)

**Files:**
- Modify: `../netxms-6.1/src/server/tools/nxdbmgr/upgrade_v61.cpp`
- Modify: `../netxms-6.1/include/netxmsdb.h`

- [x] add `H_UpgradeFromV36` at the top of `upgrade_v61.cpp`: repair body wrapped in `if (GetSchemaLevelForMajorVersion(60) < 38)`, with `CHK_EXEC(SetSchemaLevelForMajorVersion(60, 38));` as the last statement **inside** the guard
- [x] end the procedure with `CHK_EXEC(SetMinorSchemaVersion(37));` **outside** the guard
- [x] add map entry `{ 36, 61, 37, H_UpgradeFromV36 },` at the top of `s_dbUpgradeMap[]` — it must be the chain tip (see Ordering invariant: it raises `SchemaVersionLevel.60` to 38, above the existing `< 35` / `< 36` / `< 37` guards)
- [x] bump `DB_SCHEMA_VERSION_MINOR` to `37` in `include/netxmsdb.h`
- [x] verify guard key is `(60, 38)` and the body matches stable-6.0's verbatim
- [x] build: `make -C src/server/tools/nxdbmgr` in the 6.1 worktree — must pass before Task 4

### Task 4: stable-6.2 — v60 and v61 chain extensions

**Files:**
- Modify: `../netxms-6.2/src/server/tools/nxdbmgr/upgrade_v60.cpp`
- Modify: `../netxms-6.2/src/server/tools/nxdbmgr/upgrade_v61.cpp`

- [x] `upgrade_v60.cpp`: correct both broken literals; rename jump `H_UpgradeFromV37` → `H_UpgradeFromV38` (60.38 → 61.0); add `H_UpgradeFromV37` (60.37 → 60.38) with the verbatim repair body; update map to `{ 38, 61, 0, ... }` / `{ 37, 60, 38, ... }`
- [x] `upgrade_v61.cpp`: rename jump `H_UpgradeFromV36` (body `SetMajorSchemaVersion(62, 0)`) → `H_UpgradeFromV37`, comment becomes `Upgrade from 61.37 to 62.0`
- [x] `upgrade_v61.cpp`: add `H_UpgradeFromV36` (61.36 → 61.37), copied verbatim from stable-6.1 including the `(60, 38)` level guard
- [x] update v61 map to `{ 37, 62, 0, H_UpgradeFromV37 },` above `{ 36, 61, 37, H_UpgradeFromV36 },`
- [x] do **not** bump `netxmsdb.h` (62.32 stays) and do **not** add a 62.x procedure
- [ ] build — **not run** (6.2 tree not configured, user decision). Substitute evidence only: both new procedures are byte-identical to the stable-6.1 versions, which do compile. This is not equivalent to compiling this translation unit.

### Task 5: master — v60 and v61 chain extensions

**Files:**
- Modify: `src/server/tools/nxdbmgr/upgrade_v60.cpp`
- Modify: `src/server/tools/nxdbmgr/upgrade_v61.cpp`

- [x] apply the same v60 changes as Task 4 (literals, renamed jump, new 60.37 → 60.38 procedure, map)
- [x] apply the same v61 changes as Task 4 (renamed jump to `H_UpgradeFromV37`, new guarded `H_UpgradeFromV36`, map)
- [x] do **not** bump `netxmsdb.h` (70.8 stays) and do **not** add a 70.x procedure
- [x] confirm the new procedures do not call `CreateIDataTable()` — on this branch that is the code-generated implementation, and `upgrade_v60.cpp:1680` freezes `CreateIDataTable_V60()` specifically to keep historical paths off it (issue #3204)
- [x] confirm all four branches now carry an identical repair body (`diff` across worktrees) — all 7 copies (v60 on 6.0/6.1/6.2/master, v61 on 6.1/6.2/master) hash identically after dedenting the guarded v61 copies by one level
- [ ] build — **not run** (worktree not configured, user decision). Substitute evidence only: the full v60+v61 diff is byte-identical to the 6.2 change, and the same procedures compile on 6.1. This is not equivalent to compiling this translation unit.

### Task 6: Verify acceptance criteria

- [x] every `SetMinorSchemaVersion(n)` argument matches its procedure's target minor, on each edited file (the classic backport bug — cf. `60b4d2977a`) — scripted check over all 7 edited files, no mismatch; `SetMajorSchemaVersion()` args also cross-checked against their comments
- [x] jump procedures renamed and the top two map entries correct on 6.1, 6.2, master for both chains — verified against the actual `s_dbUpgradeMap[]` arrays
- [x] `netxmsdb.h` bumped by exactly one on 6.0 (37→38) and 6.1 (36→37); untouched on 6.2 (62.32) and master (70.8) — verified by `git diff HEAD~1 HEAD -- include/netxmsdb.h` on each branch (empty diff on 6.2/master)
- [x] on each edited branch, `DB_SCHEMA_VERSION_MINOR` equals the highest `SetMinorSchemaVersion()` argument in that branch's **own-chain** file (`upgrade_v60.cpp` on 6.0, `upgrade_v61.cpp` on 6.1, `upgrade_v62.cpp`/`upgrade_v70.cpp` unchanged on 6.2/master) — 6.0: 38/38, 6.1: 37/37, 6.2: 32/32, master: 8/8; own-chain files on 6.2/master confirmed untouched by our commit
- [x] no `DB_SCHEMA_VERSION_V*_MINOR` constant added for a major other than the branch's own — `netxmsdb.h` diff is a single-line `DB_SCHEMA_VERSION_MINOR` bump on 6.0/6.1 and empty on 6.2/master
- [x] no `sql/*.in` change (fresh installs were never broken) — confirm `sql/metadata.in` already has `PRIMARY KEY(item_id,idata_timestamp)` — no `sql/` path in any of the four commits; 6.0/6.1 `metadata.in` carries the correct `idata_timestamp` PK (on 6.2/master the row is absent by design, deleted by upgrade 62.7 in favour of code-generated DDL)
- [x] repo-wide grep for the poisoned literal returns nothing: `git grep -n "item_id,tdata_timestamp))' WHERE var_name='IDataTableCreationCommand'"` on each branch — no line on any branch co-locates `idata` with `tdata_timestamp`; the remaining `item_id,tdata_timestamp` hits are all legitimate `tdata_*` table DDL and are still present
- [x] cross-branch body identity — all 7 copies of the repair body (v60 on 6.0/6.1/6.2/master, v61 on 6.1/6.2/master) hash identically after dedent (sha256[:16] `0d6bd38146e334ce`)
- [ ] scratch Oracle database: upgrade from a poisoned 61.36 dump → template corrected, missing `idata_` tables created, new node creation succeeds — **NOT RUN (no Oracle available: no Oracle instance, no Oracle DB container image, no Oracle Instant Client/OCI SDK, and the 6.0/6.1 trees were configured `--with-pgsql` only so the Oracle driver is not built)**
- [ ] scratch Oracle database: upgrade from a poisoned 60.x dump straight to 6.2 → repair applied exactly once (v60 procedure applies it, `SchemaVersionLevel.60 = 38` recorded at the jump, v61 guard skips) — **NOT RUN (no Oracle, as above; additionally the 6.2 tree is not configured/built)**. The *guard bookkeeping* half of this was exercised on PostgreSQL: a 60.34 database walked 60.37 → 60.38 → 61.0 → … → 61.37, with `SchemaVersionLevel.60=38` recorded at the jump, so the 61.37 guard evaluated false. The Oracle-only repair body itself was never executed.
- [ ] idempotency: **fresh 6.1 Oracle install** upgraded to 61.37 → guard is true (no `SchemaVersionLevel.60` row → `-1 < 38`), body runs and is a clean no-op; no table recreated, no error — **NOT RUN (no Oracle, as above)**. The level guard was confirmed true on a fresh (`SchemaVersionLevel` empty) 6.1 PostgreSQL install, but on PostgreSQL the inner Oracle block is skipped, so the no-op behaviour of the body was not exercised.
- [x] non-Oracle regression: upgrade a PostgreSQL database on 6.1 → repair block skipped, upgrade reaches 61.37 cleanly — **executed for real** (PostgreSQL 18 container, 6.1 nxdbmgr built from the fixed tree). Fresh 61.36 PG install → `nxdbmgr upgrade` → "Upgrading from version 61.36 to 61.37", "Database upgrade succeeded". `IDataTableCreationCommand` left untouched as the PostgreSQL variant (`idata_timestamp bigint`) — not clobbered with the Oracle literal, confirming the `g_dbSyntax == DB_SYNTAX_ORACLE` guard; `SchemaVersionLevel.60=38` recorded; no `idata_` table created; re-running upgrade is a clean no-op. Also walked the full chain from a 60.34 PG install through the new 60.38 procedure and the renamed 60.38 → 61.0 jump up to 61.37 with no missing-procedure error, exercising both chain extensions.

**Verification status — read before trusting the checkboxes above.**

The fix has **never been executed against Oracle**, which is the only platform it affects. What was actually verified:

- Compiles and links on stable-6.0 and stable-6.1 (`make -C src/server/tools/nxdbmgr`). Not compiled on 6.2 or master — those trees are unconfigured by user decision; they carry a byte-identical body, but that is inference, not a build.
- Static checks (versions, maps, `SetMinorSchemaVersion` args, cross-branch body identity) all pass.
- End-to-end upgrade runs on **PostgreSQL** only, where the Oracle repair block is skipped by design. These prove the chain extensions, the level-guard bookkeeping and the non-Oracle no-op — they prove nothing about the repair body itself.

The Oracle-specific behaviour (template corrected, missing `idata_` tables created, idempotency on re-run) rests on code review alone. `tests/test-libnxdb/oracle.cpp` (`TestOracleBatch()`) does exercise `DBQueryEx("CREATE TABLE ...")` and `DBIsTableExist()` against a live Oracle via `oracle.ddr` — the two primitives this repair depends on — so the area is not as untestable as "no nxdbmgr upgrade harness" suggests, but it still needs an Oracle server that is not available here.

### Task 7: [Final] Commit and close out

- [x] one commit per branch, message referencing the issue (e.g. `fixed Oracle idata table creation command (issue #3411)`)
- [ ] comment on issue #3411 with the fix and the schema versions it landed in
- [ ] move this plan to `docs/plans/completed/`

## Post-Completion

**Follow-up issues to file** (out of scope here, both noticed during investigation):

- `src/server/core/objects.cpp:270` ignores the `DBQuery()` return value when creating per-object data tables. That is what turned a template typo into silently broken data collection instead of a visible failure at object creation.
- `src/server/tools/nxdbmgr/upgrade_v62.cpp:56` on master: `H_UpgradeFromV31` ("Upgrade from 62.31 to 62.32") calls `SetMinorSchemaVersion(6)`. `stable-6.2`'s copy of the same procedure correctly sets 32. Looks like a merge accident; it would knock a database back to 62.6.

**Customer workaround already published** on issue #3411 for anyone who cannot upgrade yet: correct the `metadata` row by hand with the server stopped, then run `nxdbmgr check` to create the missing `idata_` tables.

**Databases already on 62.x** that were poisoned on 6.0/6.1 are not repaired by any upgrade procedure by design — they need `nxdbmgr check`. Note that this remedy has not been exercised against Oracle either; it is the only path available to that population.

## Known limitations (accepted, Oracle-only)

**Operator guidance — do not use `-X` if this upgrade stops partway.** Oracle has no savepoints (`CreateSavePoint()` in `src/server/include/nxdbmgr_tools.h:135` returns 0 for every syntax except PGSQL/TSDB) and implicitly commits each DDL statement, so the `DBBegin`/`DBRollback` wrapper around the upgrade protects nothing once the first `CREATE TABLE` runs. If the repair loop fails partway — the plausible cause being `ORA-01536: space quota exceeded for tablespace` when creating many tables at once — nxdbmgr prints "Rolling back last stage due to upgrade errors...", but nothing is actually rolled back: the tables created so far stay, and the schema version is not advanced. **The correct recovery is to fix the underlying cause (e.g. raise the tablespace quota) and re-run the upgrade** — the body is idempotent, existing tables are skipped. Re-running with `-X` (`g_ignoreErrors`) would instead swallow the failure, advance the version and record `SchemaVersionLevel.60 = 38`, permanently retiring the repair with tables still missing.

**Two spurious SQL errors are printed on the 60.37 → 60.38 step** when upgrading an Oracle database from the 6.0 chain with a 6.1 or newer `nxdbmgr`. `GetDataCollectionTargets()` on 6.1+ also selects from `resources` and `cloud_domains` (`check.cpp:78-79`), tables that are only created at 61.5 → 61.6, so at schema 60.37 they do not exist yet. `CollectObjectIdentifiers()` ignores the failure and the repair proceeds correctly with the remaining object classes (neither class can have instances at 60.37), but `SQLSelect()` unconditionally prints `SQL query failed (ORA-00942: table or view does not exist)`. Accepted as cosmetic: the pre-existing `H_UpgradeFromV13` (`upgrade_v60.cpp`) already calls `GetDataCollectionTargets()` from the same chain position and produces exactly the same noise, and suppressing it would mean either duplicating that helper's logic or making the repair body differ per branch, breaking the byte-identical invariant.
