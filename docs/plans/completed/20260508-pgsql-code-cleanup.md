# pgsql Subagent — Code-Quality Cleanup

## Overview
- Block 5 (final) of a 5-block audit of `src/agent/subagents/pgsql`. Primarily cleanup — one small behavioral fix (the missing leading slash in a config path) is bundled in because it lives next to the config-parsing refactor.
- Items:
  1. **`NULL` → `nullptr`** — 16 occurrences across `db.cpp` (3) and `main.cpp` (13). Project standard per root CLAUDE.md.
  2. **Integer-type unification** — replace legacy `UINT32`/`INT64` with `uint32_t`/`int64_t` and the `_LL()` macro with `INT64_C()` (or strip when not load-bearing). Touch points: `db.cpp:105` (`INT64 connectionTTL = (INT64)... * _LL(1000);`) and `db.cpp:154` (`(UINT32)...`). The latter line is also touched by Block 3; coordinate.
  3. **Stale Oracle comment** at `main.cpp:392` ("Handler for Oracle.DBInfo.Version parameter") — copy-paste residue. Fix to "Handler for PostgreSQL.Version parameter".
  4. **Missing leading slash** at `main.cpp:521` — `config->getEntry(_T("pgsql/server"))` is inconsistent with all other entries (`/pgsql/...`). Likely benign because `Config` normalises, but worth fixing for consistency.
  5. **Duplicated config-parsing path** — `SubAgentInit` (`main.cpp:502-580`) parses the simple `pgsql/...` block (lines 514-535) and the `pgsql/servers/<id>/...` block (lines 537-566) with nearly identical code. Extract a small helper. (Pure refactor; ensure exact behavioral equivalence.)

## Context
- Files in scope: `src/agent/subagents/pgsql/db.cpp`, `src/agent/subagents/pgsql/main.cpp`.
- No SQL changes. No header changes. No build-system changes.
- Coordinate with Block 3 if both blocks land close in time — `db.cpp:154` is touched by both.

## Development Approach
- **Testing approach**: manual verification only. The build serves as the primary check; runtime behavior is unchanged.
- One topic per task. Cleanup is by definition mechanical, but keeping tasks separate makes git-blame more useful.

## Solution Overview

### `NULL` → `nullptr` pass
Direct replacement in C++ contexts. The 16 sites are all in C++ functions, so no C-only headers leak `NULL` semantics. After the pass, `grep '\bNULL\b' main.cpp db.cpp` should return nothing.

### Integer type unification
- `INT64` → `int64_t`. `UINT32` → `uint32_t`. `_LL(1000)` → `INT64_C(1000)` or just drop the cast (`m_info.connectionTTL * 1000LL` is also fine and more idiomatic).
- Style note from root CLAUDE.md: "Modern code avoids Hungarian notation; older code may use it." This pass nudges `pgsql/` toward modern.

### Oracle comment fix
Trivial one-line edit. Make sure no other Oracle copy-paste residue lurks in the file (grep for "Oracle").

### Missing slash
Trivial one-line edit at `main.cpp:521`.

### Config-parsing helper
The two call sites have **asymmetric** pre/post-parse logic — a naive single-arg helper would silently break behavior. Specifically:

| Aspect                        | Simple block (`/pgsql/...`)         | Servers block (`/pgsql/servers/<id>/...`) |
|-------------------------------|--------------------------------------|--------------------------------------------|
| Initial `id` default          | `"localdb"`                          | `e->getName()` (entry name)                |
| `parseTemplate` failure       | Silent (no warning)                  | `NXLOG_WARNING` + `continue`               |
| Empty `id` post-parse         | Fall back to `s_dbInfo.name`         | `continue` (skip)                          |
| Required: non-empty `name`    | Yes (`s_dbInfo.name[0] != 0`)        | No                                         |

Two implementation options:

- **Option (b) — narrow helper, recommended**: extract only the *shared core* — `s_dbInfo` defaults reset (`server="127.0.0.1"`, `name="postgres"`, `login="netxms"`, `connectionTTL=3600`), `DecryptPassword`, and `s_instances.add(...)`. Each call site keeps its own pre/post logic (initial `id`, parseTemplate-failure handling, empty-`id`/empty-`name` skip-or-fallback rules). Honest about the asymmetry; smaller diff.
- **Option (a) — wide helper**: take a flags struct: `bool fallbackIdToName, bool warnOnParseFailure, bool requireName`. Unifies the call sites at the cost of an opaque parameter. Fragile; not recommended.

Plan implements **option (b)**.

## What Goes Where
- **Implementation Steps** (`[ ]`): five small tasks, build sanity.
- **Post-Completion** (no checkboxes): none.

## Implementation Steps

### Task 1: NULL → nullptr

**Files:**
- Modify: `src/agent/subagents/pgsql/db.cpp`
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] Replace every `NULL` in `db.cpp` and `main.cpp` with `nullptr`. Confirm with `grep -nE '\bNULL\b' src/agent/subagents/pgsql/{db,main}.cpp` returning nothing. (Note: 6 remaining matches in main.cpp are SQL `NULL` keywords inside `_T(...)` string literals — these are SQL syntax and must not be changed. All 7 C++ `NULL` sites — 4 in main.cpp, 3 in db.cpp — converted to `nullptr`.)
- [x] Build cleanly.

### Task 2: Integer-type unification

**Files:**
- Modify: `src/agent/subagents/pgsql/db.cpp`

