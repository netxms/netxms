# Server Component (netxmsd)

> Shared guidelines: see [root CLAUDE.md](../../CLAUDE.md).

## Exporting Functions for Modules

Server core is a shared library. External modules (e.g., `webapi/`, `jira/`, `ntcb/`) link against it. Any function or class defined in `core/` that needs to be called from a module **must** be marked with `NXCORE_EXPORTABLE` in its declaration (in the header under `include/`). For exported variables, use `NXCORE_EXPORTABLE_VAR(v)`. Without this, the symbol will not be visible to modules and linking will fail at runtime.

```cpp
// In include/nms_core.h or other server header
void NXCORE_EXPORTABLE MyFunction();
class NXCORE_EXPORTABLE MyClass { ... };
extern NXCORE_EXPORTABLE_VAR(int) g_myGlobal;
```

When exporting a function for module use, also export the sibling functions declared alongside it in the same header group (unless clearly internal) — a module rarely calls exactly one function from a subsystem, and exporting the whole family up front avoids a churn of one-symbol export edits and rebuild/reinstall cycles later.

The converse also matters: a missing `NXCORE_EXPORTABLE` on a function declared in a server header can be deliberate. Leaving it off keeps the symbol reachable only from within core, which is how a security-sensitive primitive is restricted to a single call site — the linker enforces the rule instead of a comment. `ConsumeAuthenticationToken()` (`include/nms_users.h`) is the reference case: it is the sole spend point for single-use authentication tokens and is undecorated on purpose. Before adding the decoration to an undecorated declaration, check for a comment explaining why it is absent.

## String Handling

Server code is always built in Unicode mode. Use `L"..."` string literals and wide-character functions (`wcsncmp`, `wcslen`, etc.) directly. Do **not** use `_T()` / `TCHAR` / `_tcsncmp` wrappers in new server code — those are remnants from older versions that supported non-Unicode builds. The `_T()` abstraction is only needed in agent code and shared libraries that may be built in either mode.

For formatting into a `wchar_t *` buffer, use `nx_swprintf(buffer, size, L"...", ...)` rather than `swprintf()` / `_snwprintf()`: `%s` means `wchar_t*` on Windows but `char*` on glibc, and `nx_swprintf` normalizes `%s` to `wchar_t*` on both platforms. Same argument order as `swprintf`.

**Opaque pass-through blobs stay UTF-8.** When a string member is an opaque blob the server only stores and forwards without processing (XML/JSON configuration blobs like `Node::m_chassisPlacementConf`), declare it as UTF-8 `char*`, not `wchar_t*` — this avoids wide↔UTF-8 conversion on every DB and NXCP round-trip. Load with `DBGetFieldUTF8`, bind with `DB_CTYPE_UTF8_STRING`, use `setFieldFromUtf8String` / `getFieldAsUtf8String` on NXCP, and emit to JSON with `json_string()` directly (no conversion).

## Server Core Conventions

- **No encryption guards.** The server is always built with encryption enabled — do not wrap TLS/crypto-dependent code under `src/server/` in `#ifdef _WITH_ENCRYPTION` or "built without encryption" runtime checks. Those guards are only meaningful in agent code and in shared libraries (libnetxms) that can genuinely be built without OpenSSL.

- **Cache hot-path config, refresh on change.** For a `config` DB variable read on a hot path, cache it in a file-scoped static loaded at init and refresh it from `OnConfigVariableChange` (`core/config.cpp`) — do not call `ConfigReadInt`/`ConfigReadStr` on every use. Add an `else if` prefix-match branch dispatching to a per-subsystem handler (mirrors the existing OTLP/Syslog/SNMP/WindowsEvents branches); the config cache is updated before the handler runs, so a fresh `ConfigReadInt` inside it returns the new value. This keeps the variable runtime-tunable (`need_server_restart=0`).

- **Errors to clients are RCC codes, not strings.** When conveying an error or warning over NXCP, send a numeric `RCC_*` code — the client localizes it via `RCC.getText()`. Server-formed message strings cannot be translated client-side; keep messages generic (no node IDs baked in). Fatal errors go in `VID_RCC`; a non-fatal warning on a success response goes in a separate field (e.g. `VID_WARNING_RCC`). Adding a new RCC touches `include/nxcldefs.h`, the `RCC.java` mirror, and an `RCC_0NNN=` entry in `netxms-client-messages.properties`. To translate an agent `ERR_*` code, use `AgentErrorToRCC()` (libnxsrv) — don't hand-roll a switch. The inverse applies at *external* integration boundaries (OTLP export, webhooks): never emit internal numeric enum codes there — encode them as symbolic name strings (numeric identifiers like event/DCI/object IDs are fine as integers).

- **Range scans use the parallel primitives.** Never probe a multi-host IP range in a serial per-IP loop. Use `ScanAddressRangeICMP` (`core/discovery.cpp`), `TCPScanAddressRange` (libnxagent), `SnmpScanAddressRange` (libnxsnmp), or the high-level `CheckRange` wrapper (handles zone proxies and well-known ports/communities, coupled to `NetworkDiscovery.ActiveDiscovery.*` config) with a callback. The callback fires once per (IP, protocol) detection — aggregate by IP in your context if you need per-host records.

