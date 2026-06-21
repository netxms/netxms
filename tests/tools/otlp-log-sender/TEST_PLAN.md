# Test plan — OTLP log ingestion (issue #3292)

Tests the OpenTelemetry **log ingestion** pipeline: OTLP/HTTP protobuf logs are
received on `POST /otlp-backend/v1/logs`, matched to a node, normalized,
processed through the core LogParser pipeline (event generation), and stored in
the `otel_log` table.

- **Endpoint handler:** `src/server/otlp/logs.cpp` — `H_OtlpLogs`, helpers
  `MapOtelSeverity`, `BytesToHex`, `ProcessLogRecord`. Route registered in
  `src/server/otlp/main.cpp` (`RouteBuilder("otlp-backend/v1/logs").POST(H_OtlpLogs).acceptProtobuf()`).
- **Resource → node matching:** `src/server/otlp/matching.cpp` —
  `MatchResourceToNode` over `otlp_matching_rules`, with SHA-256 match cache
  (`OTLP.MatchCacheTTL`, default 300s).
- **Attribute flattening:** `src/server/otlp/otlp_attributes.h` —
  `AnyValueToString`, `ExtractResourceAttributes`, `ExtractDataPointAttributes`.
- **Core processing/storage:** `src/server/core/otellog.cpp` —
  `QueueOtelLog`, `OtelLogProcessingThread`, `OtelLogWriterThread` (+`_PGSQL`),
  LogParser callback (`OtelLogParserCallback`), absence detection, start/stop.
- **Record struct:** `OtelLogRecord` in `src/server/include/nms_core.h`.
- **Schema:** `otel_log` table in `sql/schema.in` (ms epoch on non-TSDB,
  `timestamptz` on TSDB; PK `id` / `(id,log_timestamp)`).
- **Config:** `OTLP.Logs.EnableStorage` (B, default 1), `OTLP.Logs.RetentionTime`
  (I, days, default 90), `OTLP.LogUnmatchedResources`, `OTLP.MatchCacheTTL`
  (`sql/setup.in`).
- **Housekeeping:** `src/server/core/hk.cpp` — `DeleteExpiredLogRecords(... "otel_log",
  "log_timestamp", "OTLP.Logs.RetentionTime", ..., millisecondTimestamp=true)`.
- **Event plumbing:** `EventOrigin::OPENTELEMETRY = 8` (`nms_events.h`),
  `EventReferenceType::OTEL_LOG = 7`.
- **Debug tags:** `otlp` (receiver/matching), `otel.log` (core processing/writer).
  Run `netxmsd -D6` (or `-D7` for per-record/attribute detail).

## Tooling / how to run

- **Sender:** `otlp_log_sender.py` in this directory (see `README.md`). Logs in
  via WebAPI, builds protobuf, POSTs to the endpoint, decodes the response.
- **Storage verification** — query the DB directly (records are written
  asynchronously by the writer thread; allow up to a few hundred ms):
  ```sql
  SELECT id, node_id, severity_number, severity_text, service_name, scope_name,
         trace_id, span_id, origin_timestamp, observed_timestamp, body, attributes
  FROM otel_log ORDER BY id DESC LIMIT 20;
  ```
  On TSDB, `log_timestamp` is `timestamptz`; to read it as ms use
  `timestamptz_to_ms(log_timestamp)`. On other backends `log_timestamp` is the
  raw epoch-ms `SQL_INT64`.
- **UI verification** — *Logs → OpenTelemetry logs* view (global and per-object
  context menu), and the record-details viewer.
- **Counters** — `g_otelLogsReceived` (total received records).

---

## 1. Build & static checks

| # | Test | Expected |
|---|------|----------|
| 1.1 | Build server with OTLP module (`./configure --with-server`; `make -j$(nproc)`) | `logs.cpp`, `otellog.cpp` compile & link; module loads |
| 1.2 | Windows build (`otlp.vcxproj` includes `logs.cpp`; `nxcore.vcxproj` includes `otellog.cpp`) | builds; no `nx_swprintf`/`%s` portability issues |
| 1.3 | Debug tags registered | `otlp` and `otel.log` present in `doc/internal/debug_tags.txt` |
| 1.4 | Fresh DB has table + config + rules | `otel_log` table created; `OTLP.Logs.*` config rows present; default `otlp_matching_rules` seeded |
| 1.5 | Upgrade DB from prior version | `H_UpgradeFromV19` (62.20) creates `otel_log` + config; `netxmsdb.h` minor=20 |

