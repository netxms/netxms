# Mock OTLP collector

`otlp_mock_collector.py` is a standalone HTTP server that pretends to be an
OpenTelemetry collector. It is used to test the NetXMS **OTLP metric export**
driver (issue #3304, `src/server/otlp/metric_export.cpp`) without standing up a
real collector / Prometheus / observability backend.

It receives `POST .../v1/metrics` requests, decodes the `ExportMetricsServiceRequest`
protobuf, pretty-prints every metric / data point / attribute, runs structural
assertions (required resource attributes, `netxms.dci.id` presence, gauge-vs-sum
shape, temporality), and replies with a response you choose so the driver's
success / partial-success / error / retry / timeout paths can all be exercised.

## Requirements

- Python 3.6+ with the `protobuf` runtime (`pip install protobuf`)
- `protoc` (`apt install protobuf-compiler`) — the script compiles the vendored
  `.proto` files in `src/server/otlp/` on startup, so the decoder always matches
  the exact wire format the server emits. No dependency on `opentelemetry-proto`.

If the repo layout differs, point the script at the protos with
`OTLP_PROTO_DIR=/path/to/src/server/otlp`.

## Quick start

```bash
# 1. Run the collector
./otlp_mock_collector.py --port 4318

# 2. Configure netxmsd.conf and (re)start the server
#    [OpenTelemetry/MetricExport]
#    Endpoint = http://127.0.0.1:4318/v1/metrics
#    FlushInterval = 2
#    BatchSize = 10
```

## Response modes (`--mode`)

| Mode | Purpose / driver path exercised |
|------|----------------------------------|
| `ok` (default) | 200 + empty `ExportMetricsServiceResponse` (full success) |
| `partial` | 200 + `partial_success{rejected_data_points, error_message}` → driver logs WARNING, does **not** retry. Tune with `--reject N --message "..."` |
| `http-error` | returns `--status` (default 503) every time → driver retries `SEND_RETRY_COUNT` times then drops the batch and bumps `messageDrops` |
| `drop-after N` | accept first `N` requests, then 503 forever → verify queue grows and `messageDrops` increases after the outage starts |
| `slow` | stall `--delay` seconds (default 40) before replying → exceed the driver `Timeout` and verify curl-timeout handling/retry |
| `empty-ok` | 200 with empty body → verify driver treats no-body as success |

## Assertion options

| Option | Checks |
|--------|--------|
| `--auth "Bearer secret"` | every request carries this exact `Authorization` header (matches `AuthToken=secret` in config) |
| `--expect-header "X-Custom: value"` | header is present (matches a `Header=` config line); repeatable |
| `--json` | emit one machine-readable JSON object per request to stdout (for scripted assertions) |

Content-Type is always checked to be `application/x-protobuf`.

## Examples

```bash
# Verify auth + custom header plumbing
./otlp_mock_collector.py --auth "Bearer s3cr3t" --expect-header "X-Tenant: acme"

# Exercise retry + drop accounting
./otlp_mock_collector.py --mode http-error --status 500
#   then read Server.PDS.DriverStat(OTLP, messageDrops) from the server

# Exercise partial_success warning path
./otlp_mock_collector.py --mode partial --reject 5 --message "quota exceeded"

# Exercise HTTP timeout (set Timeout=10 in config, delay longer)
./otlp_mock_collector.py --mode slow --delay 20
```

See `TEST_PLAN.md` for the full test matrix this tool supports.
