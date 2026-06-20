# Test plan — OTLP metric export (issue #3304)

Tests the OTLP metric export driver: collected DCI values exported as OTLP
metrics over HTTP/protobuf to an external collector.

- Implementation: `src/server/otlp/metric_export.cpp` (driver `OtlpMetricExportDriver`,
  registered as PDS driver name **`OTLP`**), shared helpers in `src/server/otlp/otlp.h`.
- Registration path: `RegisterOtlpMetricExporter()` ← module `InitModule()`
  (`src/server/otlp/main.cpp:55`) ← `RegisterPerfDataStorageDriver()`
  (`src/server/core/pds.cpp:53`, issue #3303).
- Fan-out into the driver: `PerfDataStorageRequest(DCItem*, …)`
  (`src/server/core/pds.cpp:148`) called from `DCItem::processNewValue`
  (`src/server/core/dcitem.cpp:1292`) **only for retained, transformed values**
  when `AF_PERFDATA_STORAGE_DRIVER_LOADED` is set.
- Internal metrics surfaced as `Server.PDS.DriverStat(OTLP, <metric>)`
  (`src/server/core/server_stats.cpp:704`).
- Debug tag: **`otlp.metrics`** (`doc/internal/debug_tags.txt:172`). Run server
  with `-D6` (level 6 for batch delivery) or `-D7` (per-value decisions).

## Conventions / how to run

- **Tooling:** `otlp_mock_collector.py` in this directory (see `README.md`).
- **Config block** in `netxmsd.conf`:
  ```
  [OpenTelemetry/MetricExport]
  Endpoint = http://127.0.0.1:4318/v1/metrics
  BatchSize = 10
  FlushInterval = 2
  Timeout = 10
  QueueSizeLimit = 1000
  ```
- **Trigger data flow fast:** create internal DCIs with a short poll interval
  (e.g. `Status`, `Dummy`, `Server.AverageDCIQueuingTime`) or push values with
  `nxpush` / NXSL `PushDCIData`. Remember: only values that pass transformation
  and are retained reach the driver.
- **Read internal metrics** from a client or `nxget`-style query against the
  server: `Server.PDS.DriverStat(OTLP, queueSize)` and
  `Server.PDS.DriverStat(OTLP, messageDrops)`.

---

## 1. Build & static checks

| # | Test | Expected |
|---|------|----------|
| 1.1 | Build server with otlp module (`./configure --with-server`; `make -j$(nproc)`) | otlp module builds; `metric_export.cpp` compiles & links |
| 1.2 | Windows build (`otlp.vcxproj` includes `metric_export.cpp`) | builds; no `nx_swprintf`/`%s` portability issues |
| 1.3 | Debug tag registered | `otlp.metrics` present in `doc/internal/debug_tags.txt` |
| 1.4 | Driver count cap | `MAX_PDS_DRIVERS` (8) not exceeded when OTLP + ClickHouse + others all active |

## 2. Registration & lifecycle

| # | Test | Expected |
|---|------|----------|
| 2.1 | **Not configured** (no `Endpoint`) | `RegisterOtlpMetricExporter` is a no-op; log `OTLP metric export is not configured` (tag `otlp.metrics`, level 2); driver **not** registered; `AF_PERFDATA_STORAGE_DRIVER_LOADED` not set by OTLP |
| 2.2 | **Configured** with valid `Endpoint` | `init()` succeeds; log `…driver initialized (endpoint=…, batchSize=…, flushInterval=…)`; `INFO: Performance data storage driver OTLP registered`; flush thread starts (`flush thread started`) |
| 2.3 | **Endpoint set but cURL init fails** (simulate) | `init()` returns false; `RegisterPerfDataStorageDriver` deletes the instance and logs init failure; server continues without OTLP export |
| 2.4 | Empty `Endpoint=` value (key present, blank) | treated as not configured (2.1 path); no driver |
| 2.5 | Clean shutdown (`netxmsd` stop) | `shutdown()` sets stop, wakes thread; **final flush** of remaining buffer happens in `flushThread` before exit; log `flush thread stopped` then `…shutdown completed`; no leak/crash (run under ASAN if available) |
| 2.6 | Shutdown ordering | module `pfShutdown` runs before `StopDataCollection`; verify no fan-out after driver deleted (no use-after-free) |

## 3. Configuration parsing (`init`)

| # | Test | Expected |
|---|------|----------|
| 3.1 | Defaults | unset `BatchSize`/`FlushInterval`/`Timeout`/`QueueSizeLimit` → 100 / 5s / 30s / 10000 |
| 3.2 | Zero / invalid values | `BatchSize=0`→100; `FlushInterval=0`→5; `Timeout<=0`→30 (see `init()` guards) |
| 3.3 | `QueueSizeLimit=0` | limit disabled — buffer unbounded (verify no drops under load; document risk) |
| 3.4 | `AuthToken=secret` | request carries `Authorization: Bearer secret` — assert with `--auth "Bearer secret"` |
| 3.5 | `Header=X-A: 1` (single) | header present — `--expect-header "X-A: 1"` |
| 3.6 | `Header` repeated (multi-line) | **all** values applied (loop over `getValueCount()`) — assert several `--expect-header` |
| 3.7 | `VerifyPeer=no` | TLS peer/host verification disabled (test against self-signed HTTPS collector) |
| 3.8 | `VerifyPeer=yes` + self-signed | delivery fails (curl verify error), batch retried then dropped |
| 3.9 | Always-on header | `Content-Type: application/x-protobuf` present on every request |

## 4. Value conversion & validation (`saveDCItemValue`)

Drive each DCI data type and confirm the data point on the collector.

| # | DCI transformed data type / value | Expected on collector |
|---|-----------------------------------|------------------------|
| 4.1 | `DCI_DT_INT` / `-42` | `as_int = -42` (signed, base auto via `wcstoll(…,0)`) |
| 4.2 | `DCI_DT_INT64` / large | `as_int` int64 |
| 4.3 | `DCI_DT_UINT`,`UINT64`,`COUNTER32`,`COUNTER64` | parsed via `wcstoull`, stored as int64 (`as_int`) |
| 4.4 | `DCI_DT_FLOAT` / `42.5` | `as_double = 42.5` |
| 4.5 | `DCI_DT_STRING` / null | **dropped**, not sent; D7 log `data type cannot be represented as OTLP number` |
| 4.6 | Empty value (`*value==0`) | returns early, nothing enqueued |
| 4.7 | Non-numeric value for numeric type (e.g. `"abc"`) | dropped; D7 log `did not pass number validation` (trailing `*eptr!=0`) |
| 4.8 | Trailing garbage (`"42x"`) | dropped (`*eptr!=0`) |
| 4.9 | Out-of-range (`ERANGE`, e.g. huge int) | dropped; D7 validation-failure log |
| 4.10 | Hex value (`"0x1F"` for int type) | accepted (base 0 → 31) |

## 5. Metric kind mapping (gauge vs sum) — decision table

`OtlpMetricKind` is derived from **delta calculation method** × **counter-ness**.
Verify the metric's protobuf shape (the collector prints `kind`, `monotonic`,
`temporality`).

