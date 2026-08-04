# Single-Use Authentication Tokens

> **Superseded by** [20260804-single-use-token-type.md](20260804-single-use-token-type.md). This
> plan modelled single-use as a boolean orthogonal to `AuthenticationTokenType`; the shipped code
> has `SINGLE_USE` as a fourth value of that enum. Two consequences for the text below: references
> to a `U` display flag in `show authtokens` are obsolete — the listing has a `Type` column naming
> the type — and the composition of single-use with `SERVICE` no longer exists.

## Overview

Add single-use (one-shot) authentication tokens to the NetXMS server. A single-use token is a
bootstrap credential: it authenticates exactly one login and is destroyed in the process.

Primary use case is the existing `nxmc-launcher` (separate repository). Today the launcher logs in
with the user's credentials, requests an ephemeral token valid for 600 seconds, **closes its own
connection**, and spawns `java -jar <cached build> -server=<address> -token=<token> -auto`. This
change lets that token be single-use.

**The token must have a non-zero TTL, and that is precisely what makes single-use necessary.** nxmc
takes time to start, so the token cannot be instantaneous — it has to stay valid across the launch.
During that window the value sits in the new process's command line, where any local user running
`ps` can read it. Single-use bounds that exposure: the moment nxmc completes its login the observed
value is worthless. The current 600-second ephemeral token stays replayable for its entire lifetime
by anyone who saw it.

Single-use is modelled as a property **orthogonal** to the existing `AuthenticationTokenType` enum
rather than as a fourth enum value, so it composes freely with EPHEMERAL and SERVICE semantics.
The feature is memory-only: no schema change, no `nxdbmgr` upgrade procedure, and no cross-branch
DB backport.

Key benefits:

- A handoff credential that cannot be replayed, even though it is necessarily observable on the
  command line during the launch window.
- A single designated spend point, so a stolen token cannot be burned through some unrelated API.
- No database or schema impact whatsoever.

## Context (from discovery)

All line numbers below were verified against HEAD `24539db517` with a clean working tree.

Files/components involved:

- `src/server/include/auth-token.h` — `AuthenticationTokenType` enum (EPHEMERAL / PERSISTENT /
  SERVICE) and `AuthenticationTokenDescriptor` with `fillMessage()` (fields at `baseId+0` ..
  `baseId+7`, stride 10 — `baseId+8` is free) and `toJson()`.
- `src/server/core/authtokens.cpp` — `IssueAuthenticationToken()` (line 128),
  `ValidateAuthenticationToken()` (line 269), the `validFor` extension block (lines 299-307),
  `CheckUserAuthenticationTokens()` expiry sweep (line 321), `ShowAuthenticationTokens()` console
  dump (line 389, prints `P`/`S`/`V` at lines 397-400), and the static
  `SynchronizedSharedHashMap<UserAuthenticationTokenHash, AuthenticationTokenDescriptor> s_tokens`
  (line 95).
- `src/server/include/nms_users.h:617-622` — declarations of both functions.
- `src/server/core/session.cpp` — `authenticateUserByToken()` (line 2560) with its validator call
  (2571) and the service-token `closeOtherSessions = false` line (2581); the 300s ephemeral
  reconnect token issued after every successful login (3016); `issueAuthToken()` /
  CMD_REQUEST_AUTH_TOKEN handler (3041); the deliberate audit skip for self-issued ephemeral tokens
  (3057-3059).
- `src/server/core/webapi_router.cpp:306` — REST token validation, executed on every authenticated
  request; returns 401 on failure (lines 307-313). Passes `AUTH_TOKEN_VALIDITY_TIME` (14400s) as
  `validFor`.
- `src/server/webapi/users.cpp:334` — `H_UserTokenCreate`, POST `/v1/users/:user-id/tokens`. Note
  its `persistent` parameter defaults to **true** (line 360).
- `include/nms_cscp.h` — VID constants. `VID_PERSISTENT` is 596; `VID_MARKDOWN` = 1023 (line 1792)
  is the top of the base range, so **1024 is free**. `NXCPCodes.java` is contiguous through 1023
  (line 1602) — there are no gaps to fill.
- `src/client/java/netxms-client/.../NXCSession.java:2936` —
  `requestAuthenticationToken(boolean, int, String, int)`;
  `org.netxms.client.users.AuthenticationToken` reads `baseId+0` .. `baseId+6` only.

**All three existing `ValidateAuthenticationToken()` call sites**, and what each becomes:

| Site | After this change |
|------|-------------------|
| `session.cpp:2571` (`authenticateUserByToken`) | switches to `ConsumeAuthenticationToken()` — **the sole spend point** |
| `session.cpp:4203` (`enableAnonymousObjectAccess`) | validation call unchanged; rejects single-use tokens. Its token is issued PERSISTENT (line 4217) so it can never be single-use anyway. The surrounding handler did change — see "Changes that landed beyond the original plan" |
| `webapi_router.cpp:306` | unchanged; rejects single-use tokens → 401, **zero changes needed** |

Related patterns found:

- `SERVICE` tokens already suppress `closeOtherSessions` (session.cpp:2581) — the mechanism this
  feature reuses.
- `VolatileCounter` + `InterlockedIncrement` is the established atomic-counter idiom.
  `SynchronizedSharedHashMap::remove()` returns `void` (`include/nms_util.h:3741-3746`) and there is
  no atomic take-and-report primitive, so an explicit claim counter is the minimal correct
  mechanism, not gold-plating.

Dependencies identified:

- The `tests/ha` precedent cited during design is **not** a usable model: `halease.cpp` includes only
  `<halease.h>` and is self-contained, and `tests/ha` has no `Makefile.w32` at all (the HA harness is
  deliberately POSIX-only — see `Makefile.w32:155-157`). Use `tests/test-libnxsrv/` as the template
  for both the autotools and Windows wiring.