## 2. Endpoint, transport & authentication

| # | Test (sender) | Expected |
|---|---------------|----------|
| 2.1 | Valid protobuf, valid token, matched host | HTTP 200; `rejected_log_records = 0`; `otlp` D6 `Received OTLP logs batch with 1 resource logs` |
| 2.2 | `--no-auth` (no `Authorization`) | HTTP 401; nothing queued |
| 2.3 | `--token INVALID...` | HTTP 401 |
| 2.4 | `--content-type application/json` | HTTP 415. The OTLP backend routes opt out of the default JSON acceptance via `.acceptJson(false)` (`src/server/otlp/main.cpp`), so only `application/x-protobuf` is accepted. (Before that flag existed the router treated `application/json` as universally valid and the body was blindly parsed as protobuf — see "content-type strictness" below.) |
| 2.5 | `--content-type text/plain` / `--no-content-type` | HTTP 415 |
| 2.6 | `--malformed` (garbage body) | HTTP 400; `otlp` D4 `Failed to parse OTLP ExportLogsServiceRequest` |
| 2.7 | Empty request (0 resources, `--scenario empty`) | HTTP 400 — an empty `ExportLogsServiceRequest` serializes to 0 bytes, and `H_OtlpLogs` rejects a no-body POST as `Invalid protobuf payload`. (Not a real OTLP case; 400 is acceptable.) |
| 2.8 | GET / PUT on the endpoint | 405 Method Not Allowed |
| 2.9 | TLS via reproxy (`https://...:port`, `--insecure`) | works through TLS termination (reproxy enabled) |

Covered by: `--scenario auth content-type malformed empty`.

## 3. Resource → node matching

Default rules: `host.name`→node name, `host.name`→DNS name, `host.ip`→IP address.

| # | Test | Expected |
|---|------|----------|
| 3.1 | `host.name` = existing node name | matched; record stored with that `node_id`; `otlp` D6 `Matched OTel resource to node … via rule 1 (… node name)` |
| 3.2 | `host.name` = node primary DNS/host name (≠ object name) | matched via rule 2 (DNS name) |
| 3.3 | `host.ip` = node primary IP | matched via rule 3 (IP address) |
| 3.4 | Unknown `host.name` (`--scenario unmatched`) | no match; HTTP 200; `rejected_log_records` = number of records in that resource; nothing stored |
| 3.5 | `OTLP.LogUnmatchedResources=1`, D3+ | `otlp` D3 `No matching node found for OTel resource …` listing attributes |
| 3.6 | Mixed batch: 1 matched + 1 unmatched resource (`--scenario mixed`) | matched resource stored; `rejected = 1`; per-resource accounting correct |
| 3.7 | Custom-attribute rule (add rule `node_property=3`, `custom_attr_name=X`) + node CA `X=val` + resource attr that maps to `val` | matched via custom attribute |
| 3.8 | Regex transform on a rule (e.g. strip domain from FQDN) | transformed value used for match; `otlp` D7 `Regex transform applied` |
| 3.9 | Match cache hit | second identical resource within `OTLP.MatchCacheTTL` resolved from cache (no rule walk); verify via repeated sends |
| 3.10 | Match cache TTL expiry / rule reload | after TTL or `ReloadMatchingRules`, cache cleared and re-evaluated |
| 3.11 | Match against a node in a non-zero zone | `zone_uin` stored = node's zone UIN |

## 4. Field extraction & conversion (`ProcessLogRecord`)

Verify each `otel_log` column against what was sent.

| # | Sent | Stored / Expected |
|---|------|-------------------|
| 4.1 | `severity_number = 17`, `severity_text = "ERROR"` | `severity_number=17`; `severity_text='ERROR'`; mapped severity MAJOR (used by parser) |
| 4.2 | `service.name` resource attr | `service_name` column (truncated to 127 chars) |
| 4.3 | scope name | `scope_name` column (truncated to 127) |
| 4.4 | `trace_id` = 16 bytes | `trace_id` = 32 lowercase hex chars |
| 4.5 | `span_id` = 8 bytes | `span_id` = 16 lowercase hex chars |
| 4.6 | no trace/span | `trace_id`/`span_id` empty |
| 4.7 | `flags`, `dropped_attributes_count` | stored verbatim |
| 4.8 | over-long `severity_text` (>31), `service.name`/`scope` (>127), `trace_id`/`span_id` longer than buffer | truncated to column width, no overflow/crash (buffers sized to varchar+1) |
| 4.9 | Body string with multibyte UTF-8 | round-trips correctly (UTF-8 storage) |

