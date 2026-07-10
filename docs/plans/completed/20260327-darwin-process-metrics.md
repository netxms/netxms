# Implement Full Process Metrics Suite for Darwin Subagent

## Overview
- Implement all process-related parameters for the Darwin (macOS) subagent, bringing it to parity with FreeBSD
- Resolves GitHub issue #2364 (NX-2156): "User filter for Process.Count parameter on darwin"
- The Darwin subagent currently has zero process metrics despite `system.h` declaring `H_ProcessCount`, `H_ProcessInfo`, `H_ProcessList`
- Uses `sysctl(KERN_PROC)` API for process enumeration (BSD-standard, no extra frameworks)

## Context (from discovery)
- Files/components involved:
  - `src/agent/subagents/darwin/proc.cpp` (new - main implementation)
  - `src/agent/subagents/darwin/system.h` (modify - add PROCINFO_* enums)
  - `src/agent/subagents/darwin/darwin.h` (modify - add proc.h include or declarations)
  - `src/agent/subagents/darwin/darwin.cpp` (modify - register new parameters, lists, tables)
  - `src/agent/subagents/darwin/Makefile.am` (modify - add proc.cpp to sources)
- Reference implementation: `src/agent/subagents/freebsd/proc.cpp` (closest platform match)
- Darwin `kinfo_proc` (from `<sys/sysctl.h>`) differs from FreeBSD:
  - Process name: `kp_proc.p_comm` (not `ki_comm`)
  - PID: `kp_proc.p_pid` (not `ki_pid`)
  - UID: `kp_eproc.e_ucred.cr_uid` (not `ki_uid`)
  - RSS: `kp_eproc.e_xrssize` or via `proc_pidinfo(PROC_PIDTASKINFO)` for accurate RSS
  - No `ki_login` field - must resolve username from UID via `getpwuid()`
  - No `ki_numthreads` - need `proc_pidinfo(PROC_PIDTASKINFO)` for thread count
  - No `ki_size` (vmsize) - need `proc_pidinfo(PROC_PIDTASKINFO)` for virtual size
  - No `ki_runtime` - CPU time from `kp_proc.p_rtime` or `proc_pidinfo()`
- Command line: use `sysctl(KERN_PROCARGS2)` per-process (no `/proc` filesystem on Darwin)

## Development Approach
- **testing approach**: Regular (code first, manual testing on macOS)
- Complete each task fully before moving to the next
- Make small, focused changes
- **Note**: This is a platform-specific subagent - automated unit tests are not practical (requires running on macOS with specific process states). Testing is manual via `nxagentd -D6` and querying parameters.
- Run build after each change to verify compilation
- Maintain backward compatibility

## Testing Strategy
- **Build verification**: `make` in the darwin subagent directory after each task
- **Manual testing**: Run agent with `-D6`, query each parameter via `nxget` or management console
- **Regression**: Ensure existing Darwin subagent parameters still work

## Progress Tracking
- Mark completed items with `[x]` immediately when done
- Add newly discovered tasks with ➕ prefix
- Document issues/blockers with ⚠️ prefix
- Update plan if implementation deviates from original scope

## Implementation Steps

### Task 1: Add PROCINFO enums and declarations to system.h

**Files:**
- Modify: `src/agent/subagents/darwin/system.h`

- [x] Add `PROCINFO_CPUTIME`, `PROCINFO_MEMPERC`, `PROCINFO_RSS`, `PROCINFO_THREADS`, `PROCINFO_VMSIZE` enum
- [x] Add `H_ProcessTable` declaration (for System.Processes table handler)
- [x] Verify existing declarations for `H_ProcessCount`, `H_ProcessInfo`, `H_ProcessList` are present (they are)
- [x] Build to verify header compiles cleanly

### Task 2: Implement process enumeration core in proc.cpp

**Files:**
- Create: `src/agent/subagents/darwin/proc.cpp`

- [x] Add includes: `<sys/sysctl.h>`, `<sys/proc.h>`, `<libproc.h>`, `<pwd.h>`
- [x] Implement `ReadProcCmdLine()` using `sysctl(KERN_PROCARGS2, pid)` to get command line for a given PID
- [x] Implement `GetProcessUserName()` helper to resolve UID to username via `getpwuid_r()`
- [x] Implement `MatchProcess()` that takes process info, extended-match flag, name/cmdline/user filters and returns bool (following FreeBSD pattern: exact match for basic, regex for extended)
- [x] Build to verify compilation

### Task 3: Implement H_ProcessCount handler

**Files:**
- Modify: `src/agent/subagents/darwin/proc.cpp`

- [x] Implement `H_ProcessCount()` handling four arg modes:
  - `'P'` - Process.Count(name): exact name match, count matching processes
  - `'E'` - Process.CountEx(name, cmdline, user): regex match with user filter
  - `'S'` - System.ProcessCount: return total process count
  - `'T'` - System.ThreadCount: sum thread counts across all processes
- [x] Use `sysctl(CTL_KERN, KERN_PROC, KERN_PROC_ALL)` to enumerate all processes
- [x] For thread count, use `proc_pidinfo(pid, PROC_PIDTASKINFO)` to get per-process thread count
- [x] Build to verify compilation

### Task 4: Implement H_ProcessInfo handler

**Files:**
- Modify: `src/agent/subagents/darwin/proc.cpp`

