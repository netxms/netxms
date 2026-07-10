# pgsql Subagent — Database Visibility & SQL Semantics

## Overview
- Block 1 of a 5-block audit of `src/agent/subagents/pgsql`. Fixes the per-database queries so every reachable database produces a row, even when it has no current activity.
- Today, `DB_CONNECTIONS`, `DB_STAT`, and `DB_TRANSACTIONS` are sourced from activity views and silently omit databases that have zero current sessions / zero block reads / zero prepared transactions. The agent then returns `SYSINFO_RC_NO_SUCH_INSTANCE` for those databases (NetXMS error 921), even though the database exists and is reachable.
- Also fixes two silent metric corruptions discovered during the audit: `DB_CONNECTIONS.idle` is filtered with the same predicate as `active`, and `CONNECTIONS.total_pct` uses integer division.
- Folds in the autovacuum-classification fix for PG ≥ 10 (use `backend_type = 'autovacuum worker'` instead of `query LIKE '%autovacuum%'`) — same query is being rewritten anyway.
- Cross-block dependency: this plan **assumes Block 2 has landed** (PostgreSQL version detection works). Without Block 2, the new PG ≥ 10 / PG ≥ 12 query variants in this plan are version-gated and unreachable on default-configured servers.

## Context
- File in scope: `src/agent/subagents/pgsql/main.cpp` — `g_queries[]` definition (currently lines 33-127).
- No header changes. No handler-code changes. No build-system changes.
- The handler `H_InstanceParameter` (`main.cpp:316-372`) is correct as-is — it builds the cache key `tag@instance` and looks it up. The bug is upstream: the cache simply doesn't have entries for inactive databases.
- All `g_queries[]` entries are static SQL strings — no user input ever reaches them, no SQL-injection vector introduced.
- Brainstorm decisions: filter by `has_database_privilege(current_user, d.datname, 'CONNECT')` so only reachable DBs are reported; return `0` (not NULL/missing) for cache-hit-ratio when block reads/hits are both zero; keep the autovacuum fix in this plan.

## Development Approach
- **Testing approach**: manual verification only. Same rationale as Block 2 — no unit-test harness for the pgsql subagent; the SQL strings are self-contained and best validated against a live PG server.
- Each query rewrite is one task. Keep diffs small and reviewable.
- Do not touch handler code, the macro, the connection lifecycle, or non-targeted queries (`BGWRITER`, `ARCHIVER`, `REPLICATION`).
- Update plan checkboxes immediately as work progresses.

## Solution Overview

### Pattern for per-database queries
Drive row sets from `pg_database`, outer-join the activity/stats source, restrict to reachable DBs:

```sql
FROM pg_catalog.pg_database d
LEFT JOIN <activity_or_stats> s ON s.<datid|database> = d.<oid|datname>
WHERE NOT d.datistemplate
  AND d.datallowconn
  AND has_database_privilege(current_user, d.datname, 'CONNECT')
GROUP BY d.datname
```

Wrap counters with `count(s.<col>)` (which returns 0 for empty groups) or `COALESCE(s.<col>, 0)` for sums.

### Per-query changes

**`DB_CONNECTIONS`** — three version variants:

1. PG ≥ 10 (NEW): `count(s.pid) FILTER (WHERE s.backend_type = 'autovacuum worker')` for autovacuum count. `count(s.pid) FILTER (WHERE s.state = 'idle')` for the idle column (was incorrectly `state = 'active'`). Outer-join from `pg_database`, reachability filter.
2. PG ≥ 9.6, < 10.0: same shape, but autovacuum stays as `query LIKE '%autovacuum%'` (no `backend_type` column). Idle filter typo fixed.
3. PG < 9.6: legacy `waiting` boolean column kept; same outer-join shape and reachability filter; idle filter typo fixed.

**`DB_STAT`** — two version variants (≥12 with `checksum_failures`, <12 without):
- Drop `s.blks_read > 0` filter.
- `COALESCE` every counter column to 0 (LEFT JOIN can produce NULL).
- Cache-hit-ratio expression: `CASE WHEN COALESCE(s.blks_hit + s.blks_read, 0) = 0 THEN 0 ELSE round(s.blks_hit::numeric * 100 / (s.blks_hit + s.blks_read), 2) END`.
- Existing `CASE WHEN has_database_privilege ... pg_database_size ... ELSE NULL END` becomes redundant (the WHERE clause already restricts to reachable DBs) — replace with plain `pg_database_size(d.datname)`.
- Add reachability filter to WHERE.

**`DB_TRANSACTIONS`**:
- Source from `pg_database d LEFT JOIN pg_prepared_xacts p ON p.database = d.datname` (note: `pg_prepared_xacts.database` is `name` text, joined on `datname`, not OID).
- `count(p.gid) AS prepared`.
- Reachability filter.

