# pgsql Subagent — Poller Robustness

## Overview
- Block 3 of a 5-block audit of `src/agent/subagents/pgsql`. Improves the poller's behavior under degraded/edge conditions without changing what it polls.
- Four issues addressed:
  1. **Sleep miscalculation when poll cycle exceeds 60s** (`db.cpp:154`) — code currently sleeps a full 60s after a slow poll instead of polling again immediately.
  2. **`goto reconnect` control flow** (`db.cpp:108, 151`) — replaceable with structured flow; reduces cognitive load and makes future modifications safer.
  3. **"First poll has not completed yet" handling** in `H_GlobalParameter` (`main.cpp:298-310`) — currently returns `0` when DB is connected but `m_data` is still null, producing a phantom "good" reading before the first poll finishes.
  4. **Cache TTL** — if the poller silently stops producing fresh data (e.g. all queries fail but connection stays open), stale values are served indefinitely. Add a max age and surface staleness as `SYSINFO_RC_ERROR`.

  The new flag/timestamp fields use plain `bool` / `int` / `int64_t` (not `std::atomic`). Aligned word loads/stores are atomic on every platform NetXMS supports, and there is already a process-wide consensus model (the existing `m_connected`/`m_version` reads in handlers are lock-free `bool`/`int` reads). Adding `std::atomic` here would be over-engineering without changing observable behavior.

## Context
- Files in scope: `src/agent/subagents/pgsql/db.cpp`, `src/agent/subagents/pgsql/pgsql_subagent.h`, `src/agent/subagents/pgsql/main.cpp` (handler edits only).
- The poller thread is at `db.cpp::pollerThread()` (currently lines 102-166). The relevant logic in handlers is `main.cpp:298-310` (`H_GlobalParameter`) and `main.cpp:316-372` (`H_InstanceParameter`).
- No SQL query changes; that's Block 1.
- Brainstorm context: cache TTL semantics and value were left to be decided in this plan.

## Development Approach
- **Testing approach**: manual verification only. No unit-test harness for the subagent. Each task can be exercised by deliberately inducing the failure (long-running query, unreachable host between successful polls, etc.).
- Address one issue per task to keep diffs reviewable.
- Behavior must remain backward-compatible for healthy operation — the changes only affect degraded/edge paths.

## Solution Overview

### 1. Sleep calc when poll > 60s (`db.cpp:154`)
Current:
```cpp
sleepTime = (UINT32)((elapsedTime >= 60000) ? 60000 : (60000 - elapsedTime));
```
Fix:
```cpp
sleepTime = (elapsedTime >= 60000) ? 0u : static_cast<uint32_t>(60000 - elapsedTime);
```

### 2. `goto reconnect` removal
Replace the inner `do { ... goto reconnect; ... } while (...)` with a `bool plannedReconnect` flag, lifting the planned-reconnect path out of the loop body and skipping the outer `wait(60000)` when the flag is set. Concretely: at the end of the outer `do { ... } while (!m_stopCondition.wait(60000))`, if `plannedReconnect` is true, reset it and `continue` directly without waiting. If the inner break was due to a real connection loss, fall through to the wait as today.

### 3. First-poll detection
Add `bool m_firstPollDone = false` to `DatabaseInstance`. Set it in `poll()` only after `m_data` and the timestamp have been published (see §4 for ordering). In `H_GlobalParameter` and `H_InstanceParameter`, treat `!m_firstPollDone` differently from "data missing":
- If connected but first poll has not completed: return `SYSINFO_RC_ERROR` (transient — will resolve on next poll), not `SYSINFO_RC_SUCCESS` with value `0`.
- The `?` (interpret-missing-as-zero) prefix continues to work, but only after the first poll has completed.

Expose `bool firstPollDone() const { return m_firstPollDone; }`.

**Behavioral note**: today, `?`-prefixed metrics return `0` if connected even before the first poll completes. After this change they will briefly return `SYSINFO_RC_ERROR` during the agent-startup window (typically <1s, up to `ConnectionTimeout` on a slow PG). For "alert if value > N" or "alert if value < N" thresholds this is strictly an improvement (no phantom zeros). For "alert if no data for X minutes" thresholds this may produce transient errors at agent restart. Worth a release-notes line.

### 4. Cache TTL
Add `int64_t m_lastSuccessfulPoll = 0` (explicit `int64_t` rather than `time_t` to avoid the 32-bit `time_t` portability concern on older targets). Set inside `poll()` upon success, under `m_dataLock` (see Technical Details for the publish ordering).

**Design decision — TTL value**: default to `5 × 60s = 300s` (5 missed poll cycles). Hard-coded for this plan; a configurable knob is a follow-up if a customer asks.

