# Traffic Observer Integration — Detailed Design

**Status:** Approved design, ready for implementation planning
**Scope:** Integration of external traffic analysis systems into the NetXMS object model.
First (and initially only) connector: ntopng via REST API v2.

This document is the detailed, code-grounded successor of the initial high-level design
draft ("NetXMS ↔ ntopng Integration — Motivation and High-Level Design"). Motivation and
non-goals from that draft still apply; this document records the resolved design decisions
and maps every part of the design onto existing NetXMS mechanisms.

---

## 1. Summary of Resolved Decisions

| # | Decision | Resolution |
|---|----------|------------|
| 1 | Framework shape | Dedicated first-class object classes with a thin connector seam ("C-lite"): generic classes + module-provided connector interface, ntopng as first connector |
| 2 | Class naming | `TrafficObserver` (parent, class id 40), `ObservationPoint` (child, class id 41) |
| 3 | Per-node DCI mechanism | Dedicated DCI origin `DS_TRAFFIC_OBSERVER` (15); user-authored templates with instance discovery; no automatic DCI creation |
| 4 | "Observed by" representation | Host records owned by `ObservationPoint` (dedicated table + in-memory index) with reverse accessor on Node |
| 5 | Presence semantics | Host aged out of active table maps to `DCE_NO_SUCH_INSTANCE` + standard instance grace period; optional explicit `Host.Present` metric |
| 6 | Node views | Live queries through server/connector, independent of DCIs; DCIs are opt-in for trending/thresholds |
| 7 | Config sync (NetXMS → analyzer) | Module-registered scheduled task handler, one recurrent system task per observer, `objectId`-targeted |
| 8 | Credentials | `cloud_domains` pattern: `connector_name` + opaque JSON credentials blob, sensitive-data gating on NXCP/JSON surfaces |
| 9 | UI registration | `ObjectViewDescriptor` SPI gated on server component id, so views appear only against servers with a traffic connector loaded |
| 10 | Edition/product gating | Unified capability mechanism: connector reports capability set at connect time (ntopng Community vs Pro = two capability sets, same as two different products) |
| 11 | Genericity review response (post-Phase 2) | Accepted: `HOST_SET_AUTHORITATIVE` capability (§4.3/§6.5), `sampling_rate` on points (§2.2), observer→node link (§2.1), `external_id`/`host_key` stability contracts (§2.2/§2.3), sync trimmed to host aliases only (§7). Rejected/deferred: per-metric "estimated" flag, ingestion-lag hint, bulk hooks, `match_context` column (§15) |

Architectural precedents this design deliberately follows:

- **CloudDomain / Resource / CloudConnectorInterface** (`src/server/core/cloud_domain.cpp`,
  `src/server/core/resource.cpp`, `src/server/include/cloud-connector.h`,
  `src/server/core/cloud_connector.cpp`, `src/server/docker/`) — object pair driven by a
  named connector module, JSON credentials, discovery/reconciliation poll, node linking,
  dedicated DCI origin (`DS_CLOUD_CONNECTOR`).
- **WirelessDomain / AccessPoint / WirelessControllerBridge** (`src/server/core/wireless_domain.cpp`,
  `src/server/core/accesspoint.cpp`, `src/server/wlcbridge/`) — child-object reconciliation
  with grace-period retirement, and the bulk-fetch-with-short-TTL-cache pattern for serving
  many DCI reads from one external API call (`wlcbridge/ruckus.cpp`).

## 2. Object Model

### 2.1 TrafficObserver (OBJECT_TRAFFICOBSERVER = 40)

Represents one external traffic analysis instance (one ntopng daemon).

- **Base classes:** `DataCollectionTarget` (it carries health and sync-status DCIs),
  `Pollable` with `Pollable::STATUS | Pollable::CONFIGURATION`.
- **Persistent attributes** (table `traffic_observers`):
  - `connector_name` — registered connector, e.g. `ntopng` (same semantics as
    `cloud_domains.connector_name`)
  - `credentials` — opaque JSON blob owned by the connector: endpoint URL, auth token,
    TLS verification flags, timeouts, cache TTL, connector-specific options. Schema never
    changes when a connector grows an option.
  - `zone_uin` — default zone for host-to-node matching: `>= 0` = specific zone
    (0 = default zone), `-1` = match in all zones. Required for correctness in zoned
    deployments where IP ranges overlap; individual observation points may override
    (§2.2, §5.3).
  - `node_id` — optional link to the Node representing the analyzer itself (0 = not
    linked). Many observers are devices NetXMS already monitors (the ntopng box, or —
    for future device-based connectors — a firewall/controller); the link provides
    operational context (health, maintenance state) without duplicating it. Precedent:
    `resources.linked_node_id`.
  - `removal_policy`, `grace_period` — retirement policy for disappeared observation
    points (mirrors `CloudDomain::m_removalPolicy` / `m_gracePeriod`)
  - `sync_config` — JSON: which sync surfaces are enabled, scope (source containers /
    subnets), per-surface options (see §7)
  - `last_discovery_status`, `last_discovery_time`, `last_discovery_msg`
- **Runtime state:** connection state, detected backend version/edition string, capability
  set (see §4.3), last error. Exposed via NXCP fill and NXSL.
- **Valid parents:** `ServiceRoot`, `Container`, `Collector` (mirroring `CloudDomain`
  placement rules in `IsValidParentClass()`, `src/server/core/objects.cpp`).

### 2.2 ObservationPoint (OBJECT_OBSERVATIONPOINT = 41)

Represents one monitored interface / observation point of the analyzer (ntopng `ifid`).
IPFIX terminology is used deliberately — the concept is not ntopng-specific.

- **Base classes:** `DataCollectionTarget` (carries per-point traffic DCIs), `Pollable`
  with `Pollable::STATUS`.
- **Persistent attributes** (table `observation_points`):
  - `observer_id` — owning TrafficObserver
  - `external_id` — connector-side point identity (ntopng `ifid`), opaque string.
    **Stability contract:** this is a connector-*synthesized* durable identity, not
    necessarily a backend-native handle. Reconciliation and DCI instances key on it, so
    it must survive backend restarts and renumbering — a flow-collector connector must
    compose something durable (e.g. exporter IP + ifName) rather than expose a raw
    ifIndex. Changing a connector's composition rule is a breaking change for existing
    objects.
  - `point_type` — connector-reported type string (packet capture / NetFlow / sFlow /
    ZMQ / view / pcap)
  - `in_scope` — user opt-in flag; only in-scope points run host matching and host data
    collection. Default: false (explicit opt-in, per the draft's rationale — avoids noise
    from replay/test interfaces).
  - `zone_uin` — per-point zone override for host matching: `-1` = inherit from the
    observer (default), `>= 0` = explicit zone. The point is the physical locus of
    observation, so zone affinity is naturally per-point — one analyzer instance can
    host a SPAN interface in one zone and NetFlow collector interfaces fed by exporters
    in others.
  - `sampling_rate` — connector-reported sampling at this point: 0 = unknown,
    1 = unsampled (packet capture), N = 1:N sampling. Consumers: display context in the
    point views and the Node traffic tab (sampling is the most common explanation for
    divergence between observation points, §6.4 instance display), a column in the
    `Traffic.ObservationPoints` internal table so instance filter scripts can express
    policies like "collect only from unsampled points", and NXSL. Deliberately NOT a
    per-metric "estimated" flag: estimation is a property of the point (its sampling),
    not of individual metrics, and the DCI layer has no accuracy concept to carry one.
  - `local_networks` — CIDR list as reported by the analyzer (informational; also input
    for sync conflict detection)
  - `state` / `provider_state` — normalized + raw state, `Resource`-style
