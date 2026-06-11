# Prometheus Subagent Configuration

## Overview

The Prometheus subagent integrates Prometheus-style metrics into NetXMS in two ways:

- **Remote write receiver** (passive) - receives metrics pushed via Prometheus remote write protocol
- **Endpoint scraper** (active) - periodically scrapes `/metrics` endpoints exposing Prometheus text exposition format / OpenMetrics

Both modes can be used at the same time and share the same metric mapping mechanism for pushing selected metrics to the NetXMS server.

## Configuration Section

All common configuration options are placed in the `[Prometheus]` section of the agent configuration file.

### Receiver Options

```ini
[Prometheus]
EnableReceiver = yes
Port = 9090
Address = 127.0.0.1
Endpoint = /api/v1/write
```

- **EnableReceiver** (optional, default: yes) - enable or disable the remote write receiver
- **Port** (optional, default: 9090) - TCP port to listen on for incoming Prometheus remote write requests
- **Address** (optional, default: 127.0.0.1) - IP address to bind to
- **Endpoint** (optional, default: /api/v1/write) - HTTP endpoint path for receiving data

### Scrape Targets

Each scrape target is defined in its own `[Prometheus/Targets/name]` section, where `name` is a target name of your choice (used as the first argument of `Prometheus.*` metrics):

```ini
[Prometheus/Targets/kafka]
URL = http://localhost:9308/metrics
ScrapeInterval = 60
Timeout = 10
Login = monitoring
Password = secret
VerifyCertificate = yes
VerifyHost = yes
```

- **URL** (required) - HTTP or HTTPS URL of the metrics endpoint; can point to localhost or a remote host (agent acting as proxy)
- **ScrapeInterval** (optional, default: 60) - scrape interval in seconds
- **Timeout** (optional, default: 10) - request timeout in seconds
- **Login** / **Password** (optional) - credentials for HTTP basic authentication; password can be encrypted with nxencpasswd
- **BearerToken** (optional) - token for bearer authentication (takes precedence over Login/Password)
- **VerifyCertificate** (optional, default: yes) - verify server certificate when using HTTPS
- **VerifyHost** (optional, default: yes) - verify that server certificate matches host name when using HTTPS
- **CACertificate** (optional) - path to CA certificate bundle used for server certificate validation

### Metric Mappings

Define which Prometheus metrics should be forwarded to NetXMS as push DCIs and how they should be named. Mappings apply to metrics received via remote write and to scraped samples alike.

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

## Provided Metrics

Scraped samples are kept in memory (last scrape per target) and can be queried directly, without configuring mappings:

| Metric | Description |
|--------|-------------|
| `Prometheus.Value(target, metric, label=value...)` | Value of given metric; optional `label=value` arguments select a specific time series |
| `Prometheus.Target.Status(target)` | 1 if last scrape of given target was successful, 0 otherwise |
| `Prometheus.Target.LastScrapeTime(target)` | UNIX timestamp of last scrape attempt |
| `Prometheus.Target.SampleCount(target)` | Number of samples collected by last successful scrape |

| List | Description |
|------|-------------|
| `Prometheus.Targets` | Names of configured scrape targets |
| `Prometheus.LabelValues(target, metric, label)` | Unique values of given label across all time series of given metric |

| Table | Description |
|-------|-------------|
| `Prometheus.Targets` | Configured scrape targets with status information |
| `Prometheus.Series(target, metric)` | One row per time series of given metric: canonical labelset, one column per label, and current value |

## Instance Discovery

Lists and tables above are designed for DCI instance discovery, so prototype DCIs can auto-instantiate per labelset:

- **Single label** (e.g. one instance per Kafka topic): use *Agent List* `Prometheus.LabelValues(kafka, kafka_topic_partitions, topic)` for discovery and `Prometheus.Value(kafka, kafka_topic_partitions, topic={instance})` as the prototype metric.
- **Multiple labels**: use *Agent Table* `Prometheus.Series(target, metric)` for discovery; the instance column `LABELS` contains the labelset in `label=value,label=value` form (values quoted when necessary), which can be passed directly as label filters: `Prometheus.Value(target, metric, {instance})`.

## Complete Example

```ini
[Prometheus]
Port = 9090
Address = 0.0.0.0
Endpoint = /api/v1/write

Metric=node_network_transmit_drop_total:Net.Interface.TxDrops:nodename,device
Metric=node_network_receive_drop_total:Net.Interface.RxDrops:nodename,device
Metric=node_uname_info:Node.Uname:nodename:sysname,release,version

[Prometheus/Targets/haproxy]
URL = http://127.0.0.1:8404/metrics
ScrapeInterval = 30

[Prometheus/Targets/etcd]
URL = https://etcd1.example.com:2379/metrics
CACertificate = /etc/ssl/etcd-ca.pem
BearerToken = exampletoken
```

## Behavior Notes

- Mappings: only metrics with configured mappings are pushed; others are ignored (logged at debug level 6)
- If a required label is missing, an empty string is used for that argument
- Configuration is loaded at agent startup; restart required for changes
- Multiple samples per metric are supported; each sample is pushed separately
- Scraper keeps data of the last scrape only; if a scrape fails, data from the previous successful scrape remains available (monitor `Prometheus.Target.Status` to detect scrape failures)
- Histogram and summary metrics are exposed by Prometheus endpoints as separate `_bucket`/`_sum`/`_count` series and can be queried as such
- `# HELP`, `# TYPE`, and other comment lines, timestamps, and OpenMetrics exemplars are ignored by the parser
