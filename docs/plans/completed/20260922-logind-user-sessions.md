# User Sessions via systemd-logind on Hosts Without utmp (#3674)

## Overview

On Debian 13 (systemd 257 built without utmp support) the `/var/run/utmp` file still exists but is never written. The Linux agent reads that file directly for `System.ConnectedUsers`, `System.ActiveUserSessions` (list) and `System.ActiveUserSessions` (table), so all three silently report zero sessions. The session agent `nxsagent` also scans utmp to find its own session ID, so on such hosts it reports session ID 0 and the core agent can no longer match it to a table row for screen info and user-agent routing.

This change makes systemd-logind the primary source of user session information on Linux, using the `sd-login` API of libsystemd that is already linked into both the linux subagent and nxsagent. utmp remains the fallback for hosts without logind (non-systemd distributions, containers without logind, builds with `--disable-systemd`).

Benefits:
- User session metrics work again on Debian 13 and any other distribution that has dropped utmp.
- No subprocess spawn per poll (unlike a `loginctl` based approach), no dependence on `loginctl --json` availability.
- Session IDs stay consistent between the subagent table and nxsagent because both take the same value (logind session leader PID) from the same source.
- Three duplicated utmp parsing loops in `system.cpp` collapse into one enumeration function.

## Context (from discovery)

Files / components involved:
- **configure.ac** (systemd block, ~lines 2551-2565): probes `systemd/sd-bus.h` and `sd_bus_open_system`, defines `HAVE_SDBUS`, sets `SYSTEMD_LIBS="-lsystemd"`. `AM_CONDITIONAL([HAVE_SDBUS], ...)` at ~line 4420.
- **src/agent/subagents/linux/system.cpp**: `H_ConnectedUsers` (line 32), `H_UserSessionList` (line 53), `H_UserSessionTable` (line 77). Each opens `UTMP_FILE` and iterates `struct utmp` records with `ut_type == USER_PROCESS`. Table columns: ID (= `ut_pid`), USER_NAME, TERMINAL, STATE (always "Active"), CLIENT_NAME (`ut_host`), CLIENT_ADDRESS (parsed from `ut_host`), CLIENT_DISPLAY (empty), CONNECT_TIMESTAMP (unset), LOGON_TIMESTAMP (`ut_tv.tv_sec`), IDLE_TIME (`stat()` atime of `/dev/<ut_line>`).
- **src/agent/subagents/linux/linux.cpp**: registration of the three metrics (lines 269, 453, 478). Unchanged.
- **src/agent/subagents/linux/Makefile.am**: already links `@SYSTEMD_LIBS@` unconditionally. Unchanged.
- **src/agent/subagents/linux/services.cpp**: reference pattern for `#ifdef HAVE_SDBUS` guarded libsystemd code with a stub `#else` branch.
- **src/agent/nxsagent/main.cpp** (~lines 221-254): non-Windows login message builds `VID_SESSION_ID` by scanning utmp for a `USER_PROCESS` record whose `ut_host` equals `DISPLAY` / `WAYLAND_DISPLAY`, and sends `ut_pid`. Falls back to 0.
- **src/agent/nxsagent/Makefile.am**: already links `@SYSTEMD_LIBS@` under `if HAVE_SDBUS` (Wayland screenshot support). Unchanged.
- **src/agent/core/sa.cpp**: two lookups of session agent connectors: by numeric session ID (`sa.cpp:584`, used for screen info) and by session name (`sa.cpp:570`, used for screenshots from `session.cpp:756`). The name comes from nxsagent's `VID_NAME` (DISPLAY / WAYLAND_DISPLAY) and is not touched by this change. Unchanged.
- **src/server/libnxsrv/agent.cpp** `AgentConnection::getUserSessions()` (line 3793): consumes the table by column name; `connected = (STATE == "Active")`. Unchanged.
- **src/agent/subagents/winnt/system.cpp**: STATE values are `Active` / `Disconnected`. Linux mapping follows the same two values.