| # | Delta method | Data type | Expected kind / monotonic / temporality |
|---|--------------|-----------|------------------------------------------|
| 5.1 | `DCM_ORIGINAL_VALUE` | plain (int/float/uint) | **Gauge**, monotonic n/a |
| 5.2 | `DCM_ORIGINAL_VALUE` | `COUNTER32/64` | **Sum**, monotonic=true, **CUMULATIVE** |
| 5.3 | `DCM_SIMPLE` | plain | **Sum**, monotonic=false, **DELTA** |
| 5.4 | `DCM_SIMPLE` | `COUNTER32/64` | **Sum**, monotonic=true, **DELTA** |
| 5.5 | `DCM_AVERAGE_PER_SECOND` | any | **Gauge**, monotonic=false (already a rate) |
| 5.6 | `DCM_AVERAGE_PER_MINUTE` | any | **Gauge**, monotonic=false |

(Mirror image of the ingest split in `receiver.cpp`; cross-check that round-trip
ingest→export of a counter keeps the same kind.)

### 5a. Sum `start_time_unix_nano`

OTLP gives `start_time_unix_nano` a different meaning per temporality, and NetXMS
only has truthful data for one of them. Each data point exposes both
`time_unix_nano` (always = collection timestamp) and `start_time_unix_nano`.