- **Created and retired only by the observer's configuration poll** (no manual creation);
  same lifecycle as `AccessPoint` under `WirelessDomain`.
- **Valid parent:** `TrafficObserver` only.

### 2.3 Host records (NOT objects)

Matched hosts are deliberately not NetXMS objects — thousands per point, high churn.
The match result set is tuples `(observation point, host key, node)`:

- Table `observation_point_hosts`: `point_id`, `host_key` (connector-defined opaque
  identity; for ntopng `ip@vlan`), `ip_addr`, `vlan`, `node_id`, `first_seen`, `last_seen`.
  **Host key contract:** `host_key` is the authoritative host identity and the only
  qualifier core keys on — a connector encodes any extra identity dimensions (VLAN, VRF,
  tenant, MPLS context, ...) into it opaquely; core matches on `ip_addr` only and never
  parses the key. The `vlan` column is denormalized display data, not a semantic
  constraint — a VRF-aware connector works without schema changes. Same stability
  contract as `external_id` (§2.2).
- Loaded into an in-memory index at startup; maintained by the matching pass (§5.3).
- Persistence matters: DCI instance discovery and the Node traffic tab must survive a
  server restart without waiting for a full re-match cycle.
- **Reverse accessor on Node** (`Node::getObservationPoints()` backed by the index), used
  by: DCI collection dispatch, the `Traffic.ObservationPoints` internal table (§6.4), the
  Node traffic tab, and the NXSL `observationPoints` attribute (§11). Nodes do not store
  matches; ownership stays with the point, which knows when it is retired or re-matched.

This resolves the draft's rename/renumber open question structurally: when a node's IP
changes, the next matching pass reassigns `node_id` on the host records; DCI instances
that no longer resolve disappear through the standard instance-retention grace period
(`DataCollectionTarget::updateInstances`, `src/server/core/dctarget.cpp`). No bespoke
orphan-DCI policy is needed.

## 3. Class Implementation Checklist (C++ / Java)

Standard new-class touch points, verified against current code:

**C++ server:**
- `include/nxcldefs.h` — `OBJECT_TRAFFICOBSERVER 40`, `OBJECT_OBSERVATIONPOINT 41`
- `src/server/include/nms_objects.h` — class declarations (`NXCORE_EXPORTABLE`)
- `src/server/core/traffic_observer.cpp`, `src/server/core/observation_point.cpp`
  (+ `Makefile.am`, MSVC project)
- `src/server/core/netobj.cpp` — append to positional `s_classNameW[]` / `s_classNameA[]`
- `src/server/core/objects.cpp` — index add/remove switches (`NetObjInsert`,
  `NetObjDeleteFromIndexes`), `LoadObjectsFromTable<>` calls, `IsValidParentClass()`,
  JSON create path (`CreateObjectFromJSON`)
- `src/server/core/session.cpp` — NXCP create path (`ClientSession::createObject`)
- `src/server/include/nms_script.h` + `src/server/core/nxsl_classes.cpp` — NXSL classes
  (declaration, `getAttr`, global instance, `createNXSLObject` in each class)
- `doc/internal/debug_tags.txt` — new tags (proposed: `traffic.poll`, `traffic.match`,
  `traffic.sync`, `traffic.connector`)

**Java client library (`src/client/java/netxms-client`):**
- `AbstractObject.java` — `OBJECT_TRAFFICOBSERVER = 40`, `OBJECT_OBSERVATIONPOINT = 41`
- `objects/TrafficObserver.java`, `objects/ObservationPoint.java` (model on
  `WirelessDomain.java` / `Collector.java`, implement `PollingTarget`)
- `NXCSession.createObjectFromMessage` — factory cases
- `NXCObjectCreationData` / `NXCObjectModificationData` + `NXCSession.createObject` /
  `modifyObject` — connector name, credentials, zone, sync config fields
- `src/java-common/netxms-base/.../NXCPCodes.java` — new `VID_*` / `CMD_*` mirrored from
  C++ (per repository rule, also update `NXCPMessageCodeName()` in
  `src/libnetxms/nxcp.cpp` for any new command code)

**nxmc UI (beyond views, see §8):**
- `ObjectIcons.java` + two PNGs under `resources/icons/objects/`
- `ObjectBrowser.calculateClassFilter()` — visibility in Infrastructure subtree
- `ObjectSelectionFilterFactory.java` — selection dialogs
- `ObjectCreateMenuManager.java` — "Create traffic observer..." action with a bespoke
  `CreateTrafficObserverDialog` (name + connector selection from the server-provided
  connector list; the pattern is `CreateCloudDomainDialog`)
- Property pages registered in `ObjectPropertiesManager` (§8.3)

## 4. Connector Framework

### 4.1 Interface

New header `src/server/include/traffic-connector.h`, struct `TrafficConnectorInterface`,
following `cloud-connector.h` conventions (C struct of function pointers, status enum,
plain-data descriptor structs). Registry in `src/server/core/traffic_connector.cpp`
mirroring `cloud_connector.cpp` (`FindTrafficConnector(name)`,
`GetTrafficConnectorNames()`, initialization via `ENUMERATE_MODULES`). New pointer field
`trafficConnector` in `NXMODULE` (`src/server/include/nxmodule.h`).

Signatures **frozen by the Phase 0 validation spike** (see
`Traffic_Observer_Phase0_Validation.md`, §8 there for the change log against the
pre-validation draft). All data functions receive the observer's credentials JSON,
as cloud connector functions do:

```cpp
enum class TrafficConnectorStatus
{
   SUCCESS = 0,
   CONNECTOR_UNAVAILABLE = 1,
   NOT_IMPLEMENTED = 2,
   API_ERROR = 3,
   AUTH_ERROR = 4
};

enum class TrafficMetricLevel
{
   OBSERVER = 0,
   POINT = 1,
   HOST = 2
};

struct TrafficBackendInfo
{
   char product[64];          // e.g. "ntopng"
   char version[64];          // e.g. "6.7.260504"
   char edition[64];          // e.g. "Community"
   uint64_t capabilities;     // TRAFFIC_CAPABILITY_* bitmask
};

struct ObservationPointDescriptor
{
   char externalId[64];       // connector-side point identity (ntopng ifid)
   char name[MAX_OBJECT_NAME];
   char type[32];             // "packet" / "zmq" / "pcap" / "view" / ...
   int16_t state;             // OBSERVATION_POINT_STATE_*
   char providerState[64];    // raw backend state string
   StringList *localNetworks; // CIDRs reported by the backend (owned)
   ObservationPointDescriptor *next;   // next sibling (linked list, caller owns)
};

// TrafficHostEntry flags
#define TRAFFIC_HOST_LOCAL        0x0001
#define TRAFFIC_HOST_MULTICAST    0x0002
#define TRAFFIC_HOST_BROADCAST    0x0004
#define TRAFFIC_HOST_BLACKLISTED  0x0008

struct TrafficHostEntry
{
   char hostKey[128];         // connector-defined identity, ntopng: "ip@vlan"
   InetAddress ipAddr;
   int32_t vlan;
   MacAddress macAddr;
   wchar_t name[192];         // backend label (alias or resolved name)
   uint32_t flags;            // TRAFFIC_HOST_*
   time_t firstSeen;
   time_t lastSeen;
};

struct TrafficMetricDefinition   // NOT "MetricDefinition" — already taken by cloud-connector.h
{
   wchar_t name[MAX_PARAM_NAME];
   wchar_t displayName[256];
   wchar_t unit[64];
   wchar_t description[1024];
   bool isTable;
   uint64_t requiredCapability;  // 0 = always available
};

struct TrafficConnectorInterface
{
   TrafficConnectorStatus (*Initialize)();
   // No Shutdown entry point: connectors are provided by modules, and cleanup
   // (if any) goes into the module's pfShutdown hook

   // Instance level
   TrafficConnectorStatus (*TestConnection)(json_t *credentials, TrafficBackendInfo *info);
      // fills backend version, edition/build string, capability set
   uint64_t (*GetCapabilities)(json_t *credentials);

   // Discovery
   ObservationPointDescriptor *(*DiscoverPoints)(json_t *credentials);   // linked list, caller owns

   // Observer / point level data
   DataCollectionError (*GetObserverMetric)(const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
   DataCollectionError (*GetPointMetric)(const char *pointId, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
   DataCollectionError (*GetPointTable)(const char *pointId, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials);

   // Host level (pointId + hostKey addressing); connector pages and caches internally
   ObjectArray<TrafficHostEntry> *(*GetActiveHosts)(const char *pointId, json_t *credentials);
      // input for the matching pass and the live views; caller owns (Ownership::True)
   DataCollectionError (*GetHostMetric)(const char *pointId, const char *hostKey, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
   DataCollectionError (*GetHostTable)(const char *pointId, const char *hostKey, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials);

   // Metric catalog for the DCI editor
   ObjectArray<TrafficMetricDefinition> *(*GetMetricDefinitions)(TrafficMetricLevel level, json_t *credentials);

   // Push direction (may be null = unsupported). Deliberately the only sync surface in
   // v1 - further surfaces (host pools, local networks) are added to the interface when
   // a connector that can actually serve them lands; no dead entry points.
   TrafficConnectorStatus (*SyncHostAliases)(const StringMap& aliases, json_t *credentials);   // hostKey -> name
};
```

Design rules for the interface:

- **Shaped by consumers, not by ntopng's API.** Functions exist because a view, a DCI
  path, the matching pass, or a sync surface needs them.
- **Per-host granularity at the interface; batching inside the connector.** The connector
  satisfies `GetHostMetric`/`GetHostTable` from an internal short-TTL bulk cache (§5.4).
  Only the connector knows its backend has a bulk endpoint. This is the proven
  `wlcbridge/ruckus.cpp` pattern (`AccessPointCacheEntry`, `GetAccessPointData`).
- **This is an in-tree C interface between core and in-tree modules**, not a public SDK
  promise. It will be revised when a second connector lands; that is accepted and cheap,
  unlike object-schema changes.
- Anything of doubtful generality (e.g. host pool semantics) sits behind a capability bit
  rather than being force-abstracted.

### 4.2 Registration and component id

When at least one traffic connector registers successfully, core registers server
component id `TRAFFIC` (consumed by the client via
`NXCSession.isServerComponentRegistered()` to gate UI, §8.2).

### 4.3 Capabilities

`GetCapabilities()` returns a bitmask; proposed initial bits:

| Bit | Meaning |
|---|---|
| `HOST_L7` | per-host L7 protocol breakdown available |
| `HOST_PEERS` | per-host top peers available |
| `POINT_L7` | per-point L7 breakdown |
| `POINT_TOP_TALKERS` | per-point top talkers |
| `POINT_DSCP` | per-point DSCP breakdown |
| `HISTORICAL_TIMESERIES` | continuous historical polling path (ntopng Pro/ClickHouse) |
| `SYNC_HOST_ALIASES` | push of host aliases supported |
| `HOST_SET_AUTHORITATIVE` | `GetActiveHosts` returns the complete active host set (live table with idle age-out), not a top-N approximation from a query/store-based backend |

`HOST_SET_AUTHORITATIVE` exists because host-set semantics is the main axis separating
backend types: ntopng-style live tables are authoritative; query-based flow stores
(Akvorado/Kentik-class) answer "top N in window W". When a connector does not report the
bit, presence-derived behavior must degrade (§6.5): `Host.Present` is not offered by the
metric catalog, and absence from one poll must not be treated as a presence signal.

**Edition gating and product gating are the same mechanism**: the ntopng connector reports
different capability sets for Community vs Pro/Enterprise (detected in `TestConnection`),
exactly as a different product's connector would report its own set. Core and UI consume
capabilities only — no ntopng edition logic outside the connector. The capability set is
cached on the observer object and refreshed on each configuration poll; UI grays out or
hides features accordingly.

Set reported for ntopng (validated on Community 6.7): `HOST_L7`, `HOST_PEERS`,
`POINT_L7`, `POINT_TOP_TALKERS`, `POINT_DSCP`, `SYNC_HOST_ALIASES`,
`HOST_SET_AUTHORITATIVE`. Not reported: `HISTORICAL_TIMESERIES` (Pro-only,
unvalidated).

### 4.4 ntopng connector module

New module `src/server/ntopng/` (pattern: `src/server/docker/`), direct libcurl + jansson
(`InitializeLibCURL()`, `ByteStream::curlWriteFunction`, typed `json_object_get_*`
helpers). No agent involvement — the server process makes the HTTP calls, as `docker/`,
`jira/`, and `ai_provider*.cpp` already do. The web-service DCI subsystem (agent-proxied,
`src/server/core/websvc.cpp` + `src/agent/core/websvc.cpp`) is NOT used by this design;
it remains available to users as a zero-code way to poll arbitrary ntopng endpoints not
covered by the integration.