- `authtokens.cpp` includes `nxcore.h`, `nms_users.h`, and `netxms-webapi.h`. The latter includes
  `<microhttpd.h>` (`netxms-webapi.h:29`), so `@MICROHTTPD_CPPFLAGS@` is required to compile it.
- Standalone compilation confirms the server-core link surface is exactly **four** symbols:
  `ConfigReadULong`, `CreateUniqueId`, `ExecuteQueryOnObject`, `ResolveUserId`. Note
  `GetAuthTokenMaxLifetime()` is `static inline` (`netxms-webapi.h:48`) and cannot be stubbed — it
  inlines a call to `ConfigReadULong`, which is the symbol to stub. `ServerConsole::printf` is
  `LIBNXSRV_EXPORTABLE` and defined in `src/server/libnxsrv/console.cpp:44` — link `libnxsrv`, do not
  stub it.
- There are currently **no tests of any kind** covering authentication tokens.

## Development Approach

- **testing approach**: Regular (code first, then tests) — the C++ unit-test harness for
  `authtokens.cpp` does not exist yet and cannot be written before the fields it exercises exist.
- complete each task fully before moving to the next
- make small, focused changes
- **CRITICAL: every task MUST include new/updated tests** for code changes in that task
  - Tasks 1 and 2 legitimately defer their tests to Task 3, which builds the harness; this is the
    documented partial-implementation exception and is called out explicitly in those tasks
  - tests cover both success and error scenarios
- **CRITICAL: all tests must pass before starting next task** — no exceptions
- **CRITICAL: update this plan file when scope changes during implementation**
- run tests after each change
- maintain backward compatibility: `IssueAuthenticationToken()` gains only a trailing defaulted
  parameter, and `ValidateAuthenticationToken()` keeps its existing signature, so all ten existing
  call sites compile unchanged

## Testing Strategy

- **unit tests**: new C++ test binary `tests/test-authtokens`, covering the two-function contract
  and the atomic claim. Built only under `--with-tests`, wired through `@TEST_MODULES@`.
- **integration tests**: Java/Maven test in `tests/integration/` against a running server, covering
  the real end-to-end NXCP path — issue, spend, and confirm the second use fails.
- **e2e tests**: not applicable; the nxmc UI is explicitly out of scope.
- Manual verification of the WebAPI surface via `curl` (see Post-Completion), since REST handlers
  have no unit-test harness in this tree.

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope
- keep plan in sync with actual work done

## Solution Overview

**The central rule — one designated spend point, expressed as a separate function.**

A single-use token is spent at exactly one place in the codebase. Rather than encode that with a
boolean parameter on the existing validator, the two operations get two functions:

- `ValidateAuthenticationToken()` — unchanged signature. **Never** consumes, and **always** rejects
  a single-use token. Every existing caller keeps calling this and is correct by default.
- `ConsumeAuthenticationToken()` — new. Validates the token and, if it is single-use, consumes it.
  Called from exactly one place: `authenticateUserByToken()`.

Both share one internal static helper so the expiry and max-lifetime logic is not duplicated.

Why a separate function rather than a `bool consume` flag: the requirement is a *single entry
point*, and a function name states that where a defaulted boolean does not. It also removes the
failure mode where a future caller passes `true` without meaning to, and it keeps the common
signature untouched.

**`ConsumeAuthenticationToken()` must NOT be marked `NXCORE_EXPORTABLE`.** Its only caller,
`session.cpp`, sits in the same shared library as `authtokens.cpp` (both are in
`libnxcore_la_SOURCES` — `src/server/core/Makefile.am:11,34`), so an undecorated symbol resolves
intra-library on every platform. Exporting it would publish the spend primitive to every module that
includes `nms_users.h` — `webapi/users.cpp`, `webapi/grafana.cpp`, `webapi/2fa.cpp`,
`webapi/alarms.cpp`, `webapi/object_collections.cpp`, and others — which is precisely the surface
this design exists to close. Leaving the decoration off makes the single-entry-point rule structural
rather than advisory: the linker enforces it, not a comment.

Why the spend point is NXCP login and not "wherever the token is first presented": a stolen token
observed in `ps` should not be burnable through some unrelated REST call. Restricting the spend to
login means the only thing an attacker can do with the token is exactly the thing the legitimate
client does — and they lose the race the moment nxmc logs in.

**Why an orthogonal flag, not a fourth enum value.** Note that as scoped, single-use does not
actually compose with anything: `PERSISTENT + singleUse` is rejected at both issuance surfaces, and
`SERVICE` tokens are only ever issued from two hardcoded internal sites (`reporting.cpp:375` and
`:424`) that this plan does not touch — so every single-use token this change can produce is
EPHEMERAL. The argument is therefore not about composition. It is that `AuthenticationTokenType`
answers "where does this token live and how does it behave on login", while single-use answers "how
many times can it be spent" — two independent questions. A fourth enum value would force
`type == SINGLE_USE` special-casing at every site that currently tests for `PERSISTENT` or
`SERVICE`, and would have no honest value to report in the `DB_RESULT` constructor, which
reconstructs persistent tokens only. A separate bool keeps both questions answerable independently
and leaves the door open if a future caller does want `SERVICE + singleUse`.

**Why memory-only.** A persistent single-use token would require a column on `auth_tokens`, an
`nxdbmgr` upgrade procedure, and propagation across release branches — a large cost for a
credential meant to live for seconds. `PERSISTENT + singleUse` is rejected at the issuance surfaces.

