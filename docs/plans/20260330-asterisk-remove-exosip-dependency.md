# Replace eXosip2/oSIP2 with raw SIP REGISTER in Asterisk subagent

## Overview

Remove the abandoned eXosip2/oSIP2 dependency from the Asterisk subagent by implementing SIP REGISTER + digest authentication directly over sockets. The subagent only uses eXosip2 for a single purpose: testing SIP client registration against Asterisk. This is a ~110-line function that can be replaced with raw SIP protocol handling using existing libnetxms utilities.

Key decisions from brainstorm:
- **Transport**: UDP (default for bare `sip:` URIs), TCP (`sip:host;transport=tcp`), TLS (`sips:host`, port 5061)
- **URI parsing**: Standard SIP URI parameters (RFC 3261), backward compatible with existing `sip:host` configs
- **Auth**: MD5 + SHA-256 digest auth, selected based on server's `WWW-Authenticate` `algorithm=` field
- **Timeout**: Single overall timeout using `GetMonotonicClockTime()` as a wall-clock deadline — the `timeout` parameter (in seconds) serves as both the SIP `Expires` header value and the operation deadline. Remaining time is passed to each recv/poll call.

## Context

**Files involved:**
- `src/agent/subagents/asterisk/ua.cpp` — only file using eXosip2 (the `RegisterSIPClient()` function, lines 122-230)
- `src/agent/subagents/asterisk/asterisk.h` — `SIPRegistrationTest` class (unchanged)
- `src/agent/subagents/asterisk/Makefile.am` — links `@ASTERISK_LIBS@` (currently `-leXosip2`)
- `configure.ac` — eXosip2 references at: line ~213 (`ASTERISK_LIBS=""`), lines 4454-4466 (header/lib checks), line ~4846 (`AC_SUBST(ASTERISK_LIBS)`), lines ~5525-5526 (summary output)

**Existing libnetxms utilities to use:**
- `CalculateMD5Hash()`, `CalculateSHA256Hash()` — digest computation
- `BinToStrAL()` — binary to lowercase hex string (digest auth requires lowercase)
- `uuid::generate()` — Call-ID and branch tag generation (available via `nms_util.h` → `asterisk.h` include chain)
- `TLSConnection` (`include/tls_conn.h`) — handles TCP connect, TLS handshake, send/recv with timeout, socket cleanup. Use for TCP and TLS transports.
- `ConnectToHostUDP()` + `SendEx()`/`RecvEx()` — for UDP transport
- `getsockname()` — obtain local IP:port for Via and Contact headers after socket connect

**What stays unchanged:**
- `RegisterSIPClient()` function signature: `static int32_t RegisterSIPClient(const char *login, const char *password, const char *domain, const char *proxy, int32_t timeout, int *status)`
- `SIPRegistrationTest` class and all its methods (except constructor proxy normalization — see Task 2)
- All parameter handlers (`H_SIPTestRegistration`, `H_SIPRegistrationTestData`, etc.)
- SIP status code table (`s_sipStatusCodes`)
- Config format — `sip:host` continues to work (defaults to UDP)

## Development Approach
- **Testing approach**: Regular (code first, then test manually against Asterisk)
- Complete each task fully before moving to the next
- Make small, focused changes
- No unit test framework available for subagents; validation is via debug logging + registration against a real Asterisk instance
- Run `make` after each task to verify compilation

## Implementation Steps

### Task 1: Remove eXosip2/oSIP2 from build system

**Files:**
- Modify: `configure.ac`
- Modify: `src/agent/subagents/asterisk/Makefile.am`

- [ ] Remove `ASTERISK_LIBS=""` initialization (line ~213 in configure.ac)
- [ ] Remove eXosip2 header/lib checks block (lines 4454-4466): `AC_CHECK_HEADER(eXosip2/eXosip.h, ...)` and `AC_CHECK_LIB(eXosip2, ...)`
- [ ] Remove `AC_SUBST(ASTERISK_LIBS)` (line ~4846)
- [ ] Remove `ASTERISK_LIBS` from summary output (lines ~5525-5526)
- [ ] Remove `@ASTERISK_LIBS@` from `Makefile.am` `asterisk_la_LIBADD` (no additional link flags needed — `TLSConnection` and OpenSSL are part of libnetxms)
- [ ] Verify no remaining `ASTERISK_LIBS` references with grep