ntopng REST API v2 mapping — **validated against a live 6.7 Community instance**
(full behavioral findings, response shapes and caveats in
`Traffic_Observer_Phase0_Validation.md`):

| Interface function | ntopng endpoint (validated) |
|---|---|
| `TestConnection` | `/lua/rest/v2/get/ntopng/about.lua` (product, `version`, `release` = edition); health details from `get/system/status.lua` + `get/system/health/stats.lua` |
| `DiscoverPoints` | `/lua/rest/v2/get/ntopng/interfaces.lua` (+ `get/network/networks.lua?ifid=` per point for local networks) |
| `GetPointMetric` | `/lua/rest/v2/get/interface/data.lua?ifid=` (throughput, packets, hosts, flows, drops, TCP quality, uptime in one call) |
| `GetPointTable` | L7: `get/interface/l7/data.lua`; top talkers: `get/interface/top/hosts.lua`; DSCP: `get/interface/dscp/stats.lua` |
| `GetActiveHosts` | `/lua/rest/v2/get/host/active.lua?ifid=` — paged (`perPage`/`currentPage`, no total count, iterate until short page; `all=1` is broken in 6.7) |
| `GetHostMetric` / `GetHostTable` | served from the two-tier host cache (§5.4); detail via `get/host/data.lua` (includes full `ndpi.*` L7 subtree), peers via `get/flow/aggregated_live_flows.lua?host=&aggregation_criteria=client\|server` |
| local networks push | **not implementable** — ntopng has no REST endpoint for setting the local-networks list; surface dropped from the v1 interface (§7) |
| `SyncHostAliases` | `POST set/host/alias.lua` (`{"host","vlan","custom_name"}`; empty name clears) |
| host pools push | endpoints validated (`add/edit/delete/host/pool.lua`, members as `CIDR@vlan`) but surface dropped from the v1 interface — Community limits make it a toy, Pro unvalidated (§7) |

Authentication: ntopng API token (`Authorization: Token <token>`), stored in the
credentials blob — validated, including on admin-only mutating endpoints. Auth
failures and unknown endpoints both surface as a 302 redirect to the login page
(no 401/404 distinction): the connector never follows redirects and treats any
302 as an error. Parameter-validation failures can return a plain-text Lua error
instead of the JSON envelope; the response parser must tolerate non-JSON bodies.
TLS verification on by default; per-observer override in the blob
(`CURLOPT_SSL_VERIFYPEER/HOST`, same flags semantics as `WSF_VERIFY_*`).

ZMQ is explicitly not an option: the `--with-zeromq` configure flag is vestigial (no
server ZMQ code exists in the tree).

## 5. Polling and Data Flow

All polling uses the standard `Pollable` machinery — no new scheduler code.
`QueueForPolling` (`src/server/core/poll.cpp`) picks the objects up automatically;
`CMD_POLL_OBJECT` (manual "poll now") works out of the box; per-object interval overrides
already exist via `SysConfig:Objects.StatusPollingInterval` /
`...ConfigurationPollingInterval` custom attributes (`src/server/core/pollable.cpp`).

### 5.1 TrafficObserver::statusPoll (default 60 s)

Cheap health check: `TestConnection` (or a lightweight system-stats call), updates
connection state, API latency measurement, generates up/down events (§12), refreshes
health metric cache. Follows the `pollerLock(status)` / `pollerUnlock()` +
shutdown/delete-guard prologue used by `WirelessDomain::statusPoll`.

### 5.2 TrafficObserver::configurationPoll (default 3600 s, forceable)

1. Refresh backend version/edition and capability set.
2. `DiscoverPoints`; reconcile `ObservationPoint` children: create new
   (`make_shared` + `NetObjInsert`), update changed, retire missing via
   grace period / removal policy — the `WirelessDomain::configurationPoll` /
   `CloudDomain` reconciliation loop.
3. For each **in-scope** point: run the host matching pass (§5.3).
4. `executeHookScript(L"ConfigurationPoll")` as usual.

### 5.3 Host matching pass

For each in-scope point: `GetActiveHosts(pointId)`, then for each returned host entry
match against NetXMS nodes in the observer's zone:

- Primary key: IP address → `FindNodeByIP(zoneUIN, ...)` (the exact mechanism
  `CloudDomain` uses for `linkHint` resolution, `cloud_domain.cpp:491-512`).
- Zone selection: the point's effective zone (per-point override, else observer default).
  With an explicit zone, matching uses the zone-scoped lookup. With all-zones (`-1`),
  matching searches every zone — precedent: syslog/trap source resolution via
  `FindNodeByIP(zoneUIN, allZones, addr)` under `AF_TRAP_SOURCES_IN_ALL_ZONES` — but
  with a stricter collision rule: if the same IP resolves to nodes in more than one
  zone, the host is NOT linked; the collision is reported in the poller output and
  counted in a diagnostic metric. (Syslog's first-match behavior is acceptable for
  event attribution; it is not acceptable for creating a persistent observed-by
  relationship.) Deployments with overlapping addressing must configure explicit
  zones; all-zones is the convenience mode for globally unique addressing.
- v1 candidate IP policy: a host IP matches a node if it equals any unicast, non-loopback
  address of the node's interfaces or the node's primary IP. (Per-observer restriction
  policies — management IP only, interface role filters — are a possible v2 refinement;
  the policy hook lives in one function.)
- VLAN: the host key preserves the connector's identity including VLAN (`ip@vlan`).
  Multiple `(IP, VLAN)` records matching the same node are all recorded — each becomes
  its own host record and, if the user collects per-host DCIs, its own DCI instance.
  No merging in v1 (same honesty principle as the multi-observation-point decision).
- Update `observation_point_hosts`: insert new matches, refresh `last_seen`, reassign
  `node_id` where resolution changed, age out records not seen for the retention period
  (housekeeper task).

Cost: one bulk API call per in-scope point per configuration poll — O(points), not
O(nodes).

### 5.4 Collection-time caching

Per-point bulk host cache inside the connector, **two tiers** (Phase 0 finding: the
ntopng active-host list carries bytes/throughput/flows but not packets, TCP quality
or L7 — those need a per-host detail call):

1. **Active-list tier** (per point, TTL ~30 s, configurable in the credentials
   blob): one paged `host/active.lua` sweep; serves the matching pass, presence,
   and byte/flow-level host metrics — O(in-scope points) calls per TTL.
2. **Detail tier** (per host, same TTL, lazily populated): `host/data.lua`
   fetched only for host keys actually referenced by a DCI or live query within
   the TTL — O(hosts-with-detail-DCIs) per TTL, not O(active hosts).