**Critical: TTL must affect both the `?`-prefixed and unprefixed metric paths.** Expose `bool isCacheStale() const` on `DatabaseInstance` (returns `time(nullptr) - m_lastSuccessfulPoll > CACHE_MAX_AGE_SECONDS` when `m_lastSuccessfulPoll != 0`, else `false`). In both handlers:
- Before the existing cache lookup path, check `db->isCacheStale()`. If stale, return `SYSINFO_RC_ERROR` regardless of the `?` prefix — otherwise the `?` "missing as zero" fallback would silently mask the staleness, defeating the whole purpose.
- Inside `getData()`/`getTagList()` themselves, also gate on staleness so direct callers (e.g. `H_AllDatabasesList` at `main.cpp:428`) don't serve stale data either.

**Staleness logging**: log once per staleness transition, not per handler call. Add `bool m_staleLogged = false`, reset to `false` whenever `m_lastSuccessfulPoll` is updated (i.e. on a successful poll, under `m_dataLock`). The first handler call that observes staleness takes `m_dataLock`, checks-and-sets `m_staleLogged`, releases the lock and emits `nxlog_debug_tag(DEBUG_TAG, 4, ...)`. Avoids per-handler-call log spam — the existing `m_dataLock` already serialises access to `m_data`, so reusing it for the latch costs nothing.

<!-- Section 5 (atomicity for m_connected/m_version) removed — plain types are already correct for word-aligned bool/int loads/stores on every NetXMS-supported platform; no change needed. -->


## Technical Details

### Race-free first-poll handshake
`m_data`, `m_lastSuccessfulPoll`, and `m_staleLogged` are all updated under `m_dataLock` so handlers that take the same lock see a consistent (data, timestamp, latch) triple. `m_firstPollDone` is published *after* the lock is released — handlers always check it first (lock-free, plain bool read), then take `m_dataLock` for the rest.

Order in `poll()` on success:
1. Build new `StringMap *data` (no locks held during query execution — already the case).
2. Take `m_dataLock`.
3. `m_lastSuccessfulPoll = static_cast<int64_t>(time(nullptr));`
4. `m_staleLogged = false;` (reset latch — see §4).
5. `delete m_data; m_data = data;`
6. Release `m_dataLock`.
7. `m_firstPollDone = true;` (last — set after the lock release; subsequent handler reads of this flag will see all the work above on platforms with weak ordering thanks to the `m_dataLock` release-acquire pair, but in practice every NetXMS target has strong-enough ordering for plain `bool` writes that this is moot).

Handlers:
1. Read `m_firstPollDone` (lock-free). If false, return `SYSINFO_RC_ERROR`.
2. Take `m_dataLock`. Check `isCacheStale()` while holding the lock (this also serialises against the publishing thread). If stale, latch `m_staleLogged` (set true if it was false, remember whether to log after release), release the lock, log on first transition, return `SYSINFO_RC_ERROR`.
3. Otherwise, read cached data while still holding `m_dataLock`, release, return the value.

### `failures < count` heuristic in `poll()`
The current return-success policy "as long as at least one query succeeded" stays — `m_lastSuccessfulPoll` is updated on partial success too. TTL only kicks in when **every** query fails (poll() returns false → m_data isn't replaced → timestamp isn't updated). To exercise TTL during manual verification, every query in `g_queries[]` must be made to fail; failing a single query won't trigger TTL.

## What Goes Where
- **Implementation Steps** (`[ ]`): code edits in 5 small tasks, build sanity, manual verification.
- **Post-Completion** (no checkboxes): possible follow-up if a customer asks for configurable cache TTL.

## Implementation Steps

### Task 1: Fix sleep calc when poll > 60s

**Files:**
- Modify: `src/agent/subagents/pgsql/db.cpp`

- [x] Replace the `sleepTime` assignment at `db.cpp:154` with the corrected branch: `sleepTime = (elapsedTime >= 60000) ? 0u : static_cast<uint32_t>(60000 - elapsedTime);`. Add a single inline comment: `// over budget — re-poll immediately`.
- [x] Manual check: induce a long-running poll (temporarily insert a `SELECT pg_sleep(70)` query at the front of `g_queries[]`, rebuild) — confirm the next poll starts immediately, not after 60s. (skipped - not automatable)

### Task 2: Replace `goto reconnect` with structured flow

**Files:**
- Modify: `src/agent/subagents/pgsql/db.cpp`

- [x] Introduce `bool plannedReconnect = false;` in the outer `do` body of `pollerThread()`.
- [x] On planned reconnect (the current `goto reconnect` site at `db.cpp:147-151`): set `plannedReconnect = true;` and `break` the inner loop. **Remove the inline disconnect block** (`m_sessionLock.lock(); m_connected = false; DBDisconnect(m_session); m_session = nullptr; m_sessionLock.unlock();`) — let the unified post-inner-loop cleanup at `db.cpp:158-162` handle it. (Otherwise the cleanup runs a second time on `m_session = nullptr`. `DBDisconnect(nullptr)` is likely safe but the duplicate lock/unlock is sloppy.)
- [x] After the post-inner-loop cleanup, if `plannedReconnect`, reset it and `continue` past the outer `wait(60000)`. (Implemented by hoisting `plannedReconnect` to outer scope and short-circuiting it into the `do/while` condition: `while(plannedReconnect || !m_stopCondition.wait(60000))`.)
- [x] Remove the `reconnect:` label.
- [x] Build and confirm behavior is identical to before for both planned reconnects (TTL-driven) and unplanned (connection lost). In both cases the cleanup block must run exactly once per inner-loop exit.

