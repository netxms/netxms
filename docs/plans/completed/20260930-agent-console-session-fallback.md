# Agent console session fallback when Terminal Services is stopped

## Overview
- GH issue #3729: on Windows hosts where `TermService` is stopped (XP Embedded ATMs), `WTSEnumerateSessions()` fails with `RPC_S_INVALID_BINDING` and `WTSQueryUserToken()` fails too, so the agent never starts external subagents (`ExternalSubagentWatchdog`) or user agents (`UserAgentWatchdog`, `ExecuteInAllSessions`)
- fix: when a WTS call fails, fall back to the console session (`WTSGetActiveConsoleSessionId()`) and take the user token from an interactive-user process in that session
- fallback is per call, with no stored mode or state: the WTS path is always tried first, so behavior returns to normal as soon as `TermService` runs
- side fix: on XP the console is session 0, which `ExecuteInAllSessions` currently skips even with `TermService` running; the skip becomes "session 0 that is not the console"

## Context (from discovery)
- `src/agent/core/exec.cpp:265-367` - `ExecuteInSession()`, `ExecuteInAllSessions()`, `WTS_DEBUG_TAG`; includes `<WtsApi32.h>` at 27, no `<tlhelp32.h>`
- `src/agent/core/watchdog.cpp:25-26` - includes `<WtsApi32.h>`, `<tlhelp32.h>`
- `src/agent/core/watchdog.cpp:204` - local forward declaration of `ExecuteInSession`
- `src/agent/core/watchdog.cpp:224-285` - `UserAgentWatchdog()` (`WTSEnumerateSessions` + `WTSEnumerateProcesses`, case-sensitive `_tcscmp` at 252)
- `src/agent/core/watchdog.cpp:425-471` - `ReconcileExternalSubagentProcesses()`, existing Toolhelp snapshot pattern
- `src/agent/core/watchdog.cpp:479-520` - `IsSessionSuitableForExternalSubagents()` (excluded users via `WTSQuerySessionInformation`)
- `src/agent/core/watchdog.cpp:595-642` - `ExternalSubagentWatchdog()` enumeration; debug log added in 0e2f99d263 becomes redundant
- `src/agent/core/nxagentd.h:38-40` - Windows include block (`<aclapi.h>`); `1010-1021` - Windows-only declarations (`ExecuteInAllSessions`)
- `src/agent/core/nxagentd.cpp:1726` - `AutoStartUserAgent` calls `ExecuteInAllSessions`
- `src/libnetxms/tools.cpp:~5294` - `GetFileOwner()`, style reference for `LookupAccountSid` with explicit buffer sizes
- `doc/internal/debug_tags.txt` - `wts` tag not registered
- design review notes (Fable): `~/.claude/plans/design-review-request-shimmering-river.md`

## Development Approach
- **testing approach**: Regular. The code is Win32-only with no unit-test harness for agent core watchdogs. The per-task gate is a clean syntax-only cross compile of every touched file for both the XP x86 and the x64 targets (command below). Behavior is verified manually (see Post-Completion)
- complete each task fully before moving to the next; one task = one commit
- C++11 only, must run on XP: no API newer than XP in the new code. The gate compiles with `-std=gnu++14` (`gnu++11` fails at `include/nms_common.h:754` on Windows), so C++11 conformance is checked by review, not by the gate
- no comments except where behavior is unexpected (e.g. why the earliest process wins)
- **CRITICAL: update this plan file when scope changes during implementation**