Covered by: `--scenario trace severity attributes`, plus targeted single sends
(`--trace-id`, `--span-id`, `--flags`, `--dropped-attrs`, `--severity-text`).

## 5. Severity mapping (`MapOtelSeverity`)

| # | Severity number | NetXMS band (parser level) |
|---|-----------------|----------------------------|
| 5.1 | 1–12 (TRACE/DEBUG/INFO) | NORMAL |
| 5.2 | 13–16 (WARN) | WARNING |
| 5.3 | 17–20 (ERROR) | MAJOR |
| 5.4 | 21–24 (FATAL) | CRITICAL |
| 5.5 | 0 / unset | NORMAL (default band) |

`--scenario severity` sends 1/5/9/13/17/21/24 in one batch. Raw number is stored;
band drives LogParser matching (§7) and is surfaced in events as
`otelSeverity` / `otelSeverityNumber`.

## 6. Body value types & attributes

| # | Test | Expected |
|---|------|----------|
| 6.1 | `--body-type string` | stored as text |
| 6.2 | `--body-type int` (e.g. 42) | stored as `"42"` (`IntegerToString`) |
| 6.3 | `--body-type double` (3.14159) | stored as `"%f"` form |
| 6.4 | `--body-type bool true` | stored as `"true"` |
| 6.5 | `--body-type kvlist` (not flattenable) | `AnyValueToString` yields empty → `body` stored as empty string `''` (verified: not NULL; `record->body` stays null and `DBPrepareString(nullptr)` writes `''`) |
| 6.6 | `--body-type none` / `has_body=false` | `body` empty string `''` (same as 6.5) |
| 6.7 | Record attributes only | `attributes` JSON contains record attrs |
| 6.8 | Resource + record attributes | merged into `attributes` JSON (resource keys + record keys). On a duplicate key the **resource** value wins: the map is seeded from resource attrs, then record attrs are added via `std::map::emplace`, which does not overwrite an existing key. Verify this precedence with a key present in both. |
| 6.9 | Attribute value types (int/double/bool/string) | each flattened to string in JSON |
| 6.10 | No attributes at all | `attributes` column NULL (`SerializeAttributes` returns null when size 0) |

Covered by: `--scenario body-types attributes`.

## 7. LogParser event generation (`OtelLogParserCallback`)

Configure an `OpenTelemetryLogParser` (Configuration → OpenTelemetry log parser
configurator, or seed the `OpenTelemetryLogParser` CLOB config var). Rules match
on severity/source(service)/scope(logName)/body, like the Windows event parser.

| # | Test | Expected |
|---|------|----------|
| 7.1 | Body matches a parser rule `<match>` | event posted with `origin = OPENTELEMETRY`, `originTimestamp` = record origin time; `otel.log` D7 `OpenTelemetry log record matched` |
| 7.2 | Event params | `otelService` (= service name), `otelScope` (= scope), `otelSeverity`, `otelSeverityNumber`, `repeatCount`, named capture groups |
| 7.3 | Rule filters by service (`<source>`) | only matching service generates events |
| 7.4 | Rule filters by severity (level/`<severity>`) | band from §5 used; non-matching severity skipped |
| 7.5 | Capture groups → event parameters | regex captures become event params (`namedVariables`) |
| 7.6 | Rule with `setData`/context | applied; verify event fields |
| 7.7 | Owner node is UNMANAGED | no event unless `AF_TRAPS_FROM_UNMANAGED_NODES` set |
| 7.8 | Rule writes-to-database flag (`<agentAction>`/break) clears storage | record NOT stored when parser sets `writeToDatabase=false`; otherwise stored |
| 7.9 | Event reference report | `GetOtelLogEventReferences` reports `OTEL_LOG` for events used by the parser (Event configurator "used by" shows OpenTelemetry log) |
| 7.10 | Parser reload on config change | editing the parser CLOB live-reloads (`InitializeOtelLogParser` via config change), counters restored |

## 8. Absence detection

