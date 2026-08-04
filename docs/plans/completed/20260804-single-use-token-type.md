# Single-Use Authentication Token as a Token Type

## Overview

Single-use authentication tokens are currently modelled as a boolean flag (`singleUse`) that is
orthogonal to `AuthenticationTokenType`. The flag is accepted for every token type, but only one
combination is meaningful: two of the four possible pairings are either dead (`SERVICE` + single-use,
which nothing asks for) or silently corrected by the descriptor constructor (`PERSISTENT` +
single-use, forced off). Reviewers found this misleading — the API advertises a freedom of
combination that does not exist.

This change replaces the flag with a fourth `AuthenticationTokenType` value, `SINGLE_USE`. The
invalid combinations become unrepresentable instead of being defensively corrected, and the four
mutually exclusive kinds of token are described by one field instead of three booleans plus an enum.

The change is confined to the server. The NXCP and REST representations both stay **byte-identical**:
the wire has always carried a set of booleans rather than a type, so the server simply derives those
booleans from the type. No client, schema, or protocol work is required.

## Context (from discovery)

Files/components involved:

- `src/server/include/auth-token.h` — `AuthenticationTokenType` enum, `AuthenticationTokenDescriptor`
  struct, `fillMessage()`, `toJson()`. Included only by `src/server/include/nms_objects.h`, so the
  whole type is server-contained.
- `src/server/include/nms_users.h` — declarations of `IssueAuthenticationToken()`,
  `ValidateAuthenticationToken()`, `ConsumeAuthenticationToken()`.
- `src/server/core/authtokens.cpp` — issuance, the shared `ProcessAuthenticationToken()`
  implementation behind validate/consume, and `ShowAuthenticationTokens()`.
- `src/server/core/session.cpp` — `authenticateUserByToken()` (the sole spend point),
  `issueAuthToken()` (NXCP issuance boundary).
- `src/server/webapi/users.cpp` — `H_UserTokenCreate()` (REST issuance boundary).
- `tests/test-authtokens/test-authtokens.cpp` — unit harness; compiles the real `authtokens.cpp`.
- `doc/Single_Use_Authentication_Tokens.md` — design/administration document.

Related patterns found:

- `ConsumeAuthenticationToken()` is deliberately **not** marked `NXCORE_EXPORTABLE`, so the linker
  confines the spend primitive to server core. `src/server/CLAUDE.md` documents this as intentional.
- Server code is Unicode-only: `L"..."` and `wchar_t`, never `_T()` / `TCHAR`. `auth-token.h` still
  carries `TCHAR` remnants. Formatting into a `wchar_t` buffer uses `nx_swprintf`, not `_sntprintf`.
- `tests/include/testtools.h` has `AssertEquals` overloads for int32/int64/uint/size_t/ssize_t and
  `char*`/`wchar_t*` — **no `enum class` overload**.

Dependencies identified:

- No database schema dependency — token type is never persisted. `auth_tokens` rows are persistent
  tokens by construction.
- No NXCP protocol dependency — the enum value is never serialized. `VID_SINGLE_USE` stays as-is.
- `ValidateAuthenticationToken()`'s callers outside tests are unaffected by the out-param change:
  `webapi_router.cpp:306` passes `nullptr` explicitly, `session.cpp:4248` passes only two arguments
  and relies on the default. Its only non-`nullptr` third-argument caller in the tree is
  `test-authtokens.cpp:169`.
- The five `IssueAuthenticationToken()` callers not listed below (`session.cpp:3021`,
  `session.cpp:4262`, `reporting.cpp:369`, `reporting.cpp:430`, `webapi_auth.cpp:81`) all pass fewer
  than six arguments, so dropping the trailing `bool singleUse` leaves them untouched.

## Development Approach

- **testing approach**: Regular (code first, then tests) — this is a mechanical refactor of existing
  behaviour with an existing unit harness. No new behaviour is introduced, so there is nothing to
  drive with a failing test first.
- **CRITICAL — this refactor does not compile until Tasks 1-6 are all complete.** Removing three
  fields from a struct breaks every reader of those fields simultaneously; there is no intermediate
  state where the tree builds. Per-task test runs are therefore impossible, and the plan does not
  pretend otherwise: Tasks 1-6 form **one compilable unit**, and the build-and-test gate lives at the
  end of Task 6. Do not attempt `make` before then.