- [x] In `db.cpp:105`, replace `INT64 connectionTTL = (INT64)m_info.connectionTTL * _LL(1000);` with `int64_t connectionTTL = static_cast<int64_t>(m_info.connectionTTL) * 1000LL;` (commit to `1000LL`; avoids dragging `<cstdint>` macros into a multiply).
- [x] In `db.cpp:154` (or wherever it lives after Block 3 has landed), replace `(UINT32)` cast with `static_cast<uint32_t>(...)`. If Block 3 has already converted this site, leave it. (Block 3 already converted this site.)
- [x] Confirm `grep -nE '\b(UINT32|INT64|_LL\()\b' src/agent/subagents/pgsql/{db,main}.cpp pgsql_subagent.h` returns nothing pgsql-internal (matches in headers from `nms_common.h` etc. are out of scope).
- [x] Build cleanly.

### Task 3: Fix stale Oracle comment

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] Edit the comment at `main.cpp:392` to "Handler for PostgreSQL.Version parameter". (Actual line 424 after prior changes; replaced "Oracle.DBInfo.Version" with "PostgreSQL.Version".)
- [x] `grep -ni Oracle src/agent/subagents/pgsql/*.cpp src/agent/subagents/pgsql/*.h` should return nothing related to this subagent's own logic. (Confirmed: no matches.)

### Task 4: Fix missing leading slash in config path

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] In `main.cpp:521`, change `config->getEntry(_T("pgsql/server"))` to `config->getEntry(_T("/pgsql/server"))`. (Actual line 555 after prior changes — only the `pgsql/server` lookup was missing the leading slash; sibling lookups for `/pgsql/id`, `/pgsql/name`, `/pgsql/login`, `/pgsql/password` already had it.)
- [x] Build and confirm the simple-config path still loads correctly with a server-only entry present. (Build clean via `make -C src/agent/subagents/pgsql`; runtime simple-config exercise is manual / not automatable here — `Config` normalises leading slashes so behavior is unchanged regardless.)

### Task 5: Extract config-parsing helper (narrow / option-b)

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] Introduce `static void ResetDatabaseInfoDefaults()` that zeroes `s_dbInfo` and sets the shared defaults: `connectionTTL = 3600`, `server = "127.0.0.1"`, `name = "postgres"`, `login = "netxms"`. Does **not** set `id` — each call site picks its own initial id.
- [x] Introduce `static void RegisterDatabaseInstance()` that calls `DecryptPassword(s_dbInfo.login, s_dbInfo.password, s_dbInfo.password, MAX_DB_PASSWORD)` and `s_instances.add(new DatabaseInstance(&s_dbInfo))`.
- [x] Refactor the simple `pgsql/...` block (currently `main.cpp:514-535`) to: call `ResetDatabaseInfoDefaults()`; set `s_dbInfo.id = "localdb"`; run `parseTemplate`; keep the existing `s_dbInfo.name[0] != 0` guard and the `id ← name` fallback; on success call `RegisterDatabaseInstance()`. Silent on parseTemplate failure (preserves current behavior).
- [x] Refactor the `pgsql/servers/<id>/...` loop (currently `main.cpp:537-566`) to: call `ResetDatabaseInfoDefaults()`; set `s_dbInfo.id = e->getName()`; run `parseTemplate`; keep the existing `NXLOG_WARNING` + `continue` on failure and the `s_dbInfo.id[0] == 0` skip; on success call `RegisterDatabaseInstance()`. (Note: per-server loop now also memsets via the helper, which clears any leftover password between iterations — minor hardening, not a behavior regression.)
- [x] Confirm compile and that both config paths load identically — manual test (build clean via `make -C src/agent/subagents/pgsql`; runtime exercise of the three config combinations is manual / not automatable here):
  - With only the simple `pgsql/Server=...` etc. block: agent monitors one DB.
  - With only the `pgsql/servers/<id>/...` block: agent monitors the listed DBs.
  - With both: combined behavior, identical to pre-refactor.
  - **Negative path**: malform a `pgsql/servers/<bad>/...` block (e.g. unknown key) and confirm the existing `NXLOG_WARNING` "Error parsing PostgreSQL subagent configuration template <bad>" still appears.

### Task 6: Build sanity

- [x] `make -C src/agent/subagents/pgsql` cleanly with no new warnings. (Verified after clean rebuild — only platform linker note `-dylib_file is deprecated`, unrelated to source.)

### Task 7: Verify acceptance criteria
- [x] All checkboxes in Tasks 1-6 ticked.
- [x] `grep -nE '\bNULL\b|\b(UINT32|INT64|_LL\()\b' src/agent/subagents/pgsql/{db,main}.cpp` returns nothing. (Only SQL `NULL` keywords inside `_T(...)` string literals remain — these are SQL syntax, documented as expected in Task 1.)
- [x] Stale Oracle comment is gone. (`grep -ni Oracle` on the pgsql subagent returns no matches.)
- [x] Config-parsing refactor preserves behavior on all three configurations (simple-only, servers-only, both). (Verified by code review of `main.cpp:570-613`: the four asymmetric aspects from the plan table — initial id, parseTemplate-failure handling, empty-id/empty-name post-parse rules — are all retained at each call site; the helper covers only the shared core.)

### Task 8: [Final] Documentation
- [x] Move this plan to `docs/plans/completed/`.
- [x] If all five blocks have completed, the pgsql subagent audit is done — note in commit message.

## Post-Completion
*Items requiring manual intervention or external systems — informational only.*

No external dependencies. This is a pure code-quality pass.

**Audit closure:**
- After this block lands, all five audit blocks (1-5) are done. The audit document at `netxms-audit-nx_master-2026-05-06.md` (untracked, in repo root) can be archived.
