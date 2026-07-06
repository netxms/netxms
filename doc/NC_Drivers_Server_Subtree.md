# Notification Channel Drivers in Server Subtree — Design Document

Status: phases 1 (mechanical move) and 2 (v5 context interface, NXSL special case dissolved) implemented; phase 3 design draft. Tracked as issue #3382.

This document assesses moving notification channel (NC) drivers from
`src/ncdrivers` into the server subtree, giving them access to server objects
(most importantly the `Event` that triggered the notification), and examines
how the notification channel driver interface relates to the event forwarder
framework introduced in 6.2.

## 1. Background and motivation

NC drivers were originally placed outside the server subtree, with an
interface (`include/ncdrv.h`) deliberately free of server dependencies. The
intent was to allow the agent to load the same drivers and act as a remote
notification point. That idea never materialized, and the "SMS via agent" use
case is served the other way around: the `nxagent` driver runs on the server
and connects out to the agent through `AgentConnection`.

The isolation is now fiction and produces two special cases:

1. **`nxagent` driver** already compiles with `-I src/server/include` and
   links `libnxsrv.la` from outside the server subtree
   (`src/ncdrivers/Makefile.am`), breaking the layering the placement was
   supposed to guarantee.
2. **NXSL driver** cannot be a driver at all, because a driver can see
   neither `Event` nor link `libnxsl`. It is instead a hard special case
   inside `NotificationChannel` (`src/server/core/notification_channel.cpp`):
   a registry entry with `instanceFactory == nullptr` as sentinel, an
   `m_nxslScript` member on the channel, and a dedicated `sendNXSL()` branch
   in the worker thread.

Meanwhile the plumbing for event context already exists: `NotificationMessage`
carries a copied `Event *m_event` and `shared_ptr<NetObj> m_sourceObject`
solely for the NXSL branch. External drivers receive only
`send(recipient, subject, body)` — the structured context is discarded one
line above the driver call.

Several drivers would immediately benefit from event access: `webhook` (emit
full event JSON instead of pre-rendered text), `snmptrap` (real varbinds from
event code/severity/parameters/tags instead of forcing them through the text
pipe), `mqtt`, `dbtable`, and rich-formatting chat drivers (Slack, Teams,
Telegram) that could render severity, source object, and parameters natively.

## 2. Feasibility

**Feasible, with strong in-tree precedent.** The target pattern already
exists twice:

- **pdsdrv** (`src/server/pdsdrv/`) — performance data storage drivers,
  dynamically loaded, link `../../core/libnxcore.la`, receive live server
  objects (`PerfDataStorageDriver::saveDCItemValue(DCItem*, ...)`).
- **hdlink** (`src/server/hdlink/`) — helpdesk link modules, same linkage.

Both prove that dynamically loaded drivers may link against `libnxcore` and
resolve symbols at runtime against the copy already loaded in the `netxmsd`
process. NC drivers become the third instance of an established pattern, not
a new invention.

## 3. Design

### 3.1 Relocation

- Move `src/ncdrivers` → `src/server/ncdrivers`.
- Move `include/ncdrv.h` → `src/server/include/ncdrv.h` (remove from
  `include/Makefile.am`).
- Install layout is **unchanged**: drivers continue to install as
  `pkglibdir/ncdrv/*.ncd`, and the loader
  (`src/server/core/notification_channel.cpp`, `LoadDriver()` /
  directory scan) is untouched by the move itself. No packaging behavior
  changes.
- Drivers gain `-I@top_srcdir@/src/server/include` and link
  `../../core/libnxcore.la` (plus `../../libnxsrv/libnxsrv.la` where needed,
  e.g. `nxagent`).

### 3.2 Driver interface v5

Bump `NCDRV_API_VERSION` to 5 and pass event context through a context
object rather than additional positional parameters, so future extensions do
not force another version bump:

```cpp
struct NotificationContext
{
   const char *recipient;           // UTF-8, as today
   const char *subject;             // UTF-8, as today
   const char *body;                // UTF-8, as today
   const wchar_t *recipientW;       // wide originals, so drivers needing
   const wchar_t *subjectW;         // wide strings skip a UTF-8 round trip
   const wchar_t *bodyW;
   const Event *event;              // may be nullptr
   shared_ptr<NetObj> sourceObject; // may be null
   const wchar_t *channelName;
   uuid ruleId;
};

class NCDriver
{
public:
   virtual int send(const NotificationContext& context) = 0;
   virtual bool checkHealth() { return true; }
};
```

`ncdrv.h` only forward-declares `Event` and `NetObj`; a driver that does not
touch the event context needs no server headers and no `libnxcore` linkage
(holding or comparing the `shared_ptr<NetObj>` does not require the complete
type). Drivers that start calling `Event`/`NetObj` methods add
`../../core/libnxcore.la` to their `LIBADD` at that point — the pdsdrv/hdlink
precedent.

Rules:

- **`event` is nullable and drivers must handle its absence.** Digest
  messages, test sends, and the `SendNotification` overload that carries only
  `eventCode`/`eventId` have no full event object.
- The existing hard version check in `LoadDriver` rejects v4 binaries with a
  clean error. Per project policy (hard-fail, no compat shims) there is no
  dual-signature support; out-of-tree drivers recompile against v5.
- No new copies are introduced: `NotificationMessage` already copies the
  event once per message when context is present.