| # | Test | Expected |
|---|------|----------|
| 8.1 | Parser rule with absence/`reset` timer, then stop sending | absence check task (`OtelLogAbsenceCheckTask`, 60s) fires configured absence event |
| 8.2 | Absence state persistence | every 5 checks, state saved to `lp_absence_state WHERE parser_type='O'`; survives restart (`LoadOtelLogAbsenceState`) |
| 8.3 | Clean shutdown | `StopOtelLogProcessing` saves absence state before exit |

## 9. Storage / writer threads (`otellog.cpp`)

Exercise all three writer code paths.

| # | Backend | Path | Expected |
|---|---------|------|----------|
| 9.1 | SQLite / MySQL / MSSQL / Oracle | `OtelLogWriterThread` (prepared stmt, per-row) | rows inserted; `log_timestamp` = epoch-ms; ms timestamps preserved |
| 9.2 | PostgreSQL | `OtelLogWriterThread_PGSQL` (multi-row `INSERT … ON CONFLICT DO NOTHING`) | batched insert; `id` monotonic; no dup PK errors |
| 9.3 | TimescaleDB | `_PGSQL` path + `ms_to_timestamptz(timestamp)` | `log_timestamp` is `timestamptz` = correct ms instant; `timestamptz_to_ms` round-trips |
| 9.4 | Record id allocation | on start `s_recordId` = max(`FirstFreeOtelLogId`, `max(id)+1`); ids unique across restart |
| 9.5 | Writer batching | `DBWriter.MaxRecordsPerTransaction` / `MaxRecordsPerStatement` honored; `--scenario load --load-count 2000` all stored |
| 9.6 | `attributes` JSON | stored as UTF-8 (`DBPrepareStringUTF8` / `DB_CTYPE_UTF8_STRING`) |
| 9.7 | Statement/transaction failure | failed `DBExecute`/`DBQuery` breaks batch cleanly; no crash, no record leak |

## 10. Storage enable/disable & retention

| # | Test | Expected |
|---|------|----------|
| 10.1 | `OTLP.Logs.EnableStorage=1` (default) | records processed by parser AND stored |
| 10.2 | `OTLP.Logs.EnableStorage=0` | records still processed by parser (events generated) but NOT stored; live config change honored (`OnOtelLogsConfigurationChange`) |
| 10.3 | Parser `writeToDatabase=false` + storage enabled | not stored (parser veto wins) |
| 10.4 | `OTLP.Logs.RetentionTime=N` housekeeping | records older than N days deleted by housekeeper (`DeleteExpiredLogRecords`, ms-aware); recent kept |
| 10.5 | Retention with TSDB | timestamptz comparison correct (no off-by-1000 from ms vs s) |

## 11. Timestamps (ns → ms)

| # | Sent | Expected |
|---|------|----------|
| 11.1 | both `time_unix_nano` & `observed_time_unix_nano` | `origin_timestamp` = time/1e6 ms; `observed_timestamp` = observed/1e6 ms |
| 11.2 | `time_unix_nano = 0`, observed set | `origin_timestamp` falls back to observed (ms) |
| 11.3 | both 0 (`--no-time`) | origin/observed 0; `log_timestamp` = server receive time (`GetCurrentTimeMs`, set in `QueueOtelLog`) |
| 11.4 | sub-second precision | ms precision retained (not truncated to seconds) |
| 11.5 | parser origin time | `matchEvent` receives `originTimestamp/1000` (seconds) — event origin time correct |

Covered by: `--scenario timestamps`.

## 12. UI (nxmc desktop + web)

| # | Test | Expected |
|---|------|----------|
| 12.1 | Logs perspective → "OpenTelemetry Logs" global view | lists stored records; columns incl. ms timestamp (LC_TIMESTAMP_MS) |
| 12.2 | Object context menu → Logs → OpenTelemetry logs | filtered by `node_id` for that object |
| 12.3 | Record-details viewer | General tab (body, severity, service, scope, event/observed time, trace/span); Attributes tab = pretty JSON |
| 12.4 | Timestamp rendering | ms timestamp shown with sub-second precision; TSDB `timestamptz` projected via `timestamptz_to_ms` |
| 12.5 | Configuration → OpenTelemetry log parser configurator | create/edit rules; severity/service/scope fields enabled (OTEL behaves like WIN_EVENT) |
| 12.6 | Access control | requires `SYSTEM_ACCESS_VIEW_SYSLOG` (parity with syslog); user without it cannot view |

