# Agent action to reset the local database

## Overview

Add the agent action `Agent.ResetLocalDatabase`. It sets aside the agent's local SQLite database (`nxagentd.db`) and the databases of connected external subagent loaders, then restarts the agent so each one starts with a fresh database. Administrators can then recover from `SYS_AGENT_LOCAL_DATABASE_PROBLEM` ("could not open database" / schema upgrade failure) or from a corrupted database without shell access to the host. It is run with `nxaction` or an object tool of type "agent action". There is no console menu entry.

## Context and decisions

- The failure is detected only at startup. `OpenLocalDatabase()` sets `FAIL_OPEN_DATABASE` / `FAIL_UPGRADE_DATABASE` once, and they are never cleared. The SQLite driver opens with `SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE`, so a database file that is missing is simply recreated.
- **The reset must happen across a restart.** The DB handle is used by data collection, policies, the registry, logwatch, file monitoring and other code. Several of them skip their own setup when the DB is absent, so swapping the database inside a running agent is ruled out.
- **The reset request travels as a marker file.** A restart command-line flag was ruled out: a Windows service restart goes through the `-X` helper (`StopAgentService` / `StartAgentService`), and systemd restarts the unit. Both start the agent with its registered command line, so the flag would be lost. The marker is read in the new process after the old one has exited.
- **No in-agent alternative exists.** A user-defined action script that stops the service and deletes the files does not work on Windows. Action child processes run in a `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` job object, so stopping the service kills the script.
- **Precondition: nothing else has the DB open.** The basis is that the Windows service path (`WaitForService(SERVICE_STOPPED)`) and systemd wait for the old process to exit.
  - Console (non-service) mode of the master is out of scope; whoever runs it can delete the files by hand.
  - External subagent loaders are a production case on the `-X <pid>` restart path. On Windows the watchdog starts them hidden (`-H -G EXT:<name>`). On Unix, loaders started outside systemd, and any case where `RestartService()` cannot identify a unit, use the same fallback. That path continues after `TerminateProcess` / `SIGKILL` without confirming the process has exited. Task 2 makes it confirm the exit, and skips the reset on any start where the exit could not be confirmed.
  - An SQLite `BEGIN EXCLUSIVE` probe and a new cross-process lock were both considered and rejected. Codex's tests showed the probe misses idle connections and rollback-mode handles, and a lock is out of proportion for a recovery action.
- **Backup naming follows SQLite's convention.** SQLite looks for `<db>-wal` next to the database. A backup `nxagentd.db.bak` therefore needs its WAL named `nxagentd.db.bak-wal`; `nxagentd.db-wal.bak` would lose committed rows still held in the WAL. One backup generation is kept.
- **Policies get no special handling.** A fresh DB has an empty `agent_policy` table, so `RemoveOrphanPolicyFiles()` deletes the deployed policy files at startup. This is accepted:
  - config policies are merged during config loading, before the DB is opened, so the agent that does the reset already runs with them;
  - each server's configuration poll redeploys its bound policies, because the empty inventory reports version 0;
  - the rows come back with their correct owner server IDs.

  A grace window and re-registering the existing files were both considered and rejected. Re-registering cannot work: the owner server IDs are not recoverable from the files, and `GetPolicyInventory` filters on server ID.