**`DB_LOCKS`**:
- Already RIGHT-JOINs `pg_database`. Add the `has_database_privilege` filter for consistency with the other per-DB queries (otherwise users see lock metrics for DBs they don't see anywhere else).

**`CONNECTIONS`** (global, not per-DB):
- Fix `total_pct` integer division. New SQL: `count(*)::numeric * 100 / pg_catalog.current_setting('max_connections')::numeric AS total_pct` (no `::int` cast — return the full numeric percentage).
- Change the corresponding entry in `s_parameters[]` (currently `main.cpp:620`) from `DCI_DT_INT` to `DCI_DT_FLOAT`. Customers will now see fractional percentages (e.g. 0.5%) instead of values flooring to 0.

## Technical Details

### `pg_prepared_xacts.database` join key
`pg_prepared_xacts` exposes `database` as `name` (text). Join on `d.datname`, not OID.

### Cache-hit-ratio for inactive DBs
With the new outer-join, a database with no activity reports `blks_hit = NULL`, `blks_read = NULL`. The `CASE WHEN COALESCE(blks_hit + blks_read, 0) = 0 THEN 0 ...` returns 0 — matches the locked decision (return 0, not NULL).

### autovacuum classification semantic change
Pre-fix: counted user queries containing the word "autovacuum". Post-fix (PG ≥ 10): counts only real `autovacuum worker` backends. Customers will typically see this metric drop. Release-notes-worthy.

### `oldestxid` on empty groups
`count(s.pid) = 0` → `max(age(...))` returns NULL. Wrap with `COALESCE(greatest(max(age(s.backend_xmin)), max(age(s.backend_xid))), 0)` so the metric reports 0 instead of disappearing.

### Cardinality blow-up
After this change, agents on servers with many idle/quiescent databases will populate cache entries for every reachable DB. `m_data` size grows roughly linearly with DB count. Expected to be tolerable — the typical bounded customer use case is dozens to low hundreds of DBs per server. Worth flagging in release notes.

## What Goes Where
- **Implementation Steps** (`[ ]`): SQL rewrites in `g_queries[]`, build sanity, manual verification on a populated PG server.
- **Post-Completion** (no checkboxes): release-notes blurb describing the new cardinality and the autovacuum semantic change.

## Implementation Steps

### Task 1: Rewrite DB_CONNECTIONS (three version variants)

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] In `g_queries[]`, replace the existing PG ≥ 9.6 `DB_CONNECTIONS` entry (currently `main.cpp:52-59`) with the new PG ≥ 10 variant: `backend_type`-based autovacuum, `state = 'idle'` for the idle column, outer-join from `pg_database`, reachability filter, COALESCE for `oldestxid`.
- [x] Insert a new PG ≥ 9.6 / < 10.0 entry: same outer-join shape and reachability filter; autovacuum kept as `query LIKE '%autovacuum%'`; idle filter typo fixed; `wait_event IS NOT NULL` for waiting.
- [x] Replace the existing legacy `DB_CONNECTIONS` (PG < 9.6) entry (currently `main.cpp:60-67`) with the same outer-join + reachability shape; `waiting` boolean kept; idle filter typo fixed.
- [x] **In all three variants, replace every `count(*)` aggregator with `count(s.pid)`** so empty groups report 0 instead of 1. (LEFT JOIN produces a single synthetic NULL row for empty DBs; `count(*)` would count it as 1, breaking `total`/etc. `count(s.pid)` ignores NULLs and reports 0.)
- [x] Confirm `g_queries[]` ordering still respects the sentinel (terminator `{ nullptr, 0, 0, 0, nullptr }` at the end).
- [x] Manual sanity-check: every column that handlers consume (`active`, `idle`, `idle_in_transaction`, `idle_in_transaction_aborted`, `fastpath_function_call`, `oldestxid`, `autovacuum`, `waiting`, `total`) is present in all three variants with consistent names.

### Task 2: Rewrite DB_STAT (two version variants)

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] PG ≥ 12 `DB_STAT` entry (currently `main.cpp:35-43`): drop `AND s.blks_read > 0`, add reachability filter, replace size CASE with `pg_database_size(d.datname)`, COALESCE all counter columns, fix cache-hit-ratio divide-by-zero.
- [x] PG < 12 `DB_STAT` entry (currently `main.cpp:44-51`): same fixes, no `checksum_failures` column.
- [x] Confirm column name list and order is unchanged on both variants (handlers in `s_parameters[]` reference these by name and order — `setPreallocated` uses column name from `DBGetColumnName`, so column order doesn't matter for the cache, but keep stable for diff readability).
- [x] Sanity-check that `DBGetField` against the now-allowed NULL `pg_database_size` (which can't actually be NULL since the WHERE already restricts to reachable DBs) produces an empty string and the metric handler returns it cleanly.

### Task 3: Rewrite DB_TRANSACTIONS

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] Replace the `DB_TRANSACTIONS` entry (currently `main.cpp:73-75`) with the outer-join shape sourcing from `pg_database` and joining `pg_prepared_xacts` on `datname`.
- [x] Add reachability filter.
- [x] `count(p.gid) AS prepared` so empty groups report 0.

### Task 4: Add reachability filter to DB_LOCKS

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] In the `DB_LOCKS` entry (currently `main.cpp:76-88`), add `AND has_database_privilege(current_user, d.datname, 'CONNECT')` to the WHERE clause.
- [x] Leave the RIGHT JOIN structure intact — already correctly drives row set from `pg_database`.