**Why single-use suppresses `closeOtherSessions`.** `UF_CLOSE_OTHER_SESSIONS` is a per-user account
flag. This is **defensive rather than load-bearing** in the launcher flow: the launcher closes its
own connection before spawning nxmc, and its own credential login has already closed the user's
other sessions by then, so there is usually nothing left for the token login to close. It matters
when a single-use token is issued by something that stays connected — a WebAPI integration, or a
future launcher that keeps its session — where a handoff would otherwise tear down the issuer. A
handoff token is the same user starting one more client, not a fresh login that should displace
everything else.

**Why a claimed token is burnt even if login then fails.** If the token is claimed and
`AuthenticateUser()` subsequently fails (disabled account, intruder lockout), the token is already
gone and cannot be retried. This is deliberate: a one-shot credential that survives a failed
attempt is a retry oracle.

## Technical Details

Descriptor additions (`AuthenticationTokenDescriptor`):

```cpp
bool singleUse;            // token is destroyed when consumed
VolatileCounter claimed;   // atomic claim guard, 0 until consumed
```

Function signatures:

```cpp
// Trailing defaulted parameter - all 7 existing call sites unaffected
shared_ptr<AuthenticationTokenDescriptor> NXCORE_EXPORTABLE IssueAuthenticationToken(
   uint32_t userId, uint32_t validFor, AuthenticationTokenType type = AuthenticationTokenType::EPHEMERAL,
   const wchar_t *description = nullptr, uint32_t maxLifetime = 0, bool singleUse = false);

// UNCHANGED signature. Now additionally rejects single-use tokens.
bool NXCORE_EXPORTABLE ValidateAuthenticationToken(
   const UserAuthenticationToken& token, uint32_t *userId, bool *serviceToken = nullptr,
   uint32_t validFor = 0, time_t *expiresAt = nullptr, time_t *maxExpiresAt = nullptr);

// NEW. The single spend point. Validates any token; consumes it if single-use.
// Deliberately NOT NXCORE_EXPORTABLE - see Solution Overview.
bool ConsumeAuthenticationToken(
   const UserAuthenticationToken& token, uint32_t *userId, bool *serviceToken = nullptr,
   bool *singleUseToken = nullptr);
```

The `singleUseToken` out-parameter is **required**, not optional polish: on a successful consume the
descriptor is removed from `s_tokens`, so the caller cannot look it up afterwards to discover what
it was. Without it, `authenticateUserByToken()` has no way to suppress `closeOtherSessions`. It
mirrors the existing `serviceToken` out-parameter.

**It must be written unconditionally on every success path**, exactly as `serviceToken` is at
`authtokens.cpp:309-310` — not only inside the single-use branch. If it were assigned only when the
token is single-use, the caller's local would be read uninitialised on every ordinary reconnect
login, and `closeOtherSessions` would be suppressed at random on the hottest login path in the
product. The caller initialises its local to `false` as well, so neither side depends on the other
getting it right.

`ConsumeAuthenticationToken()` deliberately takes no `validFor`, `expiresAt`, or `maxExpiresAt`.
This is behaviour-preserving, not a simplification: the call being replaced
(`session.cpp:2571`) passes only three arguments today, so `validFor` is already 0 and the extension
block at `authtokens.cpp:299-307` never executes at this call site. Ordinary reconnect tokens are
unaffected.

Shared internal helper, holding the logic currently in `ValidateAuthenticationToken()`
(authtokens.cpp:269-316), with one new branch:

| Condition | Outcome |
|-----------|---------|
| token absent from `s_tokens` | FAIL (existing behaviour) |
| past `maxExpirationTime` or `expirationTime` | FAIL, remove, delete from DB if persistent (existing behaviour) |
| `singleUse && !consuming` | FAIL; level-4 debug line noting a single-use token was presented outside the login entry point |
| `singleUse && consuming` | `InterlockedIncrement(&claimed)`; if result != 1 another thread won the race → FAIL. Otherwise remove from `s_tokens` and proceed |
| `!singleUse` | proceed; `validFor` extension applies as today |

Two things about placement and the branches are load-bearing:

- **The single-use branches must sit before the `validFor` extension block (lines 299-307).**
  `webapi_router.cpp:306` passes `AUTH_TOKEN_VALIDITY_TIME` (14400s). If the rejection landed after
  the extension, every REST replay of a stolen one-shot would silently push its expiry out by four
  hours (capped by `maxExpirationTime`). The token still could not be spent, but the
  bounded-exposure property in the Overview would be substantially weakened.
- **The losing thread must not call `remove()`.** Only the caller observing an increment result of 1
  removes the entry.

Wire format: `fillMessage()` writes the flag at `baseId + 8` (`+7` is the existing service flag,
stride is 10). `toJson()` adds a `"singleUse"` boolean.

Processing flow for the launcher handoff (steps 1-4 already exist today):

1. Launcher authenticates to the server with the user's credentials.
2. Launcher requests a token — **this call gains `singleUse = true`** — with a `validFor` long enough
   to cover nxmc startup (the launcher currently uses 600s).