- **Agent ID.** The ID survives through `nxagentd.id` (in the NetXMS data directory), provided that file exists and parses as a valid ID. Today, when the ID is recovered from that file, it is not written back to the DB metadata, so a fresh DB stays without `AgentId`.
- **External subagent loaders** are separate `nxagentd` processes. Each has its own DB `nxagentd.db.<ExternalMasterAgent>`, built from its own `DataDirectory`, and possibly runs as another user. Each loader therefore writes its own request marker. The master asks for this with a request/response command, because `CMD_RESTART` is fire-and-forget: on Windows the master's shutdown calls `DisconnectNamedPipe`, which can discard a message the loader has not read yet.
- **The action is accepted asynchronously.** `nxaction` defaults to a 5-second timeout for the whole action, and object tools use the node's agent timeout. Asking each loader in turn and waiting up to 5 seconds for each reply would overrun that budget, so a caller could see a failure while the reset still happens. The handler therefore writes the master's request synchronously, so an I/O error is still reported, then does the loader fan-out and the restart on a thread-pool thread and returns success. The success reply means only "request accepted". The outcome is checked afterwards through `Agent.LocalDatabase.Status` and the agent log.
- **Out of scope:**
  - console UI entry;
  - netxms-doc admin guide update (separate repository);
  - restart-helper behaviour when the master runs in console mode;
  - `Agent.LocalDatabase.Status` never reporting 2 (GH #4045, fixed separately).

## Shared interfaces

- **DB file name:** the construction of the DB file name (data directory + `nxagentd.db` + `.<ExternalMasterAgent>` for loaders) moves out of `OpenLocalDatabase()` into a helper. The request function and the startup reset call that helper directly, so they do not depend on whether an open was attempted or got as far as building the name. The helper checks for truncation at `MAX_PATH` and returns failure instead of building a truncated path.
- **Marker files.** Two markers next to the DB; renaming one to the other is the atomic phase transition:
  - `<db>.reset` is a pending request. Its content is the request time as a UNIX timestamp; missing or unparseable content counts as stale.
  - `<db>.reset-active` means relocation is in progress. It is recovered whatever its age.
- **Request function** (`localdb.cpp`), used by both the master action and the loader command handler, and serialized by a static mutex. It returns one of three results, which the action needs to tell apart:
  - **created:** it wrote `<db>.reset` with the current time, replacing an existing `.reset`;
  - **in progress:** `.reset-active` exists;
  - **failed.**

  It never deletes backups, so a repeated or concurrent request cannot disturb a reset in progress.
- **NXCP:** new command `CMD_RESET_LOCAL_DATABASE`, sent master → loader over the external subagent pipe. The loader replies with `CMD_REQUEST_COMPLETED` carrying `VID_RCC`. No new `VID_*` fields. If the code is appended at the end of the range, `NXCPMessageCodeName()` needs both the name array and its range check updated. Older loaders already reply `ERR_UNKNOWN_COMMAND`.

## Implementation Steps

### Task 1: Markers, request function and startup reset

**Components:** agent core (`localdb.cpp`, `localdb.h`, `nxagentd.cpp` startup and `--reset-identity`).

The markers are consumed only during daemon startup (`Initialize()`, before the DB is opened). They are not consumed at all on a start where the restart helper could not confirm that the predecessor exited (see Task 2). In that case the markers stay in place, and a `.reset` left behind expires under rule 1. Choose between a separate startup step and a parameter to `OpenLocalDatabase()` according to the API rules in CLAUDE.md. Startup rules:

1. **`.reset` older than 15 minutes, or stale content:** delete it, log, open normally. This covers a loader whose prepare reply timed out and an old leftover request.
2. **Valid `.reset`:**
   1. delete the previous backup set (`<db>.bak`, `<db>.bak-wal`, `<db>.bak-journal`). Every deletion must succeed or report `ENOENT`. Otherwise log the error, delete `.reset` and open the old DB: an old `.bak-wal` left next to a new `.bak` would be replayed into it;
   2. rename `.reset` to `.reset-active`. If that fails, same handling as 2.1;
   3. if `<db>` exists, rename it to `<db>.bak`. If that rename fails, log `NXLOG_ERROR` with the system error text, delete `.reset-active` and open the old DB. Nothing else has been moved at this point, because the main file always goes first.
3. **`.reset-active`** (reached straight from rule 2, or on a later start after a crash):
   1. move whichever of `<db>`, `<db>-wal`, `<db>-journal` still exist to `<db>.bak`, `<db>.bak-wal`, `<db>.bak-journal`;
   2. delete `<db>-shm`;
   3. treat `ENOENT` as already done and any other error as a failure.

   On any failure, set `FAIL_OPEN_DATABASE`, register the `localdb-open` problem, keep `.reset-active` and do not open the DB.
4. **Finish:** delete `.reset-active`. If that delete fails, do not open the DB (same failure handling as rule 3). Otherwise log the reset at `NXLOG_WARNING` under tag `localdb`, saying the old file set was archived as `<db>.bak`. The normal open then creates a fresh DB, and the existing open logging and status report that outcome.

`--reset-identity` checks for either marker before touching anything (SQLite or identity files), and refuses with an error if one exists. Opening the DB there could recreate the main file beside a live WAL during an interrupted relocation.

In the agent ID block of `Initialize()`, an ID recovered from `nxagentd.id` is also written to the `AgentId` metadata (master only, not in external subagent loaders).

- [x] DB file name helper used by the open, the request function and the startup reset
- [x] request function: writes `.reset`, is serialized, returns created / in progress / failed, and never deletes backups
- [x] failed deletion of a previous backup file or a failed marker rename abandons the reset before any live file is moved
- [x] startup applies rules 1–4; a fresh DB is never opened while either marker exists
- [x] `--reset-identity` refuses while a marker exists
- [x] agent ID recovered from `nxagentd.id` is written to DB metadata by the master
- [ ] agent builds on Linux and Windows

➕ Implemented as `ProcessLocalDatabaseResetRequest()` (separate startup step called before `OpenLocalDatabase()`, returns false when the DB must not be opened), `RequestLocalDatabaseReset()` returning `enum class LocalDatabaseResetRequest`, and `IsLocalDatabaseResetPending()` for `--reset-identity` (exit code 8). The name helper `GetLocalDatabaseFileName()` stays file-local in `localdb.cpp` and logs the too-long-name error itself.
➕ The log tag is the existing `DEBUG_TAG_LOCALDB` (`db.agent`), not `localdb`.
➕ Renames during the reset never replace an existing target (checked before `rename()`, since POSIX `rename()` would silently overwrite a backup).
➕ Rule 2.3: when the main file cannot be renamed and `.reset-active` then cannot be deleted either, the DB is not opened (rule 3 failure handling) instead of opening the old DB beside a live `.reset-active`.
➕ A request timestamp up to 15 minutes in the future is accepted as valid (clock adjustment between request and restart).
➕ The `.reset` file is read in binary mode and must consist of exactly 1–18 decimal digits followed by nothing, `\n` or `\r\n` (checked by byte count, so embedded NULs count); a read error or a file of 32 bytes or more counts as stale, as does anything else. The time is compared against `now ± 15 min` bounds without subtracting the parsed value.
➕ Whenever a `.reset` that is being discarded (rules 1, 2.1, 2.2) cannot be deleted, the DB is not opened and the open failure is reported the same way as for `.reset-active`, so SQLite never creates a fresh DB beside a marker.
➕ The request function has no "already pending" result (revmux finding): an existing `.reset` is rewritten with the current time, because a marker left by a failed or deferred restart can expire before the retry's restart and would then be discarded at startup. If the rewrite fails the marker is removed and the request returns failure; a marker reaching this path has no job owning it, so nothing else depends on it.
➕ The `AgentId` write-back of an ID recovered from `nxagentd.id` is skipped in external subagent loaders (revmux finding), which share the master's `nxagentd.id`: a loader would otherwise keep a copy of the master ID in its own DB, which goes stale after `--reset-identity` and gets written back into `nxagentd.id`.
⚠️ Build check for Task 1: full build on macOS (clang); Windows only as a MinGW x64 `-fsyntax-only` compile of `localdb.cpp` and `nxagentd.cpp` (no Windows SDK tree here). No Linux build.

### Task 2: `Agent.ResetLocalDatabase` action, loader command, loader restart wait

**Components:**
- NXCP constants: `include/nms_cscp.h`, `NXCPMessageCodeName()` in `src/libnetxms/nxcp.cpp`, `src/java-common/netxms-base/src/main/java/org/netxms/base/NXCPCodes.java`;
- agent core: `nxagentd.cpp`, `extagent.cpp`, `master.cpp`.

The action is registered next to `Agent.Restart`, only in the master (the built-in action block is already skipped for loaders) and only when the local DB is enabled. Check that the registration point is reached after `DisableLocalDatabase` has been applied from config; `g_dwFlags` starts with `AF_DISABLE_LOCAL_DATABASE` set.

**Handler.** Only one background operation runs at a time. A static flag, checked and set under the request function's mutex, says whether a job owns the reset.
1. If a job owns the reset, return success without queueing anything.
2. Call the request function:
   - **failed:** return `ERR_IO_FAILURE`;
   - **in progress** (an agent that started with `.reset-active` after a failed move): queue only `RestartAgent()` to retry the master's relocation, without asking the loaders again;
   - **created:** queue the full job.
3. Return success.
4. The full job, which owns the reset until it finishes:
   1. for each connected loader, send `CMD_RESET_LOCAL_DATABASE` and wait for the reply, using the existing request-ID / `waitForMessage` pattern and its 5-second timeout;
   2. log each loader's outcome by name at `NXLOG_WARNING` (disconnected, timed out, RCC error). A loader failure does not stop the master reset;
   3. call `RestartAgent()`. If it fails, log the error and delete the master's `.reset` (only `.reset`, never `.reset-active`).

   A loader that acknowledged has staged its request and may still reset. That depends on the `CMD_RESTART` that `RestartAgent()` sends to loaders before it launches the master's own restart.

**Loader side:** `MasterAgentListener` handles `CMD_RESET_LOCAL_DATABASE`.
- With the local DB disabled, it replies with an explicit error RCC.
- Otherwise it calls the request function and replies with the RCC.
- It does not restart itself; the `CMD_RESTART` that follows does that.

**Restart helper, `-X <pid>` path** (`DoRestartActions`, used by loaders on Windows and by Unix agents not managed by systemd):
- **Windows:** after `TerminateProcess`, wait for the process handle to signal.
- **Unix:** after `SIGKILL`, poll until `kill(pid, 0)` reports `ESRCH`, with a bound.
- If termination, the wait or the poll fails, or the bound runs out, the helper sets a startup flag that stops the markers from being consumed on this start, and logs it. The agent still starts normally on its old DB.

The service and systemd paths are unchanged.

- [x] new command constant added in C++ and Java, with a name mapping (array and range check) in `NXCPMessageCodeName()`
- [x] action registered under the conditions above; returns `ERR_IO_FAILURE` when the request cannot be written, otherwise success right away
- [x] queued work asks loaders to prepare, logs per-loader outcomes, then restarts; the master's `.reset` is removed when the restart fails
- [x] loader handler replies with an explicit RCC, including for a disabled local DB
- [x] only one background reset job at a time; an in-progress retry only restarts the master
- [x] the `-X` restart path confirms the predecessor has exited on Windows and Unix, and markers are not consumed when it can't
- [ ] agent builds on Linux and Windows; Java `netxms-base` builds

➕ `CMD_RESET_LOCAL_DATABASE` is `0x023E`, appended after `CMD_GET_AGENT_LOG_FILE`.
➕ `ExternalSubagent::sendMessage()` now writes under a new `m_pipeLock`, and `connect()` sets and clears `m_pipe` under the same lock. Before this, a loader disconnect could free the pipe while a background writer was still using it. The race also affected the existing request paths (`getParameter()`, `restart()` and the rest); they now share the fix.
➕ The job-ownership flag has its own mutex in `nxagentd.cpp` (`s_localDatabaseResetJobLock`), held across the flag check, the `RequestLocalDatabaseReset()` call and the flag set, instead of sharing the request function's mutex in `localdb.cpp`. Only the action handler takes both, always in that order.
➕ The flag is cleared only when `RestartAgent()` fails. After a successful restart it stays set until the process exits, so a second action call in that window cannot queue a second restart helper.
➕ Loader RCCs: `ERR_SUCCESS` for created / in progress, `ERR_IO_FAILURE` when the request cannot be written, `ERR_FUNCTION_NOT_SUPPORTED` when its local DB is disabled. The master logs per loader: not connected, connection broken, timeout (`ERR_REQUEST_TIMEOUT`), rejected with the RCC, or accepted (`NXLOG_INFO`).
➕ On a start where the predecessor's exit was not confirmed, `DeferLocalDatabaseResetRequest()` replaces `ProcessLocalDatabaseResetRequest()`: `.reset` is left in place (it expires under rule 1), and if `.reset-active` exists the DB is not opened (the main file may already be moved, so SQLite would create a fresh one beside a leftover WAL). The unconfirmed exit is logged at startup under `startup`, since logging is not open yet inside `DoRestartActions()`.
➕ The action description says that the action restarts the agent and its loaders, and the restart-failure message no longer claims that no database is reset: loaders that accepted the request have already been sent `CMD_RESTART`, and an in-progress relocation completes on the next start (revmux findings).
➕ Bounds: Windows waits 10 s for the handle after `TerminateProcess`; `OpenProcess` failing with `ERROR_INVALID_PARAMETER` counts as already exited, any other failure as not confirmed. Unix polls `kill(pid, 0)` every 200 ms for up to 10 s after `SIGKILL`; exit is confirmed only by `ESRCH`.
⚠️ Build check for Task 2: full macOS build of libnetxms and agent core; MinGW x64 `-fsyntax-only` on `localdb.cpp`, `nxagentd.cpp`, `extagent.cpp`, `master.cpp` and `nxcp.cpp`; `netxms-base` compiles with Maven. No Linux build.

### Task 3: Manual verification

There is no unit-test harness for agent core, so this is verified by hand with an installed agent and server. Cases:

- Linux agent under systemd, healthy DB:
  - `nxaction <host> Agent.ResetLocalDatabase` returns success at once and produces `nxagentd.db.bak` (plus `.bak-wal` if a WAL was present), a fresh `nxagentd.db` and the warning in the log;
  - with a valid `nxagentd.id`, the agent ID is unchanged and present in the new DB metadata;
  - policies are redeployed on the next configuration poll.
- Windows service, same checks. Then hold `nxagentd.db` open from another process with read and write sharing but without delete sharing, and run the action: the rename failure is logged and the agent starts on the old DB.
- A database truncated to a few bytes (open or schema failure): the action recovers it, and `Agent.LocalDatabase.Status` returns 0 after the restart.
- External subagent loaders:
  - with the same data directory as the master and with a separate one: both databases are reset;
  - on Windows, a loader started by the watchdog is reset too;
  - with a loader stopped, a loader with `DisableLocalDatabase`, an older loader that doesn't know the command (`ERR_UNKNOWN_COMMAND`), and a delayed reply: the master still resets and the log names each loader's outcome.
- Interrupted relocation: create `.reset-active` by hand with the main file already moved and a real `-wal` left behind (a fixture with retained WAL), then start the agent. The WAL is moved and a fresh DB opens. Repeat with a sidecar that can't be moved: the agent does not open a DB and keeps `.reset-active`. Running the action again in that state leaves `.bak` intact.
- A `.reset` older than 15 minutes with an intact main file is discarded. An old `.reset-active` is still recovered.
- A pending `.reset` when the main file is already missing: no error, fresh DB.
- `nxagentd --reset-identity` refuses while either marker exists and touches no files.
- Two concurrent `nxaction` calls: one background job, one restart, no lost backup.
- A previous `.bak-wal` that can't be deleted (e.g. made read-only by changing its directory's permissions): the reset is abandoned before anything is moved, and the agent opens the old DB.
- A Unix loader started outside systemd (`-d`): the reset waits for the predecessor to exit. With the exit not confirmed (simulated), the markers stay and the agent opens its old DB.