- Test coverage must come out **neutral or better**, never negative. No new behaviour is introduced,
  so no new scenarios are needed — but Task 6 must not quietly lose coverage while migrating. Two
  specific traps, both called out in that task: the existing asserts rely on a poison-value pattern
  that a naive enum rewrite makes vacuous, and the one test being deleted needs a replacement rather
  than a pointer to a suite the build gate does not run.
- Complete each task fully before moving to the next.
- Behaviour must be preserved exactly. Any observable change other than the two intended ones (server
  console `show authtokens` column, audit-log wording) is a bug.

## Testing Strategy

- **unit tests**: `tests/test-authtokens/` — migrated in Task 6, run in Task 6's final checkbox and
  again in Task 8. Requires `./configure --with-tests`. This is the only suite the build gate runs, so
  anything that must be verified before merge has to live here.
- **integration tests**: `tests/integration/.../SingleUseTokenTest.java` — must pass **unchanged**.
  This is the load-bearing verification that the wire format did not move: it exercises issuance,
  single-use login, reuse rejection, ordinary-token reuse, and the persistent+single-use rejection
  entirely through the Java client. If it needs edits, the refactor has leaked past the server.
- **e2e tests**: not applicable — no UI change.

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope

## Solution Overview

`AuthenticationTokenType` gains `SINGLE_USE = 3`. `AuthenticationTokenDescriptor` replaces its three
booleans with a single `AuthenticationTokenType type` field, and everything that used to read a
boolean now compares against the enum.

Key design decisions:

- **No `isPersistent()` / `isService()` / `isSingleUse()` accessors.** Call sites compare directly:
  `descriptor->type == AuthenticationTokenType::PERSISTENT`. Three one-line predicates that exist
  only to shorten a comparison are the kind of indirection the project's design principles reject.
- **Validation stays at the protocol boundary.** `issueAuthToken()` and `H_UserTokenCreate()` keep
  reading the request booleans and keep rejecting `persistent && singleUse`. That check is what makes
  the combination unreachable now that the constructor no longer corrects it — it is not redundant,
  it is the replacement.
- **Wire compatibility by derivation, not by storage.** `fillMessage()` and `toJson()` compute the
  three booleans from the type at serialization time.
- **`SINGLE_USE` composes with nothing.** The previous model left room for a service token that is
  also single-use. Nothing asked for it, and dropping the possibility is the point of the change.

## Technical Details

Enum:

```cpp
enum class AuthenticationTokenType
{
   EPHEMERAL = 0,    // Not saved to database, usually short-lived
   PERSISTENT = 1,   // Saved to database, usually with long expiration time
   SERVICE = 2,      // Similar to ephemeral but will not trigger existing session disconnect
   SINGLE_USE = 3    // Memory-only, destroyed by the login that spends it
};
```

Signatures after the change:

```cpp
shared_ptr<AuthenticationTokenDescriptor> NXCORE_EXPORTABLE IssueAuthenticationToken(uint32_t userId,
   uint32_t validFor, AuthenticationTokenType type = AuthenticationTokenType::EPHEMERAL,
   const wchar_t *description = nullptr, uint32_t maxLifetime = 0);

// As shipped, the tokenType out-parameter was dropped entirely: no production caller reads it once
// the spend point moves to ConsumeAuthenticationToken(), so the wrapper passes nullptr internally.
bool NXCORE_EXPORTABLE ValidateAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId,
   uint32_t validFor = 0, time_t *expiresAt = nullptr, time_t *maxExpiresAt = nullptr);

bool ConsumeAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId,
   AuthenticationTokenType *tokenType = nullptr);
```

Request-flag → type mapping, used identically at both issuance boundaries:

```cpp
   AuthenticationTokenType type = persistent ? AuthenticationTokenType::PERSISTENT :
      (singleUse ? AuthenticationTokenType::SINGLE_USE : AuthenticationTokenType::EPHEMERAL);
```

Wire derivation in `fillMessage()` (`toJson()` mirrors it):

```cpp
   msg->setField(baseId + 2, type == AuthenticationTokenType::PERSISTENT);
   msg->setField(baseId + 7, type == AuthenticationTokenType::SERVICE);
   msg->setField(baseId + 8, type == AuthenticationTokenType::SINGLE_USE);
```

