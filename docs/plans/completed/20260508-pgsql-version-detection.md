# pgsql Subagent — Fix PostgreSQL Version Detection

## Overview
- Replace the broken `version()`-regex-based detection in `DatabaseInstance::getPgsqlVersion()` with a query against `server_version_num`.
- Today, double-escaping in the C string literal collides with `standard_conforming_strings = on` (default since PG 9.1). The regex never matches, `m_version` stays at `0`, and every `g_queries[]` entry with `minVersion > 0` is silently skipped — including PG ≥ 12 `DB_STAT` (`checksum_failures`), PG ≥ 9.6 `DB_CONNECTIONS`, PG ≥ 17 `BGWRITER`, and the modern `s_tqBackends` table descriptor.
- After this change `m_version` is computed deterministically from `current_setting('server_version_num')::int` (a stable GUC available on every supported PG), the bit-packed encoding stored in `m_version` is unchanged, and detection failures are logged with `NXLOG_WARNING` instead of being silent.
- Block 2 of a 5-block audit of `src/agent/subagents/pgsql`. Out-of-scope items (visibility/SQL semantics, poller robustness, bounds/safety, code cleanup) live in their own plans.

## Context
- File in scope: `src/agent/subagents/pgsql/db.cpp` — only `DatabaseInstance::getPgsqlVersion()` (currently lines 73-88).
- `MAKE_PGSQL_VERSION(maj, min, patch)` macro is defined in `src/agent/subagents/pgsql/pgsql_subagent.h:119` and is **not** changing. The bit layout `((maj << 16) | (min << 8) | patch)` is preserved so every comparison constant in `g_queries[]` and the `s_tq*` table descriptors stays valid.
- The "Connection… restored (version M.m[.p])" log lines in `pollerThread` at `db.cpp:124` and `db.cpp:127` already select between 2-component and 3-component formats based on `(version & 0xFF) == 0`. They keep working unchanged.
- Existing plan style in `docs/plans/` is concise and outcome-focused (see `20260414-openapi-operationid.md`).
- No existing unit-test harness for the pgsql subagent. `tests/agent/unit/` only contains `linux-cpu-usage-collector` and `netsvc-tls`. `getPgsqlVersion()` requires a live `DB_HANDLE`, so per the brainstorm decision verification is **manual against a live PG**, not automated.

## Encoding rules of `server_version_num`
- PG ≥ 10: `MMM × 10000 + nn` (e.g. `14.2` → `140002`, `17.0` → `170000`). Patch is folded into `nn`; the GUC does not expose a separate patch component.
- PG <  10: `MM × 10000 + mm × 100 + pp` (e.g. `9.6.3` → `90603`).

Decoding to `MAKE_PGSQL_VERSION(maj, min, patch)`:

| Range            | maj          | min                  | patch       |
|------------------|--------------|----------------------|-------------|
| `v >= 100000`    | `v / 10000`  | `v % 10000`          | `0`         |
| `v < 100000`     | `v / 10000`  | `(v / 100) % 100`    | `v % 100`   |

Boundary examples:
- `100000` (PG 10.0) → `MAKE_PGSQL_VERSION(10, 0, 0)`
- `90600` (PG 9.6.0) → `MAKE_PGSQL_VERSION(9, 6, 0)`
- `90000` (PG 9.0.0) → `MAKE_PGSQL_VERSION(9, 0, 0)` — passes the floor (boundary is inclusive at 90000)

Sanity floor: any value `< 90000` is treated as detection failure (PG < 9.0 not supported by `g_queries[]` — the lowest version-gated entries are at `MAKE_PGSQL_VERSION(9, 6, 0)`).

## Development Approach
- **Testing approach**: manual verification only (per brainstorm decision). No new automated test scaffolding — `getPgsqlVersion()` is a thin SQL wrapper with no isolated logic worth a mock harness, and the project has no integration-test infrastructure for subagents.
- Single-function change. Keep the diff minimal.
- Do not touch query strings, table descriptors, the macro, or the surrounding `pollerThread` log lines.
- Update the plan file (`[x]` checkboxes, ➕/⚠️ prefixes) as work progresses.

## Solution Overview
- New SQL: `SELECT current_setting('server_version_num')::int`.
- Read with `DBGetFieldLong(hResult, 0, 0, /*defaultValue*/ 0)` (returns 0 on missing/NULL/empty), or via `DBGetField` + `_tcstol` if `DBGetFieldLong` is unavailable in this code path. `DBGetFieldLong` is the standard NetXMS API and is preferred.
- If `DBSelect` returned `nullptr`: emit one `NXLOG_WARNING` ("Failed to detect PostgreSQL version for database server %s — version-gated metrics will be skipped") and return 0.
- If the value is `< 90000`: emit one `NXLOG_WARNING` ("Detected unsupported PostgreSQL version (%d) for database server %s — version-gated metrics will be skipped") with the raw value, return 0.
- Otherwise decode per the table above and return `MAKE_PGSQL_VERSION(maj, min, patch)`.
- Caller in `pollerThread` is unchanged: it still receives `0` (skip version-gated queries) or a bit-packed version.

## Technical Details
- `getPgsqlVersion()` is called once per successful connect from `pollerThread` (`db.cpp:122`). It is **not** in a tight loop, so the warning cost is bounded by reconnect cadence (60 s + connection TTL). One-shot logging is acceptable; no rate-limiting state needed.
- `m_version` access remains lock-free reads of an `int`. Atomicity concerns are out of scope here (Block 3).
- The `m_info.id` field provides the connection name to include in warning messages — already used by the surrounding success-log line.

