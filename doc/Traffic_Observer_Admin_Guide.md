# Traffic Observer — Administrator Guide (draft)

> Draft for the official administration guide. Covers the traffic observer
> subsystem as shipped after integration phases 0–4 (see
> `Traffic_Observer_Integration_Design.md` for design rationale). Validated
> against ntopng 6.7 Community; Pro/Enterprise editions are expected to work
> but are not yet validated.

## 1. Concepts

The traffic observer subsystem integrates external traffic analyzers (currently
ntopng) into NetXMS. Two object classes model the analyzer:

- **Traffic observer** — one analyzer instance (e.g. one ntopng server).
  Carries connector name, credentials, discovery/removal policy, and sync
  configuration. Container for its observation points.
- **Observation point** — one monitored interface / traffic source of the
  analyzer (an ntopng monitored interface). Created and maintained
  automatically by discovery; identified by a stable connector-defined
  *external ID* that survives analyzer restarts.

The server talks to the analyzer through a **traffic connector** — a server
module implementing the connector interface (`NTOPNG` module,
`ntopng.nxm`). The client UI enables its traffic views only when a server
reports the `TRAFFIC` component, i.e. when at least one connector module is
loaded.

**Host matching** links traffic data to monitored infrastructure: for every
active host reported on an in-scope observation point, the server tries to
find a node with the same primary IP address (within the observer's zone, or
across all zones when the observer's zone is set to *all*). Matched records
enable the node "Traffic" tab, per-node host DCIs, and host alias sync.

## 2. Requirements

- ntopng 6.x with the REST API v2 enabled and reachable from the NetXMS server.
- An ntopng **API token** for an admin-capable user (required for host alias
  sync; read-only tokens work for monitoring surfaces). Password
  authentication is not used.
- The `ntopng.nxm` server module loaded (`Module = ntopng.nxm` in
  `netxmsd.conf`).

## 3. Creating a traffic observer

Create the *Traffic Observer* object (Infrastructure Services), select the
connector (`NTOPNG`), and provide credentials as a JSON document:

```json
{
   "url": "https://ntopng.example.com:3000",
   "token": "<api token>",
   "timeout": 30,
   "verifyTls": true,
   "cacheTtl": 30
}
```

| Field | Default | Meaning |
|-------|---------|---------|
| `url` | — | Base URL of the ntopng instance (required) |
| `token` | — | API token, sent as `Authorization: Token` (required) |
| `timeout` | 30 | HTTP request timeout, seconds |
| `verifyTls` | true | Verify TLS certificate |
| `cacheTtl` | 30 | Connector-side cache TTL, seconds (active host lists, host details, interface counters) |

Credentials are write-only from the client: the object editor shows only
whether credentials are set, and an empty credentials field on the
*Connection* property page keeps the current value.

Other settings on the *Connection* property page:

- **Zone** — zone used for host matching (`-1` = match in all zones).
- **Node representing the analyzer itself** — optional link to the node
  running the analyzer, for navigation.
- **Removal policy / grace period** — what happens to observation points that
  disappear from discovery (see below).

## 4. Discovery and observation point lifecycle

Discovery runs as part of the observer's **configuration poll** (standard
polling schedule, or *Poll → Configuration* for an immediate run). Each poll:

1. Tests the connection and refreshes backend product/version/edition and the
   capability set.
2. Retrieves the analyzer's traffic sources and reconciles observation points
   by external ID — new points are created, existing ones updated (name, type,
   state, sampling rate, local networks).
3. Applies the removal policy to points missing from discovery:
   - **Mark as inactive** (default)
   - **Delete after grace period** (days; inactive until then)
   - **Delete immediately**
   - **Ignore**
4. Runs host matching for all in-scope, active points.

The observer's **status poll** is a lightweight connection test that maintains
the connection state (Connected / Unreachable / Authentication failure) and
raises the corresponding events.