### Task 2: Implement SIP URI parser and update proxy normalization

**Files:**
- Modify: `src/agent/subagents/asterisk/ua.cpp`

- [ ] Remove `#include <eXosip2/eXosip.h>`
- [ ] Remove the `eXosip_free()` helper function
- [ ] Add an enum `SIPTransport { SIP_UDP, SIP_TCP, SIP_TLS }` and a struct `SIPProxyAddress { char host[256]; uint16_t port; SIPTransport transport; }`
- [ ] Implement `static bool ParseSIPProxyURI(const char *uri, SIPProxyAddress *addr)` that extracts host, port, and transport from: `sip:host` (→ UDP:5060), `sip:host:port` (→ UDP:port), `sip:host;transport=tcp` (→ TCP:5060), `sips:host` (→ TLS:5061)
- [ ] Update `SIPRegistrationTest` constructor proxy prefix check (lines ~264 and ~287) to also recognize `sips:` as a valid prefix, so `sips:host` is not mangled to `sip:sips:host`
- [ ] Verify compilation with `make -C src/agent/subagents/asterisk`

### Task 3: Implement SIP message builder and response parser

**Files:**
- Modify: `src/agent/subagents/asterisk/ua.cpp`

- [ ] Implement `static void BuildSIPRegister(char *buffer, size_t bufferSize, const char *uri, const char *domain, const SIPProxyAddress *addr, const char *localIp, uint16_t localPort, const char *callId, int cseq, const char *branch, const char *fromTag, int expires, const char *authHeader)` — constructs a SIP REGISTER message with headers: Via, Max-Forwards, From, To, Call-ID, CSeq, Contact, User-Agent, Expires, Content-Length, and optionally Authorization. Uses `localIp`/`localPort` (obtained from `getsockname()` after connect) for Via and Contact headers.
- [ ] Implement `static int ParseSIPResponse(const char *response, size_t length)` — extracts status code from `SIP/2.0 NNN ...` first line
- [ ] Implement `static bool ExtractSIPHeader(const char *response, size_t length, const char *headerName, char *value, size_t valueSize)` — finds a header by name and copies its value (needed for WWW-Authenticate / Proxy-Authenticate)
- [ ] Verify compilation

### Task 4: Implement SIP digest authentication

**Files:**
- Modify: `src/agent/subagents/asterisk/ua.cpp`

- [ ] Define a `struct DigestChallenge { char realm[256]; char nonce[256]; char opaque[256]; char qop[64]; char algorithm[32]; }` to hold parsed challenge fields
- [ ] Implement `static bool ParseDigestChallenge(const char *wwwAuth, DigestChallenge *challenge)` — parses `Digest realm="...", nonce="...", ...` from WWW-Authenticate value
- [ ] Implement `static void ComputeDigestResponse(const char *login, const char *realm, const char *password, const char *nonce, const char *method, const char *uri, const char *nc, const char *cnonce, const char *qop, const char *algorithm, char *response, size_t responseSize)` — computes HA1, HA2, final response using MD5 (`CalculateMD5Hash`) or SHA-256 (`CalculateSHA256Hash`) based on `algorithm` parameter. Uses `BinToStrAL()` for hex encoding.
- [ ] Implement `static void BuildAuthorizationHeader(char *buffer, size_t bufferSize, const char *login, const DigestChallenge *challenge, const char *uri, const char *response, const char *nc, const char *cnonce)` — formats the `Authorization: Digest ...` header value
- [ ] Verify compilation

### Task 5: Rewrite RegisterSIPClient()

**Files:**
- Modify: `src/agent/subagents/asterisk/ua.cpp`

- [ ] Add `#include <tls_conn.h>` for TCP/TLS transport
- [ ] Replace the body of `RegisterSIPClient()` with new implementation:
  1. Parse proxy URI via `ParseSIPProxyURI()`
  2. Compute overall deadline: `int64_t deadline = GetMonotonicClockTime() + static_cast<int64_t>(timeout) * 1000`
  3. Connect: for UDP use `ConnectToHostUDP()`, for TCP/TLS use `TLSConnection::connect()` with `tls` flag for TLS
  4. Obtain local IP:port via `getsockname()` on the connected socket
  5. Generate Call-ID (uuid), branch tag, from-tag
  6. Build and send initial REGISTER (CSeq 1, no auth)
  7. Receive response with remaining time to deadline (`deadline - GetMonotonicClockTime()`)
  8. If 401/407: parse `WWW-Authenticate`/`Proxy-Authenticate`, compute digest, build authenticated REGISTER (CSeq 2, new branch), send
  9. Receive final response with remaining time to deadline
  10. If 200: compute elapsed time from start, set `*status = 200`, return elapsed
  11. On other status: set `*status` to received code, return -1
  12. On timeout (deadline exceeded at any recv): set `*status = 408`, return -1
  13. Clean up: for UDP close socket, for TCP/TLS the `TLSConnection` destructor handles cleanup