Intended observable changes (everything else must stay identical):

| Surface | Before | After |
|---|---|---|
| `show authtokens` column | `Flags` — `PSUV` characters | `Type` — `ephemeral` / `persistent` / `service` / `single-use` |
| `show authtokens` `V` flag | separate column position | dropped; `Token` column already prints `unavailable` |
| Audit log on issuance | `single-use ephemeral` | `single-use` |

## What Goes Where

- **Implementation Steps**: all server C++ changes, unit-test migration, documentation rewrite.
- **Post-Completion**: manual server-console verification, integration-test run against a live server.

## Implementation Steps

### Task 1: Add SINGLE_USE type and collapse descriptor flags

**Files:**
- Modify: `src/server/include/auth-token.h`

- [x] add `SINGLE_USE = 3` to `enum class AuthenticationTokenType` with a comment noting it is
      memory-only and destroyed by the login that spends it
- [x] replace the `persistent` / `service` / `singleUse` members of `AuthenticationTokenDescriptor`
      with a single `AuthenticationTokenType type` field, placed after `userId`; keep `claimed` and
      `validClearText` as they are
- [x] drop the `bool _singleUse` parameter from the main constructor, delete the
      `singleUse = _singleUse && (type != AuthenticationTokenType::PERSISTENT);` line and its
      comment, and assign `type = _type;` instead of the three boolean assignments
      (the `type` parameter was renamed to `_type` to avoid shadowing the new member)
- [x] collapse the DB-record constructor's `persistent`/`service`/`singleUse` assignments (and the
      "Persistent tokens are never single-use" comment) into `type = AuthenticationTokenType::PERSISTENT;`
- [x] derive the wire booleans in `fillMessage()` (baseId+2, +7, +8) and in `toJson()`
      (`persistent`, `service`, `singleUse` keys) from `type` — field IDs and JSON key names unchanged
- [x] sweep `TCHAR` / `_T()` to `wchar_t` / `L""` throughout the file: constructor parameter
      `const TCHAR *_description`, `TCHAR buffer[64]` in `toString()` and `toMaskedString()`,
      `TCHAR text[128]` in the DB constructor, and `_T("********")` / `_T("%.4s****%.4s")`
- [x] replace `_sntprintf` in `toMaskedString()` with `nx_swprintf` per `src/server/CLAUDE.md`
- [x] update the constructor doc comment: remove the `@param _singleUse` line, and reword `@param type`
      from "ephemeral, persistent, or service" to cover all four
- [x] call the Unicode sweep out separately in the commit message — it is convention-aligned per
      `src/server/CLAUDE.md` and a strict no-op (server builds are Unicode-only), but it enlarges a
      diff otherwise advertised as mechanical
- [x] do NOT build yet — the tree does not compile until Task 6

### Task 2: Update token issuance and validation API

**Files:**
- Modify: `src/server/core/authtokens.cpp`
- Modify: `src/server/include/nms_users.h`

- [x] add file-static `TokenTypeName(AuthenticationTokenType)` in `authtokens.cpp` returning
      `L"persistent"` / `L"service"` / `L"single-use"` / `L"ephemeral"` (default branch)
- [x] drop the trailing `bool singleUse` parameter from `IssueAuthenticationToken()` in both the
      definition and the `nms_users.h` declaration; remove its `@param singleUse` doc line and reword
      `@param type` to cover all four types
- [x] replace the issuance debug log's 2-way ternary (`authtokens.cpp:188-189`) with `TokenTypeName(type)`
- [x] change `ProcessAuthenticationToken()`'s `bool *serviceToken, bool *singleUseToken` parameters to
      a single `AuthenticationTokenType *tokenType`; set it from `descriptor->type` and update the
      `@param` block
- [x] change the single-use gate inside `ProcessAuthenticationToken()` to
      `descriptor->type == AuthenticationTokenType::SINGLE_USE`, leaving the `claimed`
      `InterlockedIncrement` guard, `s_tokens.remove(hash)` and both debug messages untouched
- [x] update the `ValidateAuthenticationToken()` and `ConsumeAuthenticationToken()` wrappers and their
      `nms_users.h` declarations to the new out-param; keep `ConsumeAuthenticationToken()` undecorated
      and keep the comment explaining why