- [ ] all cases above verified

Verification on macOS arm64, 2026-11-24, using the built `src/agent/core/nxagentd` with scratch configurations, data directories and the built SQLite driver:

- [x] A valid `.reset` archived the healthy main database and a live WAL, initialized a fresh database, logged the reset, and restored the unchanged agent ID from `nxagentd.id` into `AgentId` metadata. The previous backup set was replaced. A `.reset` with no main database also initialized a fresh one.
- [x] Real retained WAL fixtures were checked in both phases. A pending `.reset` moved the main database and WAL into a readable backup with the committed row. A `.reset-active` with the main database already in `.bak` moved the remaining WAL and preserved the committed row.
- [x] A conflicting `.bak-wal` blocked an active relocation before opening SQLite. The marker, old main backup and live WAL remained in place. Removing the conflict and starting again completed the reset. A previous `.bak-wal` that could not be deleted cancelled a pending reset before moving the live database, which remained readable.
- [x] An expired `.reset` and malformed marker contents were discarded while the old database remained readable. An old `.reset-active` was recovered. An undeletable invalid `.reset` prevented SQLite from opening or creating a database beside the marker.
- [x] `--reset-identity` returned exit code 8 for each marker and preserved byte-for-byte hashes of the database, identity file and marker. A corrupt database failed to open, then a valid `.reset` archived the corrupt bytes and initialized a fresh database.
- [x] On the `-X` fallback, a disposable predecessor that exited and was reaped allowed reset processing. With a predecessor that ignored SIGTERM and remained unreaped after SIGKILL, the 10-second confirmation bound left `.reset` or `.reset-active` untouched. A pending marker allowed the old database to open, while an active marker prevented database creation beside the archived main file.
- [x] The macOS agent and `libnetxms` built, and Java `netxms-base` compiled offline. Task 1 and Task 2 also had MinGW x64 syntax checks on the changed C++ files.

⚠️ The following planned integration cases remain unverified in this environment: Linux and Windows builds; Linux systemd and Windows service restarts; a Windows sharing-mode rename failure; `nxaction` return timing, retry during an active reset, two concurrent action calls and `Agent.LocalDatabase.Status`; policy redeployment by a server; connected loader fan-out with shared or separate data directories, including the Windows watchdog loader, disabled local DB, older command version and delayed reply. The local sandbox disallows TCP and Unix socket listeners, and no Linux, Windows or management server runtime is available here. The macOS `-X` checks exercise the Unix fallback rules without a connected loader.

## Tandem

| Task | Writer | Reviewer |
|------|--------|----------|
| 1 | claude | codex |
| 2 | claude | codex |
| 3 | codex | claude |
