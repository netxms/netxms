# netxmsd High Availability — Design Specification

Status: phase 0 draft (issue [#3364](https://github.com/netxms/netxms/issues/3364))

Two netxmsd instances share one database; one is **active**, one is **standby**.
Roles are decided exclusively by a database lease. This document is the
normative specification for the fault model, the lease algorithm, the change
journal, and the subsystem lifecycle contract. The GitHub issue holds the
feature-level design; where they disagree, this document wins.

## 1. Safety invariant and fault model

### 1.1 Safety invariant

> **No node starts new role-sensitive work as active without holding a
> currently-valid lease for the current term, where validity is evaluated
> against database server time.**

*Role-sensitive work* is any action that only the active node may perform:
DB writes outside the HA subsystem itself, event processing, polling,
notifications, scheduled task execution, and any externally visible side
effect (mail, SNMP trap sending, agent commands).

A secondary liveness property:

> **If the current active is unable to act (crashed, paused, fenced, or cut
> off from the database), a healthy standby with database access promotes
> within bounded time.**

### 1.2 Covered faults

| Fault | Handled by |
|---|---|
| Active crash / kill -9 | Lease expiry → standby CAS takeover |
| Graceful shutdown / switchover | Lease release + handover notification |
| DB unreachable from active only | Active self-fences on refresh failure; standby promotes on expiry (peer channel state is irrelevant) |
| DB unreachable from standby only | No role change; standby cannot promote without DB |
| DB unreachable from both | Neither node acts; monitoring stops (DB HA is out of scope) |
| Peer channel partition, both nodes healthy | No role change; standby marks itself dirty, reconciles from journal |
| Process pause (SIGSTOP, VM freeze, GC-like stall) longer than lease validity | Monotonic watchdog fences on resume before any new work |
| Hung/slow lease refresh statement | Dedicated connection with bounded statement timeout; treated as refresh failure |
| Host clock skew / step / drift between nodes | Irrelevant: all lease expiry comparisons execute in SQL on the DB server clock; local clocks are used only for the monotonic watchdog (rate, not absolute time) |
| Simultaneous start of both nodes | Atomic CAS: exactly one term winner |
| Long-running transaction crossing a promotion | Journal replay at activation happens after writes quiesce; residual commit window documented below |
| Stale in-memory ID allocators on promoted standby | Mandatory allocator rebase during activation |

### 1.3 Explicitly uncovered faults (residual risk)

- **Residual commit window.** A statement already handed to the database
  server when the fence trips can still commit. The window is bounded by the
  maximum statement execution time. Closing it would require storage-level
  fencing tokens (a term check inside every write), which is explicitly out
  of scope. Consequence for parameters: lease validity must comfortably
  exceed typical statement runtimes, and bulk writers must use bounded
  batches.
- **Byzantine behavior.** A node that lies about its identity or term is not
  defended against; both nodes are trusted code on trusted hosts. The peer
  channel is mutually authenticated only to keep third parties out.
- **Database corruption / rollback of committed data.** The DB is the
  arbiter and the source of truth; if it lies, HA lies with it.
- **More than two nodes.** The lease format is versioned so a quorum design
  can extend it later; this design assumes exactly two configured members.

## 2. Lease

### 2.1 Table

Single-row table; the row is created by the schema (never inserted at
runtime).

```sql
CREATE TABLE ha_lease
(
   lease_id integer not null,          -- always 1
   format_version integer not null,    -- 1
   term $SQL:INT64 not null,           -- monotonically increasing
   holder_guid varchar(36) null,       -- node GUID, null when never held
   holder_incarnation $SQL:INT64 not null, -- random 64-bit per process start
   holder_name varchar(63) null,       -- informational (hostname)
   holder_address varchar(255) null,   -- client-reachable address of the holder
   acquired_at $SQL:INT64 not null,    -- DB epoch seconds at acquisition
   expires_at $SQL:INT64 not null,     -- DB epoch seconds; 0 = released
   PRIMARY KEY(lease_id)
) TABLE_TYPE_DEFAULT;
```

Seed row: `term=0, holder_guid=NULL, holder_incarnation=0, acquired_at=0, expires_at=0`.

`holder_address` is written at acquisition from the `[CLUSTER] NodeAddress`
configuration option (default: local FQDN) and is purely informational for
the lease algorithm — standby nodes read it to tell redirected clients where
the active node is (section 5.5).

### 2.2 Identity

- **Node GUID**: generated once, persisted in the local data directory
  (`<datadir>/ha-node.guid`). Never stored in the shared DB config.
- **Process incarnation**: random 64-bit value generated at every process
  start. Distinguishes a restarted process from a paused-and-resumed one.
- **Cluster identity** (`g_serverId`) is unrelated to lease identity: it
  comes from shared DB metadata and is identical on both nodes by design.

On first successful start a node **binds itself to the cluster identity** by
persisting the database's server ID in `<datadir>/ha-cluster.id`; on every
subsequent start a mismatch aborts startup. Without this, a node
misconfigured to point at another cluster's database would join that
cluster's lease competition (the shared `CLUSTER` instance lock cannot tell
members of different clusters apart) — the lease would still guarantee a
single active per database, but the wrong box could win it while the node's
own cluster silently loses redundancy. Deliberate re-homing (database
migration or restore under a new server ID) is done by removing the binding
file.

### 2.3 Database time

Every expiry comparison and every timestamp written into `ha_lease` is
computed by the database server, inside the SQL statement. The client never
compares its own wall clock against a stored value. Per-syntax expression
for "epoch seconds now" (`DBNOW` below):

| Syntax | Expression |
|---|---|
| `DB_SYNTAX_PGSQL`, `DB_SYNTAX_TSDB` | `CAST(EXTRACT(EPOCH FROM now()) AS bigint)` |
| `DB_SYNTAX_MYSQL` | `UNIX_TIMESTAMP()` |
| `DB_SYNTAX_ORACLE` | `(CAST(SYS_EXTRACT_UTC(SYSTIMESTAMP) AS DATE) - DATE '1970-01-01') * 86400` |
| `DB_SYNTAX_MSSQL` | `DATEDIFF_BIG(SECOND, '1970-01-01', SYSUTCDATETIME())` |
| `DB_SYNTAX_DB2` | derived from `CURRENT TIMESTAMP - TIMESTAMP('1970-01-01')` (exact form fixed during 0.2) |
| `DB_SYNTAX_SQLITE` | **not supported** — cluster mode refuses to start on SQLite; an embedded per-process file cannot be a shared arbiter |

Timezone note: all expressions must be UTC-based or at least *consistent for
both nodes* — they hit the same DB server, so even a non-UTC server clock is
a valid arbiter as long as the same expression is used everywhere.

### 2.4 Operations

All lease statements run on a **dedicated DB connection** owned by the HA
manager thread, never through the shared connection pool (pool acquisition
can block indefinitely behind busy workers; a busy pool must not look like a
dead node). The connection sets a statement timeout of `LeaseStatementTimeout`
(default 5 s) where the backend supports it, and the HA manager additionally
enforces the timeout with the monotonic clock.

**Acquire / take over** (used at startup, on expiry, and after handover):

```sql
UPDATE ha_lease
   SET term=term+1, holder_guid=?, holder_incarnation=?, holder_name=?,
       acquired_at=DBNOW, expires_at=DBNOW+?
 WHERE lease_id=1 AND expires_at < DBNOW
```

One affected row = this node won the new term (read the term back with a
follow-up SELECT; safe because only the winner's `holder_guid` matches).
Zero rows = someone else holds a valid lease.

**Refresh** (every `LeaseRefreshInterval`, default 5 s):

```sql
UPDATE ha_lease
   SET expires_at=DBNOW+?
 WHERE lease_id=1 AND term=? AND holder_guid=? AND holder_incarnation=?
```

One affected row = still owner; the HA manager records the *local monotonic
time of the confirmation*. Zero rows = the term moved on → **fence
immediately** (another node holds a newer term; do not retry).
Statement error/timeout = refresh failure; retry until the watchdog deadline.

**Release / graceful handover**:

```sql
UPDATE ha_lease
   SET expires_at=0
 WHERE lease_id=1 AND term=? AND holder_guid=? AND holder_incarnation=?
```

Executed after the drain sequence (section 5.3). The peer's normal acquire
predicate (`expires_at < DBNOW`) then succeeds immediately; a handover
notification on the peer channel tells it to try *now* instead of at the
next poll tick.

**Standby poll** (every `LeaseRefreshInterval`):

```sql
SELECT term, holder_guid, holder_name, expires_at - DBNOW
  FROM ha_lease WHERE lease_id=1
```

The fourth column is the remaining validity *as computed by the DB*; the
standby never interprets `expires_at` against its own clock. Negative or
zero → attempt acquire.

### 2.5 Promotion rule

The standby attempts the acquire CAS when the DB reports the lease expired.
**The peer channel has no veto.** Channel state is used only to:

- promote *sooner* on an authenticated handover/DEMOTED notification;
- log a loud fencing anomaly (`SYS_HA_FENCING_ANOMALY`) if a peer still
  claims ACTIVE over a healthy channel while its lease is expired — that
  combination means self-fencing failed and must be investigated.

### 2.6 Self-fencing rule

The active node maintains `T_confirm` = monotonic timestamp of the last
*confirmed* refresh. It must stop role-sensitive work strictly before the DB
can consider the lease expired:

```
fence_deadline = T_confirm + LeaseValidity - FenceMargin
```

`FenceMargin` (default 3 s) covers the refresh statement round trip (the DB
stamped `expires_at` *before* the confirmation reached us), clock *rate*
skew over one validity window, and scheduling latency of the check itself.

Fencing triggers:

1. Refresh returns zero rows (term lost) → fence immediately.
2. `monotonic_now > fence_deadline` (refresh failing, hung, or the process
   was paused) → fence. This check runs on a **dedicated watchdog thread
   that never touches the database** — the HA manager thread itself can be
   stuck inside a hung lease statement, and any thread that talks to the DB
   can hang with it (validated by the stalled-connection harness scenario).
   It is additionally evaluated opportunistically by high-frequency
   role-sensitive dispatch points (DB writer dequeue, event queue dispatch),
   so a process resuming from a long pause fences before dispatching any
   queued work, even before the watchdog gets scheduled.

Fencing raises the global **fence flag** (a process-wide atomic, checked
lock-free). Semantics:

- Checked at *dispatch* boundaries: DB writer batch start, event processing
  loop, poller task launch, scheduler task launch, notification send, journal
  append. A set flag means the work item is not started.
- Already-running statements are not interrupted (residual window,
  section 1.3).
- After raising the flag the process completes fencing by an **orderly
  restart into standby** (section 5.4). The flag is terminal for the process
  incarnation: there is no unfencing.

### 2.7 Timing parameters

| Parameter | Default | Constraint |
|---|---|---|
| `LeaseRefreshInterval` (R) | 5 s | — |
| `LeaseValidity` (V) | 20 s | V ≥ 3R + max refresh statement time; V ≫ typical bulk statement runtime (residual window) |
| `FenceMargin` (M) | 3 s | M ≥ refresh round trip + watchdog scheduling latency; M < V − R |
| `LeaseStatementTimeout` | 5 s | ≤ R |

All configurable in the `[CLUSTER]` section; defaults are validated (and, if
needed, adjusted) by the phase 0 harness measurements.

### 2.8 Interaction with the legacy instance lock

For cluster members the lease supersedes `LockDatabase()`: both members
share the lock identity (cluster server ID) so a standby can pass the
single-instance check, while a *non-member* third instance is still
rejected. The `PeerNode` / `PeerNodeIsRunning()` stale-lock heuristic in
`main.cpp` is retired; the lease provides a strictly stronger version of the
same guarantee.

`UnlockDatabase` is a no-op for cluster members — only `nxdbmgr unlock`
clears the `CLUSTER` lock, and when it does it also offers to clear a held
(possibly stale) lease: holder fields nulled, `expires_at=0`, **term
preserved** (same state as a graceful release), so a starting node acquires
immediately instead of waiting out the validity window after whole-cluster
maintenance.

## 3. Change journal

Purpose: make the peer channel pure acceleration. Correctness of the
standby's warm state never depends on channel delivery, because every
mutation the standby must know about is durably journaled in the DB and
replayed at activation.

### 3.1 Tables

```sql
CREATE TABLE ha_change_journal
(
   seq $SQL:INT64 not null,         -- strictly increasing, allocated by active
   entity_type char(1) not null,    -- '0' = object, '1' = alarm
   change_type char(1) not null,    -- '0' = save/update, '1' = delete (tombstone)
   entity_id integer not null,      -- object id or alarm id
   entity_class integer not null,   -- object class for objects, 0 for alarms
   created_at $SQL:INT64 not null,  -- DB epoch seconds (informational)
   PRIMARY KEY(seq)
) TABLE_TYPE_DEFAULT;

CREATE TABLE ha_sync_state
(
   node_guid varchar(36) not null,
   applied_seq $SQL:INT64 not null, -- contiguous applied watermark
   updated_at $SQL:INT64 not null,
   PRIMARY KEY(node_guid)
) TABLE_TYPE_DEFAULT;
```

### 3.2 Writing (active node)

- `seq` is allocated from a process-wide atomic counter, seeded at
  activation from `max(seq)` (part of the ID rebase). No DB-native
  auto-increment: keeps the DDL portable and the value known before commit.
- The journal row is written **in the same transaction** as the object save,
  or immediately after the delete on the deletion path (deletions do not go
  through the save path — `Syncer` deletes are a separate code path, hence
  explicit tombstones).
- Alarm changes journal `entity_type='1'` on every alarm create / state
  change / terminate, alongside the alarm's own DB write.
- Journal append checks the fence flag like any role-sensitive write.

### 3.3 Applying (standby node)

Channel messages carry the journal `seq` of the change they describe. The
standby tracks:

- **applied set** — sequence numbers applied in this session;
- **watermark** — the largest W such that all seq ≤ W are applied. Only the
  watermark is persisted (`ha_sync_state`, written by the standby — writing
  its own sync row is not role-sensitive work).

Because journal entries commit out of order (seq allocated before commit,
transactions overlap), the watermark advances only over a contiguous prefix.
Gaps are resolved by the next channel message or by replay.

### 3.4 Replay

`SELECT seq, entity_type, change_type, entity_id, entity_class FROM
ha_change_journal WHERE seq > ? ORDER BY seq` — executed:

- at **activation** (mandatory reconcile step). Safe and complete because
  the old active's writes have quiesced: it is dead, fenced, or has drained
  (graceful path). In-flight transactions that still commit inside the
  residual window after replay are lost with the window — same guarantee.
- after a **channel reconnect**, to fill the gap accumulated while dirty.

Replay applies each entry the same way a live channel message would:
reload entity from DB / apply tombstone.

### 3.5 Pruning

Housekeeper on the active node deletes `ha_change_journal WHERE seq <=
min(applied_seq of all rows in ha_sync_state whose node is a current cluster
member)`. Fallback when the peer has been absent longer than
`JournalRetentionTime` (default 24 h): prune by `created_at` age and mark the
journal **truncated** (a flag row / metadata entry); a standby whose
watermark predates the truncation point must discard its warm state and
rebuild by restarting into a fresh standby.

## 4. ID allocator rebase

All in-memory ID allocators are seeded from DB maxima once and incremented
in process memory; a long-running standby holds stale values. Activation
therefore re-runs every allocator seed query **before** phase 2 startup
(after winning the lease, before any subsystem can allocate).

The definitive inventory is produced by the step 0.5 audit (section 7.2).
Known members: all `IDG_*` groups in `id.cpp` (`InitIdTable`), the event ID
counter (`LoadLastEventId`), and the journal `seq` counter (section 3.2).

## 5. Subsystem lifecycle contract

### 5.1 States and transitions

A process is in one of: **PASSIVE** (standby), **ACTIVE**, or standalone
(cluster mode off — passive bring-up and activation run back-to-back,
preserving today's behavior exactly). Transitions:

- PASSIVE → ACTIVE: once, via activation (section 5.2). One-way.
- ACTIVE → (anything): never in-process. Demotion, fencing, and dirty-state
  recovery all end the process incarnation (section 5.4).

Because activation happens at most once per incarnation, subsystems need an
`initPassive()` / `activate()` pair but **no deactivate / re-activate**
support.

### 5.2 Contract

`initPassive()`:

- may load state from DB and build in-memory structures;
- must NOT spawn threads that write to the DB, emit events, or produce any
  external side effect;
- whitelisted passive threads: HA manager (lease + watchdog), peer channel,
  local admin interface (nxadm), passive listeners with the restricted-role
  handler, debug console, log writer, watchdog.

`activate()`:

- called exactly once, on the HA manager thread, in defined order, only
  after: lease won → ID rebase → journal replay → alarm flush-back;
- starts the subsystem's worker threads and enables full request handling;
- must be idempotent-safe against a crash *during* activation: a crash
  mid-activation simply loses the incarnation; the peer (or this node,
  restarted) takes over from the DB state.

Module API: `NXMODULE` grows optional `pfInitPassive` / `pfActivate` entry
points; existing modules keep working in standalone mode unchanged (their
current init is treated as activate-time). Inventory of core subsystems and
their required split: section 7.1.

### 5.3 Graceful switchover sequence (active side)

1. Stop intake: scheduler, pollers, receivers stop accepting/launching work.
2. Drain: event queue, DB writer queues, journal appends; flush modified
   objects.
3. Wait for the peer's watermark to reach the journal head (bounded wait,
   default 30 s; on timeout proceed anyway — the peer will replay).
4. Release the lease (section 2.4) and send the handover notification.
5. Restart into standby.

### 5.4 Demotion = restart

Any path that ends the ACTIVE role (fence, graceful switchover, operator
demote) terminates the process with a distinct exit code
(`NETXMSD_EXIT_RESTART_STANDBY`) and relies on the service manager
(systemd `Restart=`, Windows service recovery) to start it again; it comes
up in PASSIVE. Rationale: in-place teardown of a running server is
equivalent to the shutdown path anyway; one bulletproof code path beats a
half-stopped state machine. The installer/service templates must configure
automatic restart for cluster deployments.

Deployment requirements:

- **systemd**: the stock unit (`contrib/startup/systemd/netxms-server.service`)
  sets `Restart=always` with `RestartSec=5`, which covers exit code 94.
  Custom units must at minimum set `RestartForceExitStatus=94` (and must not
  list 94 in `SuccessExitStatus`). `RestartSec` of a few seconds prevents a
  tight restart loop on a node that keeps fencing (e.g. persistent DB
  connectivity flap).
- **Windows**: service recovery options must be set to "Restart the Service"
  for first/second/subsequent failures (the installer configures this for
  cluster deployments).

### 5.5 Standby-role listeners (redirection)

A standby node answers instead of timing out. Listeners started during
passive bring-up, each with a per-request role gate (`cluster mode && !(g_flags
& AF_SERVER_INITIALIZED)`):

- **Client listener** (port 4701): started before parking (the
  `WaitForServerStartupCompletion` barrier is skipped in cluster mode).
  `ClientSession::processRequest` serves protocol negotiation
  (`CMD_GET_SERVER_INFO`, `CMD_REQUEST_ENCRYPTION`) normally and answers
  everything else — including login — with `RCC_SERVER_IS_STANDBY` plus
  `VID_ACTIVE_SERVER_ADDRESS` (the lease's `holder_address`), so clients can
  redirect (client-side auto-redirect is phase 3).
- **Web API listener**: the HTTP daemon starts during passive bring-up
  (`StartWebAPIListener`); the recurrent login-cleanup task stays
  activation-time. The router answers 503 for every route not explicitly
  marked `availableOnStandby()`. Marked routes: `/` (version info) and
  `GET /v1/ha/status` (unauthenticated health endpoint reporting
  `clusterMode`, `role`, `ready`, `term`, `activeNode`, `activeNodeAddress`,
  `leaseRemainingValidity`) — intended for load-balancer health checks.
- **Local admin interface** (nxadm, loopback-only): available on standby
  *without authentication* — the user database loads at activation, so
  authentication is impossible on a standby node, and HA operations
  (`ha status`) must be reachable. Trust model matches the existing
  unauthenticated `CMD_SET_DB_PASSWORD` handling on the same interface;
  normal `Server.Security.RestrictLocalConsoleAccess` rules resume once the
  node activates.
- **Agent tunnels**: listener stays activation-only. Agents maintain
  independent tunnels to every configured server and retry failed ones every
  `TunnelKeepaliveInterval` (30 s default); connection-refused on the standby
  *is* the redirect mechanism. Tunnel setup also hard-depends on loaded
  objects (certificate-to-node mapping), so a standby cannot serve it
  anyway.
- **Mobile device listener**: activation-only (same reasoning, negligible
  reconnect cost).

## 6. Validation plan (phase 0 acceptance)

Executed by the `tests/ha/` harness against two `ha-node-sim` instances
(real `halease.cpp` code + monotonic watchdog + fence flag + heartbeat stub +
continuous role-stamped writes into a scratch table) and PostgreSQL, with
each sim's DB traffic routed through a harness-controlled TCP proxy (pause =
partition, stall = hung query).

| # | Scenario | Pass criterion |
|---|---|---|
| 1 | kill -9 of active | standby promotes ≤ V + R + acquire time; new term |
| 2 | graceful handover | promotion without waiting for expiry; no write gap beyond drain |
| 3 | DB cut from active only, channel healthy | active fences ≤ fence_deadline; standby promotes; **channel does not delay promotion** |
| 4 | DB cut from standby only | no role change |
| 5 | channel cut, both healthy | no role change; standby marks dirty |
| 6 | SIGSTOP active > V, resume | zero role-stamped writes with the old term after the new term's first write |
| 7 | hung lease refresh (proxy stall) | treated as refresh failure; fences by deadline |
| 8 | simultaneous start ×100 | exactly one winner per term, every time |
| 9 | long transaction crossing promotion | any old-term commit lands within the documented residual window; detectable in scratch table |
| 10 | ID collision probe | allocator without rebase collides (demonstrates hazard); with rebase never |

Invariant checks run on the scratch table after every scenario: for each
term exactly one writer GUID+incarnation; term sequence strictly increasing;
no old-term write after the fence point except within the measured residual
window (which is recorded and reported).

Measured outputs feeding back into section 2.7: detection latency, promotion
latency, refresh round-trip distribution, observed residual window.

### 6.1 Measured results (2026-07-03, all 10 scenarios pass)

Environment: two sims + PostgreSQL 16 on one host, harness TCP proxies,
R = 1 s, V = 5 s, M = 3 s scaled to 1 s. Latencies match theory exactly:

| Scenario | Measured | Theory |
|---|---|---|
| Crash promotion (kill -9) | 6.0 s | V + R |
| Graceful handover (release, no channel notification yet) | 1.0 s | ≤ R |
| Self-fence on DB partition | 4.0 s | V − M |
| Self-fence on stalled connection (watchdog) | 4.0 s | V − M |
| SIGSTOP > V: peer promotion | 6–7 s into pause | V + R |
| SIGSTOP > V: stale writes after resume | 0 | 0 |
| Start race (10 rounds) | 1 winner/round, term +1/round | — |
| ID allocator without rebase | 5/5 collisions | hazard confirmed |
| ID allocator with rebase | 0 collisions | — |

Extrapolated to production defaults (R = 5 s, V = 20 s, M = 3 s): crash
failover ≈ 25 s, self-fence ≈ 17 s, graceful handover ≈ 5 s (near-instant
once the phase 2 channel handover notification exists). Two design changes
came out of validation: the fence-deadline watchdog must be a dedicated
non-DB thread (section 2.6), and activation-time allocator rebase is
demonstrably load-bearing, not defensive (section 4).

## 7. Audits (step 0.5, audited 2026-07-03)

### 7.1 Subsystem lifecycle inventory

Every call in `Initialize()` (main.cpp), classified for the phase-1 startup
split. "Passive-safe" = runs unchanged in phase 1. "Split" = the load part is
passive, the listed workers/writes move to `activate()`. "Activate-only" =
the whole call moves to phase 2.

**Passive-safe (read-only loads, no worker threads):**
`InitIdTable`, `LoadScripts`, `LoadMIBTree`, `PersistentStorageInit`,
`InitCryptography`, `InitCertificates`, `LoadTwoFactorAuthenticationMethods`,
`LoadAuthenticationTokens`, `InitEventSubsystem` (structures only — the
processor threads are `StartEventProcessor`), `LoadAlarmCategories`,
`InitIncidentManager`, `LoadGeoAreas`, `LoadNetworkDeviceDrivers`,
`ObjectsInit`, `LoadObjectCategories`, `LoadSshKeys`, `LoadActions`,
`LoadNotificationChannelDrivers`, `InitMappingTables`,
`InitUserAgentNotifications`, `LoadPhysicalLinks`,
`LoadPerfDataStorageDrivers`, `LoadWebServiceDefinitions`,
`LoadAssetManagementSchema`, `LoadObjectQueries`,
`LoadTopologyExcludedSubnets`, `LoadTrapMappings`, `InitWebAPI` (config/route
build only), `InitClientListeners` (pool + session manager; socket opens
separately), `InitMobileDeviceListeners` (pool only), `ValidateScripts`.
Caveats: `InitAuditLog` is passive except an optional external-syslog
announce (`AuditLog.External.Server`); `LoadHelpDeskLink` and
`LoadNetXMSModules` call module init code that may itself misbehave — the
`NXMODULE` contract extension (section 5.2) is the fix.

**Whitelisted passive threads:** log writer, `WatchdogInit`,
`LocalAdminListenerThread` (TCP 21784 loopback), HA manager + HA watchdog,
peer channel, restricted-role client/tunnel/WebAPI listeners.

**Needs initPassive/activate split:**

| Function | Passive part | Activate part |
|---|---|---|
| `InitAlarmManager` (alarm.cpp:3020) | load alarm list, seed `s_stateChangeLogRecordId` | `AlarmDbWriterThread`, `WatchdogThread` (expires alarms → events), `RootCauseUpdateThread` |
| `LoadObjects` (objects.cpp:1422) | object load, module load hooks | `CacheLoadingThread`, `ApplyTemplateThread`, conditional fix-up writes (custom-attribute prune, asset/link repair, comment-macro expansion) — all mutate objects |
| `InitUsers` / `LoadUsers` | load users/groups | `AccountStatusUpdater` (60 s timer: modifies accounts, writes audit log); first-run "Everyone"/system-account creation |
| `LoadNotificationChannels` | channel configuration load, seed `s_notificationId` | per-channel worker threads (send externally!) + driver health check thread |
| `LoadEventForwarders` | forwarder configuration load | per-forwarder worker threads + health check thread |
| `InitAIAssistant` | function/component registration | `AI-TASKS` pool + `AITaskSchedulerThread` |
| `StartWebAPI` | listener (restricted-role handler) | full route handling + `AddUniqueRecurrentScheduledTask` write |

**Activate-only (active-node behavior; every one either writes the DB,
opens a contended ingest port, or reaches out to the network):**

| Function | Why |
|---|---|
| `StartDBWriter` | all deferred-write threads (generic, raw-data, N × idata) |
| `StartEventProcessor` | event log INSERTs, EPP execution, actions/notifications |
| `InitDataCollector` | polling engine: `ItemPoller`, `CacheLoader`, DATACOLL/THREVT pools, threshold-repeat rescheduling |
| `StartHouseKeeper` | destructive retention DELETEs (~53 write sites) |
| `StartV5DataMigration` | one-time destructive migration; single-node only |
| `CheckForMgmtNode` | can create the management node (object + DCI tables + `EVENT_NODE_ADDED`), flips `setLocalMgmtFlag` on existing nodes |
| `ImportLocalConfiguration` | inserts/updates events, templates, rules from share/templates |
| `InitializeTaskScheduler` + `AddUniqueRecurrentScheduledTask` calls | executes scheduled jobs; the Add calls INSERT system tasks |
| `StartSnmpTrapReceiver` / `StartSNMPAgent` / `StartSyslogServer` / `StartWindowsEventProcessing` / `StartOtelLogProcessing` | ingest listeners (UDP 162/161/514) + log writer threads |
| `StartPackageDeploymentManager` | **re-queues PENDING deployment jobs at startup** and pushes packages to agents — must never run passive |
| `ExecuteStartupScripts` | arbitrary NXSL with full server access |
| `CheckNodeCountRestrictions` | can unmanage nodes (persists) + reschedules hourly |
| `CALL_ALL_MODULES(pfServerStarted)` | module-defined active behavior |
| direct threads: `Syncer`, `PollManager`, `DiscoveredAddressPoller`, `BeaconPoller`, `ISCListener`, `ReportingServerConnector`, `LDAPSyncThread`, `ServerStatCollector` | pollers/writers/outbound connectors |

Resolved placement for the opportunistic fence-deadline checks (open
question 4): DB writer dequeue loops (dbwrite.cpp), event processor loop
(evproc.cpp), `ItemPoller`/DATACOLL dispatch, scheduler task launch
(schedule.cpp), notification channel worker send, alarm DB writer dequeue.

### 7.2 ID allocator inventory (activation rebase list)

Two families, both seeded from DB maxima exactly once at startup and
incremented in memory. The `FirstFree*`/`Last*` config high-water marks some
of them persist are **write-back only** — they never re-read the DB, so a
long-running standby is stale regardless; every entry below must be
re-seeded during activation (design section 4).

**A. `s_freeIdTable[]` (id.cpp, ~40 `IDG_*` groups via `CreateUniqueId`):**
objects, events, DCIs, SNMP traps, actions, thresholds, users, groups,
alarms, alarm notes, packages, object tools, scripts, agent configs, graphs,
business service tickets/checks/records, mapping tables, summary tables,
alarm categories, UA messages, rack elements, physical links, web service
definitions, object categories, geo areas, SSH keys, object queries,
maintenance journal, auth tokens, package deployment jobs, incidents (+
comments, activity), storage-class migration tasks, trusted devices,
connection history, AI saved prompts. Rebase = re-run `InitIdTable()`
(hybrid config/max entries: FirstFreeObjectId, FirstFreeDCIId,
FirstFreeAlarmId, FirstFreeDeploymentJobId — max() wins, safe).

**B. Standalone log/record counters (each with its own seed site):**

| Counter | File | Seeds from |
|---|---|---|
| `s_eventId` | events.cpp (`LoadLastEventId`) | event_log.event_id / LastEventId |
| `s_trapId` | snmptrapd.cpp (`LoadTrapId`) | snmp_trap_log.trap_id |
| `s_msgId` | syslogd.cpp | syslog.msg_id |
| `s_eventId` | winevent.cpp | win_event_log.id |
| `s_recordId` | otellog.cpp | otel_log.id |
| `s_recordId` | audit.cpp | audit_log.record_id |
| `s_logRecordId` | actions.cpp | server_action_execution_log.id |
| `s_notificationId` | notification_channel.cpp | notification_log.id |
| `s_assetChangeLogId` | asset_management.cpp | asset_change_log.record_id |
| `s_logRecordId` | cert.cpp (`InitCertificates`) | certificate_action_log.record_id |
| `s_stateChangeLogRecordId` | alarm.cpp (`LoadAlarms`) | alarm_state_changes.record_id |
| `s_taskId` | schedule.cpp (`InitializeTaskScheduler`) | scheduled_tasks.id |
| `s_logRecordId` | ai_tasks.cpp | ai_task_execution_log.record_id |
| journal `seq` | halease/journal (section 3.2) | ha_change_journal.seq |

**Defect found by the audit:** AI task IDs (`s_taskId`, ai_tasks.cpp) are
seeded purely from the `AITask.LastTaskId` config value with **no max()
backstop** — stale even across a plain crash, not just HA. Phase 1 adds the
max() backstop when implementing the rebase.

Immune (query max() on demand, no in-memory counter): config repository IDs
(market.cpp), agent config sequence numbers (session.cpp), log-view record
bounds (loghandle.cpp). Ephemeral runtime counters (tunnel IDs, agent
connection IDs) reset per process by design.

Implementation shape for phase 1: gather every seed site behind one
`RebaseAllocators()` entry point called from the activation sequence
(section 5.2), so future counters cannot be forgotten — each subsystem
registers its seed function rather than being listed in a central table.

## 8. Open questions

1. DB2 epoch-seconds expression — finalize during 0.2 (or drop DB2 from
   supported cluster syntaxes if testing is impractical).
2. MSSQL: `DATEDIFF_BIG` requires SQL Server 2016+; confirm this is within
   the supported floor, otherwise fall back to a 2038-limited `DATEDIFF`
   with a documented cap.
3. Whether journal writes for object saves can always share the save
   transaction on every syntax (autocommit paths in `Syncer`), or whether a
   write-journal-first ordering is needed as fallback.
4. ~~Exact placement of opportunistic fence-deadline checks (section 2.6)~~
   — resolved by the 7.1 audit: DB writer dequeue, event processor loop,
   poller dispatch, scheduler task launch, notification send, alarm writer
   dequeue.