- **Console commands:** when adding or substantially rewriting a branch in `ProcessConsoleCommand()` (`core/console.cpp`, already >1000 lines), extract the logic into a dedicated `static void HandleXxxCommand(ServerConsole*, const wchar_t *arg)` defined earlier in the file. The branch body should be a one- or two-line call.

- **Object update from JSON (WebAPI `modifyFromJSON*` handlers):** when reading a scalar property into an object field, use the strict `json_object_update_*` helpers in `include/nms_util.h` instead of hand-writing the get/validate/cast block. Each returns `false` only when the property is present with an incompatible type — map that to `RCC_INVALID_ARGUMENT`; absent leaves the field unchanged:
  - `json_object_update_integer(json, "tag", &field)` — any integer field type; null = 0.
  - `json_object_update_boolean(json, "tag", &boolField)`.
  - `json_object_update_flag(json, "tag", BIT, &setFlags, &mask)` — collect bits for one trailing `updateFlags(setFlags, mask)`.
  - `json_object_update_string(json, "tag", wcharBuf, capacityChars)` / `json_object_update_string_utf8(json, "tag", charBuf, capacityBytes)` — fixed buffers; null/empty clear.

  These are strict (fail on wrong type) — distinct from the lenient coercing `json_object_get_*` getters. Keep blocks manual when the field has side effects, needs object-ID validation, targets a setter or `String`/`SharedString` member, or uses null-means-clear semantics — the helpers don't cover those.

- **No synthetic measurements in serialized state.** When emitting derived or aggregate fields (timer min/max/average, sample counts) to JSON or NXCP, gate them on an "observed at least once" predicate (`lastCompleted != TIMESTAMP_NEVER`, count > 0, a non-null timestamp). A default-constructed zero reads as a real "0 ms" measurement. Omit the key (or emit `json_null()`) instead; identity and status fields stay unconditional. Reference: `PollState::toJson()`.

- **Configuration import leaves existing ACLs alone.** When an import matches an existing entity by GUID and updates it, do not touch its stored access control list. Insert the default "Everyone" ACL row only on the branch that creates a new record. The unconditional delete-and-reinsert in `ImportObjectTool()` (`core/objtools.cpp`) is a bug to avoid copying, not a pattern to follow.

- **Audit subsystem for deferred actions.** Scheduled task create/update/delete is audited under `AUDIT_SYSCFG` for every handler type, including `Maintenance.*`. `AUDIT_OBJECTS` is written when the object itself changes, i.e. when the scheduled task executes. Generally: audit the scheduling under its configuration subsystem and the effect under the subsystem of the thing affected.

- **User ID 0 is the system account.** It is normally disabled and used by server code to mean "system / no user session" (`checkAccessRights` grants everything, background AI chats run as 0). Per-user features (AI memory, preferences, history) may legitimately skip it — do not report that as a bug.

- **Object tool input fields only where they are consumed.** Only `TOOL_TYPE_ACTION`, `SERVER_COMMAND`, `SERVER_SCRIPT`, `SSH_COMMAND`, `URL` and library-script execution expand input fields (via `expandText()` or NXSL `$INPUT`). `TOOL_TYPE_SNMP_TABLE`, `AGENT_TABLE` and `AGENT_LIST` never do, so do not read, log or forward input fields on those paths.

- **Use NetXMS utilities in module helper headers.** Server modules include `nms_common.h` / `nms_util.h` / `nms_core.h` through their main header, so a helper header included after them may use `IntegerToString`, `String`, containers etc. freely. The clangd LSP analyzing such a header in isolation reports `undeclared identifier` (the same false positive recurs in `nms_events.h` / `nms_core.h`); ignore it rather than rewriting to raw libc to make the header self-contained.

## Network Device Drivers

Drivers live in `drivers/<vendor>/`; the interface is `include/nddrv.h`, the generic base implementation `libnxsrv/ndd.cpp`. Use `DECLARE_NDD_ENTRY_POINT(Class)` for a single driver, or `NDD_BEGIN_DRIVER_LIST` / `NDD_DRIVER(Class)` / `NDD_END_DRIVER_LIST` / `DECLARE_NDD_MODULE_ENTRY_POINT` for several drivers in one module. `isPotentialDevice()` returns a priority (0 = not supported); the highest-priority match wins.

Do **not** override `getHardwareInformation()` in a new driver when the device supports standard ENTITY-MIB — the generic driver already reads vendor/model/serial from it. Override only for devices that expose hardware info via a proprietary MIB or lack ENTITY-MIB support.

## Database Changes

Schema rules are in [src/db/CLAUDE.md](../db/CLAUDE.md); upgrade procedures and the multi-branch backport procedure are in [tools/nxdbmgr/CLAUDE.md](tools/nxdbmgr/CLAUDE.md).

## Design Documentation

- Single-Use Authentication Tokens: `doc/Single_Use_Authentication_Tokens.md`