- [ ] Preserve all existing debug logging patterns (`nxlog_debug_tag(DEBUG_TAG, ...)`)
- [ ] Verify compilation with `make -C src/agent/subagents/asterisk`

### Task 6: Verify full build and review

- [ ] Run full `make` from project root to verify no linkage issues
- [ ] Review that no eXosip2/osip references remain: `grep -r 'eXosip\|osip' src/agent/subagents/asterisk/`
- [ ] Review that no eXosip2/osip references remain in configure.ac
- [ ] Verify the `RegisterSIPClient()` function signature is unchanged
- [ ] Verify all handlers (`H_SIPTestRegistration`, `H_SIPRegistrationTestData`, etc.) are unchanged
- [ ] Move this plan to `docs/plans/completed/`

## Technical Details

### SIP REGISTER message format
```
REGISTER sip:domain SIP/2.0\r\n
Via: SIP/2.0/<transport> <local-ip>:<local-port>;branch=z9hG4bK-<uuid>\r\n
Max-Forwards: 70\r\n
From: <sip:login@domain>;tag=<uuid>\r\n
To: <sip:login@domain>\r\n
Call-ID: <uuid>@<local-ip>\r\n
CSeq: 1 REGISTER\r\n
Contact: <sip:login@local-ip:local-port;transport=<transport>>\r\n
User-Agent: NetXMS/<version>\r\n
Expires: <timeout>\r\n
Content-Length: 0\r\n
\r\n
```
Local IP and port are obtained via `getsockname()` after connecting the socket.

### Digest auth computation (RFC 2617 / RFC 7616)
```
HA1 = H(login:realm:password)
HA2 = H(method:uri)
response = H(HA1:nonce:nc:cnonce:qop:HA2)    # when qop=auth
response = H(HA1:nonce:HA2)                    # when no qop
```
Where H is MD5 or SHA-256 based on server's `algorithm=` field. Default to MD5 when algorithm is absent.

### Timeout mechanism
The `timeout` parameter (in seconds) serves dual purpose:
1. SIP `Expires` header value in the REGISTER message
2. Overall wall-clock deadline for the operation

Deadline is computed at function entry: `deadline = GetMonotonicClockTime() + timeout * 1000`. Each `recv`/`poll` call uses `max(0, deadline - GetMonotonicClockTime())` as its timeout. If deadline is exceeded, status is set to 408 (Request Timeout).

### Transport selection from URI
| URI | Transport | Default Port |
|-----|-----------|-------------|
| `sip:host` | UDP | 5060 |
| `sip:host;transport=tcp` | TCP | 5060 |
| `sip:host;transport=udp` | UDP | 5060 |
| `sips:host` | TLS | 5061 |

### Transport implementation
| Transport | Connect | Send/Recv | Cleanup |
|-----------|---------|-----------|---------|
| UDP | `ConnectToHostUDP()` | `SendEx()`/`RecvEx()` | `closesocket()` |
| TCP | `TLSConnection::connect(addr, port, false)` | `TLSConnection::send()`/`recv()` | destructor |
| TLS | `TLSConnection::connect(addr, port, true)` | `TLSConnection::send()`/`recv()` | destructor |

## Post-Completion

**Manual verification:**
- Test registration against a real Asterisk instance over UDP
- Test with `sip:host;transport=tcp` if TCP is available
- Test with `sips:host` if TLS is configured on Asterisk
- Verify `Asterisk.SIP.TestRegistration(*)` parameter returns correct elapsed time
- Verify `SIPRegistrationTest` periodic tests report correct status codes
- Test with invalid credentials to confirm 401/403 status propagation
- Test with unreachable proxy to confirm timeout behavior (status 408)
