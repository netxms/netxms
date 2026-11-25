# Traffic Observer — Example DCI Template

This document describes an example data collection template that turns the host
records discovered by a `TrafficObserver` into per-node traffic DCIs. It is
shipped as documentation, not force-applied: users create the template once and
let the standard auto-bind + instance-discovery machinery do the rest.

The mechanism relies entirely on Phase 2 building blocks:

- the internal table **`Traffic.ObservationPoints`** served on every `Node`
  (one row per host record referencing the node), and
- the NXSL `Node` attribute **`$node->isObserved`** for the auto-bind filter.

No core code creates DCIs automatically — the template is the single point of
control.

## 1. Create the template

Create a template object (e.g. `Traffic — Per-host metrics`) under *Templates*.

### Auto-bind filter

Set the template's automatic apply filter script to:

```nxsl
return $node->isObserved;
```

Binding, instance fan-out, and instance retirement then follow the standard
mechanisms: the template binds to any node that is observed by at least one
observation point and unbinds when it is no longer observed.

## 2. Add DCI prototypes

Each prototype uses:

- **Origin:** `Traffic Observer`
- **Instance discovery method:** `Internal table`
- **Instance discovery data:** `Traffic.ObservationPoints`
- **Metric name:** the host-level metric with the `{instance}` macro, e.g.
  `Host.BytesIn({instance})`

The instance key produced by the internal table is
`<pointObjectId>:<hostKey>` (e.g. `4211:10.5.5.100@0`). At collection time
`{instance}` expands to that key, which the server resolves back to the observing
point and host, then serves through the connector's short-TTL host cache.

Suggested scalar prototypes (all origin `Traffic Observer`):

| Prototype metric | Meaning |
|---|---|
| `Host.BytesIn({instance})` | Bytes received by host (served from the active-list cache) |
| `Host.BytesOut({instance})` | Bytes sent by host |
| `Host.PacketsIn({instance})` | Packets received (per-host detail call) |
| `Host.PacketsOut({instance})` | Packets sent |
| `Host.ActiveFlows({instance})` | Active flows for host |
| `Host.TcpRetransmits({instance})` | TCP retransmissions (both directions) |
| `Host.Alerts({instance})` | Engaged alerts for host |
| `Host.Present({instance})` | 1 while the host is in the analyzer's active table, 0 otherwise |

Table prototypes (capability-gated; add only if the observer reports the bit):

| Prototype metric | Requires capability |
|---|---|
| `Host.L7Breakdown({instance})` | `HOST_L7` |
| `Host.Peers({instance})` | `HOST_PEERS` |

The DCI editor metric picker (the *Select…* button next to the metric field for
origin `Traffic Observer`) offers exactly these names, filtered by the
observer's capability set, so they need not be typed by hand.

## 3. Presence and instance lifecycle

- Absence of a host from the analyzer's active table yields `DCE_NO_SUCH_INSTANCE`
  at collection time. This starts the standard instance grace period (so stale
  instances self-clean) and can drive error-count thresholds ("host disappeared
  from SPAN").
- Operators who want a chartable/thresholdable presence series add
  `Host.Present({instance})`; it returns `0` instead of `NO_SUCH_INSTANCE` while
  the host record still exists.
- Host match records themselves are refreshed on each observer configuration poll
  and aged out by the housekeeper after `TrafficObserver.HostRetentionTime` days
  (default 7); the corresponding instances retire on the next discovery.

## 4. Friendly instance names (optional)

An instance filter script on each prototype can replace the raw
`<pointObjectId>:<hostKey>` display name with an operator-friendly one and link
the DCI to the observation point:

```nxsl
// $1 = instance key "<pointObjectId>:<hostKey>"
// $2 = StringMap of the Traffic.ObservationPoints row for this instance
sub main()
{
   instance = $2;
   $node->setInstanceName(instance->get("POINT") . " / " . instance->get("HOST_KEY"));
   op = FindObject(instance->get("POINT_ID"));
   if (op != null)
      $node->setRelatedObject(op);
   return true;
}
```

Multi-IP, multi-VLAN, and multi-point observation all fall out naturally: there
is one DCI instance per `(observation point, host record)`, with no automatic
aggregation. A node observed by points of different observers simply yields more
instances, keyed by distinct point object ids.
