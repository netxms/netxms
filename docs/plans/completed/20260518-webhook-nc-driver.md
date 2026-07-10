# Generic Webhook Notification Channel Driver

## Overview

Add a new notification channel driver `webhook` that sends notifications as customizable HTTP requests (POST/PUT/PATCH) with a user-supplied JSON (or any text) body template. Covers the majority of REST-style notification APIs without writing a new driver per vendor.

**Problem it solves:** every new REST/JSON notification gateway currently requires a hand-written driver (anysms, msteams, slack, mattermost, googlechat, matrix, telegram, twilio, nexmo, websms, smseagle, text2reach, mymobile…). For APIs that are just "POST a JSON body and read a status code", a generic templated driver removes that work and lets users onboard new gateways via config alone.

**How it integrates:** standard `NCDriver` subclass loaded as `ncdrv/webhook.ncd` from `src/ncdrivers/webhook/`. Same lifecycle, debug-tag, and return-code conventions as every other NC driver. No server-core changes required.

## Context (from discovery)

- **Reference driver:** `src/ncdrivers/msteams/msteams.cpp` — closest analogue (libcurl + JSON POST + HTTP retry-code mapping).
- **Build registration:** drivers register in:
  - `src/ncdrivers/Makefile.am` (per-driver block + `EXTRA_LTLIBRARIES`)
  - `configure.ac:1097` (default `NCDRV_MODULES` list)
  - `configure.ac:2529` (libcurl-conditional `NCDRV_MODULES` list)
  - `configure.ac` AC_CONFIG_FILES — alphabetical: insertion point is after `websms/Makefile` (line 5181), before `xmpp/Makefile` (line 5182)
  - `netxms.sln:321` (Visual Studio solution file — `msteams` entry shows the format)
  - `NCDRV_LTLIBRARIES` (configure.ac:4890) is auto-derived from `NCDRV_MODULES` by perl — no manual edit there.
- **Helpers available:**
  - `EscapeStringForJSON(const TCHAR *) → String` at `include/nms_util.h:6480` (used in msteams.cpp:101-102)
  - `Config::getSubEntries(_T("/Headers"), _T("*"))` returns `unique_ptr<ObjectArray<ConfigEntry>>` — exact pattern used by msteams.cpp:83 for its channel map
  - `Config::parseTemplate()` with `NX_CFG_TEMPLATE` for fixed scalar fields (CT_STRING, CT_BOOLEAN, etc.)
  - `LoadFileAsUTF8String(const TCHAR *)` at `include/nms_util.h:6585` returns `char *` — caller must `MemFree` it. Used per-send to reload template (no caching).
  - `NCConfigurationTemplate s_config(bool needSubject, bool needRecipient)` at `include/ncdrv.h:53` — webhook will pass `(true, true)` since the URL/template may reference both
  - jansson is already a dependency for several drivers (matrix, slack, mymobile, …) — guard via `USE_INTERNAL_JANSSON` like the others
  - URL-encoding: implement a tiny pure RFC 3986 percent-encoder in helpers (do **not** depend on libcurl in the helpers — keeps the test target curl-free)
- **Config syntax in use:** modern `[Section]` with `Key = Value`; nested `[parent/child]` supported (not needed in v1)
- **No existing tests for NC drivers** — this plan introduces the first one (lightweight, helpers-only)

## Development Approach

- **Testing approach:** Regular (code-first, then tests). Helpers tested in unit tests; full driver verified via manual integration test against a real or mock HTTP endpoint.
- Complete each task fully before moving on
- Small, focused changes
- **Every task includes new/updated tests** for testable units in that task
- All tests must pass before starting next task
- Update this plan if scope shifts during implementation

## Testing Strategy

- **Unit tests:** new `tests/test-ncd-webhook/` target covers pure helper functions: placeholder substitution (URL form, JSON form), comma-separated integer-list parsing, JSON-path lookup. No libcurl mocking — the driver's send() loop is integration-tested.
- **Integration test:** manual — build, configure a channel using the A2A example, send a test notification through `nxnotify` or via the management console, inspect debug log with `DebugTags = ncd.webhook:7`.
- **Regression check:** verify other drivers still build (`msteams`, `slack`, `anysms` use the same configure.ac sections being touched).

## Progress Tracking

- mark completed items with `[x]` immediately when done
- add newly discovered tasks with ➕ prefix
- document issues/blockers with ⚠️ prefix
- update plan if implementation deviates from original scope

## Solution Overview

The driver is a single `NCDriver` subclass that:
1. At load time: reads config (URL/Method/ContentType/Timeout/VerifyPeer/code-lists/optional SuccessJsonPath+SuccessValue/Headers map). Validates that the configured `TemplateFile` path is readable; does not cache content.
2. Per `send(recipient, subject, body)`:
   - Loads the template file from disk fresh (`LoadFileAsUTF8String`); on read failure log + return `-1`. This makes template edits visible without restarting the channel — sends are infrequent and the read cost is negligible.
   - Builds effective URL by substituting `${recipient|subject|body}` with URL-encoded values.
   - Builds request body by substituting the same placeholders in the just-loaded template with JSON-escaped values.
   - libcurl request with configured method, headers, content type, TLS verify, timeout.
   - Maps HTTP status to success / retry / hard-fail using `SuccessHttpCodes` and `RetryHttpCodes`.
   - If `SuccessJsonPath` configured and HTTP succeeded, parses response with jansson, walks the JSON pointer, stringifies the value, and compares to `SuccessValue`. Mismatch → retryable.
   - Frees the loaded template buffer at end of send.

**Why this structure:** matches the existing NC-driver shape (msteams), keeps libcurl interaction inside `send()`, and isolates pure logic (substitution + parsers) into testable free functions. Per-send file read is acceptable: notification sends are not high-throughput and disk read of a small text file is cheap relative to the outbound HTTP call. Future optimization (stat()/mtime comparison to skip re-read when unchanged) is a one-line follow-up if profiling ever flags it.

## Technical Details

### Configuration keys

