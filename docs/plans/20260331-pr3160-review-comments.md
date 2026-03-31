# PR #3160 Review Comments — Implementation Plan

## Overview
Address 6 reviewer comments on the `asterisk-remove-exosip-dependency` PR that replaced eXosip2/oSIP2 with a raw SIP implementation. The work is split into three branches for independent review:

1. **`sendex-timeout`** — enhance `SendEx` with deadline-based timeout (comment 6)
2. **`tls-connection-improvements`** — split TLS changes into separate PR (comment 3)
3. **`asterisk-remove-exosip-dependency`** — remaining fixes (comments 1, 2, 4, 5)

## Context (from discovery)
- PR changed 9 files: `configure.ac`, `doc/internal/debug_tags.txt`, `include/tls_conn.h`, `src/libnetxms/tls_conn.cpp`, `src/agent/subagents/asterisk/{Makefile.am,asterisk.h,system.cpp,ua.cpp}`
- `SendEx` is used in 35 files / 59 call sites — adding a default timeout parameter is backward compatible
- `DEBUG_TAG` is `_T("sa.asterisk")` (not `_T("asterisk")` as reviewer stated) — `sa.asterisk` already exists in `debug_tags.txt:204`
- Two nearly identical ~100-line response loops exist in `ua.cpp` (lines 1227-1323 and 1461-1558)
- Custom SIP status codes 1-4 conflict with the convention that SIP codes are 3-digit; will use 901-904

## Development Approach
- No automated test suite exists for libnetxms or the asterisk subagent
- Verification is compilation-based: build each branch and check for errors
- Each branch gets its own commit(s); asterisk branch rebases onto TLS branch
- **CRITICAL: update this plan file when scope changes during implementation**

## Progress Tracking
- Mark completed items with `[x]` immediately when done
- Add newly discovered tasks with ➕ prefix
- Document issues/blockers with ⚠️ prefix

## Implementation Steps

### Task 1: Create `sendex-timeout` branch and enhance SendEx

**Files:**
- Modify: `include/nms_util.h` (line 5774 — function declaration)
- Modify: `src/libnetxms/tools.cpp` (lines 1758-1802 — function implementation)

- [x] Create branch `sendex-timeout` from `master`
- [x] Add `uint32_t timeout = 60000` parameter to `SendEx` declaration in `nms_util.h`
- [x] In `tools.cpp`, calculate deadline at function start: `int64_t deadline = GetMonotonicClockTime() + timeout;`
- [x] Replace hardcoded `p.poll(60000)` with `int64_t remaining = deadline - GetMonotonicClockTime(); if (remaining <= 0) break; p.poll(static_cast<uint32_t>(remaining));`
- [x] Commit changes on `sendex-timeout`

### Task 2: Create `tls-connection-improvements` branch with TLS changes

**Files:**
- Modify: `include/tls_conn.h`
- Modify: `src/libnetxms/tls_conn.cpp`

- [ ] Create branch `tls-connection-improvements` from `sendex-timeout`
- [ ] Add `m_verifyPeer` member, `enablePeerVerification()`, `getSocket()` to `tls_conn.h`
- [ ] Replace `plainSend()` with `SendEx(m_socket, data, size, 0, nullptr, timeout)` in `send()` method
- [ ] Remove `plainSend()` declaration from header and implementation from `.cpp`
- [ ] Apply `tlsRecv`/`tlsSend` fixes: deadline-based timeout, fix SocketPoller direction bug (`SSL_ERROR_WANT_READ` → `SSL_ERROR_WANT_WRITE`)
- [ ] Fix `connect()`: pass `timeout` to `startTLS()` (was hardcoded `0`)
- [ ] Fix `startTLS()`: `context` → `m_context` typo, add `#include <nxcrypto.h>`
- [ ] Add peer verification support in `startTLS()`: trusted cert store, `SSL_VERIFY_PEER`, hostname/IP verification (OpenSSL version-dependent)
- [ ] Add deadline-based handshake timeout in `startTLS()`
- [ ] Commit changes on `tls-connection-improvements`

### Task 3: Rebase asterisk branch and apply fixes

**Files:**
- Modify: `src/agent/subagents/asterisk/asterisk.h`
- Modify: `src/agent/subagents/asterisk/ua.cpp`
- Modify: `configure.ac` (remove `--with-asterisk` option, remove conditional asterisk COMPONENTS/SUBAGENT_DIRS blocks)
- Modify: `src/agent/subagents/Makefile.am` (add `asterisk` to hardcoded SUBDIRS)

