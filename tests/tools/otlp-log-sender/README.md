# OTLP log sender

`otlp_log_sender.py` is a standalone tool that builds OpenTelemetry **log**
export requests (OTLP/HTTP, protobuf) and POSTs them to the NetXMS server's OTLP
backend endpoint. It is used to test the **OTLP log ingestion** pipeline
(issue #3292) — the receiving side, implemented in:

- `src/server/otlp/logs.cpp` — endpoint handler `H_OtlpLogs`
  (`POST /otlp-backend/v1/logs`), parses the protobuf, matches resources to
  nodes, builds `OtelLogRecord`, calls `QueueOtelLog()`.
- `src/server/core/otellog.cpp` — processing + writer threads, LogParser
  event generation, storage into the `otel_log` table.
- `src/server/otlp/matching.cpp` — resource→node matching rules.

> This is the ingestion-side counterpart of `../otlp-mock-collector`, which
> tests the opposite direction (DCI metric **export**, issue #3304).

## What it exercises

- Resource → node matching (`host.name` → node name / DNS name, `host.ip` →
  IP address, custom-attribute rules; see `otlp_matching_rules`).
- OTLP severity number (1–24) → NetXMS severity band mapping
  (NORMAL / WARNING / MAJOR / CRITICAL).
- Body value types, attribute flattening (resource + record merged), trace/span
  ids, origin/observed timestamps and their fallback.
- `partial_success.rejected_log_records` accounting for unmatched resources.
- Malformed payload (400), missing/invalid auth (401), wrong Content-Type (415).

## Requirements

- Python 3.6+ with the `protobuf` runtime (`pip install protobuf`)
- `protoc` (`apt install protobuf-compiler`) — the script compiles the vendored
  `.proto` files in `src/server/otlp/` on startup, so the wire format always
  matches exactly what the server parses. No dependency on `opentelemetry-proto`.

If the repo layout differs, point the script at the protos with
`OTLP_PROTO_DIR=/path/to/src/server/otlp`.

## Authentication

The endpoint requires a WebAPI Bearer token. By default the tool logs in via
`POST /v1/login` using `--user`/`--password` (defaults `admin` / `netxms`) and
reuses the issued token. Pass `--token <token>` to supply one directly and skip
login.

## Prerequisites on the server

1. Server built with the OTLP module and WebAPI enabled; WebAPI listener up
   (default port **8000**, config `/WEBAPI/Port` in `netxmsd.conf`).
2. A node object that the resource attributes will match. Easiest: a node whose
   **name** equals the `host.name` you send, or whose primary IP equals
   `host.ip`. Default matching rules (`sql/setup.in`):
   - `host.name` → node name
   - `host.name` → DNS (primary host) name
   - `host.ip`   → IP address
3. `OTLP.Logs.EnableStorage = 1` (default) to persist records to `otel_log`.
4. Run the server with `-D6` and watch debug tags `otlp` (receiver/matching)
   and `otel.log` (core processing/writer).

## Quick start

```bash
# Send one INFO record to node "node1"
./otlp_log_sender.py --url http://127.0.0.1:8000 --host-name node1 \
    --body "user login succeeded" --severity-number 9

# Send an ERROR record matched by IP
./otlp_log_sender.py --host-ip 10.0.0.1 --severity-number 17 \
    --severity-text ERROR --body "disk failure on /dev/sda"

# Run the full scenario suite against node1 (10.0.0.1)
./otlp_log_sender.py --host-name node1 --host-ip 10.0.0.1 --scenario all
```

## Single-send options (highlights)

| Option | Purpose |
|--------|---------|
| `--host-name`, `--host-ip` | resource attributes used for node matching |
| `--service-name` | `service.name` resource attr → `service_name` column |
| `--resource-attr key=value` | extra resource attribute (repeatable; `key:type=value` for int/double/bool) |
| `--scope-name` | instrumentation scope name → `scope_name` column |
| `--body`, `--body-type` | record body and its AnyValue type (`string`/`int`/`double`/`bool`/`kvlist`/`none`) |
| `--severity-number`, `--severity-text` | OTLP severity (1–24) and text |
| `--time-unix-nano`, `--no-time`, `--observed-time-nano` | timestamps (ns); `--no-time` tests observed-time fallback |
| `--trace-id`, `--span-id` | hex (32 / 16 chars) → hex columns |
| `--attr key=value` | record attribute (repeatable; typed like `--resource-attr`) |
| `--count N` | N records in one scope |
| `--batches N`, `--interval S` | send N HTTP requests, S seconds apart |
| `--malformed` | send invalid protobuf (expect 400) |
| `--no-auth`, `--content-type`, `--no-content-type` | negative transport tests |

## Scenario suite (`--scenario`)

`all` runs every scenario; or pick one:

| Scenario | Checks |
|----------|--------|
| `match` | matched resource → `rejected_log_records = 0` |
| `unmatched` | unknown host → HTTP 200 with `rejected_log_records > 0` |
| `mixed` | matched + unmatched resources in one batch → partial rejection |
| `severity` | sweep severity bands 1/5/9/13/17/21/24 |
| `body-types` | string/int/double/bool stored as text; kvlist/none → empty body |
| `timestamps` | both set / origin-0 fallback / none-set (server receive time) |
| `trace` | trace+span hex columns populated / absent |
| `attributes` | resource + record attributes merged into stored JSON |
| `malformed` | invalid protobuf → HTTP 400 |
| `auth` | missing / invalid Bearer token → HTTP 401 |
| `content-type` | `application/json` body → HTTP 415 |
| `empty` | 0-resource request → HTTP 200, `rejected = 0` |
| `load` | `--load-count` records in one batch (throughput / writer batching) |

Each scenario prints a `[PASS]`/`[FAIL]` line based on HTTP status and the
decoded `rejected_log_records`. Storage-side assertions (column values,
attribute JSON, parser-generated events) are verified separately — see
`TEST_PLAN.md`.

## Examples

```bash
# Negative: send without auth (expect 401)
./otlp_log_sender.py --host-name node1 --no-auth --expect-status 401

# Throughput: 2000 records in one POST
./otlp_log_sender.py --host-name node1 --scenario load --load-count 2000

# Drive the LogParser: send a body the configured OTLP parser rule matches
./otlp_log_sender.py --host-name node1 --service-name nginx \
    --severity-number 17 --body "connection refused from 192.168.1.5"
```

See `TEST_PLAN.md` for the full ingestion test matrix.