| # | Sum kind | Meaning of start time | NetXMS source | Expected |
|---|----------|-----------------------|---------------|----------|
| 5a.1 | **CUMULATIVE** (`DCM_ORIGINAL_VALUE` + counter) | when the counter last reset / began accumulating from zero | not tracked — server only sees collection timestamps, never the device counter's reset epoch | `start_time_unix_nano = 0` (unset). Correct & spec-permitted; **do not** fabricate (process-start / first-seen would be a wrong reset time → mis-detected resets). See `feedback_no_synthetic_state` |
| 5a.2 | **DELTA** (`DCM_SIMPLE`) | start of this delta's interval | previous collection timestamp, plumbed through the PDS API as `startTimestamp` (captured in `DCItem::processNewValue` before `m_prevValueTimeStamp` is advanced) | `start_time_unix_nano` = previous sample time, `time_unix_nano` = current sample time, `end > start` |
| 5a.3 | DELTA, **first** sample after start/restart (no previous value yet) | — | `startTimestamp` is null | `start_time_unix_nano = 0` (unset) for that first point; populated from the second sample on |
| 5a.4 | GAUGE (all gauge cases in §5) | n/a | — | `start_time_unix_nano` not set |

Implementation status: **implemented.** The PDS fan-out signature carries the
interval-start timestamp: `PerfDataStorageRequest(DCItem*, timestamp,
startTimestamp, value)` → `PerfDataStorageDriver::saveDCItemValue(…, Timestamp
startTimestamp, …)`. `dcitem.cpp` captures the previous-value timestamp before it
is overwritten and passes it; the OTLP driver sets `start_time_unix_nano` only on
the `SUM_DELTA` path (`metric_export.cpp`), leaving it 0 for gauges (5a.4) and
cumulative sums (5a.1). Other PDS drivers (ClickHouse, InfluxDB) accept and ignore
the new parameter.

## 6. Metric naming & description

| # | Data source / name | Expected metric `name` / `description` |
|---|--------------------|-----------------------------------------|
| 6.1 | `DS_SNMP_AGENT` | name = **DCI description** (OID is not meaningful) |
| 6.2 | `DS_MODBUS` | name = DCI description |
| 6.3 | `DS_INTERNAL`, DCI name begins `Dummy` | name = DCI description |
| 6.4 | Native agent metric (e.g. `System.CPU.Usage`) | name = **DCI name**; if description differs → `description` set; if equal → description omitted |
| 6.5 | DCI name == description | `description` field omitted (no duplication) |
| 6.6 | `unit` set on DCI | metric `unit` = `getUnitName()`; empty unit → field omitted |

## 7. Attributes

Data-point attributes (per value) and resource attributes (per node).

| # | Test | Expected |
|---|------|----------|
| 7.1 | Always present | data point has `netxms.dci.id` (int), `netxms.data_source` (string from `getDataProviderName`) |
| 7.2 | Instance DCI | `netxms.instance` present and = instance name; non-instance DCI → attribute absent |
| 7.3 | Related object set | `netxms.related_object` = related object name |
| 7.4 | Custom attr `tag:env=prod` on owner | data-point attr `env=prod` (4-char prefix stripped) |
| 7.5 | Custom attr `tag_region=eu` on owner | attr `region=eu` (underscore form also accepted) |
| 7.6 | Empty-valued `tag:x=` | attribute skipped (value `[0]==0`) |
| 7.7 | tag attrs on **template** of an instance-discovery DCI | resolved via parent template (`templateId==ownerId` branch) and included |
| 7.8 | tag attrs on related object | included (collected from all three: template, owner, related) |
| 7.9 | Resource attributes | each resource carries `service.name`, `host.name`, `host.id` (object GUID) |
| 7.10 | `ServiceName` config override | `service.name` = override value for all resources; unset → `service.name` = host name |

