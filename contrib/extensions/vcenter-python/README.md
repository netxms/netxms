# vCenter agent extension

NetXMS agent extension that pulls CPU / memory utilization, datastore usage and
VM / host inventory from a VMware vCenter (or standalone ESXi) server.

It speaks the [agent extension protocol](../../../doc/AGENT_EXTENSION_PROTOCOL.md)
in **spawn mode**: the NetXMS agent launches and supervises it.

## Requirements

```bash
pip install pyvmomi
```

## How it works

vCenter is polled once on a background timer (`VC_POLL_INTERVAL`, default 60 s)
into an in-memory cache, using a single PropertyCollector retrieval per entity
type. Every `get_metric` / `get_list` / `get_table` is served from that cache,
so per-DCI polling never hits vCenter.

The instance key for VMs, datastores and hosts is the **managed-object ID
(MOID)**, which survives renames. Names are exposed as a label column / list
value.

## Configuration

Environment variables (set via the agent's `Environment =` keys):

| Variable | Default | Notes |
|---|---|---|
| `VC_HOST` | — | vCenter / ESXi hostname (required) |
| `VC_USER` | — | Login, e.g. `monitor@vsphere.local` (required) |
| `VC_PASSWORD` | — | Password (`VC_PASS` also accepted) |
| `VC_PORT` | `443` | |
| `VC_POLL_INTERVAL` | `60` | Poll period, seconds |
| `VC_VERIFY_TLS` | `no` | `yes` to verify the server certificate |

`nxagentd.conf`:

```
*extensions/vcenter
Mode = spawn
Command = /usr/bin/python3 /opt/netxms/extensions/vcenter_extension.py
Environment = VC_HOST=vc.example.com
Environment = VC_USER=monitor@vsphere.local
Environment = VC_PASSWORD=secret
Environment = VC_POLL_INTERVAL=60
DebugTag = extension.vcenter
```

## Exposed data

### Metrics (parameter = MOID)

| Metric | Type |
|---|---|
| `VMware.VM.Name(*)` | string |
| `VMware.VM.CPUUsage(*)` | float % |
| `VMware.VM.CPUUsageMHz(*)` | uint |
| `VMware.VM.MemoryUsage(*)` | float % |
| `VMware.VM.MemoryUsageBytes(*)` | uint64 |
| `VMware.VM.PowerState(*)` | string |
| `VMware.VM.UptimeSeconds(*)` | uint |
| `VMware.VM.StorageCommittedBytes(*)` | uint64 |
| `VMware.Datastore.Name(*)` | string |
| `VMware.Datastore.UsagePerc(*)` | float % |
| `VMware.Datastore.FreeBytes(*)` | uint64 |
| `VMware.Datastore.CapacityBytes(*)` | uint64 |
| `VMware.Host.Name(*)` | string |
| `VMware.Host.CPUUsage(*)` | float % |
| `VMware.Host.CPUUsageMHz(*)` | uint |
| `VMware.Host.MemoryUsage(*)` | float % |
| `VMware.Host.MemoryUsageBytes(*)` | uint64 |
| `VMware.Host.NumVMs(*)` | uint |
| `VMware.Host.ConnectionState(*)` | string |

### Lists (for instance discovery → MOIDs)

`VMware.VMList`, `VMware.DatastoreList`, `VMware.HostList`

### Tables

`VMware.VMs`, `VMware.Datastores`, `VMware.Hosts`

## Instance discovery

To create per-VM DCIs automatically: set a DCI's instance discovery method to
**Agent List**, point it at `VMware.VMList`, and use `{instance}` in the metric
name, e.g. `VMware.VM.CPUUsage({instance})`. Same pattern works for datastores
(`VMware.DatastoreList`) and hosts (`VMware.HostList`). Tables support instance
discovery as well.

The discovered instance value is the **MOID** (stable across renames). To show
the human-readable name in DCI descriptions (`{instance-name}`) while keeping the
MOID as the instance key, add an instance filter that resolves the name via the
`Name` metric, e.g. for VMs:

```nxsl
name = AgentReadParameter($node, "VMware.VM.Name(" .. $1 .. ")");
return [true, $1, (name != null and name != "") ? name : $1];
```

(use `VMware.Datastore.Name` / `VMware.Host.Name` for the other two). The ready
-made template `contrib/templates/vmware_vcenter_esxi.json` already includes this
filter on every DCI.
