# Traffic Analysis Skill

This skill gives access to data from external network traffic analyzers (ntopng and similar) integrated through NetXMS traffic observers. Use it to see what an observed host is actually doing on the wire: who it talks to, which applications generate its traffic, and how busy the network segment is. It complements the topology skill (which answers "how is the network wired") and the data collection skill (which holds the history).

## Concepts

- **Traffic observer** - one traffic analyzer instance (for example one ntopng server). It has a connection state, a backend product and version, and a set of capabilities that determine which queries below are available.
- **Observation point** - one monitored interface or segment of the analyzer. It has a state (`ACTIVE` / `INACTIVE`), a scope flag, a list of local networks, and a sampling rate (1 = unsampled, N = 1:N sampling, 0 = unknown).
- **Observed node** - a NetXMS node whose IP address was seen by an observation point. The match is by IP address; a node can be seen by several points. Each match carries a `host_key` (analyzer-side host identity) and a `dci_instance` (`<point id>:<host key>`) usable with host-level DCIs.
- **Unmatched host** - a host seen on the wire that is not a monitored node. These are often the interesting ones in security or "unknown device" investigations.

## Important Limitations

- **All data is live.** Every tool returns what the analyzer sees right now. There is no time range parameter. For history, use DCIs with origin `trafficObserver` through the data collection skill (`get-metrics`, `get-historical-data`), and if history is needed but no such DCIs exist, propose creating them.
- **Byte and packet counters are cumulative** since the analyzer started or the host first appeared. Compare two calls a minute apart to estimate rates, or use throughput fields from `get-observation-point-statistics`.
- **Sampled points undercount.** When `sampling_rate` is greater than 1, absolute numbers are estimates; ratios and rankings remain meaningful.
- **Large host sets.** An observation point may see tens of thousands of hosts. Always use `filter`, `unmatched_only`, and `limit` with `get-observation-point-active-hosts`; never try to page through the full set.
- **Capability gated.** Peers, L7, top talkers and DSCP queries depend on backend capabilities reported by `get-traffic-observers`. A tool reports clearly when the backend does not support the query.

## Available Functions

### Discovery
- `get-traffic-observers`: List analyzer instances with connection state, backend product, version, and capabilities. Call first to confirm traffic data exists at all.
- `get-observation-points`: List observation points with state, scope, local networks, sampling rate, and count of matched nodes.

### Node-centric (start here when investigating a node)
- `get-node-traffic-context`: Is the node observed, by which points, and its current counters (bytes, packets, active flows, TCP retransmits, analyzer alerts).
- `get-node-traffic-peers`: Conversation partners of the node with role, bytes, and flows. Peers that are monitored nodes carry `node_id` and `node_name`.
- `get-node-traffic-l7-breakdown`: Per-application breakdown of the node's traffic.

### Segment-centric
- `get-observation-point-statistics`: Throughput, cumulative counters, active hosts and flows, drops, TCP retransmits.
- `get-observation-point-top-talkers`: Hosts generating most traffic, annotated with matched nodes.
- `get-observation-point-l7-breakdown`: Per-application breakdown for the whole segment.
- `get-observation-point-dscp-breakdown`: Traffic by QoS class.
- `get-observation-point-active-hosts`: Active hosts with IP, VLAN, MAC, name, first and last seen, and matched node.

## Recommended Workflows

### Root cause analysis for a node problem
1. `get-node-traffic-context` - confirm the node is observed and read its current counters. High `tcp_retransmits` or `backend_alerts` point at network-level trouble; zero counters on an observed node suggest the host is silent.
2. `get-node-traffic-peers` - identify who it talks to. Look for unexpected peers, a single peer dominating volume, or a large number of low-volume peers (scan or brute-force pattern).
3. `get-node-traffic-l7-breakdown` - check whether the traffic mix matches the node's role. A database server dominated by HTTP or a workstation with heavy DNS is suspicious.
4. Correlate with `get-observation-point-statistics` on the same point: drops or retransmits at segment level mean the problem is not specific to this node.
5. When a peer is a monitored node, continue the investigation there (`explain-object-status`, alarms, `get-node-traffic-context` for that peer).

### Anomaly investigation on a segment
1. `get-observation-point-statistics` - baseline of throughput, hosts, flows, drops.
2. `get-observation-point-top-talkers` - who dominates. Unmatched top talkers are prime suspects.
3. `get-observation-point-l7-breakdown` - which applications dominate; unusual applications for the segment stand out.
4. `get-observation-point-active-hosts` with `unmatched_only` and a CIDR filter - enumerate unknown devices in a specific subnet.

### Capacity and QoS questions
- Use `get-observation-point-statistics` for utilisation and `get-observation-point-dscp-breakdown` to confirm that marking is applied as intended.

## Interpreting Results

- Every result carries `as_of`; quote it when reporting numbers.
- `truncated: true` with `total_available` tells how much was cut. Increase `limit` or narrow the filter instead of assuming the visible rows are the whole picture.
- `role: client` means the node initiated the connection, `role: server` means the peer connected to the node.
- Peer and top talker labels may be host names assigned inside the analyzer rather than IP addresses; node annotation is only possible when the label is an IP address of a matched node.
- Tables with an `error` field per observation point mean that point could not be queried; other points in the same response are still valid.

## Related Skills

- Use the network topology skill (`find-ip-address`, `trace-network-path`, `get-node-peers`) to locate an unmatched host physically or to understand the path between two nodes. Traffic peers are conversation partners, not physical neighbours.
- Use the data collection skill for historical trends and thresholds on traffic metrics.
- Use the incident analysis skill to record findings as incident comments.