Syntax check (zsh syntax - run via `zsh -c` from the worktree root; `netxms-build-tag.h` exists only in the main checkout's `build/`, hence the extra `-I`):
```bash
O=$(brew --prefix openssl)/include; P=$(brew --prefix pcre2)/include
B=$(git rev-parse --git-common-dir)/../build
FLAGS="-std=gnu++14 -fsyntax-only -Wall -Wno-unknown-pragmas -Wno-sign-compare -DUNICODE -D_UNICODE -D_WITH_ENCRYPTION -Iinclude -Ibuild -I$B -I$O -I$P -Isrc/agent/core"
for f in exec watchdog nxagentd; do
  i686-w64-mingw32-g++ -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 ${=FLAGS} src/agent/core/$f.cpp
  x86_64-w64-mingw32-g++ ${=FLAGS} src/agent/core/$f.cpp
done
```
Baseline (before any change) is clean for all three files on both targets.

## Testing Strategy
- **build**: syntax-only cross compile above after every task, no new warnings; after Task 1 (which changes `nxagentd.h`) run it over all `src/agent/core/*.cpp`
- **unit tests**: not applicable (Win32 session/token APIs, no harness in `tests/suite` for agent core)
- **manual**: test matrix in Post-Completion; the XP `TermService`-stopped case is the acceptance test, Win10 multi-RDP is the regression test

## Progress Tracking
- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope

## Solution Overview
- new `UserSession` value type replaces `WTS_SESSION_INFO` at the three call sites, so callers no longer care whether the list came from WTS or from the fallback and never call `WTSFreeMemory`
- `EnumerateUserSessions()` owns the session-level fallback; `ExecuteInSession()` owns the token-level fallback; both are stateless and try WTS first
- the fallback token comes from any interactive-user process in the session, not specifically `explorer.exe`, because ATM builds often replace the shell. Among qualifying processes the earliest-created wins: it is the userinit/winlogon-spawned process of the console user, whereas `runas` and elevated processes start later and their logon SID is not on the `winsta0\default` DACL (GUI start would fail with 0xC0000142)
- `UserAgentWatchdog` switches to Toolhelp unconditionally (one code path, already used by `ReconcileExternalSubagentProcesses`)
- out of scope: winnt subagent session metrics and `GetProcessListForUserSession` (libnxagent) - GH #3730

## Technical Details
```cpp
// nxagentd.h, Windows declaration block (~1010)
struct UserSession
{
   DWORD id;
   WTS_CONNECTSTATE_CLASS state;
   TCHAR name[64];
   bool console;
};

StructArray<UserSession> EnumerateUserSessions();
HANDLE FindInteractiveUserToken(DWORD sessionId);
bool ExecuteInSession(const UserSession& session, TCHAR *command, bool allSessions, HANDLE *processHandle, DWORD *pid);

// watchdog.cpp, static, single caller
static bool GetTokenUserName(HANDLE token, TCHAR *user, size_t userSize, TCHAR *domain, size_t domainSize);
```
- `EnumerateUserSessions()`:
  - call `WTSGetActiveConsoleSessionId()` once per enumeration
  - WTS OK: copy `SessionId`, `State`, `pWinStationName` (`_tcslcpy`), `console = (SessionId == consoleId)`, then `WTSFreeMemory`; `consoleId == 0xFFFFFFFF` here only means no entry is marked console - the WTS list is never discarded
  - WTS fails: log level 6 `WTS_DEBUG_TAG` with `GetSystemErrorText`; then, only in this branch, `consoleId == 0xFFFFFFFF` → log level 6 and return empty array; otherwise return one entry `{consoleId, WTSActive, _T("Console"), true}`
- `FindInteractiveUserToken(sessionId)`:
  - `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS)`; per process: `ProcessIdToSessionId(pid) == sessionId`, `OpenProcess(PROCESS_QUERY_INFORMATION)` (`PROCESS_QUERY_LIMITED_INFORMATION` is Vista+), `OpenProcessToken(TOKEN_QUERY | TOKEN_DUPLICATE)`
  - qualifies: some `TokenGroups` entry passes `IsWellKnownSid(sid, WinInteractiveSid)`, and `TokenUser` fails `IsWellKnownSid` for `WinLocalSystemSid` / `WinLocalServiceSid` / `WinNetworkServiceSid`, and is not under `S-1-5-90` (DWM) or `S-1-5-96` (UMFD): compare `GetSidIdentifierAuthority(sid)` with a local `SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY` via `memcmp`, guard `*GetSidSubAuthorityCount(sid) > 0`, then check `*GetSidSubAuthority(sid, 0)` in {90, 96}
  - winner: earliest `GetProcessTimes()` creation time (`CompareFileTime`); a qualifier whose `GetProcessTimes` fails is skipped; losers' tokens closed
  - returns token handle (caller closes) or `nullptr`
- `GetTokenUserName()`: `GetTokenInformation(TokenUser)` into a buffer, `LookupAccountSid(nullptr, ...)` with `userSize`/`domainSize` as `DWORD` in/out
- `ExecuteInSession()`: `WTSQueryUserToken` failure is logged at level 6 with error text (demoted from the current level 4 - it is now a fallback trigger, not an error) and `FindInteractiveUserToken(session.id)` is called; if `nullptr`, log level 6 "no interactive user in session" and return false; otherwise continue into the existing `DuplicateTokenEx` → `CreateEnvironmentBlock` → `CreateProcessAsUser` body, whose level-4 logs stay unchanged
- `ExecuteInAllSessions()`: `if ((s.id == 0) && !s.console) continue;`; returns false when `EnumerateUserSessions()` returns an empty list (keeps today's `ERR_EXEC_FAILED` for `System.ExecuteInAllSessions` when nothing can be enumerated); per-session result aggregation stays as is

## What Goes Where
- **Implementation Steps**: code changes in `src/agent/core`, debug tag registration
- **Post-Completion**: Windows builds and manual runs on XP/Win10/Server

## Tandem

| Task | Writer | Reviewer |
|---|---|---|
| 1 | claude | codex |
| 2 | claude | codex |
| 3 | codex | claude |
| 4 | codex | claude |

## Implementation Steps

### Task 1: Introduce UserSession and EnumerateUserSessions, convert all call sites

**Files:**
- Modify: `src/agent/core/nxagentd.h`
- Modify: `src/agent/core/exec.cpp`
- Modify: `src/agent/core/watchdog.cpp`
- Modify: `doc/internal/debug_tags.txt`

- [x] `nxagentd.h`: add `#include <WtsApi32.h>` to the include block at 38-40 (next to `<aclapi.h>`); remove the now redundant includes at `exec.cpp:27` and `watchdog.cpp:25`
- [x] `nxagentd.h`: add `struct UserSession` and declare `EnumerateUserSessions()` and `ExecuteInSession(const UserSession&, ...)` in the declaration block at ~1010
- [x] `exec.cpp`: implement `EnumerateUserSessions()` with the WTS path, the synthetic console fallback and the `0xFFFFFFFF` case
- [x] `exec.cpp`: change `ExecuteInSession` to take `const UserSession&` (use `session.id`, `session.name` in logs); token logic unchanged in this task
- [x] `exec.cpp`: `ExecuteInAllSessions` iterates `EnumerateUserSessions()`, skips only `id == 0 && !console`, no `WTSFreeMemory`, returns false on an empty session list
- [x] `watchdog.cpp`: delete the forward declaration at 204
- [x] `watchdog.cpp`: `UserAgentWatchdog` uses `EnumerateUserSessions()` for sessions: `DWORD` loops become `int i < sessions.size()`, `pWinStationName` at 262 becomes `name`, drop `WTSFreeMemory(sessions)` at 282 (process check stays on `WTSEnumerateProcesses` until Task 4)
- [x] `watchdog.cpp`: `IsSessionSuitableForExternalSubagents` takes `const UserSession&`; `ExternalSubagentWatchdog` uses `EnumerateUserSessions()` (`SessionId` at 622/632 becomes `id`, drop `WTSFreeMemory` at 642) and drops the enumeration-failure log added in 0e2f99d263
- [x] `debug_tags.txt`: register `wts` (agent: WTS session enumeration and process start in user sessions), alphabetical position
- [x] grep `src/agent/core` for `WTS_SESSION_INFO` / `WTSEnumerateSessions` - only `EnumerateUserSessions` may remain
- [x] syntax check all `src/agent/core/*.cpp` on both targets (`nxagentd.h` changed) - clean
- ⚠️ the full `src/agent/core/*.cpp` run is not clean at baseline: `cng_engine.cpp`, `tunnel.cpp`, `modbus.cpp`, `service.cpp`, `nxsde.h` report pre-existing diagnostics (OpenSSL 4 const/deprecation, among others); gate is "diagnostics identical to HEAD", touched files have none

### Task 2: Token fallback in ExecuteInSession

**Files:**
- Modify: `src/agent/core/nxagentd.h`
- Modify: `src/agent/core/exec.cpp`

- [x] declare `FindInteractiveUserToken(DWORD sessionId)` in `nxagentd.h`; add `#include <tlhelp32.h>` to the `_WIN32` includes in `exec.cpp`
- [x] implement the Toolhelp scan with session filter, `OpenProcess` / `OpenProcessToken` access as in Technical Details
- [x] implement the qualification check with `IsWellKnownSid` and the guarded S-1-5-90 / S-1-5-96 prefix test
- [x] keep the earliest-created qualifier via `GetProcessTimes` + `CompareFileTime` (skip on `GetProcessTimes` failure), close all other token and process handles
- [x] in `ExecuteInSession`, demote the `WTSQueryUserToken` failure log to level 6 and fall back to `FindInteractiveUserToken`; no token → log level 6 and return false; logs after token acquisition unchanged
- [x] check every early-return path closes the snapshot, process and token handles
- [x] syntax check both targets - clean

### Task 3: Excluded-users check without WTS

**Files:**
- Modify: `src/agent/core/watchdog.cpp`

- [x] implement `static GetTokenUserName(HANDLE, TCHAR *user, size_t userSize, TCHAR *domain, size_t domainSize)` next to `IsSessionSuitableForExternalSubagents`
- [x] `IsSessionSuitableForExternalSubagents`: when `WTSQuerySessionInformation(WTSUserName)` fails, get user/domain via `FindInteractiveUserToken(session.id)` + `GetTokenUserName`, then `CloseHandle`
- [x] no token → skip session with the existing "no logged on user" log; name lookup failure → skip with the existing "cannot get logged on user" log
- [x] excluded-users matching (bare name or `DOMAIN\user`, case-insensitive) shared by both paths, not duplicated
- [x] syntax check both targets - clean

### Task 4: UserAgentWatchdog process check via Toolhelp

**Files:**
- Modify: `src/agent/core/watchdog.cpp`

- [x] replace `WTSEnumerateProcesses` with one `CreateToolhelp32Snapshot` per cycle, collecting session IDs (`ProcessIdToSessionId`) of processes whose `szExeFile` matches `executableName` with `_tcsicmp`
- [x] snapshot failure: log level 6 (new - today's `WTSEnumerateProcesses` failure is silent) and skip the cycle
- [x] per session: start the user agent only if its ID is not in the collected set; rest of the start logic unchanged
- [x] remove the remaining `WTSFreeMemory(processes)` path
- [x] grep `src/agent/core` for `WTSEnumerateProcesses` - none left
- [x] syntax check both targets - clean

## Post-Completion
*Manual verification - no checkboxes*

Real Windows builds (llvm-mingw x64 and the XP profile `mingw32-make -f Makefile.w32 CONFIG=config.mingw.xp`), then run with `UserAgentWatchdog`, external subagents and `AutoStartUserAgent` enabled, `DebugTags = wts:6,watchdog:6`:

| Case | Expected |
|---|---|
| XP/XPe, `TermService` stopped, user logged on | within 60 s `nxuseragent` and `nxagentd -H -G EXT:<name>` run in the console session (0); `[wts]` logs show WTS failure → console session → process token |
| same, user in `ExternalSubagentWatchdogExcludedUsers` | "logged on user ... is excluded", no external subagent; user agent still starts |
| same, custom ATM shell (no `explorer.exe`) | vendor shell token used, start succeeds |
| same, vendor service that starts an app via `LogonUser(LOGON32_LOGON_INTERACTIVE)` + `CreateProcessAsUser` in session 0 at boot | check which token wins: that process has INTERACTIVE and may predate the autologon user (XP fast logon); if picked, the GUI start fails with 0xC0000142 - the one realistic hole in the earliest-created rule |
| same, logon screen, nobody logged on | no token found, watchdogs retry quietly, nothing started as SYSTEM |
| XP, `TermService` running | WTS path, no fallback logs; `AutoStartUserAgent` and `System.ExecuteInAllSessions` now reach the console user in session 0; exactly one `nxuseragent` in session 0 (autostart and watchdog both reach it now) |
| Win10 / Server, 2 RDP sessions | WTS path, no session-level fallback logs; both RDP sessions served, session 0 skipped, no duplicate user agent. Token-level fallback logs are expected and benign for sessions without a logged-on user (e.g. an RDP connection at the login prompt - `WTSConnected`, `WTSQueryUserToken` already fails there today); the scan finds no qualifier and nothing starts |
| Win10, `TermService` stopped | observe: expected no fallback if LSM serves WTS on Vista+ (unverified claim from design review) |

Close GH #3729 after the XP case is confirmed on the customer's ATM build (agent 7.0-690 reported).