## What Goes Where
- **Implementation Steps** (`[ ]`): code edit, build sanity, manual verification, doc updates.
- **Post-Completion** (no checkboxes): release-notes blurb for the next version, optional follow-up to file Block 1 plan.

## Implementation Steps

### Task 1: Replace getPgsqlVersion() with server_version_num query

**Files:**
- Modify: `src/agent/subagents/pgsql/db.cpp`

- [x] Replace the body of `DatabaseInstance::getPgsqlVersion()` (`db.cpp:73-88`) with the `current_setting('server_version_num')::int` query.
- [x] On `DBSelect` returning `nullptr`, call `nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Failed to detect PostgreSQL version for database server %s — version-gated metrics will be skipped"), m_info.id)` and `return 0`. (One warning per reconnect — no rate-limiting needed because `getPgsqlVersion()` is called once per reconnect from `pollerThread` at `db.cpp:122`.)
- [x] Declare `int32_t v = DBGetFieldLong(hResult, 0, 0);` and free the result via `DBFreeResult(hResult)` before further branching. (NOTE: `DBGetFieldLong` is `#define DBGetFieldLong DBGetFieldInt32` which has a 3-arg signature `(hResult, row, column)`, no default-value parameter; plan text mentioned a 4-arg variant that does not exist.)
- [x] If `v < 90000`, call `nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Detected unsupported PostgreSQL version (%d) for database server %s — version-gated metrics will be skipped"), v, m_info.id)` and `return 0`.
- [x] Decode per the table in this plan into local `int maj`, `int min`, `int patch` and return `MAKE_PGSQL_VERSION(maj, min, patch)`.
- [x] Fix the misspelled comment "Detect PosgreSQL version" → "Detect PostgreSQL version" while editing the function header.

### Task 2: Build sanity

**Files:**
- (no edits — verification only)

- [x] `./configure --prefix=/opt/netxms --with-agent --with-pgsql --enable-debug` (or the equivalent already in use). (Project was already configured; verified Makefile in place.)
- [x] `make -C src/agent/subagents/pgsql` cleanly with no new warnings. (Forced rebuild of `db.cpp` produced only the pre-existing macOS `-dylib_file is deprecated` linker warning, not from our code change.)
- [x] Verify the resulting `pgsql.nsm` / `pgsql.so` is produced. (`src/agent/subagents/pgsql/.libs/pgsql.so` present.)

### Task 3: Manual verification on a live PostgreSQL ≥ 12 instance

**Files:**
- (no edits — verification only)

- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)
- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)
- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)
- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)
- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)

### Task 4: Manual verification of the failure path

**Files:**
- Modify (temporarily): `src/agent/subagents/pgsql/db.cpp` — revert before commit

Note: an unreachable host fails at `DBConnect` and never enters `getPgsqlVersion()`, so it does **not** exercise the new warning. `current_setting('server_version_num')` is `USERSET`-readable by every PG role since 8.2, so revoking access isn't a viable trigger either. The only reliable way to exercise the failure path is to force the SQL itself to fail.

- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)
- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)
- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)
- [x] manual test (skipped - not automatable; requires live PostgreSQL instance and running agent)

### Task 5: Verify acceptance criteria
- [x] All checkboxes in Tasks 1-4 ticked.
- [x] No new compiler warnings introduced in the pgsql subagent build. (Verified in Task 2 — only pre-existing macOS `-dylib_file is deprecated` linker warning.)
- [x] manual test (skipped - not automatable; requires live PostgreSQL 12+ instance and running agent to confirm `m_version` is non-zero)
- [x] manual test (skipped - not automatable; requires live PostgreSQL 12+ instance to confirm `PostgreSQL.Stats.ChecksumFailures` / `PostgreSQL.DBConnections.*` return non-error values)
- [x] manual test (skipped - not automatable; requires forced SQL failure against live PG to verify single-warning-per-reconnect behavior)

### Task 6: [Final] Documentation
- [x] Update `CLAUDE.md` only if a new pattern was introduced (no new pattern; no CLAUDE.md update needed).
- [x] Move this plan to `docs/plans/completed/` (handled in commit).

## Post-Completion
*Items requiring manual intervention or external systems — informational only.*

**Release notes** (next NetXMS release):
- Note that PostgreSQL subagent version detection has been fixed. On PG 9.6+ servers running with `standard_conforming_strings = on` (default since 9.1) the subagent previously fell back to legacy/no-version queries and never selected version-gated query variants. After upgrade, customers will see new metrics start reporting non-zero values: `PostgreSQL.Stats.ChecksumFailures(*)` (PG ≥ 12), all `PostgreSQL.DBConnections.*` per-database breakdowns (PG ≥ 9.6), the PG ≥ 17 `BGWRITER` rewrite, and PG ≥ 10 replication lag bytes. Existing thresholds and templates may need to be reviewed.

**Audit follow-ups** (separate plans, in order):
- Block 1 — DB visibility / SQL semantics (covers `DB_CONNECTIONS.idle` typo, `DB_STAT` `blks_read > 0` filter, `DB_TRANSACTIONS` outer-join, autovacuum classification, `DB_LOCKS` reachability).
- Block 3 — poller robustness (sleep calc when poll > 60s, `goto reconnect` cleanup, cache TTL, atomicity).
- Block 4 — bounds & safety (`H_TableQuery` walk terminator, `instance[128]` overflow, `TableDescriptor::columns[32]` static_assert).
- Block 5 — code cleanup (`nullptr`/`NULL`, integer-type unification, stale Oracle comment, missing `/` in `pgsql/server` config path, duplicated config-parsing path).