Related patterns found:
- libsystemd string outputs are `malloc`-allocated and must be released with `free()`, not `MemFree` (matches how `services.cpp` treats sd-bus data).
- sd-login error conventions (verified against upstream `sd-login.c`): a field absent from the session file returns `-ENXIO`, a present but empty field `-ENODATA`, an empty state `-EIO`. `sd_get_sessions()` maps a missing `/run/systemd/sessions` directory to a return of 0 with no list, so its return value cannot distinguish "logind not running" from "no sessions".
- `sd_pid_get_session()` succeeds only for processes inside a `session-<id>.scope` cgroup. On Debian 13 / GNOME, autostarted desktop applications run under the per-user service manager (`app-*.scope`), so nxsagent started from the desktop is outside the session scope and that call fails with `-ENXIO`. `sd_uid_get_display()` returns the user's primary graphical session regardless of the caller's cgroup.
- utmp on X11 desktops carried the display in both `ut_line` and `ut_host` (`:0`), so TERMINAL and CLIENT_NAME showed the display. logind provides `TTY` (for example `tty2`) and `REMOTE_HOST` only for remote sessions; the display is available separately through `sd_session_get_display()`.
- `StructArray<T>` (`include/nms_util.h:2415`) is the project container for arrays of POD structs.
- Debug tag for the linux subagent is `DEBUG_TAG` = `linux` (`linux_subagent.h:29`).

Dependencies identified:
- `sd_session_get_leader()`, `sd_session_get_start_time()` and `sd_session_get_username()` were all added in systemd 254. A configure probe for `sd_session_get_leader` gates the whole logind path so builds against older libsystemd still compile.
- No NXCP, NXSL, WebAPI, Java client or database changes: metric names, list format and table columns are unchanged.

## Development Approach

- **Testing approach**: Regular (code first, then verification). Subagent metric handlers have no unit test harness in `tests/suite`; verification is by building the agent and comparing metric output against `loginctl list-sessions` and `who` on real hosts (see Testing Strategy). Each task ends with a clean build of the affected component.
- Complete each task fully before moving to the next; keep changes small and focused.
- C++11 only, 3-space indentation, opening brace on its own line, `_T()` / `TCHAR` in agent code, `nullptr`.
- Comments only where behaviour is non-obvious (why logind is authoritative even when empty; why `free()` is used for sd-login strings).
- Do not change metric names, list line format, table column names or their order.
- **CRITICAL: update this plan file when scope changes during implementation.**

## Testing Strategy

- **Build verification** (every task): `make -C src/agent/subagents/linux` and `make -C src/agent/nxsagent` after a configure run with systemd detected; final task also builds with `--disable-systemd` to prove the utmp-only path compiles.
- **Metric verification** on a Debian 13 host (logind, no utmp) and on an older systemd host (logind and utmp both present):
  - `nxagentd -D6` and query `System.ConnectedUsers`, list and table `System.ActiveUserSessions` via `nxget`; compare rows with `loginctl list-sessions` (session count, user, leader PID as ID, tty) and, where utmp exists, with `who`.
  - SSH session → STATE `Active`; foreground graphical session → `Active`; graphical session switched away with another session in the foreground → `Disconnected`; `manager` / `manager-early` / `background` logind entries must not appear.
- **Session agent verification**: start `nxsagent` inside a graphical session on Debian 13, confirm its debug log shows the logind session ID and leader PID, and that the core agent's `Agent.SessionAgents` table row ID equals the matching `System.ActiveUserSessions` row ID.
- **Fallback verification**: on a host without logind (or with `/run/systemd/sessions` absent), confirm the utmp path is taken and output is identical to the pre-change agent.
- No project e2e UI harness applies.

## Progress Tracking

- Mark completed items with `[x]` immediately when done.
- Add newly discovered tasks with ➕ prefix; document blockers with ⚠️ prefix.
- Update the plan if implementation deviates from the original scope.

## Solution Overview

One file-static function in `system.cpp` produces the list of user sessions as a `StructArray` of a small POD struct. It tries logind first and falls back to utmp; the three metric handlers become simple loops over that array.