- [x] Implement `H_ProcessInfo()` for Process.CPUTime, Process.MemoryUsage, Process.RSS, Process.Threads, Process.VMSize, Process.WkSet
- [x] Support aggregation types (min, max, avg, sum) via second parameter
- [x] Support optional cmdline (3rd param) and user (4th param) filters
- [x] Use `proc_pidinfo(pid, PROC_PIDTASKINFO)` for accurate RSS, virtual size, thread count, CPU time
- [x] For PROCINFO_MEMPERC, get total physical memory via `sysctl(HW_MEMSIZE)` and compute percentage
- [x] Build to verify compilation

### Task 5: Implement H_ProcessList and H_ProcessTable handlers

**Files:**
- Modify: `src/agent/subagents/darwin/proc.cpp`

- [x] Implement `H_ProcessList()` for System.ProcessList (format: "pid name") and System.Processes (format: "pid cmdline")
- [x] Implement `H_ProcessTable()` for System.Processes table with columns: PID, NAME, USER, THREADS, KTIME, UTIME, VMSIZE, RSS, MEMORY_USAGE, PAGE_FAULTS, CMDLINE
- [x] Build to verify compilation

### Task 6: Register parameters and update build system

**Files:**
- Modify: `src/agent/subagents/darwin/darwin.cpp`
- Modify: `src/agent/subagents/darwin/Makefile.am`

- [x] Add process parameter registrations to `m_parameters[]` array:
  - `Process.Count(*)` → `H_ProcessCount`, arg `"P"`
  - `Process.CountEx(*)` → `H_ProcessCount`, arg `"E"`
  - `Process.CPUTime(*)` → `H_ProcessInfo`, arg `PROCINFO_CPUTIME`
  - `Process.MemoryUsage(*)` → `H_ProcessInfo`, arg `PROCINFO_MEMPERC`
  - `Process.RSS(*)` → `H_ProcessInfo`, arg `PROCINFO_RSS`
  - `Process.Threads(*)` → `H_ProcessInfo`, arg `PROCINFO_THREADS`
  - `Process.VMSize(*)` → `H_ProcessInfo`, arg `PROCINFO_VMSIZE`
  - `Process.WkSet(*)` → `H_ProcessInfo`, arg `PROCINFO_RSS`
  - `System.ProcessCount` → `H_ProcessCount`, arg `"S"`
  - `System.ThreadCount` → `H_ProcessCount`, arg `"T"`
- [x] Add list registrations to `m_enums[]`:
  - `System.ProcessList` → `H_ProcessList`, arg `"1"`
  - `System.Processes` → `H_ProcessList`, arg `"2"`
- [x] Add table registration to `m_tables[]`:
  - `System.Processes` → `H_ProcessTable`
- [x] Add `proc.cpp` to `darwin_la_SOURCES` in Makefile.am (already present)
- [x] Full build to verify everything links

### Task 7: Verify and finalize

- [x] Verify all process parameters are registered and match FreeBSD parity
- [x] Run full build of darwin subagent
- [x] Move this plan to `docs/plans/completed/`

## Technical Details

### Darwin kinfo_proc structure mapping

| Data needed | Darwin field | Notes |
|-------------|-------------|-------|
| Process name | `kp_proc.p_comm` | Max 16 chars (MAXCOMLEN+1) |
| PID | `kp_proc.p_pid` | |
| UID | `kp_eproc.e_ucred.cr_uid` | Resolve to username via `getpwuid_r()` |
| Status | `kp_proc.p_stat` | SIDL, SRUN, SSLEEP, SSTOP, SZOMB |

### proc_pidinfo() for extended data

| Data needed | Call | Struct field |
|-------------|------|-------------|
| Virtual size | `PROC_PIDTASKINFO` | `pti_virtual_size` |
| RSS | `PROC_PIDTASKINFO` | `pti_resident_size` |
| Thread count | `PROC_PIDTASKINFO` | `pti_threadnum` |
| CPU time (user) | `PROC_PIDTASKINFO` | `pti_total_user` (nanoseconds) |
| CPU time (system) | `PROC_PIDTASKINFO` | `pti_total_system` (nanoseconds) |
| Page faults | `PROC_PIDTASKINFO` | `pti_faults` |

### Command line retrieval

Use `sysctl(CTL_KERN, KERN_PROCARGS2, pid)` which returns:
1. `int argc` (4 bytes)
2. Executable path (null-terminated)
3. Padding nulls
4. argv strings (null-separated)

### Parameter syntax (matches all other platforms)

| Parameter | Args | Description |
|-----------|------|-------------|
| `Process.Count(name)` | 1: exact name | Count processes by exact name |
| `Process.CountEx(name,cmdline,user)` | 3: regex patterns | Count with regex name, cmdline, user filters |
| `Process.CPUTime(name,type,cmdline,user)` | 1-4 | CPU time (ms), type=min/max/avg/sum |
| `Process.RSS(name,type,cmdline,user)` | 1-4 | Resident set size |
| `Process.VMSize(name,type,cmdline,user)` | 1-4 | Virtual memory size |
| `Process.Threads(name,type,cmdline,user)` | 1-4 | Thread count |
| `Process.MemoryUsage(name,type,cmdline,user)` | 1-4 | Memory usage percentage |

## Post-Completion

**Manual verification:**
- Run `nxagentd -D6` on macOS and query each parameter
- Test `Process.Count(*)` with known process names (e.g., "launchd", "Finder")
- Test `Process.CountEx(*,,username)` with user filter
- Verify `System.ProcessCount` returns reasonable number
- Verify `System.Processes` table returns complete process list