All `GetHostMetric`/`GetHostTable` calls within the TTL are served from cache; one
thread refreshes while others wait (the `s_apCache` + `processing` flag pattern from
`ruckus.cpp:261-353`). Consequence: steady-state API load on the analyzer is bounded
by configured collection (points + hosts with detail DCIs), regardless of network
size. This answers the draft's bulk-polling-scale question structurally.

## 6. DCI Design

### 6.1 New origin DS_TRAFFIC_OBSERVER = 15

Touch points (each has a fresh `DS_CLOUD_CONNECTOR`/`DS_OTLP` edit to crib from):

- `include/nxcldefs.h` — constant
- `src/server/core/datacoll.cpp` — `GetItemData()` and `GetTableData()` cases
  (note: this gives traffic tables a first-class dispatch path; table origins are
  currently limited to INTERNAL / NATIVE_AGENT / SNMP / SCRIPT)
- `src/server/core/dcobject.cpp` — origin name map (add `{ DS_TRAFFIC_OBSERVER, L"traffic" }`)
- `src/server/core/nxslext.cpp` — `NXSL_ENV_CONSTANT("DataSource::TRAFFIC_OBSERVER", ...)`
- `src/server/aitools/datacoll.cpp` — origin name for AI tools
- Java `DataOrigin.java` — `TRAFFIC_OBSERVER(15)`
- nxmc `General.java` — origin availability + metric picker case (§6.2)
- Data dictionary / user documentation

Dispatch by object class:

| Object class | Serving path |
|---|---|
| `OBJECT_TRAFFICOBSERVER` | observer-level metrics via connector (`GetObserverMetric`) + core-served sync status metrics (§7) |
| `OBJECT_OBSERVATIONPOINT` | point metrics/tables via connector, addressed by the point's `external_id` |
| `OBJECT_NODE` | host metrics/tables: resolve observing point + host key from the DCI's instance data through the reverse index, then `GetHostMetric`/`GetHostTable` |
| anything else | `DCE_NOT_SUPPORTED` |

Error semantics at collection time (standard `DataCollector` handling,
`src/server/core/datacoll.cpp:368-396` applies unchanged):

- Node not (or no longer) observed by the referenced point → `DCE_NOT_SUPPORTED`
- Host currently absent from the analyzer's active table → `DCE_NO_SUCH_INSTANCE`
  (feeds instance grace period and error-based thresholds — this IS the presence
  mechanism, see §6.5)
- Analyzer unreachable → `DCE_COMM_ERROR` (thresholds configured for collection errors
  fire via `Threshold::checkError`; value thresholds are not evaluated on failed polls)

### 6.2 Metric namespace and the DCI editor

Names are scoped by origin, so no disambiguating prefix is required. A small **core
metric vocabulary** is defined that all connectors must serve where capable (the views
and example template rely on these names); connectors may add arbitrary extra metrics,
surfaced through `GetMetricDefinitions`.

Baseline vocabulary (indicative, finalized during implementation):

- **Observer level:** `Uptime`, `Version`, `CaptureDrops`, `APILatency` (measured by
  core, not the connector), plus core-served sync status metrics (§7)
- **Point level (scalar):** `ThroughputIn`, `ThroughputOut` (bps), `PacketsIn`,
  `PacketsOut`, `ActiveHosts`, `ActiveFlows`, `Drops`
- **Point level (table):** `L7Breakdown`, `TopTalkers`, `DSCPBreakdown` (capability-gated)
- **Host level on Node (scalar):** `Host.BytesIn({instance})`, `Host.BytesOut({instance})`,
  `Host.PacketsIn/Out({instance})`, `Host.ActiveFlows({instance})`,
  `Host.TcpRetransmits({instance})`, `Host.Alerts({instance})`, `Host.Present({instance})`
- **Host level on Node (table):** `Host.L7Breakdown({instance})`, `Host.Peers({instance})`

DCI editor: new metric selection dialog wired into the per-origin switch in
`General.java` (`MetricSelector.selectionButtonHandler`). The dialog queries the server
(new NXCP request routed to the connector's `GetMetricDefinitions` with the appropriate
level for the selected target class), so the offered list is live, connector-specific and
capability-filtered. This is the decisive advantage of the dedicated origin: the
`DS_INTERNAL` picker (`SelectInternalParamDlg`) is a client-side static list and cannot
express dynamic, capability-dependent metrics.

### 6.3 DCIs on TrafficObserver and ObservationPoint

Ordinary DCIs configured on the objects themselves (both are `DataCollectionTarget`s),
origin `DS_TRAFFIC_OBSERVER`, no instance addressing needed — the object identity supplies
the address (as with `DS_CLOUD_CONNECTOR` on `Resource`). Default template shipped as
documentation/example, not force-applied.

### 6.4 Per-node DCIs via template instance discovery

No automatic DCI creation in core. The mechanism:

1. Core serves an internal table on Node: **`Traffic.ObservationPoints`** — one row per
   host record referencing this node. Columns: instance key (see below), observer name,
   point name, point object id, host key, IP, VLAN. Served from the reverse index; empty
   (not an error) on unobserved nodes.
2. Users author a template with DCI prototypes: origin `DS_TRAFFIC_OBSERVER`, instance
   discovery method `IDM_INTERNAL_TABLE` over `Traffic.ObservationPoints`
   (instance-discovery machinery is origin-agnostic; `doInstanceDiscovery` in
   `dctarget.cpp` works unchanged).
3. **Instance key: `<pointObjectId>:<hostKey>`** (e.g. `4211:192.168.1.5@0`). This makes
   multi-IP, multi-VLAN and multi-point observation all fall out naturally: one DCI
   instance per (point, host record), which is exactly the "per-observation-point,
   no auto-aggregation" decision from the draft (§7 there). Prototype names use
   `{instance}` (standard `expandInstance()` substitution); the instance filter script
   can set the display name to something operator-friendly (point name + VLAN) and the
   related object to the ObservationPoint (`m_relatedObject`).
4. Template auto-bind filter uses the new NXSL Node attribute (§11):
   `return $node->isObserved;` — so binding, instance fan-out, and instance retirement
   are all standard mechanisms.
5. An example template ships with the documentation.

Multi-instance correlation (same node observed by points of *different* observers) needs
no special handling: instances are keyed by point object id, observers are just more
points.

### 6.5 Presence semantics

- Default: absence from the active host table = `DCE_NO_SUCH_INSTANCE` on collection,
  which (a) can drive error-count thresholds ("host disappeared from SPAN"), and
  (b) starts the instance grace period so stale instances self-clean.
- Optional explicit metric `Host.Present({instance})` (0/1) for operators who want a
  chartable/thresholdable presence series; served from the same cache, returns 0 instead
  of NO_SUCH_INSTANCE while the host record still exists. Users add it from the template
  like any other metric — core does not force the doubled DCI count.