Key design decisions:
- **logind first, utmp second, decided by logind presence.** Neither source's content can be trusted for detection: on Debian 13 the utmp file may be empty or absent, and on a host without logind `sd_get_sessions()` returns 0 rather than an error. logind is considered present when `/run/systemd/sessions` exists as a directory (logind creates it at startup). When present, its enumeration is authoritative even when it yields zero sessions, so an idle Debian 13 host correctly reports no sessions and never errors because utmp is missing. Only `ENOENT` from the presence check selects utmp (non-systemd distribution, container, `--disable-systemd` build); any other `stat()` failure or a non-directory is reported as a metric error. This is a runtime-state heuristic, not a liveness check: the unit sets `RuntimeDirectoryPreserve=yes`, so after logind has run and is then stopped or masked the directory persists and the agent keeps reporting logind's stale or empty state rather than falling back to utmp. That is accepted and documented; a D-Bus name-owner query to establish liveness is not worth the added surface for this fix. utmp is never consulted on a logind host: a content-based rule ("use utmp when logind is empty") would resurrect closing sessions still recorded in utmp and could not merge the two sources consistently anyway. Known limitation, recorded here only (metric descriptions do not name a data source and stay unchanged): logins that bypass `pam_systemd` (for example sshd with `UsePAM no`) are not reported on logind hosts, which matches what `loginctl` and Debian 13's `who` show.
- **Session ID = logind leader PID.** IDs stay PID-based, with logind's session leader used on the logind path. No equality with the old `ut_pid` is promised: for OpenSSH, `pam_systemd` registers the privileged parent while the utmp record carries the user child PID. Correctness only requires that the subagent and nxsagent pick from the same source, which the shared presence rule guarantees.
- **Filter to utmp-equivalent sessions.** Sessions whose class begins with `user` are kept (`user`, `user-early`, `user-light`, `user-early-light`), except `user-incomplete`, which is a session still authenticating and would not have appeared in utmp. `manager`, `manager-early`, `background`, `background-light`, `greeter`, `lock-screen` and `none` are skipped. Sessions in state `closing` are skipped as the equivalent of utmp `DEAD_PROCESS`.
- **State mapping** follows the Windows subagent: logind `active` → `Active`, `online` → `Disconnected`. `opening` (session created, scope not yet started) and `closing` are skipped. Seatless sessions (SSH) are always `active` in logind.
- **Display representation.** To keep the table content equivalent to the utmp version for desktop sessions, CLIENT_NAME is the remote host when present, otherwise the display from `sd_session_get_display()`; TERMINAL is the tty when present, otherwise the display. The display never goes into CLIENT_DISPLAY, which the server parses as `WxHxBPP`. `sd_session_get_display()` is X11-only, so Wayland sessions keep their tty in TERMINAL and an empty CLIENT_NAME; that is accepted. Observed limitation for SSH: sshd registers the logind session through `pam_systemd` before it allocates a pty, so logind has no `TTY` for SSH sessions and TERMINAL stays empty; IDLE_TIME is then also unset, since it is derived from the terminal device's atime. Recovering the pty would need a second attribution mechanism (process tree or device scan) and is not worth it for this fix; session count and ID matching are the acceptance criteria. Note that on Debian 13 `who` (coreutils 9.7, linked against libsystemd) also reads logind and prints the session leader's process name in the LINE column, so its output is not evidence of utmp content.
- **Optional fields.** Any negative return from `sd_session_get_tty`, `_get_remote_host`, `_get_display` or `_get_start_time` means "not available" (empty string or 0). A session is skipped when `sd_session_get_class`, `sd_session_get_state`, `sd_session_get_leader` or `sd_session_get_username` fails.
- **nxsagent session lookup order.** When logind is present by the same directory rule as the subagent, candidates are tried in order and the first eligible one wins: `XDG_SESSION_ID` from the environment; then `sd_pid_get_session(getpid())`; then `sd_uid_get_display(getuid())`, which is the user's primary graphical session and therefore best effort only. Every candidate passes the same eligibility check the table applies (owner UID equals `getuid()`, class `user*` except `user-incomplete`, state neither `opening` nor `closing`) and must resolve a leader; any failure moves to the next candidate. When logind is not present, the existing utmp DISPLAY scan runs instead; it is never used as a fourth candidate on a logind host, because an ID from utmp could not match a table that logind produced. The presence rule is the same as the subagent's: only `ENOENT` from `stat()` selects utmp; any other failure or a non-directory leaves the session ID at 0 with a debug message, because the subagent table returns an error in that state and no ID could match a row anyway. Known limitation: with two graphical sessions of one UID and no `XDG_SESSION_ID` in the environment, both nxsagent instances can report the primary session's leader.
- **Non-goal.** nxsagent keeps reporting `USER_SESSION_ACTIVE` as its own state even when the table shows `Disconnected` for a switched-away graphical session. That inconsistency predates this change.
- **Build gating** is `#if defined(HAVE_SDBUS) && defined(HAVE_SD_SESSION_GET_LEADER)`. On build hosts with libsystemd older than 254 the logind path compiles out entirely; those hosts still maintain utmp so nothing is lost.

## Technical Details

### Data structure (file-static in `system.cpp`)