**Scope:** each observation point has an *in scope* flag (property page, or
the in-scope toggle in the observer's "Observation Points" view). Only
in-scope points participate in host matching and appear in per-node data.
Points discovered on loopback or otherwise uninteresting interfaces can be
left out of scope; data collection DCIs can still be created on them.

## 5. Host matching and per-node data

Matching is by host primary IP address against nodes in the observer's zone.
Match records are kept in memory and persisted (`observation_point_hosts`);
records not refreshed within `TrafficObserver.HostRetentionTime` (server
configuration variable, default 7 days) are removed by the housekeeper.

For a matched node:

- The node object gains the **Traffic** tab (visible whenever at least one
  observation point currently observes the node), showing all observing
  points side by side with per-host statistics, L7 breakdown, and top peers.
- The internal table `Traffic.ObservationPoints` (origin *Internal*) lists the
  observing points with columns INSTANCE, OBSERVER, POINT, POINT_ID, HOST_KEY,
  IP, VLAN, SAMPLING_RATE. Its INSTANCE column is designed for instance
  discovery of host-level DCIs.
- Host-level metrics can be collected with the **instance key**
  `<pointObjectId>:<hostKey>` (e.g. `200:10.5.5.100@0`) — the `hostKey` is
  connector-defined (ntopng: `ip@vlan`).

## 6. Data collection

DCIs use origin **Traffic observer**. The metric picker in the DCI editor
lists available metrics for the selected object, filtered by the backend's
capability set. With the ntopng connector:

**Observer level** (on the traffic observer object): `Version`, `Uptime`, and
the core-served sync status metrics `Sync.HostAliases.LastRun`,
`Sync.HostAliases.RecordsSynced`, `Sync.HostAliases.Errors` (see §7).

**Observation point level:** `ThroughputIn`/`ThroughputOut` (bps),
`PacketsIn`/`PacketsOut`, `BytesIn`/`BytesOut`, `ActiveHosts`, `ActiveFlows`,
`Drops`, `TcpRetransmits`; tables `L7Breakdown`, `TopTalkers`,
`DSCPBreakdown` (capability-gated). Direction is relative to the monitored
network (download = In, upload = Out).

**Host level** (on nodes, instance = `<pointId>:<hostKey>`):
`Host.BytesIn(...)`, `Host.BytesOut(...)`, `Host.PacketsIn(...)`,
`Host.PacketsOut(...)`, `Host.ActiveFlows(...)`, `Host.TcpRetransmits(...)`,
`Host.Alerts(...)`, `Host.Present(...)` (1/0, requires authoritative host
set); tables `Host.L7Breakdown(...)`, `Host.Peers(...)`.

An example template with instance discovery over `Traffic.ObservationPoints`
is in `Traffic_Observer_Example_Template.md`.

Note on freshness: the connector caches analyzer responses for `cacheTtl`
seconds, so collected values and the DCI editor's "test" button reflect the
last cache refresh.

## 7. Configuration sync (host aliases)

When enabled, NetXMS pushes the names of matched nodes to the analyzer as
host aliases, so operators see NetXMS node names in the analyzer UI. This is
the only sync surface in v1 and requires the backend capability *host alias
sync* (ntopng: admin token).

- Enable on the observer's **Synchronization** property page: checkbox plus
  interval (seconds, default 3600, minimum 60).
- A single system scheduled task `Traffic.ConfigSync` runs every minute and
  dispatches sync for observers whose interval has elapsed; sync is skipped
  while the analyzer connection is down. The task exists only on servers with
  a traffic connector module loaded.
- For bespoke timing, a user-created scheduled task with handler
  `Traffic.ConfigSync` bound to a specific observer runs that observer's sync
  immediately (interval and connection-state gates bypassed).
- NetXMS is authoritative for matched hosts: a manually set analyzer alias
  for a matched host is overwritten on the next run. Aliases are not cleared
  when a match disappears.
- Status is visible in the observer view header ("Host alias sync" line) and
  collectable via the `Sync.HostAliases.*` metrics; failures raise
  `SYS_TRAFFIC_SYNC_FAILED`.

## 8. Views

- **Observation Points** (on the observer) — connection/backend/capability
  header, host alias sync status, point list with state, sampling, in-scope
  toggle and last discovery result.
- **Traffic** (on an observation point) — live stat header (throughput,
  hosts, flows, drops, retransmits), active host list with matched-node
  annotation, L7 breakdown, top talkers.
- **Traffic** (on a node with observation coverage) — observing points with
  per-point selection, per-host statistics, L7 breakdown, top peers.

All views are fed by live queries and work with zero DCIs configured.

## 9. Events

| Event | Raised by | Meaning |
|-------|-----------|---------|
| `SYS_TRAFFIC_OBSERVER_UNREACHABLE` | status/config poll | analyzer API unreachable or authentication failed (params: connectorName, reason, details) |
| `SYS_TRAFFIC_OBSERVER_RECOVERED` | status/config poll | connectivity restored (params: connectorName) |
| `SYS_OBSERVATION_POINT_STATE_CHANGED` | discovery | point state transition (params: externalId, oldState, newState, providerState) |
| `SYS_TRAFFIC_SYNC_FAILED` | sync task | a sync surface failed (params: surface, error) |

## 10. NXSL

- `$node->isObserved` — true if the node currently has observation coverage.
- `$node->observationPoints` — array of ObservationPoint objects observing
  the node.
- **TrafficObserver** class attributes: `capabilities`, `connectionState`,
  `connectorName`, `edition`, `gracePeriod`, `lastDiscoveryStatus`,
  `lastDiscoveryTime`, `linkedNode`, `points`, `product`, `removalPolicy`,
  `version`, `zoneUIN` (plus data collection target attributes).
- **ObservationPoint** class attributes: `effectiveZoneUIN`, `externalId`,
  `inScope`, `lastDiscoveryTime`, `localNetworks`, `observer`,
  `providerState`, `samplingRate`, `state`, `type`, `zoneUIN`.

## 11. Server configuration variables

| Variable | Default | Meaning |
|----------|---------|---------|
| `TrafficObserver.HostRetentionTime` | 7 days | Retention for host match records not refreshed by matching; enforced by housekeeper |

## 12. Troubleshooting

Debug tags (`nxadm -c "debug <tag> <level>"` or server debug console):

| Tag | Coverage |
|-----|----------|
| `traffic.connector` | connector registration and interface calls |
| `traffic.poll` | observer polls, discovery, host matching |
| `traffic.sync` | configuration sync runs |
| `ntopng` | ntopng module: HTTP requests, parsing, caching |

Common issues:

- **Authentication failure immediately after setup** — ntopng reports both
  bad tokens and unknown endpoints as a 302 redirect to the login page; check
  the token and the ntopng version (REST v2 required).
- **Sync enabled but never runs** — check the observer's connection state
  (sync is skipped while not connected), the backend capability set shown in
  the observer view (host alias sync requires an admin token), and that the
  `Traffic.ConfigSync` system task exists.
- **Host not matched** — matching uses the node's primary IP within the
  observer's zone; check zone configuration and that the observation point is
  in scope and active.
- **Capabilities differ between ntopng editions** — Community lacks some
  surfaces; the capability line in the observer view shows what the backend
  reports. Capability-gated metrics and tables are hidden from the metric
  picker and views when unsupported.

## 13. Data dictionary (new tables)

**traffic_observers** — one row per traffic observer object.

| Column | Type | Description |
|--------|------|-------------|
| id | integer | object ID (PK) |
| connector_name | varchar(63) | traffic connector name (e.g. `NTOPNG`) |
| credentials | varchar(4000) | connector credentials, JSON |
| zone_uin | integer | zone for host matching, -1 = all zones |
| node_id | integer | linked node representing the analyzer, 0 = none |
| removal_policy | smallint | 0 inactive / 1 delete after grace / 2 delete / 3 ignore |
| grace_period | integer | days before deletion under policy 1 |
| sync_config | varchar(4000) | sync surface configuration, JSON |
| last_discovery_status | smallint | 0 success / 2 failure |
| last_discovery_time | integer | UNIX timestamp of last discovery |
| last_discovery_msg | varchar(4000) | last discovery error message |

**observation_points** — one row per observation point object.

| Column | Type | Description |
|--------|------|-------------|
| id | integer | object ID (PK) |
| observer_id | integer | owning traffic observer |
| external_id | varchar(127) | stable connector-side point identity |
| point_type | varchar(31) | backend point type (`packet`, `zmq`, ...) |
| in_scope | char(1) | participates in host matching |
| zone_uin | integer | zone override, -1 = inherit from observer |
| sampling_rate | integer | 0 unknown, 1 unsampled, N = 1:N |
| local_networks | varchar(4000) | CIDR list reported by backend |
| state | smallint | 0 unknown / 1 active / 2 inactive |
| provider_state | varchar(255) | raw backend state string |
| last_discovery_time | integer | last time seen in discovery |

**observation_point_hosts** — host match records (index of active hosts per
point, maintained by matching, aged by housekeeper).

| Column | Type | Description |
|--------|------|-------------|
| point_id | integer | observation point object ID (PK part) |
| host_key | varchar(127) | connector-defined host identity, ntopng `ip@vlan` (PK part) |
| ip_addr | varchar(48) | host IP address |
| vlan | integer | VLAN ID |
| node_id | integer | matched node, 0 = unmatched |
| first_seen | integer | UNIX timestamp |
| last_seen | integer | UNIX timestamp |