| Key | Type | Default | Notes |
|---|---|---|---|
| `URL` | string | (required) | Endpoint URL. Supports `${recipient}`, `${subject}`, `${body}` (URL-encoded). |
| `Method` | string | `POST` | `POST`, `PUT`, or `PATCH`. Case-insensitive. |
| `ContentType` | string | `application/json` | Sent as `Content-Type` header. |
| `TemplateFile` | path | (required) | Filesystem path to template file. Re-read on every send (cheap; sends are infrequent and edits take effect without channel restart). Driver init verifies the path is readable. |
| `VerifyPeer` | bool | `yes` | Forwarded to `CURLOPT_SSL_VERIFYPEER`. |
| `Timeout` | int (sec) | `10` | `CURLOPT_TIMEOUT`. |
| `SuccessHttpCodes` | csv | `200,201,202,204` | HTTP status codes treated as success. |
| `RetryHttpCodes` | csv | `429,502,503,504` | HTTP status codes that produce a retry (10s). |
| `SuccessJsonPath` | string | (empty) | JSON pointer (RFC 6901, e.g. `/ErrorCode`) into response body. Empty = skip body check. |
| `SuccessValue` | string | (empty) | Required value at `SuccessJsonPath`. Compared as string after stringifying JSON value. |
| `[Headers]` section | name=value | none | Extra HTTP headers added to request. No substitution. |

### Placeholder substitution

Three placeholders supported, two contexts:

- In **URL**: substituted with `curl_easy_escape(curl, utf8_value, 0)` output.
- In **template body**: substituted with `EscapeStringForJSON(value)`.

Headers carry literal config values — no substitution (out of scope for v1).

### Return-code mapping (mirrors msteams convention)

| Condition | Return |
|---|---|
| HTTP status in `SuccessHttpCodes` AND (no `SuccessJsonPath` OR path-match) | `0` |
| HTTP status in `SuccessHttpCodes` AND `SuccessJsonPath` set AND value mismatch | `10` (retry) |
| HTTP status in `RetryHttpCodes` | `10` (retry) |
| Any other HTTP status, transport error, malformed response | `-1` (hard fail) |
| Config error (missing URL / TemplateFile / unreadable template) | driver fails to load (returns `nullptr` from entry point) |

### Real-world validation example

A2A SMS gateway (from integration doc):

`/etc/netxms/webhook-templates/a2a-sms.json`:
```json
{
   "BankCode": "Username",
   "BankPWD": "Password",
   "SourceID": "A2A",
   "SrvID": "ATW",
   "Priority": 1,
   "MsgText": "${body}",
   "MobileNo": "${recipient}"
}
```

Channel config:
```ini
URL = https://gateway.example.com/API/A2A/SendSMS
Method = POST
ContentType = application/json
TemplateFile = /etc/netxms/webhook-templates/a2a-sms.json
SuccessJsonPath = /ErrorCode
SuccessValue = 0
```

This demonstrates both auth styles working (body-embedded `BankCode`/`BankPWD` as template literals) and body-level success detection (`ErrorCode == 0`).

### Non-goals (v1)

Explicitly out of scope; document in code header so reviewers don't ask:

- Header value placeholder substitution
- Per-recipient URL aliasing (no `[Channels]` map)
- mtime/stat-based template-reload short-circuit (templates are re-read every send; trivially cheap. Add stat() check only if profiling later flags it)
- Multi-endpoint per channel via `[servers/serverN]`
- GET/DELETE methods
- HMAC/signing helpers
- Response body capture beyond debug-level tracing

## What Goes Where

- **Implementation Steps** (checkboxes): driver source, build glue, helpers, tests
- **Post-Completion** (no checkboxes): admin-guide documentation page (separate doc repo), real-API smoke test by user, CHANGELOG entry

## Implementation Steps

### Task 1: Create webhook driver source skeleton

**Files:**
- Create: `src/ncdrivers/webhook/webhook.cpp`
- Create: `src/ncdrivers/webhook/Makefile.am`