```cpp
struct UserSessionInfo
{
   uint32_t id;          // logind session leader PID or utmp ut_pid
   char user[64];
   char terminal[64];
   char remoteHost[256];
   time_t loginTime;
   bool active;
};
```

### Enumeration flow

```
static bool ReadUserSessions(StructArray<UserSessionInfo> *sessions)
   #if logind supported
      rc = stat("/run/systemd/sessions", &st)
      if rc == 0 and !S_ISDIR(st.st_mode): debug(6), return false
      if rc != 0 and errno != ENOENT:      debug(6), return false
      if rc == 0:
         n = sd_get_sessions(&ids)         // 0 is a valid "no sessions" answer here
         if n < 0: debug(6), return false
         for each id:
            class   = sd_session_get_class(id)      -> keep only "user*" except "user-incomplete"
            state   = sd_session_get_state(id)      -> skip "opening" and "closing"; active = (state == "active")
            leader  = sd_session_get_leader(id)     -> skip session on any error
            user    = sd_session_get_username(id)   -> skip session on any error
            tty     = sd_session_get_tty(id)        -> "" on any error
            host    = sd_session_get_remote_host(id)-> "" on any error
            display = sd_session_get_display(id)    -> "" on any error (X11 only)
            start   = sd_session_get_start_time(id) -> usec / 1000000, 0 on any error
            terminal   = tty  non-empty ? tty  : display
            remoteHost = host non-empty ? host : display
            free() every returned string; free() ids array and each id
         debug(6): "logind: N user sessions"
         return true                        // authoritative even when N == 0
      debug(6): "logind not present, reading utmp"
   #endif
   utmp loop (existing logic): id = ut_pid, user = ut_user, terminal = ut_line,
      remoteHost = ut_host, loginTime = ut_tv.tv_sec, active = true
   debug(6): "utmp: N user sessions"
   return fopen succeeded
```

`InetAddress::parse()` of a display string such as `:0` is invalid and leaves CLIENT_ADDRESS empty, same as before.

### Handlers

- `H_ConnectedUsers`: `ret_uint(value, sessions.size())`.
- `H_UserSessionList`: per session `"%hs" "%hs" "%hs"` with user, terminal, remoteHost (unchanged format).
- `H_UserSessionTable`: same columns as today; ID = `id`, STATE = `active ? "Active" : "Disconnected"`, CLIENT_NAME = `remoteHost`, CLIENT_ADDRESS from `InetAddress::parse(remoteHost)`, LOGON_TIMESTAMP = `loginTime`, IDLE_TIME from `stat("/dev/<terminal>")` only when the terminal is non-empty and does not start with `:`.
- All three return `SYSINFO_RC_ERROR` when `ReadUserSessions()` returns false (neither logind nor utmp readable).

### nxsagent

Before the utmp DISPLAY scan, under the same guard:

```
static uint32_t EligibleSessionLeader(const char *sessionId)   // 0 when not eligible or not resolvable
   uid  = sd_session_get_uid(sessionId)     -> must equal getuid()
   cls  = sd_session_get_class(sessionId)   -> "user*" except "user-incomplete"
   st   = sd_session_get_state(sessionId)   -> not "opening", not "closing"
   pid  = sd_session_get_leader(sessionId)  -> returned; 0 on any failure above

sid = 0
rc = stat("/run/systemd/sessions", &st)                   // same presence rule as the subagent
if rc == 0 and S_ISDIR(st.st_mode):
   env = getenv("XDG_SESSION_ID")
   if env non-empty:
      sid = EligibleSessionLeader(env);  debug(3): "Login: XDG_SESSION_ID %hs, leader %u", env, sid
   if sid == 0 and sd_pid_get_session(getpid(), &id) >= 0:
      sid = EligibleSessionLeader(id);   debug(3): "Login: process session %hs, leader %u", id, sid;  free(id)
   if sid == 0 and sd_uid_get_display(getuid(), &id) >= 0:
      sid = EligibleSessionLeader(id);   debug(3): "Login: primary display session %hs, leader %u (best effort)", id, sid;  free(id)
   if sid == 0: debug(3): "Login: no eligible logind session found"
else if rc != 0 and errno == ENOENT
   existing utmp DISPLAY matching
else
   debug(3): "Login: cannot use logind (<errno text> or not a directory)", sid stays 0
```

### configure.ac

Inside the existing `if test "x$HAVE_SDBUS" = "xyes"` branch that defines `HAVE_SDBUS`, mirror the sd-bus header-then-library pattern:

```
AC_CHECK_HEADER(systemd/sd-login.h,
   [AC_CHECK_LIB(systemd, sd_session_get_leader,
      [AC_DEFINE(HAVE_SD_SESSION_GET_LEADER, 1, Define to 1 if libsystemd provides sd_session_get_leader)])])
```

The action-if-found argument keeps autoconf from appending `-lsystemd` to `LIBS`; `SYSTEMD_LIBS` already carries it for both binaries.

## What Goes Where

- **Implementation Steps**: configure probe, subagent refactor, nxsagent change, build verification.
- **Post-Completion**: manual metric comparison on real hosts, packaging notes.

## Implementation Steps

### Task 1: Configure probe for systemd 254 sd-login functions

**Files:**
- Modify: `configure.ac`

- [x] in the systemd block, after `AC_DEFINE(HAVE_SDBUS, ...)`, add `AC_CHECK_HEADER(systemd/sd-login.h, ...)` wrapping `AC_CHECK_LIB(systemd, sd_session_get_leader, ...)` that defines `HAVE_SD_SESSION_GET_LEADER`
- [x] run `autoreconf` (via `./init-source-tree` or `autoreconf -fi`) and `./configure --with-agent` on a host with libsystemd >= 254; confirm `build/config.h` contains `#define HAVE_SD_SESSION_GET_LEADER 1`
- [x] confirm `./configure --with-agent --disable-systemd` leaves both `HAVE_SDBUS` and `HAVE_SD_SESSION_GET_LEADER` undefined
- [x] build passes before task 2

### Task 2: Shared user session enumeration in the linux subagent

**Files:**
- Modify: `src/agent/subagents/linux/system.cpp`

- [x] add `struct UserSessionInfo` and file-static `ReadUserSessions(StructArray<UserSessionInfo>*)` above `H_ConnectedUsers`
- [x] implement the logind branch under `#if defined(HAVE_SDBUS) && defined(HAVE_SD_SESSION_GET_LEADER)` with `#include <systemd/sd-login.h>`: presence check via `stat()` on `/run/systemd/sessions` (directory → logind, `ENOENT` → utmp, anything else → error), `sd_get_sessions`, class / state filtering (`user*` except `user-incomplete`, skip `opening` and `closing`), leader, username, tty, remote host, display, start time; display substitutes for empty tty and empty remote host; any negative return on an optional field means empty; `free()` all sd-login allocations; debug level 6 messages on the `linux` tag for source used and session count
- [x] return success from the logind branch even with zero accepted sessions; the utmp loop runs only when logind is not present, filling the same struct with `active = true`
- [x] note the `pam_systemd` bypass limitation in the description string of the three metrics in `linux.cpp` only if the existing description text mentions the data source; otherwise leave descriptions unchanged
- [x] rewrite `H_ConnectedUsers`, `H_UserSessionList` and `H_UserSessionTable` as loops over the array; table STATE becomes `Active` / `Disconnected` from the `active` flag; idle time `stat()` only for real tty names; column set and order unchanged
- [x] remove the now-unused per-handler `fopen(UTMP_FILE)` code; keep `#include <utmp.h>` for the fallback
- [x] build `src/agent/subagents/linux` with systemd detected and with `--disable-systemd`; both must compile without warnings in the changed file
- [x] fallback check: run the built agent in a plain container without logind (for example `docker run debian`) or with `systemd-logind` masked, log in via a utmp-writing path, and confirm the debug log shows the utmp source and the table has rows (done on hyperion in a user mount namespace: tmpfs over `/run` hides `/run/systemd/sessions` and holds a seeded `utmp`; `DEAD_PROCESS` record dropped, unterminated 32-byte user and 256-byte host fields copied within bounds)
- [x] build passes before task 3

### Task 3: nxsagent session ID from logind

**Files:**
- Modify: `src/agent/nxsagent/main.cpp`