- **All presence semantics are conditional on `HOST_SET_AUTHORITATIVE`** (§4.3). On a
  connector without the bit, "absent from the query result" does not mean "gone" —
  `Host.Present` is capability-gated out of the metric catalog
  (`TrafficMetricDefinition.requiredCapability`), and operators should not configure
  error-count thresholds on absence. Instance retirement is unaffected either way: it is
  driven by host-record retention (`TrafficObserver.HostRetentionTime`, default 7 days,
  housekeeper) plus the DCI instance grace period, never by single-poll absence.
- The Pro-only historical-timeseries polling path from the draft (continuous data,
  no presence ambiguity) is deferred; it would appear later as an alternative serving
  path behind `HISTORICAL_TIMESERIES` without changing DCI configuration.

## 7. Config Sync (NetXMS → analyzer)

Executed as scheduled tasks, not polls (pushing config to an external system is
semantically not a poll):

- Core registers one `ScheduledTaskHandler` (id `Traffic.ConfigSync`,
  `SyncTrafficObserverConfig` in `traffic_observer.cpp`) and creates a **single
  recurrent system dispatcher task** (`* * * * *`) — created only when at least one
  traffic connector is registered (`IsComponentRegistered("TRAFFIC")` in `main.cpp`).
- The dispatcher iterates all observers each minute and runs sync for those whose
  per-observer interval (`sync_config` → `hostAliases.interval`, default 3600 s,
  minimum 60 s) has elapsed and whose connection state is CONNECTED (prevents
  failure-event spam while the analyzer is down — the unreachable event already
  covers that). Schedule ownership thus lives on the object, not in the scheduler:
  no per-object task lifecycle to manage. The handler also accepts an `objectId`-bound
  invocation (runs that observer immediately, ignoring interval and connection gate) —
  users needing bespoke timing can hand-create a per-observer scheduled task; those
  are user-owned, so the server never creates/deletes them. Handler access right:
  `SYSTEM_ACCESS_USER_SCHEDULED_TASKS`.
- Per-observer sync enablement and interval are edited by the `TrafficObserverSync`
  property page writing the `sync_config` JSON: `{"hostAliases":{"enabled":bool,"interval":seconds}}`.