3. Server issues an EPHEMERAL single-use token and returns the clear-text value in the response.
   (The value also remains readable through a token listing for as long as the descriptor is in
   memory — it is the descriptor's removal on consumption, not the response, that ends exposure.)
4. Launcher closes its own connection and spawns nxmc with `-token=<token>`. The value is
   observable in the process command line for the duration of the startup window.
5. nxmc logs in over NXCP; `authenticateUserByToken()` calls `ConsumeAuthenticationToken()`.
6. Token is atomically claimed, removed from `s_tokens`, and `closeOtherSessions` is forced false.
7. Session receives its normal 300s ephemeral reconnect token (session.cpp:3016) as usual.
8. Any replay of the still-visible value fails — the descriptor no longer exists. The exposure
   window closes at step 5 rather than at token expiry.

Note that step 2 is a change in `nxmc-launcher`, a **separate repository**, and is not part of this
plan. See Post-Completion.

## What Goes Where

- **Implementation Steps** (`[ ]` checkboxes): all code, build wiring, tests, and documentation
  changes inside this repository.
- **Post-Completion** (no checkboxes): the `nxmc-launcher` change, GitHub issue creation, manual REST
  verification, and follow-up observations recorded but deliberately not fixed here.

## Implementation Steps

### Task 1: Add single-use fields to the token descriptor

**Files:**
- Modify: `src/server/include/auth-token.h`

- [x] add `bool singleUse` and `VolatileCounter claimed` members to `AuthenticationTokenDescriptor`
- [x] add a trailing `bool singleUse = false` parameter to the issuing constructor; initialise
      `claimed` to 0
- [x] in the `DB_RESULT` constructor, always set `singleUse = false` and `claimed = 0` — persistent
      tokens are never single-use
- [x] extend `fillMessage()` to write the flag at `baseId + 8` (leaving the service flag at
      `baseId + 7` untouched)
- [x] extend `toJson()` with a `"singleUse"` boolean
- [x] tests deferred to Task 3, which builds the harness these fields are exercised by — no test
      infrastructure for `authtokens.cpp` exists yet
- [x] verify the tree still compiles: `make -C src/server/core`

### Task 2: Split validate and consume, and add single-use issuance

**Files:**
- Modify: `src/server/include/nms_users.h`
- Modify: `src/server/core/authtokens.cpp`

- [x] add trailing `bool singleUse = false` to the `IssueAuthenticationToken()` declaration
      (`nms_users.h:617-618`) and definition (`authtokens.cpp:128`); pass through to the descriptor
- [x] extract the body of `ValidateAuthenticationToken()` (`authtokens.cpp:269-316`) into a file-static
      helper taking a `bool consuming` argument, the existing out-parameters, **and the new
      `bool *singleUseToken`**
- [x] have the helper write `*singleUseToken` (when non-null) **unconditionally on every success
      path**, mirroring how `serviceToken` is handled at `authtokens.cpp:309-310` — assigning it only
      inside the single-use branch would leave the caller reading an uninitialised value on every
      ordinary reconnect login
- [x] add the two single-use branches from Technical Details to that helper, positioned after the
      expiry/max-lifetime checks (ending line 298) and **before** the `validFor` extension block
      (lines 299-307)
- [x] use `InterlockedIncrement(&descriptor->claimed)` for the claim; only the caller observing a
      result of 1 wins, and **only that caller** calls `s_tokens.remove()`
- [x] add a comment at the claim site recording that a claimed-then-failed login deliberately burns
      the token (fail-closed; a survivable one-shot is a retry oracle)
- [x] keep `ValidateAuthenticationToken()`'s signature exactly as it is; reimplement it as a call to
      the helper with `consuming = false`
- [x] add `ConsumeAuthenticationToken()` in `nms_users.h` next to the existing declarations, as a
      call to the helper with `consuming = true`, exposing the `singleUseToken` out-parameter
- [x] declare it **without** `NXCORE_EXPORTABLE`. Its only caller is in the same shared library
      (`src/server/core/Makefile.am:11,34`), so it links fine undecorated; exporting it would publish
      the spend primitive to the five webapi sources that include `nms_users.h` and defeat the
      single-entry-point rule
- [x] document in a comment above both functions that `ConsumeAuthenticationToken()` is the single
      designated spend point, and that the missing export decoration is deliberate so a future
      maintainer does not "fix" it
- [x] add a level-4 debug line for the `singleUse && !consuming` rejection, using
      `token.toMaskedString()` as the surrounding code does
- [x] add a `U` flag character to `ShowAuthenticationTokens()` (`authtokens.cpp:397-400`) — the
      format is `%c%c%c` followed by three spaces inside a 7-char column whose separator rule is at
      line 392, so drop one trailing space when adding the 4th `%c`
- [x] confirm no changes are needed in `CheckUserAuthenticationTokens()` — unclaimed single-use
      tokens expire through the existing `expirationTime` sweep
- [x] tests deferred to Task 3 (harness does not exist yet)
- [x] verify the tree still compiles and links: `make -C src/server` — libnxcore, netxmsd and webapi
      build clean. ⚠️ `nxdbmgr`, `nxget`, `nddload`, `nxwsget` and `nxminfo` fail to link in this
      build tree with missing `nx_wcsnicmp`/`nx_wcslwr`/`nx_wcsupr`; verified identical with the
      change stashed, so it is a pre-existing stale-libnetxms issue unrelated to this work

### Task 3: Build the C++ unit-test harness and cover both functions

**Files:**
- Create: `tests/test-authtokens/Makefile.am`
- Create: `tests/test-authtokens/Makefile.w32`
- Create: `tests/test-authtokens/test-authtokens.cpp`
- Modify: `configure.ac`
- Modify: `tests/suite/netxms-test-suite.in`
- Modify: `tests/suite/netxms-test-suite.cmd`
- Modify: `Makefile.w32`

- [x] create `tests/test-authtokens/Makefile.am` modelled on **`tests/test-libnxsrv/Makefile.am`**
      (not `tests/ha`): `bin_PROGRAMS = test-authtokens`, sources
      `test-authtokens.cpp ../../src/server/core/authtokens.cpp`
- [x] set CPPFLAGS to `-I@top_srcdir@/include -I@top_srcdir@/src/server/include -I../include
      -I@top_srcdir@/build -DNXCORE_EXPORTS @MICROHTTPD_CPPFLAGS@`. All four additions are load
      bearing: `-DNXCORE_EXPORTS` because otherwise `NXCORE_EXPORTABLE` resolves to
      `__declspec(dllimport)` (`src/server/include/nms_core.h:32`, `include/symbol_visibility.h:34`)
      and defining dllimport-decorated symbols is a hard error on MinGW; `@MICROHTTPD_CPPFLAGS@`
      because `netxms-webapi.h:29` includes `<microhttpd.h>`; `-I../include` for `testtools.h`
- [x] set LDADD to the `libnxsrv` chain from `tests/test-libnxsrv/Makefile.am:15-22` (libnxsrv,
      libnxsnmp, libnxsl, libnxdb, libnxagent, libnetxms, plus the `USE_INTERNAL_JANSSON`
      conditional) — `ServerConsole::printf` lives in `libnxsrv` and must be linked, not stubbed
- [x] add stub definitions for exactly four server-core symbols: `ConfigReadULong`,
      `CreateUniqueId`, `ExecuteQueryOnObject`, `ResolveUserId`. Do **not** try to stub
      `GetAuthTokenMaxLifetime` — it is `static inline` in `netxms-webapi.h:48` and inlines
      `ConfigReadULong`
- [x] restrict all test cases to EPHEMERAL and SERVICE tokens so the DB code paths are never entered
      and `ExecuteQueryOnObject` can stay a no-op
- [x] write test: `ValidateAuthenticationToken()` rejects a single-use token **and leaves it valid**
      for a subsequent `ConsumeAuthenticationToken()` call
- [x] write test: `ConsumeAuthenticationToken()` succeeds once on a single-use token, reports
      `singleUseToken = true`, and fails on every subsequent attempt
- [x] write test: `ConsumeAuthenticationToken()` on a non-single-use token succeeds, reports
      `singleUseToken = false`, and leaves the token reusable — guards the reconnect-token regression
- [x] write test: `ValidateAuthenticationToken()` on a non-single-use token behaves exactly as before
      this change, including `validFor` extension
- [x] write test: rejection happens before expiry extension. `ValidateAuthenticationToken()` writes
      `*expiresAt` only on success (`authtokens.cpp:311-312`), so a rejected call reports nothing —
      instead hold the `shared_ptr<AuthenticationTokenDescriptor>` that `IssueAuthenticationToken()`
      returns (`authtokens.cpp:180`), record `descriptor->expirationTime`, validate the single-use
      token with a large `validFor`, and assert the field is unchanged
- [x] write test: concurrent `ConsumeAuthenticationToken()` from N threads yields exactly one success
- [x] write test: unclaimed single-use token past its expiration time is rejected. Use **two
      separate expired tokens**, one per function — the first call to either function hits the expiry
      branch and calls `s_tokens.remove()` (`authtokens.cpp:293`), so reusing one token would make
      the second call exercise the "does not exist" branch at line 274 instead
- [x] wire into `configure.ac`: add `test-authtokens` to the `TEST_MODULES` assignment at line 1295
      (the `--with-server` group) and `tests/test-authtokens/Makefile` to `AC_CONFIG_FILES` near
      line 4817. `tests/Makefile.am:11` already picks up `@TEST_MODULES@` and needs no edit.
      ➕ also added to the `--with-dist` `TEST_MODULES` list (line 1046) so maintainer dist builds
      pick it up, and added the binary to `.gitignore` alongside the other test binaries
- [x] create `Makefile.w32` modelled on `tests/test-libnxsrv/Makefile.w32`; add the directory to the
      **nested** `ifeq ($(BUILD_SERVER),1)` block at `Makefile.w32:173` (not the flat `TESTS_DIRS`
      list), and add a dependency edge alongside `Makefile.w32:274`. A `vpath` entry locates
      `authtokens.cpp` under `src/server/core` so the object lands in the test's own directory
- [x] add a guarded `if [ -x $BINDIR/test-authtokens ]` block to `tests/suite/netxms-test-suite.in`
      and the equivalent to `netxms-test-suite.cmd`
- [x] regenerate the build system after editing `configure.ac`: `./init-source-tree` (or
      `autoreconf`), then re-run `./configure --with-tests ...`
- [x] `make && make install` — the suite runner executes binaries from `$BINDIR`
      (`netxms-test-suite.in:8-16`), so an uninstalled binary will be silently skipped.
      ⚠️ top-level `make install` aborts in `src/client/nxshell` (`Shell.java` calls
      `NXCSession.parseConnectionAddress()`, which does not exist); pre-existing and unrelated, so
      the C++ libraries and `tests` were installed directly
- [x] run tests — must pass before next task: `./tests/suite/netxms-test-suite` — full suite green,
      including the 7 new `test-authtokens` cases

### Task 4: Make NXCP login the single spend point

**Files:**
- Modify: `src/server/core/session.cpp`

- [x] in `ClientSession::authenticateUserByToken()` (line 2560), replace the
      `ValidateAuthenticationToken()` call at line 2571 with `ConsumeAuthenticationToken()`, passing
      a `bool singleUseToken = false` out-parameter — initialise the local explicitly rather than
      relying on the helper always writing it
- [x] set `loginInfo->closeOtherSessions = false` when `singleUseToken` is true, alongside the
      existing service-token line at 2581
- [x] add a debug line recording that a single-use token was consumed for this login
- [x] verify by inspection that this is the **only** `ConsumeAuthenticationToken()` call site in the
      tree, and that `webapi_router.cpp:306` and `session.cpp:4203` still call
      `ValidateAuthenticationToken()` unchanged — confirmed by tree-wide grep; the anonymous access
      site is now at `session.cpp:4207` after the edit
- [x] behavioural coverage is provided by the Java integration test in Task 8; the login path
      requires a running server and has no C++ unit-test harness
- [x] run tests — must pass before next task: `./tests/suite/netxms-test-suite` — full suite green,
      including all 7 `test-authtokens` cases

### Task 5: Add the NXCP issuance surface

**Files:**
- Modify: `include/nms_cscp.h`
- Modify: `src/java-common/netxms-base/src/main/java/org/netxms/base/NXCPCodes.java`
- Modify: `src/server/core/session.cpp`

- [x] add `VID_SINGLE_USE` to `include/nms_cscp.h` as 1024, immediately after `VID_MARKDOWN` (1023,
      line 1792); re-confirm 1024 is still free at implementation time — confirmed free
- [x] mirror the constant into `NXCPCodes.java` after line 1602. The file is contiguous through
      1023 — there are no adjacent gaps to fill
- [x] in `ClientSession::issueAuthToken()` (line 3041), read `VID_SINGLE_USE` and pass it through to
      `IssueAuthenticationToken()` (explicit `maxLifetime = 0` so the trailing flag can be passed)
- [x] reject `persistent && singleUse` with `RCC_INVALID_ARGUMENT` before issuing
- [x] extend the audit log to record the single-use flag, and ensure single-use tokens are audited
      **even when self-issued** — the deliberate skip at lines 3057-3059 targets high-frequency
      ephemeral reconnect tokens, which these are not. Token type is now a single `tokenType` string
      (`persistent` / `single-use ephemeral` / `ephemeral`) shared by both audit branches
- [x] behavioural coverage is provided by Task 8
- [x] run tests — must pass before next task: `./tests/suite/netxms-test-suite` — full suite green,
      including all 7 `test-authtokens` cases. `make -C src/server` now builds clean end to end; the
      stale-libnetxms link failures noted in Task 2 are gone after the Task 3 install

### Task 6: Add the WebAPI issuance surface

**Files:**
- Modify: `src/server/webapi/users.cpp`
- Modify: `src/server/webapi/openapi.yaml`

- [x] in `H_UserTokenCreate` (line 334), read `"singleUse"` from the request body, defaulting to
      false
- [x] reject `persistent && singleUse` with HTTP 400. Because `persistent` defaults to **true**
      (line 360), a body of `{"singleUse": true}` alone hits this path — the error message must
      state explicitly that `singleUse` requires `"persistent": false`
- [x] pass the flag through to `IssueAuthenticationToken()` and include it in the audit log entry
      (token type string mirrors the NXCP handler: `persistent` / `single-use ephemeral` /
      `ephemeral`)
- [x] add `singleUse` to the **`AuthenticationTokenCreateInput`** schema (`openapi.yaml:11036`),
      documenting the rejected combination and the 400 response
- [x] add `singleUse` to the **`AuthenticationToken`** schema (`openapi.yaml:11008`) — this is the
      response schema fed by `toJson()`, used by both the 201 response and
      `GET /v1/users/{user-id}/tokens`
- [x] document in the `singleUse` property description that such a token can only be spent on an
      NXCP login and will be rejected if presented as a REST bearer credential
- [x] verify the `toJson()` output added in Task 1 surfaces `"singleUse"` in the 201 response body —
      `auth-token.h:209` emits it unconditionally, so it appears in both the 201 body and the list
- [x] REST handlers have no unit-test harness in this tree; manual `curl` verification is recorded
      in Post-Completion
- [x] run tests — must pass before next task: `./tests/suite/netxms-test-suite` — full suite green,
      including all 7 `test-authtokens` cases; `make -C src/server/webapi` builds clean

### Task 7: Extend the Java client

**Files:**
- Modify: `src/client/java/netxms-client/src/main/java/org/netxms/client/NXCSession.java`
- Modify: `src/client/java/netxms-client/src/main/java/org/netxms/client/users/AuthenticationToken.java`

- [x] add a `requestAuthenticationToken(boolean persistent, int validFor, String description,
      int userId, boolean singleUse)` overload (line 2936 area) that sets `VID_SINGLE_USE`
- [x] keep the existing four-argument signature working by delegating with `singleUse = false` —
      `nxmc-launcher` depends on it and must keep building until it opts in
- [x] read the flag at `baseId + 8` in the `AuthenticationToken` NXCP constructor and add an
      `isSingleUse()` getter with Javadoc
- [x] do **not** touch the orphaned `service` flag at `baseId + 7` — out of scope, recorded in
      Post-Completion
- [x] build the client libraries: `mvn -f src/java-common/netxms-base/pom.xml install` then
      `mvn -f src/client/java/netxms-client/pom.xml install` — both install clean
- [x] run tests — must pass before next task: `./tests/suite/netxms-test-suite` (guards against C++
      side regressions; the Java behavioural coverage lands in Task 8) — full suite green, including
      all 7 `test-authtokens` cases

### Task 8: Add the end-to-end integration test

**Files:**
- Create: `tests/integration/src/test/java/org/netxms/tests/SingleUseTokenTest.java`

- [x] create the test following the conventions of the existing tests in that directory — extends
      `AbstractSessionTest`, uses `TestConstants` for connection parameters, JUnit 5
- [x] write test: issue a single-use token, log in with it, assert success (`testLoginWithSingleUseToken`,
      also asserts `isSingleUse()` and `!isPersistent()` on the issued token)
- [x] write test: assert a second login with the same token value fails (`testSingleUseTokenCannotBeReused`,
      expects `RCC.ACCESS_DENIED`)
- [x] write test: assert a non-single-use ephemeral token still supports repeated reconnect —
      guards the regression that the login path must not consume ordinary tokens
      (`testOrdinaryTokenIsNotConsumed`, three consecutive logins)
- [x] write test: assert requesting `persistent = true` together with `singleUse = true` is rejected
      (`testPersistentSingleUseTokenRejected`, expects `RCC.INVALID_ARGUMENT`)
- [x] the WebAPI-bearer-401 case is deliberately **not** covered here: `tests/integration/pom.xml`
      declares no HTTP client and the harness supplies no WebAPI base URL, and Task 3 already covers
      that rule at the validator where it lives
- [x] run the integration suite against a running server (skipped — not automatable in this
      environment). ⚠️ No NetXMS server is running locally and no local database matches this build's
      schema (dev databases are at 62.26, build requires 70.15), so the suite cannot be executed here.
      Verified instead that the module compiles clean: `mvn -f tests/integration/pom.xml test-compile`
      (required installing `src/mobile-agent/java` into the local repository first). The live run is
      recorded as manual verification and is repeated as a checkbox in Task 9.

### Task 9: Verify acceptance criteria

- [x] verify all requirements from Overview are implemented — orthogonal `singleUse` flag (not a
      fourth enum value), memory-only, single NXCP spend point, both issuance surfaces, Java client
      support, `U` console flag
- [x] verify the single-entry-point rule holds: grep the tree for `ConsumeAuthenticationToken` and
      confirm the call inside `authenticateUserByToken()` (line 2572 today) is the only call site —
      confirmed; the only other occurrences are the declaration, the definition, two doc comments,
      and `tests/test-authtokens`
- [x] verify `ConsumeAuthenticationToken()` is **not** marked `NXCORE_EXPORTABLE`, and that the
      webapi module still builds — proving nothing outside libnxcore reaches for it. Declaration at
      `nms_users.h:638` is undecorated and `make -C src/server/webapi` builds clean
- [x] verify every `ValidateAuthenticationToken()` call site rejects single-use tokens — structural:
      both remaining sites (`session.cpp:4219`, `webapi_router.cpp:306`) go through
      `ProcessAuthenticationToken(..., consuming = false)`, whose rejection branch is covered by two
      unit tests
- [x] verify no existing call site of `IssueAuthenticationToken()` (7 sites) or
      `ValidateAuthenticationToken()` (3 sites, one of which moves to the new function) needed
      signature changes — the diff touches only the two issuance sites that deliberately pass the new
      flag (`session.cpp:3065`, `webapi/users.cpp:372`); `reporting.cpp:375,424`,
      `webapi_auth.cpp:80`, `session.cpp:3020,4233` are unmodified
- [x] verify `webapi_router.cpp` is genuinely unmodified — `git diff master...HEAD` on that file is
      empty
- [x] verify no database schema change was introduced anywhere in the diff — no `nxdbmgr`,
      `upgrade_v*`, `netxmsdb.h` or SQL file appears in the branch diff
- [x] verify server code added uses `L"..."` literals and no `_T()`/TCHAR, 3-space indent, and
      C++11 constructs only. ➕ fixed one new `_T()` literal in the `debugPrintf()` line added by
      Task 4 (`session.cpp:2573`). The `_T()` uses left in `ShowAuthenticationTokens()` are the
      pre-existing statement being extended, not new code
- [x] run full test suite: `./tests/suite/netxms-test-suite` — 566 cases OK, exit 0
- [x] run the Java integration suite against a running server (skipped — not automatable in this
      environment, same blocker as Task 8: no running server and no database matching this build's
      schema level). Compile-level verification stands; the live run remains manual
- [x] confirm the MinGW build still succeeds, since Task 3 adds a Windows test directory — a real
      cross-build is not possible here (`build/config.mingw` and the Windows dependency SDK tree are
      absent), so verified by inspection against `src/server/core/Makefile.w32` and
      `tests/test-libnxsrv/Makefile.w32`. ⚠️ that inspection found a genuine defect: the new
      `tests/test-authtokens/Makefile.w32` compiled `authtokens.cpp` without
      `-I$(MICROHTTPD_ROOT)/include`, which `authtokens.cpp` needs via
      `netxms-webapi.h` → `<microhttpd.h>` — the Windows counterpart of the `@MICROHTTPD_CPPFLAGS@`
      already present in `Makefile.am`. Fixed; the MinGW build would otherwise have failed on that
      directory

### Task 10: [Final] Update documentation

**Files:**
- Modify: `CLAUDE.md` (only if a new pattern emerged)
- Move: `docs/plans/20260731-single-use-auth-tokens.md` → `docs/plans/completed/`

- [x] document single-use token semantics for administrators — including that they can only be spent
      on a login, and that they do not survive a server restart. Written as
      `doc/Single_Use_Authentication_Tokens.md`, an explanation-type document following the
      conventions of the other standalone guides in `doc/` (pandoc YAML header). Covers the spend
      point, the REST 401 / non-consuming rejection, memory-only lifetime and restart behaviour, the
      `persistent + singleUse` rejection at both surfaces (including the REST `persistent: true`
      default), suppressed `closeOtherSessions`, the burnt-on-failed-login trade-off, always-on
      auditing, the `U` flag in `show auth-tokens`, and the `auth` debug tag levels
- [x] no change needed to `doc/internal/debug_tags.txt`: the `auth` tag already exists (line 18) and
      this change adds no new tag — confirmed
- [x] update `CLAUDE.md` only if a genuinely new pattern emerged. ➕ recorded in
      `src/server/CLAUDE.md` under "Exporting Functions for Modules": a *missing*
      `NXCORE_EXPORTABLE` can be deliberate, restricting a primitive to intra-core callers so the
      linker enforces a single call site. That is the generalisable half of the validate/consume
      split; the split itself is feature-specific and already documented in code comments and in the
      new admin document
- [x] move this plan to `docs/plans/completed/`

## Post-Completion

*Items requiring manual intervention or external systems — no checkboxes, informational only*

**Follow-up change in `nxmc-launcher` (separate repository):**

- This plan delivers only the server and client-library support. The `nxmc-launcher` repository must
  be updated separately to pass `singleUse = true` when it requests its handoff token (currently an
  ephemeral 600s token, per step 5 of its README). Until that change lands, nothing about the
  launcher's behaviour changes — the new flag defaults to false.
- Keep the TTL as it is. Single-use bounds the exposure window by consumption, not by expiry, so
  there is no reason to shorten it and risk a token expiring during a cold start on a slow machine.

**Contribution workflow:**

- The project follows an issue-first workflow. A GitHub issue describing this feature and the design
  decisions above should exist before the pull request is opened, and the PR must reference it.

**Manual verification:**

- Exercise the REST surface against a running server:
  - `POST /v1/users/{id}/tokens` with `{"validFor": 60, "persistent": false, "singleUse": true}` →
    201 with `"singleUse": true` and a clear-text value in the response.
  - The same body with `"persistent": true` → 400 with a message naming the constraint.
  - The issued token in an `Authorization: Bearer` header on any authenticated route → 401, and the
    token still usable for a subsequent nxmc login (rejection must not consume it).
  - The same token used for an nxmc login → success; a second login attempt → failure.
- Confirm the handoff end to end with a user account that has `UF_CLOSE_OTHER_SESSIONS` set.
- Verify `show authtokens` in the server console names the token type and that the column
  alignment still matches the separator rule.

**Deliberate omission — REST token exchange:**

- There is no REST equivalent of the NXCP login spend point: a single-use token cannot be exchanged
  for a session token over HTTP. A `POST /v1/tokens/exchange` route accepting a single-use token and
  returning a fresh ephemeral one would give REST clients the same handoff capability, and returning
  a session token over TLS is acceptable from a security standpoint. It is omitted because the
  launcher flow does not need it and it adds an auth-critical route. Revisit if a REST-only
  integration needs the handoff.

**Observations recorded but deliberately not fixed** (per the project's Scope rule — pre-existing and
unrelated to single-use tokens, in files this change touches):

- `AuthenticationToken.java` never reads the `service` flag the server writes at `baseId + 7`.
- The clear-text token value is returned by token *listings*, not only by the issue response, for as
  long as the descriptor is held in memory (`auth-token.h`, `validClearText`). Access is gated on
  own-user-or-`SYSTEM_ACCESS_MANAGE_USERS` and a successful listing is not audited. This is
  pre-existing behaviour that the server console and the token management UI rely on; narrowing it to
  issue time is a product decision and needs its own issue.

**Changes that landed beyond the original plan** (recorded here because the plan requires scope
changes to be written down; each is small, and all are in the token issuance paths this change
already rewrites):

- `MAX_PERSISTENT_TOKEN_EXPIRATION_TIME` clamp plus rejection of a persistent token whose expiration
  time would not survive the 32-bit database column, at both issuance surfaces.
- Rejection of `validFor == 0` on `CMD_REQUEST_AUTH_TOKEN`. Note this is an NXCP behaviour change:
  such a request previously returned `RCC_SUCCESS` with an already-expired token and now returns
  `RCC_INVALID_ARGUMENT`. `IssueTokenDialog` computes validity from a picked date, so a date at or
  before now now yields a generic error popup — worth a follow-up to validate in the dialog.
- Rejection of an over-long `description` for a persistent token on the REST surface, which
  previously let the `INSERT` fail with only a warning while returning 201 (`MAX_TOKEN_DESCRIPTION_LENGTH`).
- Null checks on every `IssueAuthenticationToken()` result. The earlier revision of this plan recorded
  the `fillMessage()` call site as deliberately unfixed; it and the four other unchecked call sites
  (`session.cpp` reconnect token, `webapi_auth.cpp` `CompleteLogin()`, both `reporting.cpp` sites)
  were fixed instead, because `reporting.cpp:424` issues a token for a user id captured when a report
  was *scheduled* and crashes the server if that user has since been deleted.
- `getAuthenticationTokens()` in `NXCSession.java` advanced the field id by 9 instead of the server's
  stride of 10, so every listed token but the first was decoded from the wrong offsets. This one is a
  prerequisite: without it `isSingleUse()` is wrong for every token after the first.
- `RevokeAuthenticationToken(tokenId, userId)` now rejects `tokenId == 0` with `RCC_INVALID_TOKEN_ID`.
  Only persistent tokens are assigned an ID, so a lookup by 0 previously matched an arbitrary
  memory-only token — including one belonging to another user when the caller holds `MANAGE_USERS`.
- `enableAnonymousObjectAccess()` grants `OBJECT_ACCESS_READ` to the `anonymous` user only after the
  token was issued, and returns `RCC_RESOURCE_NOT_AVAILABLE` when it cannot be. Previously a failed
  issuance left the object readable by `anonymous` with no token handed out.
- Rejection of an over-long `description` for a persistent token on the NXCP surface as well, so that
  both issuance surfaces answer the same request the same way instead of one truncating silently.

**Deliberately out of scope — do not add without a new decision:**

- nxmc UI (`IssueTokenDialog.java`, `TokenManagement.java`) — server and API only.
- NXSL binding for token issuance.
- Server console / nxadm issuance command (the token type display in `show authtokens` is not issuance).
- Persistent single-use tokens and any accompanying schema change.