- [x] replace `descriptor->persistent` / `d->persistent` with
      `... type == AuthenticationTokenType::PERSISTENT` at `authtokens.cpp` lines 217, 245, 269, 306,
      315, 409
- [x] do NOT build yet

### Task 3: Replace flags column with type name in server console

**Files:**
- Modify: `src/server/core/authtokens.cpp`

- [x] in `ShowAuthenticationTokens()`, replace the `Flags` header with a `Type` header 10 characters
      wide and widen the separator row's second column to 12 dashes (10 + the two surrounding spaces)
- [x] replace the four `%c` flag arguments with `TokenTypeName(descriptor->type)` under a `%-10s`;
      `persistent` and `single-use` are both exactly 10 characters, so nothing truncates
- [x] drop the `V` flag — `validClearText` is false only for descriptors built by the DB-record
      constructor, and the `Token` column already prints `unavailable` in exactly that case
- [x] convert the touched `printf` literals from `_T()` to `L""` and the lambda's
      `TCHAR userName[MAX_USER_NAME]` to `wchar_t`, so the block is not left half-converted
- [x] note in the commit message that this also fixes a pre-existing alignment bug: the header used
      `%s` for a 5-character `"Flags"` while the data row emitted 6 characters
- [x] do NOT build yet

### Task 4: Map request flags to token type in NXCP session handler

**Files:**
- Modify: `src/server/core/session.cpp`

- [x] in `authenticateUserByToken()`, replace the `bool serviceToken` / `bool singleUseToken` locals
      with one `AuthenticationTokenType tokenType` passed to `ConsumeAuthenticationToken()`;
      initialize it explicitly to `AuthenticationTokenType::EPHEMERAL` — `enum class` has no
      zero-state and the variable is only read on the success path, so leaving it uninitialized
      invites `-Wmaybe-uninitialized`
- [x] gate the "single-use token consumed" debug message on
      `tokenType == AuthenticationTokenType::SINGLE_USE`
- [x] change the close-other-sessions suppression to
      `(tokenType == AuthenticationTokenType::SERVICE) || (tokenType == AuthenticationTokenType::SINGLE_USE)`
      and reword its comment to refer to handoff tokens
- [x] in `issueAuthToken()`, keep the `VID_PERSISTENT` / `VID_SINGLE_USE` reads and the
      `persistent && singleUse` → `RCC_INVALID_ARGUMENT` rejection; map the pair to an
      `AuthenticationTokenType` local and pass it to `IssueAuthenticationToken()` without the trailing
      boolean (the audit-log string local was renamed `tokenTypeName` to free `type` for the enum)
- [x] change the audit-log type string from `L"single-use ephemeral"` to `L"single-use"`
- [x] verify `ValidateAuthenticationToken()` at `session.cpp:4248` still compiles unchanged (it passes
      only `&tokenUserId`)
- [x] do NOT build yet

### Task 5: Map request flags to token type in WebAPI handler

**Files:**
- Modify: `src/server/webapi/users.cpp`

- [x] in `H_UserTokenCreate()`, keep the `singleUse` JSON read, the `persistent && singleUse` → HTTP
      400 rejection and its error text unchanged
- [x] map the pair to an `AuthenticationTokenType` local and pass it to `IssueAuthenticationToken()`
      without the trailing boolean
- [x] change the audit-log type string from `L"single-use ephemeral"` to `L"single-use"`
- [x] verify `ValidateAuthenticationToken()` at `webapi_router.cpp:306` still compiles unchanged (it
      passes `nullptr` for the out-param) — the file lives in `src/server/core/`, not `src/server/webapi/`
- [x] do NOT build yet

### Task 6: Update authentication token unit tests and build

**Files:**
- Modify: `tests/test-authtokens/test-authtokens.cpp`

- [x] change the `IssueToken()` helper to
      `IssueToken(uint32_t validFor, AuthenticationTokenType type = AuthenticationTokenType::EPHEMERAL, uint32_t maxLifetime = 0)`
      and drop the `singleUse` argument it forwards