### Task 5: Fix CONNECTIONS.total_pct integer division and DCI type

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] In the `CONNECTIONS` entry (currently `main.cpp:68-72`), change `total_pct` expression to drop the integer truncation: `count(*)::numeric * 100 / pg_catalog.current_setting('max_connections')::numeric AS total_pct`.
- [x] In `s_parameters[]`, change the `PostgreSQL.GlobalConnections.TotalPct(*)` entry (currently `main.cpp:620`) from `DCI_DT_INT` to `DCI_DT_FLOAT`. Update the description if needed (`{instance-name}` not relevant — this is a global metric).

### Task 6: Build sanity

- [x] `make -C src/agent/subagents/pgsql` cleanly with no new warnings.

### Task 7: Manual verification — visibility on a populated PG server

**Files:**
- (no edits — verification only)

- [x] manual test (skipped - not automatable): Set up a PG ≥ 12 server with at least three databases: one busy (`postgres`), one idle (no current sessions), one inaccessible to the configured user.
- [x] manual test (skipped - not automatable): Also include a database the monitoring user has CONNECT on but is **not** the owner of (verifies `pg_database_size` works for non-owners).
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.Databases(<id>)'` lists busy and idle, but **not** the inaccessible one.
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.DBConnections.Idle(<id>,<idle-db>)'` returns `0`.
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.DBConnections.Idle(<id>,<busy-db>)'` returns the actual idle count.
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.Stats.CacheHitRatio(<id>,<idle-db>)'` returns `0`.
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.Stats.DatabaseSize(<id>,<idle-db>)'` returns a value.
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.Transactions.Prepared(<id>,<idle-db>)'` returns `0`.
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.Locks.Total(<id>,<idle-db>)'` returns a value.
- [x] manual test (skipped - not automatable): `nxget localhost 'PostgreSQL.GlobalConnections.TotalPct(<id>)'` returns a fractional percentage.

### Task 8: Manual verification — autovacuum classification

**Files:**
- (no edits — verification only)

- [x] manual test (skipped - not automatable): On a PG ≥ 10 instance: trigger an autovacuum (or wait for one) and confirm `PostgreSQL.DBConnections.Autovacuum(<id>,<db>)` reports the worker count.
- [x] manual test (skipped - not automatable): Run a user query containing the literal word `autovacuum` (e.g. `SELECT 'autovacuum';`) and confirm it does **not** count toward the metric (was previously over-counted).

### Task 9: Verify acceptance criteria
- [x] All checkboxes in Tasks 1-8 ticked.
- [x] No new compiler warnings in the pgsql subagent build.
- [x] manual test (skipped - not automatable): Inaccessible databases continue to be hidden from discovery and per-DB metrics.
- [x] manual test (skipped - not automatable): Reachable but idle databases now produce values (zero where appropriate) instead of `921: No such instance`.

### Task 10: [Final] Documentation
- [x] Update `CLAUDE.md` only if a new pattern emerged (none expected).
- [x] Move this plan to `docs/plans/completed/`.

## Post-Completion
*Items requiring manual intervention or external systems — informational only.*

**Release notes** (next NetXMS release):
- PostgreSQL subagent now reports per-database metrics for every database the configured monitoring user can `CONNECT` to, regardless of current activity. Previously, databases with no current sessions / no block reads / no prepared transactions were silently omitted and queries returned `921: No such instance`. After upgrade:
  - Cardinality of `PostgreSQL.Databases(*)` and `PostgreSQL.AllDatabases` will grow on servers with idle databases. Existing instance-discovery rules will pick up new DCIs on the next discovery poll.
  - `PostgreSQL.DBConnections.Idle(*)` now actually counts idle backends (previously a copy-paste bug made it equal to `Active`). Thresholds tuned to the broken value will alert differently.
  - `PostgreSQL.DBConnections.Autovacuum(*)` on PG ≥ 10 now counts only real `autovacuum worker` backends, not user queries that happen to contain the word "autovacuum". The reported number will typically be smaller.
  - `PostgreSQL.GlobalConnections.TotalPct(*)` DCI type changes from `INT` to `FLOAT` and reports the true fractional percentage (e.g. 0.5). Templates that store this value as `INT` should be updated.
  - `PostgreSQL.Stats.CacheHitRatio(*)` reports 0 (rather than a missing instance) for databases with no block reads or hits yet.
  - Inaccessible databases (no `CONNECT` privilege for the monitoring user) are explicitly hidden across all per-DB metrics — including `DB_LOCKS`, which previously could leak names.

**Audit follow-ups:**
- Block 3 — poller robustness.
- Block 4 — bounds & safety.
- Block 5 — code cleanup.