- **v1 ships exactly one sync surface: host aliases** — node names for matched host
  keys via `SyncHostAliases` (only for hosts present in `observation_point_hosts`),
  capability-gated on `SYNC_HOST_ALIASES`. This is the "close the loop" feature from
  the original motivation (NetXMS node names appearing in the analyzer's UI), and it is
  the only surface validated end to end in Phase 0.
- **Dropped from v1** (genericity review outcome — no dead entry points; the interface
  gains functions when a connector that can serve them lands):
  - *Local networks* — Phase 0 established ntopng has **no REST endpoint** for setting
    the local-networks list (source-verified against the full v2 endpoint catalog);
    the surface would have shipped unusable against the only connector in existence.
    The full-overwrite ownership policy sketch is preserved in the Phase 0 report and
    this document's history for whenever a capable connector (or a future ntopng API)
    appears.
  - *Host pools* — validated round-trip on ntopng, but Community limits (5 pools ×
    8 members, from `get/ntopng/limits.lua`) make it a toy outside Pro/Enterprise,
    which is itself unvalidated. Returns together with Pro validation if warranted;
    the validated endpoints and `CIDR@vlan` member format are recorded in the Phase 0
    report.
- Status is exposed as core-served `DS_TRAFFIC_OBSERVER` metrics on the observer
  (intercepted by prefix in `TrafficObserver::getMetricFromConnector`, appended to the
  observer-level metric catalog by the `CMD_GET_TRAFFIC_METRIC_DEFS` handler):
  `Sync.HostAliases.LastRun`, `Sync.HostAliases.RecordsSynced`,
  `Sync.HostAliases.Errors` — thresholdable like any DCI. Runtime state, not
  persisted (a restart re-syncs once — idempotent). Failures also raise
  `SYS_TRAFFIC_SYNC_FAILED` (§12), and the observer view header shows a
  "Host alias sync" status line fed by these metrics.
- Aliases pushed: node name for every matched record in `observation_point_hosts`
  across the observer's points (hostKey → node name, deduplicated by hostKey).
  NetXMS is authoritative for matched hosts — a manually set ntopng alias for a
  matched host is overwritten on the next run. Aliases are not cleared when a match
  disappears (records age out of the index; a stale alias in the analyzer is
  cosmetic and self-corrects if the host is re-matched).

## 8. UI

### 8.1 Views

Three surfaces, all RWT/SWT-shared, all fed by **live queries** (new NXCP request, §9)
so they work with zero DCIs configured; charts additionally use collected data when
matching DCIs exist:

1. **Observer view** (context: `TrafficObserver`) — connection/health header, backend
   version/edition + capability display, observation point list with per-point state and
   key stats, sync surface status. List-style view modeled on `AccessPointsView`.
2. **Observation point view** (context: `ObservationPoint`) — throughput/hosts/flows
   stat header, L7 breakdown and top-talkers tables (`TableValueViewer` /
   `SortableTableViewer`), drops. Chart layout modeled on `PerformanceView` /
   `InterfaceTrafficChart`.
3. **Node "Traffic" tab** (context: `Node` with non-empty observation set;
   `isValidForContext` checks the observation relationship from the object message) —
   all observation points side by side (multi-source visibility per the draft's §7),
   per-host L7 breakdown, top peers, retransmit trend. This is the primary
   operator-facing surface.

Deliberately out of scope (unchanged from draft): flow-level drill-down / traffic
explorer — that stays in ntopng.

### 8.2 Registration

Directly in `ObjectsPerspective.configureViews()`, guarded by
`session.isServerComponentRegistered("TRAFFIC")` — the tabs simply don't exist against
servers without a traffic connector module. (The `ObjectViewDescriptor` SPI was
considered but is reserved for out-of-tree plugins; core views register directly.)

### 8.3 Property pages

- `TrafficObserverConnection` — connector (read-only after creation), endpoint URL,
  token, TLS verification, timeouts (fields map into the credentials blob; editor shows
  token write-only, standard sensitive-field handling)
- `TrafficObserverSync` — sync surface enablement, scope selection, host pool mapping,
  schedule
- `ObservationPointScope` — in-scope flag (also exposed as a checkbox/action in the
  observer view's point list for one-click opt-in)

Registered in `ObjectPropertiesManager`, visibility gated on object class
(`SensorProperties` is the template).

## 9. NXCP / Client Library

- New command **`CMD_QUERY_TRAFFIC_DATA`** (0x0218; `NXCPMessageCodeName()` and
  `NXCPCodes.java` updated per repository rules): request carries target object id +
  query type (`VID_TRAFFIC_QUERY_TYPE`); handled in `session.cpp`, resolved through the
  reverse index and connector, result serialized as a table. One command with a
  query-type field keeps the command-code namespace clean. *Implementation note:* only
  the **active host list** query type exists — point/host metrics and tables ride the
  generic `CMD_QUERY_PARAMETER`/`CMD_QUERY_TABLE` with origin `DS_TRAFFIC_OBSERVER`
  (served since Phase 2), and metric definitions have their own
  `CMD_GET_TRAFFIC_METRIC_DEFS`; duplicating those paths under this command would add
  surface without value. Future flow-level or time-window queries get new query types
  here.
- Object messages: `TrafficObserver` fills connector name, backend info, capability
  mask, sync status, zone; `ObservationPoint` fills external id, type, in-scope flag,
  state, observer reference; `Node` fills its observation point id list (for
  `isValidForContext` and the tab header). Credentials never leave the server except in
  write direction (`NXCObjectModificationData` → modify message), mirroring
  `CloudDomain` handling.
- New `VID_*` constants — reuse existing semantic matches where present before minting
  (repository rule).

## 10. Database

New tables in `sql/schema.in` + upgrade procedures in
`src/server/tools/nxdbmgr/upgrade_v70.cpp` (tables created at 70.8; genericity-review
columns added at 70.9):

```sql
CREATE TABLE traffic_observers
(
   id integer not null,
   connector_name varchar(63) not null,
   credentials varchar(4000) null,
   zone_uin integer not null,
   node_id integer not null,
   removal_policy smallint not null,
   grace_period integer not null,
   sync_config varchar(4000) null,
   last_discovery_status smallint not null,
   last_discovery_time integer not null,
   last_discovery_msg varchar(4000) null,
   PRIMARY KEY(id)
) TABLE_TYPE;

CREATE TABLE observation_points
(
   id integer not null,
   observer_id integer not null,
   external_id varchar(127) not null,
   point_type varchar(31) null,
   in_scope char(1) not null,
   zone_uin integer not null,
   sampling_rate integer not null,
   local_networks varchar(4000) null,
   state smallint not null,
   provider_state varchar(255) null,
   last_discovery_time integer not null,
   PRIMARY KEY(id)
) TABLE_TYPE;

CREATE TABLE observation_point_hosts
(
   point_id integer not null,
   host_key varchar(127) not null,
   ip_addr varchar(48) not null,
   vlan integer not null,
   node_id integer not null,
   first_seen integer not null,
   last_seen integer not null,
   PRIMARY KEY(point_id, host_key)
) TABLE_TYPE;
```

Poll timestamps persist automatically via the existing `pollable_objects` mechanism.
Host record aging runs from the housekeeper. `observation_point_hosts` rows are deleted
with their point; points are deleted with their observer (standard
`deleteFromDatabase` chaining).

## 11. NXSL

- Classes `TrafficObserver` (attributes: connectorName, version, edition, capabilities,
  connectionState, points array) and `ObservationPoint` (externalId, type, inScope,
  state, observer, hosts count) — declared in `nms_script.h`, implemented in
  `nxsl_classes.cpp`, returned from `createNXSLObject`.
- `Node` gains `observationPoints` (array of ObservationPoint) and `isObserved`
  (boolean) — required for template auto-bind filters (§6.4) and generally useful for
  automation.
- `DataSource::TRAFFIC_OBSERVER` constant.

## 12. Events

Modest initial set (registered in the standard event template list):

| Event | Raised by | Meaning |
|---|---|---|
| `SYS_TRAFFIC_OBSERVER_UNREACHABLE` | status poll | API unreachable / auth failure |
| `SYS_TRAFFIC_OBSERVER_RECOVERED` | status poll | connectivity restored |
| `SYS_OBSERVATION_POINT_STATE_CHANGED` | config/status poll | point state transition (includes disappeared/recovered) |
| `SYS_TRAFFIC_SYNC_FAILED` | sync task | a sync surface failed (parameters: surface, error) |

Traffic anomaly alerting stays DCI/threshold-driven (that is the point of the
integration); analyzer-side alert forwarding into NetXMS remains a separate, out-of-scope
vector as in the draft.

## 13. WebAPI

New object classes are exposed through the generic object endpoints automatically once
the JSON create path and `toJson` are implemented (§3). A dedicated traffic query
endpoint (mirroring `CMD_QUERY_TRAFFIC_DATA`) is deferred until there is a concrete
consumer; noted here so the WebAPI impact check is on record.

## 14. Risks and Early Validation

Carried over from the draft where still relevant, with mitigations now concrete:

- **Schema commitment** — mitigated by generic naming (classes/tables carry no product
  name), the connector seam, and the credentials/sync-config JSON blobs absorbing
  connector-specific evolution. The connector C interface is explicitly revisable.
- **API validation** — **done for Community** (ntopng 6.7, see
  `Traffic_Observer_Phase0_Validation.md`): endpoint mapping corrected (§4.4),
  paging/VLAN/rate-field behavior of the bulk host endpoint characterized, interface
  signatures frozen (§4.1). **Pro/Enterprise validation is still outstanding**
  (edition strings, capability differences, `HISTORICAL_TIMESERIES`/ClickHouse) —
  tracked as a Phase 1 exit criterion.
- **Polling load** — bounded by design (O(points) bulk calls per cache TTL); still
  measure against a busy interface (10k+ active hosts) early: response size and paging
  strategy of `host/active.lua` determine whether the cache holds full host stats or
  only the matching subset.
- **REST API drift across ntopng versions** — `TestConnection` records the backend
  version; connector maintains a supported-version range and logs (once) when outside
  it. Integration tests against multiple ntopng versions.
- **Operator understanding of per-point instances** — instance display names include
  the point name; the Node traffic tab shows points side by side; documentation
  explains the no-auto-aggregation decision.

## 15. Open Items (deliberately deferred)

- Per-observer matching policy refinements (management-IP-only, interface role filters).
- User-declared aggregation roles across observation points (v2+ per the draft).
- Historical-timeseries serving path behind `HISTORICAL_TIMESERIES`.
- Dedicated WebAPI traffic endpoints.
- Second connector (candidates if ever needed: Akvorado, ElastiFlow, Kentik, Flowmon —
  or a future NetXMS-native flow collector, which would slot in as just another
  connector). Selection guidance from the genericity review: pick for *user demand*
  (UniFi/FortiGate-class device observers have real installed base and a clean
  host-centric fit), not for cheap validation — sFlow-RT/FastNetMon-class backends sit
  where ntopng already sits and would validate nothing.
- **Deferred with intent, from the genericity review** (all live in the revisable C
  interface — zero migration cost when needed):
  - Ingestion-lag hint (`dataLagSeconds` in `TrafficBackendInfo` or per-point) for
    query-based backends with delayed data; plus a DCI-editor warning when polling
    interval < lag + granularity. Build with the first query-based connector.
  - Observer-wide bulk hooks (`GetActiveHostsForObserver`-style, nullable) for
    high-point-cardinality backends where per-point bulk calls multiply.
  - High-cardinality opt-in UX (thousands of exporter-interface points): selection by
    linked `Interface` objects rather than per-point checkboxes; possibly a
    `linked_interface_id` on points. Deliberately not baked in now — a second connector
    may equally choose coarser point granularity, which is the connector's decision.
  - Sync surfaces beyond host aliases (host pools, local networks) — see §7 for what
    was validated and why they were dropped from v1.
  - A read-only spike against Akvorado (free, self-hosted, query-based, sampled,
    high point cardinality — the opposite corner of both backend axes from ntopng)
    is the recommended de-risking step *before* committing to connector #2, not a
    gate for shipping connector #1.

## 16. Implementation Phasing

1. **Phase 0 — API validation spike:** exercise the ntopng REST v2 endpoints (Community
   + Pro) listed in §4.4; freeze `TrafficConnectorInterface` signatures.
   **Done** for Community (report: `Traffic_Observer_Phase0_Validation.md`);
   Pro-instance validation deferred to Phase 1 exit.
2. **Phase 1 — core skeleton:** object classes (C++/Java/NXCP/DB/NXSL), connector
   registry + `NXMODULE` field, ntopng connector with `TestConnection` +
   `DiscoverPoints`; observer/point polls; object creation UI. **Done.**
3. **Phase 2 — data path:** `DS_TRAFFIC_OBSERVER` origin end to end (dispatch, connector
   metric/table serving, bulk cache), host matching pass + host records,
   `Traffic.ObservationPoints` internal table, instance discovery, example template,
   DCI editor metric picker. **Done** — verified end to end against ntopng 6.7 Community
   (point metrics + L7/TopTalkers/DSCP tables, host matching, host metrics incl.
   BytesIn/Out/PacketsIn/Out/ActiveFlows/TcpRetransmits/Present and Host.L7Breakdown,
   internal table, metric picker). Example template: `Traffic_Observer_Example_Template.md`.
   The connector serves `getMetricForClient`/`getTableForClient` too, so the DCI editor
   "test" button and `queryMetric`/`queryTable` work; note the two-tier cache TTL means the
   editor's live value reflects the last cache refresh. Observer-level metrics are limited to
   `Version`/`Uptime` (ntopng has no instance-wide stats endpoint).
4. **Phase 3 — views:** the three UI surfaces + `CMD_QUERY_TRAFFIC_DATA`. **Done** —
   verified end to end against ntopng 6.7 Community (active-host query with matched-node
   annotation, node observation-point list in the object message, all view data paths).
   Views registered directly in `ObjectsPerspective.configureViews()` gated on server
   component `TRAFFIC`, package `org.netxms.nxmc.modules.traffic`:
   observer view (health header + point list with in-scope toggle), observation point
   view (live stat header + active hosts / L7 / top talkers), node "Traffic" tab
   (observation point records + per-host stats / L7 / peers). Live tables share
   `TrafficQueryTable` (a `BaseTableValueViewer` over `queryTable`/
   `getActiveTrafficHosts`). `TrafficObserverConnection` property page added
   (credentials, node link, removal policy/grace period). The connector caches
   `interface/data.lua` per TTL, so a view's multi-metric stat header costs one backend
   call per refresh. Host matching now pushes node object updates when a node's
   observation set changes, keeping tab visibility current. Deferred to later phases:
   retransmit trend chart (needs collected DCIs), sync surface status in the observer
   header (Phase 4).
5. **Phase 4 — sync:** scheduled task, host-alias sync (the single v1 surface, §7),
   status metrics, events. Observer property pages (connection settings, node link,
   sync enablement) land with Phase 3/4 UI work. **Done** — verified end to end
   against ntopng 6.7 Community (alias for matched host visible in ntopng's active
   host list after the dispatcher run; `RecordsSynced=1 Errors=0`; per-observer
   interval pacing confirmed). Point discovery already rode the configuration poll
   since Phase 1, so this phase delivered the sync side per the dispatcher model in
   §7: `Traffic.ConfigSync` handler + single every-minute system task (created only
   when a connector is registered), `TrafficObserver::runConfigSync`/`syncHostAliases`,
   `GetObservationPointMatchedHosts` index accessor, core-served
   `Sync.HostAliases.{LastRun,RecordsSynced,Errors}` metrics (intercepted before the
   connector, appended to the observer-level metric catalog), `NtopngSyncHostAliases`
   (`POST set/host/alias.lua`, aborts on auth error), POST support in the ntopng API
   helper, `TrafficObserverSync` property page, and the sync status line in the
   observer view header. Debug tag `traffic.sync`. No schema or NXCP changes —
   `sync_config`/`VID_SYNC_CONFIG` existed since Phase 1; sync status is runtime-only.
6. **Phase 5 — hardening:** capability gating polish, docs (admin guide, data
   dictionary, debug tags), integration tests against multiple ntopng versions.
   **Done** (except multi-version testing — see below). Hardening review outcome
   (all fixed and rebuilt):
   - `saveToDatabase` in observer/point executed the statement *after* releasing
     the properties lock with `DB_BIND_STATIC` bindings — a concurrent modify could
     free the bound buffers mid-execute; both now execute under the lock
     (interface.cpp convention).
   - External-ID reconciliation used 64-wchar buffers against the 128-char
     `externalId` contract — silent truncation could suppress removal-policy action
     for points sharing a 63-char prefix; widened to 128 (also the state-change
     event parameter).
   - `TestConnection`/`DiscoverPoints` are now null-guarded like every other
     connector entry point (partial third-party connectors must not crash polls).
   - Discovery descriptor list is RAII-owned (leaked on shutdown-cancellation
     mid-poll before).
   - ntopng connector: HTTP response bodies capped at 64 MB (bounded write
     callback), active-host accumulation capped (credentials `maxHosts`, default
     100000, truncation logged) — a compromised/misbehaving analyzer can no longer
     OOM the server; `NtopngClearCache` skips in-flight cache entries (latent UAF
     if clear ever runs concurrently with a fetch).
   - UI: sync-status job no longer throws on non-numeric/absent metric values;
     sync property page tolerates malformed `sync_config`; observation point stat
     header guards against stale-object races; node view host L7/Peers tables are
     capability-gated like the point view's (no error dialogs against backends
     lacking those capabilities).
   - Docs: `Traffic_Observer_Admin_Guide.md` (portable draft for the official
     admin guide, incl. data dictionary tables for the three new DB tables);
     debug tags registered in `doc/internal/debug_tags.txt`.
   - The review also flagged that the interface's `Shutdown` entry point was never
     invoked; resolved by removing it — connectors are provided by modules, which
     already have a `pfShutdown` hook (the ntopng module clears its caches there).
   - Known follow-up: multi-version/Pro-edition integration testing still blocked
     on instance availability (open Phase 1 exit criterion).