- [x] translate the nine `IssueToken()` call sites (lines 82, 104, 130, 145, 162, 195, 235, 265, 266)
      by pattern — drop the boolean, and where it was `true` pass `AuthenticationTokenType::SINGLE_USE`
      instead of the type that followed it. Six distinct shapes: `(600, true)`, `(600, false)`,
      `(600, false, SERVICE)`, `(60, false, EPHEMERAL, 3600)`, `(60, true, EPHEMERAL, 86400)`, `(0, true)`
- [x] replace the paired `bool serviceToken` / `bool singleUseToken` locals with a single
      `AuthenticationTokenType tokenType` at lines 89-91, 107-117, 133-150, 166-171, 221-222, 271-272
- [x] assert with `AssertTrue(tokenType == AuthenticationTokenType::X)` — `testtools.h` has no
      `enum class` `AssertEquals` overload, so a comparison avoids casting both sides
- [x] **preserve the poison-value pattern.** The `bool` locals being replaced are pre-set to the
      opposite of the expected result (`serviceToken = true` at line 107, `singleUseToken = true` at
      133, 139 and 147, `= false` at 89) precisely so the assertion proves the out-param was *written*,
      not merely left alone. Initialize each `tokenType` local to a value the call must overwrite —
      e.g. `SERVICE` before consuming a single-use token, `SINGLE_USE` before validating an ephemeral
      one. Initializing to the expected value would make every one of these assertions vacuous and
      would pass against a `ProcessAuthenticationToken()` that never writes `*tokenType`
- [x] delete `TestPersistentIsNeverSingleUse()` (lines 283-292) and its `main()` call: it asserted the
      constructor's silent forcing, which no longer exists as state to check, and cannot be rewritten
      because the `_singleUse` parameter is gone
- [x] **replace it** with a `TestSingleUseIsMemoryOnly()` covering the half of that invariant which is
      still a runtime property: issue via `IssueToken(600, AuthenticationTokenType::SINGLE_USE)` and
      assert `descriptor->tokenId == 0` and `descriptor->type == AuthenticationTokenType::SINGLE_USE`.
      Do not rely on `SingleUseTokenTest.testPersistentSingleUseTokenRejected()` to cover this — it
      lives in the Maven integration suite, which `netxms-test-suite` does not run, so without this
      test the build gate has no coverage of the exclusivity at all
- [x] add the new test to `main()` in place of the deleted call
- [x] verify `TestConcurrentConsume()`, `TestExpiredSingleUse()` and `TestPersistentExpirationClamp()`
      keep their existing assertions — the claim race and the expiration clamp are unaffected