- [x] add `#include <systemd/sd-login.h>` under `#if defined(HAVE_SDBUS) && defined(HAVE_SD_SESSION_GET_LEADER)`
- [x] add a file-static `EligibleSessionLeader()` applying the same owner / class / state checks as the subagent table and returning the leader PID or 0
- [x] in the non-Windows login message code, when `/run/systemd/sessions` is a directory, resolve `sid` from candidates in order: `XDG_SESSION_ID`, `sd_pid_get_session(getpid())`, `sd_uid_get_display(getuid())`; a failed candidate moves to the next; log at debug level 3 for each candidate tried, before the `free()` of its string
- [x] keep the utmp DISPLAY scan only for the case where logind is not present (`stat()` fails with `ENOENT`); do not run it after a failed logind lookup on a logind host, nor on any other `stat()` failure or non-directory, where `sid` stays 0 with a debug message
- [ ] ⚠️ (needs a graphical host) verify on Debian 13 / GNOME that a desktop-autostarted nxsagent (outside `session-<id>.scope`) resolves its session through `sd_uid_get_display` and logs the leader PID
- [x] update the comment above the utmp scan so it describes both sources
- [x] build `src/agent/nxsagent` with systemd detected and with `--disable-systemd`
- [x] build passes before task 4

### Task 4: Verify acceptance criteria

- [x] on Debian 13: `System.ConnectedUsers` and both `System.ActiveUserSessions` forms match `loginctl list-sessions` (class `user` rows only, ID = leader PID, user, tty) — verified on hyperion (Debian 13, systemd 257, no `/var/run/utmp`): SSH sessions listed with leader PID, remote IP and logon time; `manager` / `manager-early` rows absent; the packaged pre-change agent on the same host returns `500: Internal error` for all three metrics
- [x] on Debian 13: SSH session shows `Active`; foreground graphical session `Active`; switched-away graphical session `Disconnected`; no `manager`/`manager-early`/`background` rows; graphical rows carry the display in TERMINAL and CLIENT_NAME when no tty or remote host applies — ⚠️ SSH `Active` verified on hyperion; graphical cases need a Debian 13 desktop host
- [ ] on an older systemd host with utmp: output matches `who` and `loginctl` (logind path is taken, not utmp) — ⚠️ no such host available in this session
- [x] on a host without logind: utmp fallback is taken and output is identical to the previous agent version
- [ ] on Debian 13 with nobody logged in and `/var/run/utmp` absent: all three metrics succeed with zero sessions rather than returning an error — ⚠️ hyperion always has the SSH session doing the query; the zero-session return path is the same `ReadLogindSessions()` success path, verified only by code review
- [ ] on a logind host, log out of a session and query while it is `closing`: the row disappears and is not resurrected from a stale utmp record — ⚠️ not observed: closing SSH sessions on hyperion disappear too fast to catch; the `closing` filter is covered by Codex's stub harness for nxsagent and by the same string check in the subagent
- [ ] on a logind host, stop or mask `systemd-logind` after it has run: confirm the agent keeps using the preserved directory (stale or empty result) and does not fall back to utmp, as documented — ⚠️ needs root on the test host
- [ ] two graphical sessions for one UID on Debian 13: each nxsagent reports its own session leader when `XDG_SESSION_ID` is present in its environment; record the observed behaviour when it is not — ⚠️ needs a Debian 13 desktop host
- [ ] nxsagent on Debian 13, both started from a terminal inside the session scope and autostarted by the desktop, logs the leader PID and `Agent.SessionAgents` row ID equals the `System.ActiveUserSessions` row ID for that session — ➕ verified for an SSH session on hyperion via `XDG_SESSION_ID` (row IDs equal); desktop autostart / `sd_uid_get_display` path still needs a graphical Debian 13 host
- [x] full build with `./configure --with-agent --disable-systemd` succeeds
- [x] run `./tests/suite/netxms-test-suite` to confirm nothing else regressed (hyperion, all suites OK)

### Task 5: [Final] Update documentation

- [x] check `doc/` and `contrib/` for any text stating Linux user sessions come from utmp; update if found
- [x] no new debug tag was added, so `doc/internal/debug_tags.txt` needs no change; confirm
- [ ] changelog entry in the external changelog repository (`https://github.com/netxms/changelog`) if the maintainer wants one for this user-visible fix
- [x] commit referencing issue #3674
- [x] move this plan to `docs/plans/completed/`

## Post-Completion

**Manual verification:**
- Screen info and user-agent routing from the management console to a Debian 13 desktop session (relies on the session ID match verified in Task 4).
- Idle time for tty sessions still reflects `/dev/<tty>` atime on both code paths.

**External system updates:**
- Debian 13 packages must be built on a host with libsystemd >= 254 so `HAVE_SD_SESSION_GET_LEADER` is defined; packages built on Debian 12 (systemd 252) will silently keep the utmp-only path and remain broken on Debian 13.
