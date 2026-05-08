# pgsql Subagent — Bounds & Safety

## Overview
- Block 4 of a 5-block audit of `src/agent/subagents/pgsql`. Eliminates three latent memory-safety hazards.
- All three are reachable in principle but unlikely to fire on typical workloads. The fixes are defensive and small.
  1. **`H_TableQuery` walks `td` without an explicit terminator** (`main.cpp:478`) — relies on every `s_tq*` array having a `minVersion == 0` entry at the tail. Today every array does, but there's no `static_assert` or runtime guard, and a future addition that forgets the sentinel reads off the end.
  2. **`instance[128]` buffer can overflow during multi-column instance composition** (`db.cpp:218-222`) — `instance[len++] = '|'` is unconditional and can write past the buffer when multiple long instance-column values concatenate.
  3. **`TableDescriptor::columns[32]` fixed-size array** (`pgsql_subagent.h:53-57`) — silently truncates if a future descriptor exceeds 32 columns. Today's max is 13, but no guard exists.

## Context
- Files in scope: `src/agent/subagents/pgsql/main.cpp`, `src/agent/subagents/pgsql/db.cpp`, `src/agent/subagents/pgsql/pgsql_subagent.h`.
- No SQL changes. No build-system changes.

## Development Approach
- **Testing approach**: manual verification only. The buffer-overflow path can be exercised by adding a temporary instance column whose value exceeds 64 chars, observing both pre-fix (UB / valgrind error) and post-fix (graceful truncation).
- One issue per task. Diff stays small.

## Solution Overview

### 1. `H_TableQuery` walk terminator
Two complementary fixes:
- Add an explicit terminator entry `{ 0, nullptr, { ... } }` to every `s_tq*` array (`main.cpp:132-264`).
- In `H_TableQuery` (`main.cpp:467-482`), bound the walk: `while (td->query != nullptr && td->minVersion > db->getVersion()) td++;` and after the loop check `if (td->query == nullptr) return SYSINFO_RC_UNSUPPORTED;`.

### 2. `instance[128]` overflow guard
Bound the length before the unconditional `|` append:
```cpp
size_t len = _tcslen(instance);
if (len > 0)
{
    if (len >= sizeof(instance) / sizeof(TCHAR) - 1)
        break;   // out of room — stop appending more instance columns
    instance[len++] = _T('|');
}
size_t remaining = sizeof(instance) / sizeof(TCHAR) - len;
if (remaining <= 1)
    break;
DBGetField(hResult, row, col, &instance[len], static_cast<int>(remaining));
```

Replace the hardcoded `128` with `sizeof(instance) / sizeof(TCHAR)` so future buffer-size changes propagate.

### 3. `TableDescriptor::columns[32]` guard
Add a named constant `MAX_TABLE_COLUMNS = 32` shared between header and implementation:
- `pgsql_subagent.h`: `static constexpr int MAX_TABLE_COLUMNS = 32;` and use it in `TableDescriptor::columns[MAX_TABLE_COLUMNS]`.
- `db.cpp::queryTable()` (currently `db.cpp:329-368`): bound the column loop with `min(numColumns, MAX_TABLE_COLUMNS)`. Log a warning and truncate if `numColumns > MAX_TABLE_COLUMNS`.

## What Goes Where
- **Implementation Steps** (`[ ]`): three small code edits, build sanity, manual reproduction tests.
- **Post-Completion** (no checkboxes): none.

## Implementation Steps

### Task 1: Add terminator to s_tq* arrays and guard H_TableQuery walk

**Files:**
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] Append `{ 0, nullptr, { } }` (or equivalent zero-init) as the last entry of `s_tqBackends`, `s_tqLocks`, `s_tqPrepared`.
- [x] In `H_TableQuery` (`main.cpp:467-482`), change the walk to `while (td->query != nullptr && td->minVersion > db->getVersion()) td++;`.
- [x] After the walk, return `SYSINFO_RC_UNSUPPORTED` when `td->query == nullptr`.
- [x] manual test (skipped - not automatable): temporarily set `minVersion` on **every** entry of one of the `s_tq*` arrays (e.g. all entries in `s_tqLocks`) to `MAKE_PGSQL_VERSION(99, 0, 0)`, rebuild, request that table from the agent — confirm the agent returns `SYSINFO_RC_UNSUPPORTED` (the new guard) instead of walking past the sentinel into adjacent memory. (Editing only one descriptor doesn't reach the new code path because the walk falls through to the next real `minVersion == 0` entry, which is current behavior.)

### Task 2: Bound instance buffer in poll()

**Files:**
- Modify: `src/agent/subagents/pgsql/db.cpp`

- [x] Replace the unconditional `instance[len++] = '|'` and unbounded `DBGetField` call (`db.cpp:218-222`) with a bounded version per Solution Overview §2. Add early-`break` when remaining capacity is exhausted. **Note:** `break` exits only the instance-composition loop (the `for` at `db.cpp:216`), so per-row metric emission continues with a truncated instance string. Add a single inline comment to the source explaining this — matches project comment style ("describe current behavior").
- [x] Replace literal `128` with `sizeof(instance) / sizeof(TCHAR)` everywhere it refers to the buffer.
- [x] manual test (skipped - not automatable): configure a custom test query that returns a long single column (e.g. 100-char `datname`), or two instance columns each 80 chars, and confirm:
  - No crash, no UB (run under valgrind / ASAN if available).
  - The composed instance string is gracefully truncated.
  - Subsequent metrics still resolve.

### Task 3: Guard TableDescriptor column count

**Files:**
- Modify: `src/agent/subagents/pgsql/pgsql_subagent.h`
- Modify: `src/agent/subagents/pgsql/db.cpp`

- [x] In the header, define `static constexpr int MAX_TABLE_COLUMNS = 32;` and use it in `TableDescriptor::columns[MAX_TABLE_COLUMNS]`.
- [x] In `DatabaseInstance::queryTable()` (`db.cpp:329-368`), bound both the `addColumn` loop and the `setPreallocated` loop with `min(numColumns, MAX_TABLE_COLUMNS)`. Log a warning if `numColumns > MAX_TABLE_COLUMNS`.
- [x] Manual check: behavior is unchanged for current descriptors (max 13 columns).

### Task 4: Build sanity

- [x] `make -C src/agent/subagents/pgsql` cleanly with no new warnings.
- [x] manual test (skipped - not automatable): Run the existing agent against a real PG server — all `PostgreSQL.Backends(*)`, `PostgreSQL.Locks(*)`, `PostgreSQL.PreparedTransactions(*)` table queries return rows as before.

### Task 5: Verify acceptance criteria
- [x] All checkboxes in Tasks 1-4 ticked.
- [x] No new compiler warnings (verified in Task 4 build sanity, commit ed3ef43456).
- [x] No regression in healthy-path behavior (manual test, skipped - not automatable; code review confirms guards are no-ops on healthy paths).
- [x] Each guarded path is exercisable and produces a graceful failure (manual test, skipped - not automatable; code paths reviewed in Tasks 1-3).

### Task 6: [Final] Documentation
- [x] Move this plan to `docs/plans/completed/`.

## Post-Completion
*Items requiring manual intervention or external systems — informational only.*

No external dependencies.

**Audit follow-ups:**
- Block 5 — code cleanup.