## 13. Concurrency / robustness

| # | Test | Expected |
|---|------|----------|
| 13.1 | High throughput (many batches/records) | processing & writer queues drain; no crash; `g_otelLogsReceived` increases correctly |
| 13.2 | Concurrent senders to multiple nodes | per-node `node_id`/`zone_uin` correct; no cross-talk |
| 13.3 | Run under ASAN/TSAN if buildable | no races on `s_parser`/queues; no leaks of `OtelLogRecord`/attributes JSON/body |
| 13.4 | Config reload of parser while ingesting | no deadlock (`s_parserLock`); counters restored |
| 13.5 | Shutdown mid-ingest | `StopOtelLogProcessing` drains both queues (INVALID_POINTER_VALUE sentinels), saves absence state, no use-after-free |
| 13.6 | Server restart with backlog in DB | record id continuation correct; no PK collision (esp. PGSQL `ON CONFLICT`) |

## 14. End-to-end against a real OTLP source (optional, recommended)

| # | Test | Expected |
|---|------|----------|
| 14.1 | Point an OpenTelemetry Collector `otlphttp` log exporter at `http://<server>:8000/otlp-backend/v1/logs` with a Bearer token header | logs appear in the OpenTelemetry log view for the matched node |
| 14.2 | Real app logs (e.g. via otel SDK) with `host.name`/`service.name` resource attrs | matched, severity/body/attributes correct |
| 14.3 | Round-trip with #3302 (events → OTLP logs forwarder) pointed back at this endpoint | forwarded events ingested as OTLP logs (self-loop sanity, beware feedback) |

---

## Known gaps / limitations to confirm

- **nxdbmgr migrate/convert gap (tracked in #3300):** `otel_log` is **not** yet
  wired into `nxdbmgr` migration tooling (`tables.cpp` `g_tables[]`,
  `migrate.cpp`, `convert.cpp`, `nxdbmgr.cpp` clear-log). Its ms columns need
  `ms_to_timestamptz`/`timestamptz_to_ms` handling (only idata/tdata are wired).
  **Until #3300 lands:**
  - `nxdbmgr export` / `import` / `migrate` will skip or mis-handle `otel_log`.
  - Migrating a DB with stored OTLP logs to/from TSDB will corrupt timestamps if
    naively added. Confirm migrate currently skips the table (no crash), and do
    NOT rely on it for OTLP log data until #3300.
  - `nxdbmgr` clear-log should be checked to include/exclude `otel_log` as
    intended.
- **Content-type strictness (framework change):** the WebAPI router previously
  treated `application/json` as a universally-valid request content type for every
  POST/PUT/PATCH route, so the protobuf-only OTLP backend routes (and the
  image-upload routes) accepted JSON-labeled bodies and parsed them blindly. A
  `RouteBuilder::acceptJson(bool)` flag was added (default **true**) and set to
  `false` on `otlp-backend/v1/logs`, `otlp-backend/v1/metrics`, and
  `v1/image-library[/:guid]`, so those routes now return **415** for anything
  other than their binary type. Regression-check that all other JSON routes still
  accept `application/json` (the flag defaults to true). Files:
  `src/server/include/netxms-webapi.h`, `src/server/core/webapi_router.cpp`,
  `src/server/otlp/main.cpp`, `src/server/webapi/main.cpp`.
- **HTTP/protobuf only** — no gRPC OTLP receiver (v1 scope).
- **Single matching attempt per resource** — first rule that resolves wins;
  no multi-node fan-out.
- **Parser config** — `OpenTelemetryLogParser` CLOB is not seeded in `setup.in`
  (parity with Syslog/WindowsEventParser); empty parser = store-only, no events.

## Automated coverage status

- **`otlp_log_sender.py`** drives sections 2–6, 9–11 (send side) and asserts HTTP
  status + `rejected_log_records`. Storage-side column/JSON assertions and
  event/parser/UI checks are manual (SQL queries + UI), since they require a live
  DB and a configured parser.
- **No pure unit test** for `ProcessLogRecord` / `MapOtelSeverity` / `BytesToHex`:
  they depend on live protobuf objects and the core queue. If desired, factor
  `MapOtelSeverity` and `BytesToHex` into a header-testable form and add a
  server-side gtest for §5 and §4.4–4.6 deterministically.