## 8. Opt-out selection (`pds:ignore` / `otel:ignore`)

| # | Where flag set | Expected |
|---|----------------|----------|
| 8.1 | `pds:ignore` on owner node | DCI **not** exported; D7 log `ignore flag set on owner object` |
| 8.2 | `otel:ignore` on owner node | same (both spellings honored, case-insensitive) |
| 8.3 | `pds:ignore` on **template** | instance DCIs from template not exported; D7 `ignore flag set on template object` |
| 8.4 | `pds:ignore` on **related** object | not exported; D7 `ignore flag set on related object` |
| 8.5 | Flag removed at runtime | export resumes on next value (no restart needed; CAs read each call) |
| 8.6 | Mixed: flag on one node only | other nodes still export |

## 9. Batching, grouping & merging (`sendBatch`)

| # | Test | Expected |
|---|------|----------|
| 9.1 | Values from 2 nodes in one batch | **2 ResourceMetrics** (grouped by node id); correct per-node resource attrs |
| 9.2 | Same metric name+kind+unit, multiple samples (same node) | merged into **one Metric** with multiple `data_points` (see `metricByKey`) |
| 9.3 | Same name, different unit | **separate** Metric entries (unit in merge key) |
| 9.4 | Same name, different kind/monotonic | separate Metric entries (kind+monotonic in merge key) |
| 9.5 | Same metric name across **different** nodes | separate metrics (node id in merge key) and separate resources |
| 9.6 | Gauge vs sum routing of data points | gauge → `metric.gauge.data_points`; sum → `metric.sum.data_points` |
| 9.7 | Timestamp | `time_unix_nano` = DCI value ms × 1e6 (ms→ns). Verify ms precision preserved. For sums, `start_time_unix_nano` per §5a |

## 10. Flush timing

| # | Test | Expected |
|---|------|----------|
| 10.1 | Buffer reaches `BatchSize` | immediate wakeup (`m_wakeup.set()`), batch sent before `FlushInterval` elapses |
| 10.2 | Below batch size | flushed on the `FlushInterval` timer tick |
| 10.3 | Idle (no data) | timer fires, buffer empty → `continue`, no HTTP request |
| 10.4 | Whole buffer swapped per flush | `batch.swap(m_buffer)` — buffer emptied atomically; new values during send go to a fresh buffer |

## 11. HTTP transport & failure handling

| # | Collector mode | Expected driver behavior |
|---|----------------|--------------------------|
| 11.1 | `ok` (2xx empty) | batch accepted; D6 `batch of N data points delivered` |
| 11.2 | `empty-ok` (2xx no body) | treated as success (response parse skipped when size 0) |
| 11.3 | `partial` (`rejected_data_points>0`) | **WARNING** `OTLP collector rejected N data points (<msg>)`; batch **not** retried (server accepted) |
| 11.4 | `http-error --status 503` | retried up to `SEND_RETRY_COUNT` (2) → 3 attempts total; curl handle recreated between attempts; ultimately batch dropped, WARNING `Failed to deliver … dropped`, `messageDrops += batch size` |
| 11.5 | `http-error --status 400` | same retry/drop path (any non-2xx) |
| 11.6 | Connection refused (collector down) | `curl_easy_perform` fails; D5 log with curl error; retried then dropped |
| 11.7 | `slow --delay 20` with `Timeout=10` | curl times out; treated as failure → retry/drop |
| 11.8 | Collector recovers (`drop-after 2`) | first 2 batches delivered; after outage, batches dropped & counted; when collector restored, subsequent batches deliver again (no permanent wedge) |
| 11.9 | User-Agent | request carries `NetXMS OTLP Exporter/<version>` |