- [ ] Switch to `asterisk-remove-exosip-dependency` branch
- [ ] Rebase onto `tls-connection-improvements`
- [ ] Resolve any conflicts (TLS file changes should disappear from diff since they're now in the base)

**Build always by default (no external deps anymore):**
- [ ] In `configure.ac`: remove `AC_ARG_WITH(asterisk, ...)` block (lines ~552-556)
- [ ] In `configure.ac`: remove `check_substr "$COMPONENTS" "asterisk"` / `SUBAGENT_DIRS="$SUBAGENT_DIRS asterisk"` block (lines ~1437-1440)
- [ ] In `configure.ac`: remove static agent asterisk check block (lines ~1185-1189)
- [ ] In `src/agent/subagents/Makefile.am`: add `asterisk` to hardcoded SUBDIRS (alongside filemgr, logwatch, ping, etc.)

**C2 — Revert `atomic<bool>` to `bool` (comment 4):**
- [ ] In `asterisk.h:182`, change `atomic<bool> m_stop;` → `bool m_stop;`

**C3 — Convert SIP status codes 1-4 → 901-904 (comment 5):**
- [ ] Update `s_sipStatusCodes[]` entries: `1→901`, `2→902`, `3→903`, `4→904`
- [ ] Update all `*status = 1` assignments → `901` (lines ~1023, 1035, 1061, 1068)
- [ ] Update all `*status = 2` assignments → `902` (lines ~1081, 1100, 1152)
- [ ] Update `*status = 3` → `903` (line ~1117) and `*status = 4` → `904` (lines ~1207, 1448)

**C4 — Extract `ReceiveAndValidateSIPResponse` helper (comment 2):**
- [ ] Define `SIPConnectionContext` struct to group connection params (transport, udpSocket, tlsConn, streamState)
- [ ] Create `ReceiveAndValidateSIPResponse()` function above `RegisterSIPClient()`:
  - Parameters: SIPConnectionContext*, responseBuffer, responseBufferSize, responseLength (out), deadline, retransmitMsg, retransmitMsgLen, expectedCSeq, expectedCallId, uri, proxy
  - Returns: SIP status code (>=100) on success, -1 on timeout
  - Handles: UDP retransmission (T1→T2 backoff), TLS stream receive, response parsing, CSeq/Call-ID validation, provisional (1xx) handling
- [ ] Replace initial REGISTER loop (lines ~1227-1323) with call to helper
- [ ] Replace authenticated REGISTER loop (lines ~1461-1558) with call to helper
- [ ] Commit all asterisk changes

### Task 4: Verify and reply to reviewer

- [ ] `grep -r "plainSend" src/` — confirm no leftover references
- [ ] `grep -rn "\*status = [1-4];" src/agent/subagents/asterisk/` — confirm all old codes updated
- [ ] Reply to PR comment 1 clarifying: `DEBUG_TAG` is `_T("sa.asterisk")` not `_T("asterisk")`, and `sa.asterisk` already exists in `debug_tags.txt:204`
- [ ] Move this plan to `docs/plans/completed/`

## Technical Details

### SendEx enhanced signature
```cpp
ssize_t LIBNETXMS_EXPORTABLE SendEx(SOCKET hSocket, const void *data, size_t len, int flags, Mutex* mutex, uint32_t timeout = 60000);
```

### SIPConnectionContext struct
```cpp
struct SIPConnectionContext
{
   SIPTransport transport;
   SOCKET udpSocket;
   TLSConnection *tlsConn;
   SIPStreamState *streamState;
};
```

### ReceiveAndValidateSIPResponse signature
```cpp
static int ReceiveAndValidateSIPResponse(
   SIPConnectionContext *conn,
   char *responseBuffer,
   size_t responseBufferSize,
   ssize_t *responseLength,
   int64_t deadline,
   const char *retransmitMsg,
   size_t retransmitMsgLen,
   int expectedCSeq,
   const char *expectedCallId,
   const char *uri,
   const char *proxy)
```

### Branch dependency chain
```
master → sendex-timeout → tls-connection-improvements → asterisk-remove-exosip-dependency (rebased)
```

## Post-Completion

**PR management:**
- Push `sendex-timeout` branch and open PR targeting `master`
- Push `tls-connection-improvements` and open PR targeting `sendex-timeout` initially (retarget to `master` after sendex-timeout merges)
- Force-push rebased `asterisk-remove-exosip-dependency` (depends on tls-connection-improvements merging first)
- Reply to reviewer on PR #3160 about comment 1 (debug_tags.txt)