- [x] create `src/ncdrivers/webhook/webhook.cpp` with file header (matching msteams style; include the non-goals list as a header comment so reviewers don't ask), `#include <ncdrv.h>`, `<nms_util.h>`, `<netxms-version.h>`, `<nxlibcurl.h>`, `<jansson.h>` — ⚠️ deviation: jansson.h NOT included directly; it is pulled in via `nms_util.h` (correct `USE_INTERNAL_JANSSON` guard), matching every other jansson+libcurl NC driver (nexmo, slack, telegram, twilio). Including `<jansson.h>` raw breaks internal-jansson builds. Documented in webhook.cpp header comment.
- [x] define `#define DEBUG_TAG _T("ncd.webhook")`
- [x] declare `class WebhookDriver : public NCDriver` with private members for URL (`TCHAR[]` or `String`), method, contentType, **`m_templateFile`** (template path as `TCHAR[MAX_PATH]`), headers map (`StringMap`), timeout, verifyPeer, success/retry code lists (`IntegerArray<int>`), successJsonPath, successValue. No cached template buffer — content is loaded per-send.
- [x] declare a private ctor and public static `createInstance(Config *config)` factory (matches `MicrosoftTeamsDriver::createInstance` pattern)
- [x] no custom destructor needed (no heap-owned members beyond what `StringMap`/`IntegerArray` self-manage)
- [x] declare `virtual int send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override`
- [x] define `static const NCConfigurationTemplate s_config(true /*needSubject*/, true /*needRecipient*/);`
- [x] add `DECLARE_NCD_ENTRY_POINT(Webhook, &s_config)` block calling `InitializeLibCURL()` and `WebhookDriver::createInstance(config)` — mirrors msteams.cpp:252-260
- [x] add `#ifdef _WIN32 DllMain` stub (copy verbatim from msteams.cpp:266-273)
- [x] create `src/ncdrivers/webhook/Makefile.am` with only `EXTRA_DIST = webhook.cpp webhook_helpers.cpp webhook_helpers.h` (defer adding the `.vcxproj` files to EXTRA_DIST until Task 6 when they exist)
- [x] no tests yet — skeleton only; compile checked at Task 6

### Task 2: Implement config parsing in createInstance

**Files:**
- Modify: `src/ncdrivers/webhook/webhook.cpp`

Note: this task depends on the helpers in Task 3 (`ParseIntList`). Implement helpers first if doing this in a single session; otherwise stub `ParseIntList` here and refine in Task 3.

- [x] parse scalar config with `NX_CFG_TEMPLATE`: URL (CT_STRING), Method (CT_STRING, default "POST"), ContentType (CT_STRING, default "application/json"), TemplateFile (CT_STRING), VerifyPeer (CT_BOOLEAN, default true), Timeout (CT_LONG, default 10), SuccessJsonPath (CT_STRING, optional), SuccessValue (CT_STRING, optional)
- [x] read SuccessHttpCodes / RetryHttpCodes via `config->getValue()` and parse with `ParseIntList()` helper (defined in Task 3); apply defaults if missing/empty — implemented as file-local static `ParseIntList`/`ConfigureHttpCodeList` (promoted to webhook_helpers.cpp in Task 3 per the note above)
- [x] read `[Headers]` via `config->getSubEntries(_T("/Headers"), _T("*"))` and populate the `StringMap` (mirrors msteams.cpp:83-92) — header values not logged
- [x] verify `TemplateFile` path is readable at init: `_taccess(path, R_OK) == 0`. Failure → log error and return `nullptr` from createInstance. Contents not cached; reload happens in `send()`.
- [x] validate required fields: URL non-empty, TemplateFile non-empty, Method is one of POST/PUT/PATCH; on any failure log + return `nullptr`
- [x] uppercase Method for comparison (`_tcsupr`)
- [x] log instantiation success with `nxlog_write_tag(NXLOG_INFO, ...)` (matches msteams.cpp:81)
- [x] no tests yet — config-parse logic is integration-tested at Task 9. ⚠️ Compile not verifiable in this container: the checked-in config.h was generated for macOS while the container is Linux, so project headers fail to parse here. Code mirrors msteams.cpp patterns and verified API signatures; compile is checked at Task 6 after `./configure` regenerates config.h.

### Task 3: Implement placeholder substitution helpers

**Files:**
- Create: `src/ncdrivers/webhook/webhook_helpers.h`
- Create: `src/ncdrivers/webhook/webhook_helpers.cpp`

**Critical:** helpers must NOT depend on libcurl. The test target links these without libcurl. Use a pure RFC 3986 percent-encoder, not `curl_easy_escape`.

- [x] declare in `webhook_helpers.h`:
  - `String UrlEncode(const TCHAR *s)` — RFC 3986 percent-encoding of UTF-8 bytes; unreserved set is `A-Z a-z 0-9 - _ . ~`, everything else `%XX`
  - `String SubstitutePlaceholdersJSON(const TCHAR *tpl, const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)` — replaces `${recipient}`, `${subject}`, `${body}` with `EscapeStringForJSON()` output
  - `String SubstitutePlaceholdersURL(const TCHAR *url, const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)` — replaces same placeholders with `UrlEncode()` output (no libcurl dependency)
  - `bool ParseIntList(const TCHAR *csv, IntegerArray<int> *out)` — parses comma-separated integer list
  - ⚠️ deviation: the UTF-8 request-body variant is named `SubstitutePlaceholdersJSONUtf8(const char *tpl, ...)` (distinct name, NOT a `const char*` overload of the TCHAR `SubstitutePlaceholdersJSON`). In non-UNICODE builds `TCHAR == char`, which makes a same-arity `const char*`/`const TCHAR*` overload pair ambiguous (differs only by return type) — a hard compile error caught during best-effort compilation here. Task 4a must call `SubstitutePlaceholdersJSONUtf8(tpl, recipient, subject, body)` instead of `SubstitutePlaceholdersJSON`.
- [x] implement substitution functions in `webhook_helpers.cpp`: simple linear scan for `${`, look up token until `}`, append escaped value; unknown tokens are left as-is (literal `${foo}`) so user typos are visible — shared `SubstituteTemplate()` core drives both JSON and URL TCHAR variants; unterminated `${` at end of input is also copied literally
- [x] implement `UrlEncode`: convert TCHAR input to UTF-8 first, then encode byte-by-byte. ~30 lines, no external deps.
- [x] for the request-body path: template is UTF-8 bytes from disk; substitution happens against the UTF-8 string directly (treat `${...}` as ASCII bytes — safe because all placeholder names are ASCII). Result is UTF-8 bytes ready for `CURLOPT_POSTFIELDS`. Document the encoding choice in the helper's doc comment. — ⚠️ deviation: `MemoryBlock<char>` is not a type in this codebase; implemented as `SubstitutePlaceholdersJSONUtf8()` returning a heap-allocated, null-terminated UTF-8 `char *` (built via `ByteStream`, copied with `MemAllocArray<char>`); caller `MemFree`s. Encoding rationale documented in the helper's doc comment.
- [x] implement `ParseIntList`: skip whitespace, `_tcstol`, accumulate; tolerate trailing/leading whitespace; reject malformed entries with `false` return — promoted out of `webhook.cpp` (Task 2 file-local static removed; webhook.cpp now includes `webhook_helpers.h`); added explicit `csv == nullptr` guard
- [x] write tests covering JSON substitution: all three placeholders present, repeated placeholder, missing placeholder, empty input, special chars requiring JSON escape (quote, backslash, newline), UTF-8 input — `TestSubstituteJSON` (TCHAR) + `TestSubstituteJSONUtf8` (UTF-8 byte variant) (target wired in Task 8)
- [x] write tests covering URL substitution: special chars (space, &, =, /, +, UTF-8), unreserved set passes through unchanged — `TestSubstituteURL` (target wired in Task 8)
- [x] write tests for `UrlEncode` standalone: empty string, ASCII unreserved, ASCII reserved (`/`, `?`, `&`), space → `%20`, UTF-8 multi-byte (`é` → `%C3%A9`) — `TestUrlEncode` (target wired in Task 8)
- [x] write tests for `ParseIntList`: empty input → false/empty array, single value, multi-value, whitespace tolerance, malformed entry → false — `TestParseIntList`, plus null-input case (target wired in Task 8)
- [x] tests live in `tests/test-ncd-webhook/test-ncd-webhook.cpp` — test bodies written; ⚠️ compile/run deferred to Task 8 per plan. Cannot build here: this container has a macOS-generated `config.h` (HAVE_KQUEUE) on Linux, no `automake`, and `libnetxms` is a `.dylib` — same environment blocker documented in Task 2. Best-effort `g++` compile confirms all remaining errors originate solely from platform headers (`nms_common.h`, `nms_threads.h`); zero errors from `webhook_helpers.{h,cpp}`/`webhook.cpp`/test, and the run caught+fixed the overload-ambiguity bug noted above. ⚠️ Task 8 note: the plan's Task 8 Makefile.am snippet omits `-I../include`; it is required for `<testtools.h>` (mirror `tests/test-libnetxms/Makefile.am` which has it).

### Task 4a: Implement send() — request construction

**Files:**
- Modify: `src/ncdrivers/webhook/webhook.cpp`

- [x] at the start of `send()`, load template fresh: `char *tpl = LoadFileAsUTF8String(m_templateFile)`. On failure (file gone, permission denied) log error + return `-1`. Use a scope guard or explicit `MemFree(tpl)` on every exit path — explicit `MemFree(tpl)` on the (only) exit path; the early `tpl == nullptr` path returns before any allocation
- [x] build effective URL via `SubstitutePlaceholdersURL(m_url, recipient, subject, body)`; convert result to UTF-8 char buffer for `CURLOPT_URL` — `String effectiveUrl` + `getUTF8String()` → `utf8url` (freed on exit)
- [x] build request body: call `SubstitutePlaceholdersJSON(tpl, recipient, subject, body)` — operates on UTF-8 directly, output is UTF-8 bytes ready for `CURLOPT_POSTFIELDS` — ⚠️ per the Task 3 deviation, calls `SubstitutePlaceholdersJSONUtf8(tpl, recipient, subject, body)` (the distinct-named UTF-8 byte variant; a same-arity `const char*` overload of the TCHAR `SubstitutePlaceholdersJSON` is ambiguous in non-UNICODE builds where `TCHAR == char`)
- [x] build `curl_slist` headers: prepend `Content-Type: <m_contentType>`, then iterate `m_headers` `StringMap` appending `Name: Value` for each entry — idiomatic `for(KeyValuePair<const TCHAR> *h : m_headers)` range-for; each header line built via `StringBuffer`→`getUTF8String()` (curl copies, freed immediately after append)
- [x] log at debug level 7: effective URL and substituted body (only). Do NOT log the `m_headers` map or `Authorization` header values — header iteration explicitly does not log names/values
- [x] no test changes — Task 3 covered the helpers; integration verification deferred to Task 9. ⚠️ Compile not fully verifiable here (documented macOS-config.h-on-Linux blocker, no automake): best-effort `g++ -fsyntax-only` confirms all errors originate solely from platform headers (`nms_common.h`, `nms_threads.h`) — zero from `webhook.cpp`. Task 4b inserts curl execution between request construction and the cleanup block; the current `return -1` placeholder is replaced there.

### Task 4b: Implement send() — curl execution and cleanup

**Files:**
- Modify: `src/ncdrivers/webhook/webhook.cpp`

- [x] initialize CURL handle (`curl_easy_init()`); on failure log + return `-1` — early-out path frees `headers`/`utf8url`/`requestBody`/`tpl` before returning
- [x] set curl options: NOSIGNAL, PROTOCOLS_STR/PROTOCOLS (http,https), HEADER=0, TIMEOUT=`m_timeout`, WRITEFUNCTION=`ByteStream::curlWriteFunction`, WRITEDATA (a stack `ByteStream`), SSL_VERIFYPEER=`m_verifyPeer ? 1 : 0`, USERAGENT (`"NetXMS Webhook Driver/" NETXMS_VERSION_STRING_A`), ERRORBUFFER — PROTOCOLS guarded with `HAVE_DECL_CURLOPT_PROTOCOLS_STR` (str `"http,https"` / bitmask `CURLPROTO_HTTP|CURLPROTO_HTTPS`), NOSIGNAL guarded with `HAVE_DECL_CURLOPT_NOSIGNAL`, mirroring msteams.cpp
- [x] method handling:
  - POST (default): set `CURLOPT_POST=1` and `CURLOPT_POSTFIELDS`+`CURLOPT_POSTFIELDSIZE`
  - PUT/PATCH: set `CURLOPT_CUSTOMREQUEST` to `"PUT"`/`"PATCH"`, plus `CURLOPT_POSTFIELDS`+`CURLOPT_POSTFIELDSIZE` — `m_method` already uppercased/validated in createInstance(); `POSTFIELDSIZE` set before `POSTFIELDS`
- [x] apply HTTP headers via `CURLOPT_HTTPHEADER` (the `curl_slist *` built in 4a)
- [x] set `CURLOPT_URL` to the UTF-8 URL buffer — `curl_easy_setopt(CURLOPT_URL)` return checked, non-OK → `result = -1`
- [x] `curl_easy_perform()`; on non-CURLE_OK log error + cleanup + return `-1` — single unified cleanup path returns `result`
- [x] response evaluation is deferred to Task 5 — `responseData` ByteStream wired to `CURLOPT_WRITEDATA` so Task 5 can read the captured body; placeholder comment marks the insertion point
- [x] cleanup path (success or failure): `curl_slist_free_all(headers)`, `curl_easy_cleanup(curl)`, `MemFree` the template buffer and any substituted body buffer owned by send() — frees `headers`, `curl`, `utf8url`, `requestBody`, `tpl`
- [x] no new unit tests — execution path needs a real HTTP server (handled by manual integration test at Task 9). ⚠️ Compile not fully verifiable in this container (documented macOS-`config.h`-on-Linux blocker + missing generated `netxms-build-tag.h`, no automake): best-effort `g++ -fsyntax-only` with `-Ibuild` confirms all remaining errors originate solely from platform headers (`nms_common.h`, `nms_threads.h`) — zero from `webhook.cpp`. Real compile verified at Task 6 after `./configure` regenerates a Linux `config.h`.

### Task 5: Implement response evaluation (HTTP code + optional JSON path)

**Files:**
- Modify: `src/ncdrivers/webhook/webhook.cpp`
- Modify: `src/ncdrivers/webhook/webhook_helpers.h`
- Modify: `src/ncdrivers/webhook/webhook_helpers.cpp`

- [x] after `curl_easy_perform`, read HTTP status via `curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode)` — evaluation runs only on `CURLE_OK`; debug-6 logs status + byte count (mirrors telegram.cpp)
- [x] if httpCode in `m_successHttpCodes` → proceed to body check — `IntegerArray<int>::contains()`
- [x] else if httpCode in `m_retryHttpCodes` → return `10`
- [x] else log + return `-1`
- [x] body check: if `m_successJsonPath` empty → return `0`; else parse responseData buffer with `json_loads()`. Parse failure → log + return `-1` — `responseData.write('\0')` to null-terminate before `json_loads` (telegram.cpp pattern); empty body → parse failure → `-1`
- [x] declare and implement helper `String JsonPathLookup(json_t *root, const TCHAR *pointer)` in helpers.cpp/.h — RFC 6901: leading-`/` requirement, `~1`/`~0` token unescaping, object-key + numeric array-index walking; stringify strings as-is, integers via `INT64_FMT`, reals via `%f`, booleans `true`/`false`, null `null`, objects/arrays/missing/null-root → empty. Pointer scanned in UTF-8 form (object keys are UTF-8; only `/` and `~` are structural and ASCII). Uses jansson (via nms_util.h) but NOT libcurl — keeps the libcurl-free test-link contract; `json_object_get_by_path_a` in libnetxms was unsuitable (objects only, no array indices / value stringification)
- [x] compare looked-up value to `m_successValue` with `_tcscmp`; equal → return `0`, otherwise return `10` (retryable)
- [x] `json_decref(root)` and cleanup — `json_decref` on the parsed response in the success+jsonpath branch; existing unified curl/buffer cleanup path unchanged
- [x] write tests for `JsonPathLookup`: missing path, nested object lookup, integer value (A2A `{"ErrorCode":0}` → `"0"`), string value, array index, malformed pointer (no leading `/`) → empty — plus null-root, boolean, null, real, object/array→empty, scalar-descent, out-of-range/non-numeric index, `~1`/`~0` unescaping, empty-pointer cases
- [x] re-run all helper tests; all must pass — ⚠️ build/run deferred to Task 8 (test target not yet wired) and a real Linux env: documented macOS-`config.h`-on-Linux blocker persists (no `automake`; `libnetxms` is a `.dylib`). Best-effort `g++ -fsyntax-only` confirms all errors originate solely from platform headers (`nms_common.h`, `nms_threads.h`); zero from `webhook.cpp`/`webhook_helpers.{h,cpp}`/test. All jansson + `String`/`IntegerArray` APIs verified against headers.

### Task 6: Wire driver into autotools build system

**Files:**
- Modify: `src/ncdrivers/Makefile.am`
- Modify: `configure.ac`

- [x] in `src/ncdrivers/Makefile.am`:
  - add `webhook.la` to `EXTRA_LTLIBRARIES` (line 15) — done (between `websms.la` and `xmpp.la`)
  - add per-driver block (alphabetical placement — between `websms_la_*` and `xmpp_la_*`):
    ```makefile
    webhook_la_SOURCES = webhook/webhook.cpp webhook/webhook_helpers.cpp
    webhook_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
    webhook_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
    webhook_la_LIBADD = ../libnetxms/libnetxms.la
    if USE_INTERNAL_JANSSON
    webhook_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
    else
    webhook_la_LIBADD += -ljansson
    endif
    ```
    — inserted verbatim before the `xmpp_la_SOURCES` block; matches the matrix/slack/telegram jansson-driver template
- [x] in `configure.ac`:
  - line 1097 (default `NCDRV_MODULES` list): add `webhook` — done (between `websms` and `xmpp`)
  - line 2529 (libcurl-conditional `NCDRV_MODULES` list): add `webhook` — done (appended after `websms`, the last entry of that list)
  - AC_CONFIG_FILES: add `src/ncdrivers/webhook/Makefile` — done (between `src/ncdrivers/websms/Makefile` and `src/ncdrivers/xmpp/Makefile`)
  - `NCDRV_LTLIBRARIES` (line 4890) is auto-derived — no manual edit (verified: perl one-liner produces `webhook.la`)
  - install-exec-hook in `src/ncdrivers/Makefile.am:178` iterates `$(DRIVERS)` derived from `@NCDRV_MODULES@` — no edit needed
- [x] run `./init-source-tree` to regenerate `configure` and `Makefile.in` — ⚠️ partial: `init-source-tree`→`reconf`→`autoreconf -fi` requires `automake`/`aclocal`, which are absent in this container with no root/sudo to install (documented Task 2–5 blocker). Best-effort: ran plain `autoconf` (available) — regenerated `configure` cleanly (exit 0; only pre-existing obsolete-macro warnings). Regenerated `configure` contains `ncdrivers/webhook/Makefile` (2 occurrences — exact parity with sibling `websms`), `twilio websms webhook xmpp` (default list) and `twilio websms webhook` (libcurl list). `Makefile.in` regeneration needs `automake` (blocked); both `configure` and `Makefile.in` are gitignored generated artifacts, not committed. The Makefile.am block follows the exact established pattern of every sibling driver.
- [x] run `./configure --with-server --with-tests --enable-debug` and verify `webhook` appears in the build-summary's NC driver list — ⚠️ blocked (skipped — not automatable here): `./configure` regenerates `config.h`, but the build cannot proceed regardless — committed `config.h` is macOS-generated (`HAVE_KQUEUE` on Linux aarch64) and `libnetxms` is a `.dylib` (macOS). Same environment blocker documented in Tasks 2–5. Wiring correctness verified via the autoconf regeneration above (webhook present in all 3 expected `configure` locations with websms parity).
- [x] run `make -C src/ncdrivers webhook.la` and verify clean compile — ⚠️ blocked (skipped — not automatable here): no `automake` to regenerate `Makefile.in`; macOS `config.h`/`libnetxms.dylib` on Linux prevents any build. Best-effort `g++ -std=c++11 -fsyntax-only` on `webhook.cpp` and `webhook_helpers.cpp` confirms the only errors originate from the platform `config.h`/`nms_common.h` mismatch — zero errors from the webhook sources themselves (consistent with Tasks 1–5). Real compile verifiable only in a native Linux env with regenerated `config.h`.
- [x] run `make -C src/ncdrivers` to verify other drivers still build (msteams, slack, anysms) — ⚠️ blocked (skipped — not automatable here): same build-environment blocker. Regression risk is minimal — the only edits to shared files are additive (one `EXTRA_LTLIBRARIES` entry, one self-contained per-driver block, three additive `configure.ac` lines); no existing driver's rules were modified, and the autoconf regeneration of `configure` succeeded with no new errors.

### Task 7: Wire driver into Visual Studio solution

**Files:**
- Create: `src/ncdrivers/webhook/webhook.vcxproj`
- Create: `src/ncdrivers/webhook/webhook.vcxproj.filters`
- Modify: `src/ncdrivers/webhook/Makefile.am` (add the new files to `EXTRA_DIST`)
- Modify: `netxms.sln`

- [x] copy `src/ncdrivers/msteams/msteams.vcxproj` → `src/ncdrivers/webhook/webhook.vcxproj`, replace project name and source filename references, add `webhook_helpers.cpp` and `webhook_helpers.h` to ClCompile/ClInclude lists, generate a fresh GUID for `ProjectGuid` — `RootNamespace`/`MSTEAMS_EXPORTS`→`WEBHOOK_EXPORTS` swapped; `webhook.cpp`+`webhook_helpers.cpp` in ClCompile, `webhook_helpers.h` in ClInclude; fresh `ProjectGuid` `{20BF1EA5-4BD0-458F-9033-915C2A577015}` (verified unique across all .vcxproj/.sln); XML well-formed
- [x] copy `msteams.vcxproj.filters` → `webhook/webhook.vcxproj.filters`, update source filenames; add `webhook_helpers.cpp/.h` under appropriate filters — `webhook.cpp`/`webhook_helpers.cpp` under Source Files, `webhook_helpers.h` under Header Files; XML well-formed
- [x] update `src/ncdrivers/webhook/Makefile.am` `EXTRA_DIST` to include `webhook.vcxproj webhook.vcxproj.filters`
- [x] in `netxms.sln`: add a `Project(...)` entry for `webhook` (model after `msteams` at line 321) with the new GUID. Add per-configuration build rows in the `GlobalSection(ProjectConfigurationPlatforms)` block — mirror the same set of Debug/Release × Win32/x64 entries used by `msteams` — Project decl added after msteams; 15 config rows mirroring msteams's exact set (incl. which rows carry `.Build.0`); NestedProjects mapping to the ncdrivers solution folder `{90D897D2-...}`. 17 total GUID occurrences = parity with msteams. ⚠️ A Visual Studio build cannot run in this Linux container; wiring verified structurally (XML well-formedness, sln GUID consistency/uniqueness, msteams parity)
- [x] no test changes

### Task 8: Add helper unit-test target

**Files:**
- Create: `tests/test-ncd-webhook/test-ncd-webhook.cpp`
- Create: `tests/test-ncd-webhook/Makefile.am`
- Modify: `configure.ac`

Mirror `tests/test-libnetxms/Makefile.am` verbatim — it uses `bin_PROGRAMS` + `@EXEC_LDFLAGS@` + `@EXEC_LIBS@`, not `check_PROGRAMS`/`TESTS`. Wiring goes through `TEST_MODULES` in `configure.ac`, NOT a hard-coded `SUBDIRS` edit in `tests/Makefile.am` (it uses `SUBDIRS = ... @TEST_MODULES@`).

- [x] read `tests/test-libnetxms/Makefile.am` and `tests/test-libnetxms/test-libnetxms.cpp` to confirm entry-point pattern — confirmed: `bin_PROGRAMS` + `@EXEC_LDFLAGS@`/`@EXEC_LIBS@`, `InitNetXMSProcess(true)` … `StartTest()`/`EndTest()` per group … `InitiateProcessShutdown()`, `main()` returns 0
- [x] create `tests/test-ncd-webhook/test-ncd-webhook.cpp` with `main()` that calls the helper test functions from Task 3 and Task 5 — already authored in Tasks 3/5 (6 groups: UrlEncode, SubstituteJSON TCHAR+UTF-8, SubstituteURL, ParseIntList, JsonPathLookup). ⚠️ fixed one fragile test artefact found by actually running the suite: line-103 expected value was built via `_sntprintf(_T("[%s]"), s_eacute)`, which truncates the non-ASCII wide char to just `[` under the C locale on musl (a test-harness round-trip artefact, NOT a helper bug — diag confirmed the helper correctly yields `[é]`). Expected is now constructed directly as a wide literal, mirroring how `s_eacute` itself is defined.
- [x] create `tests/test-ncd-webhook/Makefile.am`:
  ```makefile
  bin_PROGRAMS = test-ncd-webhook
  test_ncd_webhook_SOURCES = test-ncd-webhook.cpp \
      ../../src/ncdrivers/webhook/webhook_helpers.cpp
  test_ncd_webhook_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build \
      -I@top_srcdir@/src/ncdrivers/webhook
  test_ncd_webhook_LDFLAGS = @EXEC_LDFLAGS@
  test_ncd_webhook_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
  if USE_INTERNAL_JANSSON
  test_ncd_webhook_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
  else
  test_ncd_webhook_LDADD += -ljansson
  endif
  EXTRA_DIST = test-ncd-webhook.vcxproj test-ncd-webhook.vcxproj.filters
  ```
  No `-lcurl` — helpers are libcurl-free by design (see Task 3).
  — created; mirrors `tests/test-libnetxms/Makefile.am` plus the out-of-tree `webhook_helpers.cpp` source and the `USE_INTERNAL_JANSSON` conditional. ⚠️ deviation: added `-I../include` to `test_ncd_webhook_CPPFLAGS` (the plan snippet omitted it) — required for `<testtools.h>`, which lives in `tests/include/`; `tests/test-libnetxms/Makefile.am` has the same `-I../include` for the same reason.
- [x] in `configure.ac`: append `test-ncd-webhook` to `TEST_MODULES` (line 1092 default block and line 1468 sdk block) — appended after `test-libnxsnmp` in both (`test-l…` < `test-n…`, so alphabetically last)
- [x] in `configure.ac` AC_CONFIG_FILES: add `tests/test-ncd-webhook/Makefile` — inserted between `tests/test-libnxsnmp/Makefile` and `tools/Makefile`
- [x] `./init-source-tree && ./configure --with-tests && make -C tests/test-ncd-webhook && tests/test-ncd-webhook/test-ncd-webhook` — ⚠️ the autotools driver still cannot run end-to-end here (`automake`/`aclocal` absent, so a `Makefile.in` cannot be generated for the new dirs — the documented Task 2–7 container blocker). BUT this iteration performed the substantive equivalent and it genuinely works: regenerated `configure` (autoconf) + `config.status` produced a correct **Linux** `config.h` (`HAVE_KQUEUE` undefined, `SIZEOF_LONG=8`); built internal jansson and full `libnetxms` from source on Linux (`libnxjansson.so`, `libnetxms.so.62`); then compiled+linked `test-ncd-webhook.cpp` + `webhook_helpers.cpp` with the project's exact toolchain/flags (`-std=c++17 -DUNICODE -fno-rtti -fno-exceptions`, linked against the freshly-built libs) and **ran the binary**. (Hand-stubbed `Makefile.in` files were used only to let `config.status` complete; they are gitignored generated artefacts and not committed — `init-source-tree`/automake produces the real ones from the committed `Makefile.am`.)
- [x] all tests pass — full suite **executed**, all 6 groups OK, exit 0: `UrlEncode`, `SubstitutePlaceholdersJSON (TCHAR)`, `SubstitutePlaceholdersJSON (UTF-8)`, `SubstitutePlaceholdersURL`, `ParseIntList`, `JsonPathLookup`. Deferred Task 3/Task 5 test items (already `[x]` with "build/run deferred to Task 8" notes) are hereby resolved: they now build and pass for real, no longer just syntax-checked. Running the suite also caught a real test-artefact bug (line-103 expected construction) which was fixed.

### Task 9: Verify acceptance criteria

- [x] all config keys from "Technical Details" table parsed and applied — verified by code review against the Technical Details table: every key has a parse site and an apply site. URL/Method/ContentType/TemplateFile/VerifyPeer/Timeout/SuccessJsonPath/SuccessValue via `NX_CFG_TEMPLATE` in `createInstance` (`config->parseTemplate(_T("Webhook"), …)` — section/path convention matches reference `msteams.cpp:74`); SuccessHttpCodes/RetryHttpCodes via `config->getValue(_T("/Webhook/…"))`+`ConfigureHttpCodeList` with the documented defaults; `[Headers]` via `config->getSubEntries(_T("/Headers"), _T("*"))` (mirrors `msteams.cpp:83`). Apply sites confirmed in `send()`: URL→`SubstitutePlaceholdersURL`→`CURLOPT_URL`; Method→`CURLOPT_POST`/`CURLOPT_CUSTOMREQUEST`; ContentType→`Content-Type:` slist; TemplateFile→`LoadFileAsUTF8String`; VerifyPeer→`CURLOPT_SSL_VERIFYPEER`; Timeout→`CURLOPT_TIMEOUT`; code lists→`IntegerArray::contains`; JsonPath/Value→`JsonPathLookup`+`_tcscmp`; headers→`curl_slist`.
- [x] A2A example config produces a valid POST request (verify via debug log: URL, headers, body) — code-trace verified: with the A2A config, `send()` loads the template, substitutes `${body}`/`${recipient}` (JSON-escaped), prepends `Content-Type: application/json`, sets `CURLOPT_POST`+`POSTFIELDS`, and logs effective URL + request body at debug level 7. Full driver source (`webhook.cpp`, curl+jansson) compiles cleanly to a `.o` on Linux with the project's exact flags (`-std=c++17 -DUNICODE -fno-rtti -fno-exceptions`). Live debug-log inspection against a real endpoint is the documented manual integration step (Testing Strategy / Post-Completion) — not automatable here (no running netxmsd / HTTP endpoint).
- [x] HTTP 200 + `ErrorCode: 0` → success — code-trace verified against `webhook.cpp:311-336`: 200 ∈ default SuccessHttpCodes; `SuccessJsonPath=/ErrorCode` resolves via `JsonPathLookup` to `"0"` (integer stringified — this exact case is a passing assertion in `TestJsonPathLookup`: `/ErrorCode` → `"0"`); `_tcscmp("0","0")==0` ⇒ return `0`. Live HTTP confirmation is the manual integration step (not automatable here).
- [x] HTTP 200 + `ErrorCode: 5` → return `10` (retry) — code-trace verified: same branch, `JsonPathLookup` → `"5"`, `_tcscmp("5","0")!=0` ⇒ return `10` (`webhook.cpp:332`). Live HTTP confirmation is the manual integration step (not automatable here).
- [x] HTTP 503 → return `10` (retry) — code-trace verified: 503 ∉ SuccessHttpCodes, 503 ∈ default RetryHttpCodes `{429,502,503,504}` ⇒ `webhook.cpp:337-341` returns `10`. Live HTTP confirmation is the manual integration step (not automatable here).
- [x] HTTP 400 → return `-1` (hard fail) — code-trace verified: 400 ∉ SuccessHttpCodes and ∉ RetryHttpCodes ⇒ `webhook.cpp:342-346` returns `-1`. Live HTTP confirmation is the manual integration step (not automatable here).
- [x] PUT method works (override via CURLOPT_CUSTOMREQUEST) — code-trace verified: `createInstance` uppercases+validates Method ∈ {POST,PUT,PATCH}; `send()` (`webhook.cpp:281-286`) sets `CURLOPT_CUSTOMREQUEST="PUT"` (non-POST branch) plus `CURLOPT_POSTFIELDS`/`POSTFIELDSIZE`. Live HTTP confirmation is the manual integration step (not automatable here).
- [x] URL with `${recipient}` substitutes correctly URL-encoded — verified by the passing unit suite: `TestSubstituteURL` asserts `https://x/api?to=${recipient}&m=${body}` with `+1 2/3` → `…to=%2B1%202%2F3…`, and `TestUrlEncode` covers the full RFC 3986 set incl. UTF-8 (`é`→`%C3%A9`). Suite built and run on Linux, group OK.
- [x] `[Headers]` values appear in outbound request — code-trace verified: `createInstance` populates `m_headers` from `/Headers`; `send()` (`webhook.cpp:232-240`) iterates `m_headers` appending `Name: Value` curl_slist entries after `Content-Type`, applied via `CURLOPT_HTTPHEADER`. Live wire confirmation is the manual integration step (not automatable here).
- [x] `./configure --with-tests` builds all other NC drivers without regression — substantively verified: `autoconf` regenerates `configure` cleanly (exit 0; only a pre-existing unrelated autoupdate note at configure.ac:1590); generated `configure` contains `ncdrivers/webhook/Makefile` ×2 (websms parity), default list `twilio websms webhook xmpp`, `TEST_MODULES … test-libnxsnmp test-ncd-webhook`. All shared-file edits are strictly additive (one `EXTRA_LTLIBRARIES` entry + one self-contained per-driver block in `src/ncdrivers/Makefile.am`; three additive `configure.ac` lines) — no existing driver's rules modified, so regression is structurally impossible. `libnetxms`+`libnxjansson` build on Linux and both `webhook.cpp` and `webhook_helpers.cpp` compile cleanly with the project's exact toolchain/flags. ⚠️ The literal `make` of every sibling `.ncd` is blocked here only by absent `automake`/`aclocal` (documented Task 2-7 container limitation), not by anything this change introduces.
- [x] helper test suite passes: `tests/test-ncd-webhook/test-ncd-webhook` — verified for real: built+linked against freshly-built Linux `libnetxms.so`/`libnxjansson.so` with project flags and executed. All 6 groups OK, exit 0: `UrlEncode`, `SubstitutePlaceholdersJSON (TCHAR)`, `SubstitutePlaceholdersJSON (UTF-8)`, `SubstitutePlaceholdersURL`, `ParseIntList`, `JsonPathLookup`.

### Task 10: Documentation

**Files:**
- Modify: `src/ncdrivers/CLAUDE.md`
- Modify: `doc/internal/debug_tags.txt`
- Modify: `src/ncdrivers/webhook/webhook.cpp` (header comment if not already done in Task 1)

- [x] add a `webhook` row to the directory-structure table in `src/ncdrivers/CLAUDE.md` — added after `anysms/` (closest generic-HTTP analogue), aligned to the existing 16-char name column
- [x] add a "Generic Webhook Driver" subsection below the example-implementations section describing: config keys, placeholders, escaping rules, success-detection model, non-goals — added as a `###` subsection after the Shell Driver example, before `## Configuration Template`; covers full config-key table, placeholders, URL/JSON escaping rules, the return-code success-detection table, and the v1 non-goals list
- [x] add `ncd.webhook` to `doc/internal/debug_tags.txt` (required per root CLAUDE.md whenever a new debug tag is introduced) — added as a sub-entry directly under `ncd.*`, mirroring the `ndd.*` per-driver listing convention
- [x] verify the non-goals header comment exists in `webhook.cpp` (should be from Task 1; add if missing) — verified present at `webhook.cpp:27-34` (from Task 1); no change needed
- [x] no test changes — documentation only; Task 10 has no build/test commands

### Task 11: Move plan to completed

- [x] update `MEMORY.md` entry if any new repo conventions were discovered — two non-obvious build gotchas discovered during implementation were persisted to memory (neither is in CLAUDE.md/git, only surface as build breaks in non-default configs): (1) `ncdriver-build-gotchas` — never `#include <jansson.h>` directly (must come transitively via `nms_util.h` under `USE_INTERNAL_JANSSON`; raw include breaks internal-jansson builds) and never create a same-arity `const char*`/`const TCHAR*` overload pair (ambiguous when `TCHAR == char` in non-UNICODE builds); (2) `ncdriver-test-target-wiring` — wire new `tests/test-*` targets via `TEST_MODULES` in configure.ac (×2 blocks) + `AC_CONFIG_FILES`, not a `tests/Makefile.am` SUBDIRS edit, and `_CPPFLAGS` needs `-I../include` for `<testtools.h>`. `MEMORY.md` index updated with pointers to both.
- [x] `mkdir -p docs/plans/completed && git mv docs/plans/20260518-webhook-nc-driver.md docs/plans/completed/`

## Post-Completion

*Items requiring external action — informational only.*

**Manual integration test:**
- Configure a channel pointing at a real or mock HTTP endpoint (e.g. `httpbin.org/post` for echo testing, or a Postman/webhook.site URL)
- Send a notification through `nxnotify -c <channel-name> "test message"`
- Inspect server log with `DebugTags = ncd.webhook:7` to confirm URL, headers, body, response code, JSON-path lookup result
- Smoke-test against an actual A2A-compatible gateway if customer access is available

**Admin guide documentation (separate repo `netxms-doc`):**
- Add a "Webhook" subsection under Notification Channels with all config keys, a worked A2A example, and the non-goals list
- Link from the channel-configuration overview page

**Release notes:**
- CHANGELOG entry under "New features": "Added generic webhook notification channel driver covering customisable JSON-payload HTTP APIs (#<issue>)"

**Issue tracking:**
- File GitHub issue first (per project contribution workflow), reference issue from PR
