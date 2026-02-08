# Prometheus Subagent Configuration

## Overview

The Prometheus subagent receives metrics via Prometheus remote write protocol and pushes selected metrics to the NetXMS server.

## Configuration Section

All configuration options are placed in the `[Prometheus]` section of the agent configuration file.

### Server Options

```ini
[Prometheus]
Port = 9090
Address = 127.0.0.1
Endpoint = /api/v1/write
```

- **Port** (required) - TCP port to listen on for incoming Prometheus remote write requests
- **Address** (optional, default: 127.0.0.1) - IP address to bind to
- **Endpoint** (optional, default: /api/v1/write) - HTTP endpoint path for receiving data

### Metric Mappings

Define which Prometheus metrics should be forwarded to NetXMS and how they should be named.

#### Format 1: Based on Prometheus metric value

```ini
Metric=prometheus_name:netxms_name:name_arguments
```

Pushes the metric's numeric value.

**Parameters:**
- `prometheus_name` - Name of the Prometheus metric (value of `__name__` label)
- `netxms_name` - Base name for the NetXMS metric
- `name_arguments` - Comma-separated list of label names to use as metric arguments

**Example:**

```ini
Metric=node_network_transmit_drop_total:Net.Interface.TxDrops:nodename,device
```

Incoming data:
```
node_network_transmit_drop_total{device="eth0", instance="server1.local", job="vm-stats", nodename="server1"} = 42
```

Pushed to NetXMS:
```
Net.Interface.TxDrops(server1,eth0) = 42.0
```

#### Format 2: Based on metrics attribute

```ini
Metric=prometheus_name:netxms_name:name_arguments:value_arguments
```

Pushes concatenated label values as a string.

**Parameters:**
- `prometheus_name` - Name of the Prometheus metric
- `netxms_name` - Base name for the NetXMS metric
- `name_arguments` - Comma-separated list of label names to use as metric arguments
- `value_arguments` - Comma-separated list of label names to concatenate as the value (space-separated)


Actual value from the data sample is ignored.


**Example:**

```ini
Metric=node_uname_info:Node.Uname:nodename:sysname,release,version
```

Incoming data:
```
node_uname_info{domainname="(none)", instance="server1.local", job="vm-stats", machine="x86_64", nodename="server1", release="5.14.0-570.17.1.el9_6.x86_64", sysname="Linux", version="#1 SMP PREEMPT_DYNAMIC Tue May 13 06:41:18 EDT 2025"}
```

Pushed to NetXMS:
```
Node.Uname(server1) = "Linux 5.14.0-570.17.1.el9_6.x86_64 #1 SMP PREEMPT_DYNAMIC Tue May 13 06:41:18 EDT 2025"
```

## Complete Example

```ini
[Prometheus]
Port = 9090
Address = 0.0.0.0
Endpoint = /api/v1/write

Metric=node_network_transmit_drop_total:Net.Interface.TxDrops:nodename,device
Metric=node_network_receive_drop_total:Net.Interface.RxDrops:nodename,device
Metric=node_uname_info:Node.Uname:nodename:sysname,release,version
```

## Behavior Notes

- Only metrics with configured mappings are processed; others are ignored
- Unmatched metrics are logged at debug level 6
- If a required label is missing, an empty string is used for that argument
- Label values are not escaped; special characters (parentheses, commas) may cause issues
- Configuration is loaded at agent startup; restart required for changes
- Multiple samples per metric are supported; each sample is pushed separately