## 12. Queue limit / overflow / drop accounting

| # | Test | Expected |
|---|------|----------|
| 12.1 | `QueueSizeLimit=50`, stall collector (`slow`/down), generate >50 values | once buffer ≥ limit, new values dropped; **one** WARNING `buffer is full (limit=50)…`; `m_droppedCount` increments per drop |
| 12.2 | Buffer drains below limit | D4 log `buffer is below limit`; overflow warning re-armed |
| 12.3 | `Server.PDS.DriverStat(OTLP, messageDrops)` | reflects queue-overflow drops **plus** send-failure drops (both bump `m_droppedCount`) |
| 12.4 | `Server.PDS.DriverStat(OTLP, queueSize)` | current `m_buffer.size()`; rises when collector stalled, returns to ~0 after flush |
| 12.5 | Unknown metric name in `DriverStat` | `getInternalMetric` returns `DCE_NOT_SUPPORTED`, empty value |
| 12.6 | `DriverStat(WRONGNAME, queueSize)` | `GetPerfDataStorageDriverMetric` → `DCE_NO_SUCH_INSTANCE` |

## 13. Concurrency / robustness

| # | Test | Expected |
|---|------|----------|
| 13.1 | High DCI throughput (many nodes, short intervals) | no crash; `queueSize` bounded; buffer lock not contended into deadlock (note copy-DCI-unlocked fan-out in dcitem.cpp:1289) |
| 13.2 | Run under TSAN/ASAN if buildable | no data races on `m_buffer`/`m_droppedCount`/`m_queueOverflow`; no leaks of curl handle/headers |
| 13.3 | Config reload while running | document behavior — driver config read once at `init`; reload does not re-init (acceptable for v1, single endpoint) |
| 13.4 | DCI whose owner is being deleted during flush | fan-out operates on `DCItem copy`; no use-after-free |

## 14. End-to-end against a real collector (optional, recommended)

| # | Test | Expected |
|---|------|----------|
| 14.1 | Point `Endpoint` at an OTel Collector (`otlphttp` receiver) → Prometheus/file exporter | NetXMS DCIs appear as metrics with correct names/labels/values |
| 14.2 | Counter DCI over time | rendered as a cumulative counter (rate query sane) |
| 14.3 | Gauge DCI | rendered as gauge; matches NetXMS history |
| 14.4 | Labels | `host.name`, `service.name`, `tag:*`, `netxms.dci.id`, `netxms.instance` present as expected |

---

## Notes / known v1 limitations to confirm during testing

- **Table DCIs** (`saveDCTableValue`) not implemented for export — only `DCItem`
  values are exported. Confirm table DCIs are silently ignored (base no-op).
- **Single endpoint** only (PDS drivers configured once at load).
- **`start_time_unix_nano`** — see §5a.
  - *Cumulative* sums (raw counters): genuinely unknowable — NetXMS never observes
    the device counter's reset epoch, only collection timestamps. Left unset (0),
    which is correct and spec-permitted; not a fixable defect. Confirm collectors
    accept it.
  - *Delta* sums (`DCM_SIMPLE`): **implemented** — set to the previous collection
    timestamp (interval start). Verify per §5a.2/5a.3.
- Opt-out only (no per-DCI export checkbox).
- Counter reset / restart semantics (cumulative sum without start time) — note any
  collector complaints.

## Automated coverage status

- **Mock collector (`otlp_mock_collector.py`)** — automates protobuf decode +
  structural assertions and injects all response/error modes. Drives sections
  4–12 from a running server.
- **No pure unit test** for the conversion/mapping logic: it is embedded in
  `OtlpMetricExportDriver::saveDCItemValue` and depends on live `DCItem` / `NetObj`
  objects, so it cannot be exercised in isolation without refactoring the
  value→record mapping into a free function. If desired, extract a
  `BuildMetricExportRecord(const DCItem&, …) -> MetricExportRecord` helper and add
  a libnxsl/server-side gtest covering sections 4–7 deterministically.
