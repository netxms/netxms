# NetXMS — review calibration

## What this software is

NetXMS is an enterprise network and infrastructure monitoring system. A distributed C++ server
(`netxmsd`) talks to C++ agents (`nxagentd`, 47+ subagents) and to network devices over SNMP/SSH,
stores everything in a SQL database, and serves Java clients (SWT desktop, RWT web) over NXCP plus
REST clients over a WebAPI module. NXSL is an in-tree scripting language users write automation in.

It runs unattended for months as a privileged daemon, in installations where it is the thing that
notices an outage. That shapes every judgement below.

## Blast radius — what a defect actually costs here

Rank findings by this, not by how ugly the code looks.

- **Silent monitoring loss is the worst outcome.** A poller thread that dies, a queue that stalls, a
  DCI that stops collecting, an event that never fires — the operator sees a green screen and learns
  nothing is being watched only after the outage they were monitoring for. Worse than a crash, which
  is at least visible.
- **A crash takes the whole server down.** The server is one process; a null deref, unaligned access
  or unbounded stack allocation in one code path stops monitoring for every node. There is no
  per-subsystem sandbox.
- **Deadlock and lock inversion are production killers.** Object locks, `RWLock`, session locks and
  DB handles interleave heavily. A held lock across a blocking call (DB, network, agent RPC) wedges
  the server just as effectively as a crash and is far harder to diagnose.
- **Persistent state corruption cannot be rolled back.** Anything writing the DB, a schema upgrade
  procedure, or a config value with `need_server_restart` is one-way for the customer.
- **Access-control mistakes are security bugs.** Object ACLs (`OBJECT_ACCESS_*`) and system rights
  (`SYSTEM_ACCESS_*`) are the only thing between a low-privileged user and every device credential,
  script and alarm in the installation. A missing or wrong-constant check is major or critical, never
  minor — this repo has shipped fixes for exactly that class.
- **Long-running leaks matter more than in short-lived software.** A leak per poll cycle is a leak
  per 60 seconds, forever. So is an unclosed DB handle or a thread pool task that never completes.
- **Cross-surface drift is a real defect, not a nit.** A feature usually spans server C++, the Java
  client library, both UIs, NXSL bindings, WebAPI and the DB schema. A server-side change whose Java
  or `openapi.yaml` counterpart is missing ships a half-working feature.

## What a real failure looks like

Concretely, in this codebase:

- unchecked return from `DBSelect`/`DBGetField*`/`FindObjectById` used as if it succeeded
- an object pointer used without a `shared_ptr` hold, or a `NetObj` accessed after unlock
- a lock acquired and not released on every path (prefer `LockGuard`; flag hand-rolled lock/unlock
  with early returns between them)
- `alloca` or a fixed stack buffer sized from network/agent/user input
- unbounded growth: a queue, cache or map with no eviction, on a per-poll or per-event path
- an NXCP field ID reused for two meanings, or a `VID_*` added where an existing one fits
- integer/format mismatches in `nx_swprintf` — `%s` is `wchar_t*` on Windows and `char*` on glibc,
  which is why `nx_swprintf` exists; raw `swprintf` with `%s` is a portability defect
- a DB upgrade procedure that does not set the schema level, is not idempotent, or is missing from a
  branch it must be backported to
- an exported-symbol mistake: a core function called from a module without `NXCORE_EXPORTABLE`
  (runtime link failure), or `NXCORE_EXPORTABLE` added to a declaration that was deliberately
  undecorated to keep a security primitive core-only (`ConsumeAuthenticationToken` is the reference
  case — check for a comment before assuming an omission)
- a WebAPI handler returning a status code outside the documented convention, or missing the
  `openapi.yaml` update that makes the endpoint real to external consumers
- a blocking or slow operation added to a poller or the main loop rather than a thread pool

## Deliberate conventions — do not report these as findings

These are house style, argued and settled. Flag a **violation** of them; never propose replacing them.

- **C++11 is a hard ceiling.** Must build with GCC 4.8+, Clang 3.3+, VS2015+. Proposing C++14/17/20
  features — structured bindings, `std::optional`, `if constexpr`, `std::string_view`, designated
  initializers — is wrong, not modern. Guarded C++17 in libnetxms is an existing exception, not a
  precedent to extend.
- **In-house primitives are the standard library here.** `MemAlloc`/`MemFree`/`MemCopyString`,
  `String`/`StringBuffer`/`StringList`/`StringMap`, `ObjectArray`/`IntegerArray`/`HashMap`/
  `ObjectQueue`, `Mutex`/`RWLock`/`Condition`, `ThreadPoolExecute`. Do not suggest `std::vector`,
  `std::string`, `std::mutex` or `new`/`delete` in their place. `std::map` **is** preferred for maps
  of plain scalars, where `HashMap` would force a pointless per-entry allocation.
- **Server code is Unicode-only**: `L"..."`, `wcslen`, `wchar_t*`. `_T()`/`TCHAR`/`_tcs*` in new
  *server* code is the defect; in agent code and shared libraries it is correct.
- **3-space indent, brace on its own line, `m_`/`g_`/`s_` prefixes, PascalCase functions,
  camelCase methods.** Old code carries Hungarian notation (`dwResult`, `szName`); leave it alone.
- **No SQL NULL as a marker.** Empty integer values are `0`, not NULL. Do not propose nullable
  columns to express "unset".
- **No speculative abstraction.** Wrapper functions, indirection layers, and "for future
  extensibility" parameters are explicitly unwanted. Call the existing method directly. Do not pair a
  value argument with a boolean meaning "ignore that argument". Do not promote a value to a global or
  a config-template field for one caller.
- **No transitional dual paths.** When a mechanism is replaced, every in-tree user is converted and
  the legacy machinery deleted in the same change set. A change that removes the old path is correct;
  "you should keep a compatibility shim" is not a finding here.
- **Comments are sparse on purpose.** Comment only what is unexpected. "This function lacks a doc
  comment" is not a finding. Comments must describe current behavior, never change history — a
  comment narrating what used to be there *is* a finding.
- **NXCP command codes and `VID_*` constants** must be mirrored: new `CMD_*` into
  `NXCPMessageCodeName()` in `src/libnetxms/nxcp.cpp` and into `NXCPCodes.java`. A missing mirror is a
  real finding.
- **Java**: 3-space indent, minimal external dependencies, and every referenced class gets an
  `import` — an inline fully-qualified name (outside `java.lang` and same-package) is a finding.

## Reporting bar

- **Report** anything in "blast radius" or "what a real failure looks like", with the path through
  the code that reaches it. A concrete failing input, sequence or lock order beats a category name.
- **Report** cross-surface omissions: server change without the Java/UI/NXSL/WebAPI/schema half,
  `openapi.yaml` not updated, DB upgrade not backported, debug tag not added to
  `doc/internal/debug_tags.txt`.
- **Do not report** style preferences, alternative-library suggestions, missing comments, or
  speculative refactors. This project treats those as noise.
- **Scope discipline is enforced.** A finding that a change should have *also* fixed some unrelated
  pre-existing problem belongs in pre-existing, not in findings.
- CI already runs semgrep (blocks on ERROR), cppcheck nightly, and a `-Wcast-function-type` gate.
  Do not spend findings on what those catch; do flag a function-pointer cast, since that one blocks
  the build.