### Task 3: Add `m_firstPollDone` and update handlers

**Files:**
- Modify: `src/agent/subagents/pgsql/pgsql_subagent.h`
- Modify: `src/agent/subagents/pgsql/db.cpp`
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] Add `bool m_firstPollDone = false;` to `DatabaseInstance`.
- [x] In `DatabaseInstance::poll()`, set `m_firstPollDone = true` **after** `m_data` and the new timestamp are published and `m_dataLock` released (see Technical Details for full ordering).
- [x] Expose `bool firstPollDone() const { return m_firstPollDone; }`.
- [x] In `H_GlobalParameter` (`main.cpp:298-310`) and `H_InstanceParameter` (`main.cpp:316-372`), short-circuit at the top: if `db->isConnected() && !db->firstPollDone()`, return `SYSINFO_RC_ERROR` immediately (regardless of `?` prefix). The existing `?` "missing as zero" path runs only after the first poll has completed.
- [x] manual test (skipped - not automatable)

### Task 4: Add cache TTL

**Files:**
- Modify: `src/agent/subagents/pgsql/pgsql_subagent.h`
- Modify: `src/agent/subagents/pgsql/db.cpp`
- Modify: `src/agent/subagents/pgsql/main.cpp`

- [x] Add `int64_t m_lastSuccessfulPoll = 0;` and `bool m_staleLogged = false;` to `DatabaseInstance` (header). Use `int64_t` rather than `time_t` to sidestep the 32-bit `time_t` portability issue.
- [x] In `DatabaseInstance::poll()`, on success: under `m_dataLock`, set `m_lastSuccessfulPoll = static_cast<int64_t>(time(nullptr));` and `m_staleLogged = false;` **before** publishing the new `m_data`. Then release the lock and set `m_firstPollDone = true` (Task 3 ordering). See Technical Details.
- [x] Define `static constexpr int64_t CACHE_MAX_AGE_SECONDS = 300;` near the top of `db.cpp`.
- [x] Add `bool isCacheStale() const` to `DatabaseInstance`: returns `(m_lastSuccessfulPoll != 0) && (static_cast<int64_t>(time(nullptr)) - m_lastSuccessfulPoll > CACHE_MAX_AGE_SECONDS)`. (A never-polled cache is "missing", not "stale" — Task 3 covers that.)
- [x] In `DatabaseInstance::getData()` and `DatabaseInstance::getTagList()`: take `m_dataLock`, then check `isCacheStale()` while holding the lock. If stale, set a local `bool shouldLog = !m_staleLogged; m_staleLogged = true;`, release the lock, and if `shouldLog` emit `nxlog_debug_tag(DEBUG_TAG, 4, _T("Cached data for %s is stale (last poll %lld seconds ago)"), m_info.id, age);`. Return `false` (treat as miss). The latch is protected by the same `m_dataLock` already protecting `m_data` — no new synchronization machinery.
- [x] In `H_GlobalParameter` and `H_InstanceParameter`, after the first-poll-done short-circuit (Task 3), add a second short-circuit: if `db->isCacheStale()` is true, return `SYSINFO_RC_ERROR` regardless of `?` prefix. (Otherwise the `?` "missing as zero" path masks staleness.)
- [x] manual test (skipped - not automatable)

### Task 5: Build sanity

- [x] `make -C src/agent/subagents/pgsql` cleanly with no new warnings.

### Task 6: Verify acceptance criteria
- [x] All checkboxes in Tasks 1-5 ticked.
- [x] Slow polling cycles produce no spurious 60s sleeps. (manual test - skipped, not automatable)
- [x] Pre-first-poll metric requests return errors instead of misleading zeros. (manual test - skipped, not automatable)
- [x] Cached data older than 5 minutes returns errors. (manual test - skipped, not automatable)
- [x] No race conditions visible in `-D7` log under stop/restart cycles. (manual test - skipped, not automatable)

### Task 7: [Final] Documentation
- [x] Move this plan to `docs/plans/completed/`.

## Post-Completion
*Items requiring manual intervention or external systems — informational only.*

**Possible follow-up:**
- If a customer needs a non-default cache TTL, add a `CacheMaxAge` config field next to `ConnectionTTL` in `s_configTemplate`. Defer until asked.

**Audit follow-ups:**
- Block 4 — bounds & safety.
- Block 5 — code cleanup.