Note on string width: v4 moved the driver API to UTF-8, while `Event` and
`NetObj` are wide-string classes, so v5 hands drivers wide strings again
through the context. This is accepted — the UTF-8 core migration will
eventually convert `Event` itself, and the context object localizes the
churn.

### 3.3 Dissolving the NXSL special case

The NXSL driver is now a regular `NCDriver` implementation (`NXSLDriver`,
registered built-in in core by `LoadNotificationChannelDrivers()`), holding
its compiled program in the instance. Because its configuration is raw NXSL
script text rather than XML, the driver descriptor carries an
`xmlConfiguration` flag: when false, the channel code skips the XML parse and
hands the raw text to the standard `NcdCreateInstance(Config*, ...)` factory
as the value of `/Configuration` in a synthetic `Config` object.

Removed from `NotificationChannel`:

- the `instanceFactory == nullptr` sentinel,
- `m_nxslScript` / `setNXSLScript()` / `isNXSL()`,
- the `sendNXSL()` branch in `workerThread()`.

The NXSL driver reads the wide originals from the context, avoiding the
UTF-8→wide conversion entirely.

Two minor behavior changes: script compilation errors now put the generic
"Unable to create instance of driver NXSL" into the channel error message
(line number and diagnostic text go to the server log, as for any driver's
configuration failure), and NXSL channels now report `driverInitialized =
true` to clients (previously they showed as uninitialized because only
`m_driver` was checked).

### 3.4 `nxagent` driver

No functional change; its `libnxsrv` linkage and server-include path simply
become ordinary once the driver lives in the server subtree.

## 4. Relationship to event forwarders

After this change, `NCDriver::send(context-with-Event)` and
`EventForwarderDriver::forward(const Event&, recipient, source)`
(`src/server/include/efdrv.h`) become nearly the same signature. The two
subsystems should nevertheless **not** be merged:

- **Notification channels** deliver rendered, human-oriented text with
  recipient routing, throttling, digests, retry, and per-channel status.
- **Event forwarders** deliver complete structured event records
  machine-to-machine, with their own queue/retry semantics.

Users configure them differently and the console surfaces them differently.
What should be unified is the **driver housing**:

- Today EF drivers are only module-registered
  (`RegisterEventForwarderDriver`; `isc` built-in, `otlp` via module) — there
  is no dynamic `.efd` loading. With both interfaces in the server subtree,
  the directory scan can allow one driver binary to export either or both
  entry points (`NcdCreateInstance` and/or an EF factory).
- **`snmptrap`** is the clearest migration candidate: as an NC driver it
  forces structured trap data through the subject/body text pipe; as an EF
  driver it can emit proper varbinds from event code, severity, parameters,
  and tags. Once the EF driver covers it, the NC variant can be deprecated.
- **`mqtt`** can meaningfully be both: NC driver for plain-text publishes,
  EF driver for event-as-JSON to a topic.

## 5. Build system impact

- **configure.ac**: `NCDRV_MODULES` / `NCDRV_LTLIBRARIES` handling (lines
  ~166, ~1078, ~2319, ~3936) and `AC_CONFIG_FILES` paths for all driver
  Makefiles. NC drivers now build exactly when the server builds (via
  `src/server/Makefile.am` SUBDIRS). The `--with-sdk` switch, which used to
  build `ncdrivers` without the server, was removed entirely as a follow-up:
  extensions (including private Raden modules) are built in-tree from
  sub-repositories checked out under `private/`, so the installed-SDK
  workflow (header install, static libs, `pkgdatadir/sdk` support files) had
  no consumers.
- **Makefile.am hierarchy**: driver Makefiles move under
  `src/server/ncdrivers/`, relative `LIBADD` paths change
  (`../libnetxms/...` → `../../../libnetxms/...` etc.).
- **Windows**: 26 references in `netxms.sln` plus every driver `.vcxproj`'s
  relative include/lib paths.
- **Tests**: `test-ncd-webhook` compiles `webhook_helpers.cpp` directly and
  links only `libnetxms`. Keep helper translation units free of nxcore
  includes so the unit test continues to build without the server.

## 6. Risks and notes

| Item | Assessment |
|------|------------|
| Out-of-tree private drivers | Break at v5; hard version check gives a clean load failure. Recompile required. |
| License | `ncdrv.h` is LGPL today; drivers linking GPL `libnxcore` are effectively GPL. Non-issue in-tree, but the header license text should be aligned when it moves. |
| Event lifetime during retries | Retry loop (up to `NotificationChannels.MaxRetryCount`) holds the per-message `Event` copy and source `shared_ptr` — already the case via `NotificationMessage`; no new cost. |
| Driver development model | `ncdrv.h` moves out of the public `include/` set. Third-party driver development follows the same model as all other extensions: build in-tree from a sub-repository under `private/` (the `--with-sdk` installed-header workflow was removed). |
| UTF-8 migration interplay | v5 exposes wide-string server objects after the v4 UTF-8 switch; accepted and localized in the context object (see 3.2). |

## 7. Phasing

1. **Mechanical move** — relocate sources and header, rewire autotools +
   vcxproj/sln, no interface change. Isolates the build churn; easy to
   verify (identical runtime behavior).
2. **v5 context interface** — add `NotificationContext`, bump API version,
   convert all in-tree drivers, dissolve the NXSL special case.
3. **Driver housing unification** — allow scanned driver binaries to export
   EF factories; implement `snmptrap` (and optionally `mqtt`) as event
   forwarder drivers; deprecate the `snmptrap` NC driver once covered.

Phases 1–2 have no database schema or client protocol impact.
