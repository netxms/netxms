# Traffic Observer Integration — Phase 0 API Validation Report

**Status:** Complete (ntopng Community validated; Pro/Enterprise pending)

> **Post-Phase-2 note:** following the interface genericity review, the sync half of
> `TrafficConnectorInterface` was trimmed to `SyncHostAliases` only — the
> `SyncLocalNetworks` and `SyncHostPools` entry points and their capability bits were
> removed (no dead entry points; functions return to the interface when a connector can
> serve them). The host-pool endpoint validation in §3/§7 below is retained as the
> reference for that future work. `TRAFFIC_CAPABILITY_HOST_SET_AUTHORITATIVE` was added
> (ntopng reports it — §5's live table with idle age-out is authoritative), and
> `ObservationPointDescriptor` gained `samplingRate`.
**Scope:** Live validation of the ntopng REST API v2 endpoints mapped in
`Traffic_Observer_Integration_Design.md` §4.4, and the resulting frozen
`TrafficConnectorInterface` (design §4.1).

Validation target:

| | |
|---|---|
| Backend | ntopng 6.7.260504 (rev. 28074), **Community** edition |
| Platform | Debian GNU/Linux 13 (trixie), x86_64 |
| nDPI | 5.1.0 |
| Access | HTTP, API token authentication |
| Monitored interfaces | `lo` (ifid 1), `ens192` (ifid 2, packet interface, ~8 active hosts) |
| Validation date | 2026-07-13 |

Endpoint availability was additionally cross-checked against the complete REST v2
script catalog in the ntopng source tree (`scripts/lua/rest/v2/`, dev branch,
300 endpoints), so "endpoint does not exist" statements below are source-verified,
not merely probe failures.

## 1. Authentication and Transport

- `Authorization: Token <token>` works on **all** REST v2 endpoints tested,
  including admin-only mutating endpoints (`set/host/alias.lua`,
  `add/host/pool.lua`). The token carries the role of the user it belongs to;
  sync surfaces require a token of an admin user.
- **Failure mode is a `302` redirect to `/lua/login.lua` with an empty body — for
  both invalid tokens and nonexistent endpoints.** There is no 401/404
  distinction. Connector rules:
  - never follow redirects (`CURLOPT_FOLLOWLOCATION` off);
  - treat any 302 as an error;
  - to distinguish "bad credentials" from "endpoint missing in this ntopng
    version", probe a known-good endpoint (`get/ntopng/interfaces.lua`) first —
    this is what `TestConnection` does anyway.
- The `Server:` response header (`ntopng 6.7.260504 (x86_64)`) leaks the version
  even for unauthenticated requests — usable as a pre-auth sanity signal, not
  relied upon.
- Token auth is also accepted by at least some non-REST pages
  (`/lua/admin/prefs.lua` returned 200). Not to be relied upon; the connector
  uses REST v2 endpoints only.

## 2. Response Envelope and Error Semantics

Standard envelope on all REST v2 endpoints:

```json
{"rc": 0, "rc_str": "OK", "rc_str_hr": "Success", "rsp": <payload>}
```

- `rc = 0` success; negative values are errors (`-5 INVALID_ARGUMENTS` observed
  with HTTP 400).
- HTTP status is not a reliable success signal on its own: parameter-validation
  failures inside `http_lint.lua` can return a **plain-text Lua error message
  instead of a JSON envelope** (observed on `set/pool/members.lua`). The
  connector's response parser must survive non-JSON bodies.

## 3. Validated Endpoint Mapping

Replaces the indicative mapping of design §4.4. "✓" = exercised live against 6.7.

| Interface function | Endpoint(s) | Status |
|---|---|---|
| `TestConnection` | `GET /lua/rest/v2/get/ntopng/about.lua` — product, `version` ("6.7.260504"), `release` ("Community"/"Professional"/...), nDPI version, platform | ✓ |
| health details for status poll | `GET .../get/system/status.lua` (license info, CPU/RAM), `GET .../get/system/health/stats.lua` (memory, alerts queue, storage per interface) | ✓ |
| `DiscoverPoints` | `GET .../get/ntopng/interfaces.lua` — `ifid`, `name`, `is_packet_interface`, `is_zmq_interface`, `is_pcap_interface` | ✓ |
| per-point local networks | `GET .../get/network/networks.lua?ifid=` (one extra call per point) | ✓ |
| `GetPointMetric` | `GET .../get/interface/data.lua?ifid=` — see §4 | ✓ |
| `GetPointTable` (L7) | `GET .../get/interface/l7/data.lua?ifid=` — per-application bytes/packets sent/rcvd, flows, breed | ✓ |
| `GetPointTable` (top talkers) | `GET .../get/interface/top/hosts.lua?ifid=` — label + bytes | ✓ |
| `GetPointTable` (DSCP) | `GET .../get/interface/dscp/stats.lua?ifid=` — label + bytes | ✓ |
| `GetActiveHosts` | `GET .../get/host/active.lua?ifid=` — see §5, **must be paged** | ✓ |
| `GetHostMetric` / `GetHostTable` | `GET .../get/host/data.lua?ifid=&host=` — see §6; field-selected subset via `get/host/custom_data.lua?field_alias=` | ✓ |
| host L7 table | contained in `host/data.lua` (`ndpi.*` subtree); standalone: `get/host/l7/data.lua?ifid=&host=` | ✓ |
| host peers table | `GET .../get/flow/aggregated_live_flows.lua?ifid=&host=&aggregation_criteria=client` and `=server` — peer `ip`/`vlan_id`/`label`, bytes sent/rcvd, flows. Alternative: client-side aggregation of `get/flow/active.lua?host=` (paged, has `totalRows`) | ✓ |
| `SyncLocalNetworks` | **No REST v2 endpoint exists.** `edit/network/edit.lua` only sets alias/site of an existing network; the local-networks list is CLI/config/prefs-UI only. Surface not implementable for ntopng — see §7 | ✗ |
| `SyncHostAliases` | `POST .../set/host/alias.lua`, JSON body `{"host","vlan","custom_name"}`; admin required; empty `custom_name` clears | ✓ round-trip |
| `SyncHostPools` | `POST .../add/host/pool.lua` (`pool_name`, `pool_members`), `edit/host/pool.lua`, `delete/host/pool.lua` (`pool`), `GET get/host/pools.lua?ifid=`; member format `CIDR@vlan` (e.g. `10.5.5.111/32@0`). `set/pool/members.lua` rejects its documented `members` key in 6.7 — use `edit/host/pool.lua` | ✓ create/delete round-trip |

Notes:

- The draft mapping's `system/stats.lua` **does not exist**; `interface/l7/stats.lua`
  exists but rejects `ifid`-only queries (`l7/data.lua` is the correct table source).
- `get/ntopng/limits.lua` returns current/max pairs for Community limits — input
  for capability computation (§7).
- `get/ntopng/license.lua` exists but only distinguishes licensed/unlicensed;
  `about.lua`'s `release` field is the edition source.

## 4. Point Metrics (`interface/data.lua`)

One call returns everything needed for the point-level scalar vocabulary and part
of the observer-level one:

| Design metric | Field |
|---|---|
| `ThroughputIn` / `ThroughputOut` | `throughput.download.bps` / `throughput.upload.bps` (also `.pps`; totals in `throughput_bps`/`throughput_pps`) |
| `PacketsIn` / `PacketsOut` | `packets_download` / `packets_upload` (cumulative; also `bytes_download`/`bytes_upload`) |
| `ActiveHosts` | `num_hosts` (also `num_local_hosts`) |
| `ActiveFlows` | `num_flows` |
| `Drops` | `drops`, `tot_pkt_drops` |
| TCP quality | `tcpPacketStats.retransmissions/out_of_order/lost` |
| Observer `Uptime` | `uptime_sec` (numeric; process-wide) |
| ntopng host health | `system_host_stats.*` (CPU states, memory — duplicated by `system/health/stats.lua`) |

Direction semantics: ntopng reports download/upload relative to the monitored
network, not in/out of an interface. The connector maps download→In, upload→Out;
documented in the metric descriptions.

Also present: `engaged_alerts`, `alerted_flows`, `num_devices`, `speed`,
`is_view`, `epoch` (server time — usable for staleness checks).

## 5. Active Host List (`host/active.lua`) — Critical Findings

This endpoint feeds the matching pass and the bulk host cache; it was the main
§14 risk item.

- **Paging is mandatory.** Response carries `perPage` (default 10), `currentPage`
  (1-based) and `data` — **no total row count**. Iterate `currentPage` until a
  page returns fewer than `perPage` rows.
- **The `all=1` parameter is broken in 6.7** (sets `perPage=-1` internally and
  returns zero rows). Do not use.
- No snapshot consistency across pages; under churn, hosts can be skipped or
  duplicated between pages. Mitigation: sort deterministically
  (`sortColumn=ip&sortOrder=asc`) and treat the matching pass as convergent
  (next poll repairs omissions). Duplicates are harmless (idempotent upsert by
  host key).
- The upper bound on `perPage` is not documented and was not testable on an
  8-host instance; validate `perPage=1000` against a busy instance before tuning
  defaults (residual risk, §9).
- **VLAN is a separate integer field** (`vlan`); the row's `key` field is a
  URL-encoded IP not directly usable as an API parameter. The connector builds
  its host key as `ip@vlan`, which **is** accepted as the `host` parameter by all
  per-host endpoints (`host=10.5.5.100@0` and `host=10.5.5.100&vlan=0` both
  validated).
- Useful filters validated: `mode=local` (only hosts in ntopng's local networks),
  `vlan=`, `network_cidr=`. Note `mode` composes with paging, not with `all`.
- **`is_localhost` is unreliable for matching purposes** — multicast group
  addresses are reported with `is_localhost=true`. The matching pass must skip
  entries with `is_multicast`/`is_broadcast` set and should not use
  `is_localhost` as a filter (NetXMS zone-scoped IP lookup decides relevance).
- Per-row content: `ip`, `vlan`, `mac`, `name` (alias/DNS label), `key`,
  `first_seen`/`last_seen` (epoch), `bytes.sent/recvd/total`, `thpt.bps/pps`,
  `num_flows`, `score`, `num_alerts`, `is_localhost`, `is_multicast`,
  `is_broadcast`, `is_blacklisted`, `pool`, `country`, `os`.
- **Per-host packet counters are NOT in the list view** — only bytes and
  throughput. Consequence for the bulk cache: see §6.
- Observed size ≈ 540 bytes/host ⇒ ~5.4 MB total for 10k hosts (spread across
  pages). At `perPage=1000`, one page ≈ 0.5 MB — acceptable.

## 6. Per-Host Detail (`host/data.lua`) and Cache Strategy

`host/data.lua?ifid=&host=` (~5.8 KB/host) covers the full host-level vocabulary:

| Design metric | Field |
|---|---|
| `Host.BytesIn/Out` | `bytes.rcvd` / `bytes.sent` |
| `Host.PacketsIn/Out` | `packets.rcvd` / `packets.sent` |
| `Host.ActiveFlows` | `active_flows.as_client` + `active_flows.as_server` |
| `Host.TcpRetransmits` | `tcpPacketStats.sent.retransmissions` + `tcpPacketStats.rcvd.retransmissions` (also lost/out-of-order, both directions) |
| `Host.Alerts` | `num_alerts` (engaged), `total_alerts` |
| `Host.L7Breakdown` | `ndpi.<App>.{bytes,packets,num_flows,duration,breed}` subtree |
| throughput | `throughput_bps`, `throughput_pps` |
| presence bookkeeping | `seen.first`, `seen.last` |

`get/host/custom_data.lua?field_alias=bytes.sent,bytes.rcvd,...` returns a
selected subset (validated) — an optimization option, still per-host.

**Cache strategy refinement vs. design §5.4:** the single bulk call
(`host/active.lua`) serves the matching pass, presence (`Host.Present`),
`Host.BytesIn/Out` and `Host.ActiveFlows`(approx.) — but packets, TCP
retransmissions and L7 require `host/data.lua` per host. The connector therefore
keeps a two-tier cache:

1. **Active-list tier** (per point, TTL ~30 s): one paged `host/active.lua`
   sweep — O(points) calls per TTL.
2. **Detail tier** (per host, same TTL, lazily populated): `host/data.lua`
   fetched only for host keys actually referenced by a DCI or live query within
   the TTL — O(hosts-with-detail-DCIs) per TTL, not O(active hosts).

Steady-state load is thus bounded by configured collection, not by network size;
the design's O(in-scope points) claim holds exactly only for the metrics served
from tier 1.

## 7. Sync Surfaces

- **Host aliases — validated.** `set/host/alias.lua` round-trip confirmed: alias
  appears as `name` in the active list (label resolution), empty string clears
  it. Note the alias does not appear in `host/data.lua`'s `name` field.
- **Host pools — validated with limits.** Create with members / verify / delete
  round-trip confirmed. Member format `CIDR@vlan`. **Community limits (from
  `get/ntopng/limits.lua`): max 5 host pools, max 8 members per pool** — pool
  sync at any real scale is Pro-only; the connector computes the
  `SYNC_HOST_POOLS` capability from the reported limits (or reports it with the
  limits attached), not merely from the edition string.
- **Local networks — not implementable for ntopng.** No REST v2 endpoint sets
  the local-networks list (source-verified against the full endpoint catalog);
  it is a startup/preferences item. The `SyncLocalNetworks` interface function
  stays (it is generic), the ntopng connector sets it to `nullptr` and never
  reports `SYNC_LOCAL_NETWORKS`. Design §7 surface 1 is dormant until a
  connector that supports it (or a future ntopng API) appears.

Other Community limits worth recording: `num_local_hosts` max **256**,
`num_flow_exporters` max 4, `num_flow_exporter_interfaces` max 128,
`num_flows` max 131072 (build-time), `num_hosts` max 0 (= unlimited).

## 8. Frozen `TrafficConnectorInterface`

The authoritative frozen interface is in design §4.1. Changes against the
pre-validation draft, with reasons:

1. **`TrafficHostEntry` gains `flags` (multicast/broadcast/local/blacklisted
   bits), `firstSeen`/`lastSeen`.** The matching pass needs the flags to skip
   non-matchable entries (§5); the timestamps come free and feed
   `observation_point_hosts`.
2. **`MetricDefinition` renamed to `TrafficMetricDefinition`.**
   `cloud-connector.h` already defines `MetricDefinition` in the global
   namespace; both headers will be included by `session.cpp`. Traffic metrics
   carry no aggregation dimension, so the struct is simpler (name, display name,
   unit, description, table flag, required capability).
3. **`GetActiveHosts` returns a heap array with count, not `StructArray`** —
   entries contain `InetAddress`/`MacAddress` value objects; ownership stays
   trivially with the caller (`ObjectArray<TrafficHostEntry>` with
   `Ownership::True`).
4. **`SyncHostPools` signature fixed to `const StringMap&`** (pool name →
   comma-separated `CIDR@vlan` member list) — matches the validated ntopng wire
   format and stays connector-neutral.
5. **`wchar_t` replaces `TCHAR`** per current server code conventions
   (server is Unicode-only; `cloud-connector.h` predates the rule).
6. Sync function pointers are explicitly nullable (= unsupported); ntopng sets
   `SyncLocalNetworks = nullptr` (§7).

## 9. Residual Risks / Follow-ups

- **Pro/Enterprise instance not validated.** Edition string values of
  `about.lua:release` beyond "Community", Pro capability differences
  (`HISTORICAL_TIMESERIES`/ClickHouse, higher pool limits) and any Pro-only
  endpoint behavior must be validated when a Pro instance is available.
  Tracked as an explicit Phase 1 exit criterion.
- **Scale not validated** (8 active hosts): `perPage` upper bound, response
  time and paging behavior of `host/active.lua` at 10k+ hosts.
- `set/pool/members.lua` parameter mismatch suggests REST v2 endpoints are not
  uniformly maintained — integration tests per supported ntopng version remain
  necessary (design §14 unchanged).
- ntopng reports epoch timestamps in server-local time zone context
  (`localtime` field available in `interface/data.lua`) — verify clock-skew
  handling when computing staleness in Phase 2.