- [x] build the tree (used the tree's existing configuration — `--with-server --with-agent --with-client
      --with-tests` against the local prefix `/Users/alk/netxms/master` with the strict clang warning
      set — rather than the generic line below, which would drop the macOS pgsql/openssl/microhttpd
      paths this host needs): `make -j$(nproc) && make install`. Clean build; no warnings originate in
      `authtokens.cpp` or `test-authtokens.cpp`
- [x] run `./tests/suite/netxms-test-suite` — must pass before Task 7 (whole suite exits 0; all
      `test-authtokens` cases OK; the binary had nine cases at this point, later review passes
      brought it to fifteen)

### Task 7: Rewrite single-use token documentation

**Files:**
- Modify: `doc/Single_Use_Authentication_Tokens.md`

- [x] update the "Context" section (line 18): "three kinds" → four, and add a single-use bullet to the
      list alongside ephemeral, persistent and service (the follow-on "All of them share one property"
      sentence became "The first three share one property" — replayability is exactly what single-use
      does not have, so listing it above that sentence required the qualifier)
- [x] rewrite "Observing them" (lines 106-111): the `PSUV` flags column is now a `Type` column showing
      the type name, and a consumed token still disappears from the listing (also noted that the
      `Token` column reads `unavailable`, which is what replaced the dropped `V` flag)
- [x] rewrite "Where this fits" (lines 118-136) to the opposite conclusion: single-use is a token type,
      the four types are mutually exclusive, and the `SERVICE` + single-use composition is deliberately
      gone because nothing asked for it. Lines 128-132 (no REST spend point) stay as they are; lines
      133-136 need only "The flag defaults to false everywhere" reworded, since the request field is
      still a boolean and still defaults to false — the statement remains true, the noun does not
- [x] keep the "Memory only, and what that costs" section: the persistent+single-use rejection, its
      `RCC_INVALID_ARGUMENT` / HTTP 400 codes and the `"persistent": false` note are all still accurate
- [x] confirm the "Further reading" pointers still resolve (`openapi.yaml` schema fields, Java client
      methods, debug tags) — none of them moved (`singleUse` at `openapi.yaml:11031`/`:11073`,
      `NXCSession.requestAuthenticationToken()`, `AuthenticationToken.isSingleUse()`, `auth` tag)

### Task 8: Verify acceptance criteria

- [x] `grep -rn "singleUse\|_singleUse" src/server/` returns only the request-field reads at the two
      issuance boundaries and the JSON/NXCP key names — no descriptor field, no function parameter
      (hits: `session.cpp:3057-3112` and `users.cpp:364-400` request flags plus their audit strings,
      `auth-token.h:223` JSON key, and `openapi.yaml` schema/doc text)
- [x] `grep -rn -- "->persistent\|->singleUse" src/server/` returns nothing. Do **not** anchor the
      pattern on `descriptor->` — two of the six live sites (`authtokens.cpp:269` and `:409`) spell it
      `d->persistent` inside lambdas and would slip through a half-done refactor
- [x] `grep -n -- "->service\b" src/server/core/authtokens.cpp src/server/include/auth-token.h` returns
      nothing; scope it to those two files, since a tree-wide `->service` false-positives on
      `record->serviceName` in `otellog.cpp` and `otlp/`
- [x] confirm `include/nms_cscp.h`, `src/server/webapi/openapi.yaml`, the Java client and
      `SingleUseTokenTest.java` are untouched in `git diff --stat` — compared against `d91e2931ce`
      (the plan commit, i.e. the pre-refactor state), not `master`: the branch's earlier commit
      `8e0769433b` introduced single-use tokens themselves and legitimately touches all four. The
      refactor's own diff spans only `authtokens.cpp`, `session.cpp`, `auth-token.h`, `nms_users.h`,
      `users.cpp`, `test-authtokens.cpp` and the two docs
- [x] run the full suite: `./tests/suite/netxms-test-suite` — exits 0; all `test-authtokens`
      cases OK (nine at the time of this run, fifteen after the review passes that followed). Note
      the generated script lost its executable bit, so it was invoked as
      `sh tests/suite/netxms-test-suite`
- [x] start `netxmsd -D6`, run `show authtokens` on the server console and confirm the `Type` column
      renders and aligns for at least an ephemeral and a persistent token — manual test (skipped, not
      automatable: needs a live server and database). Alignment verified statically instead: header
      and data rows both use `%-10s` for the type and the separator row's second column is 12 dashes
- [x] confirm `ConsumeAuthenticationToken()` is still undecorated in `nms_users.h` and its rationale
      comment survived (`nms_users.h:630-637`, both intact)

### Task 9: [Final] Update documentation

- [x] confirm `src/server/CLAUDE.md`'s `ConsumeAuthenticationToken()` paragraph is still accurate — it
      is, unchanged: the symbol is still the sole spend point for single-use tokens, still undecorated
      in `nms_users.h:637`, and its rationale comment (`nms_users.h:631-636`) survived the refactor.
      Only the out-parameter type changed, which the paragraph does not mention
- [x] confirm `doc/internal/debug_tags.txt` needs no change — no new debug tags introduced; the
      refactor adds no `nxlog_debug_tag()` call, and every touched message keeps the existing `auth`
      tag (`debug_tags.txt:18`)
- [x] move this plan to `docs/plans/completed/`

## Post-Completion

*Items requiring manual intervention or external systems — no checkboxes, informational only*

**Manual verification:**

- Run `tests/integration/.../SingleUseTokenTest.java` against a live server built from this branch.
  It must pass **without modification** — that is the proof the wire format did not move. All four
  cases matter: single-use login, reuse rejection, ordinary-token reuse, and persistent+single-use
  rejection.
- Confirm the management console still reconnects after a restart, exercising the ephemeral reconnect
  token issued by `finalizeLogin()`.
- Check a report execution end to end — `reporting.cpp` issues `SERVICE` tokens and is the only
  in-tree consumer of that type.

**External system updates:**

- `nxmc-launcher` lives in its own repository and still does not request single-use tokens. Nothing
  there needs updating; it is unaffected because the wire format is unchanged.
