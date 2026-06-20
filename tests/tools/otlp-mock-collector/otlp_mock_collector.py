#!/usr/bin/env python3
#
# NetXMS - Network Management System
# Copyright (C) 2026 Raden Solutions
#
# Mock OTLP collector for testing the NetXMS OTLP metric export driver (issue #3304).
#
# Listens for OTLP/HTTP protobuf metric export requests (POST .../v1/metrics),
# decodes ExportMetricsServiceRequest, pretty-prints the received metrics/data
# points/attributes, runs structural assertions, and replies with a configurable
# response so the driver's success / partial-success / error / retry paths can all
# be exercised against a single tool.
#
# The protobuf Python bindings are generated on the fly with protoc from the
# vendored .proto files in src/server/otlp/ so the decoder always matches the
# exact wire format the server emits (no pip dependency on opentelemetry-proto).
#
# Usage:
#   ./otlp_mock_collector.py                       # listen on 0.0.0.0:4318, reply 200
#   ./otlp_mock_collector.py --port 4318 --mode ok
#   ./otlp_mock_collector.py --mode partial --reject 3 --message "over quota"
#   ./otlp_mock_collector.py --mode http-error --status 503   # exercise retry/backoff
#   ./otlp_mock_collector.py --mode drop-after 2              # accept N then 503 (test drops)
#   ./otlp_mock_collector.py --mode slow --delay 40           # exceed driver Timeout
#   ./otlp_mock_collector.py --auth "Bearer secret"           # assert auth header present
#   ./otlp_mock_collector.py --expect-header "X-Custom: value"
#   ./otlp_mock_collector.py --json                           # one JSON object per request to stdout
#
# Endpoint to put in netxmsd.conf:
#   [OpenTelemetry/MetricExport]
#   Endpoint = http://127.0.0.1:4318/v1/metrics
#
import argparse
import datetime
import json
import os
import subprocess
import sys
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# --------------------------------------------------------------------------
# Generate and import protobuf bindings from the vendored .proto files
# --------------------------------------------------------------------------
def load_protobuf():
    here = os.path.dirname(os.path.abspath(__file__))
    proto_dir = os.environ.get(
        "OTLP_PROTO_DIR",
        os.path.normpath(os.path.join(here, "..", "..", "..", "src", "server", "otlp")))
    protos = ["common.proto", "resource.proto", "metrics.proto", "metrics_service.proto"]
    for p in protos:
        if not os.path.isfile(os.path.join(proto_dir, p)):
            sys.exit("ERROR: cannot find %s in %s (set OTLP_PROTO_DIR)" % (p, proto_dir))
    outdir = tempfile.mkdtemp(prefix="otlp_pb_")
    cmd = ["protoc", "--proto_path=" + proto_dir, "--python_out=" + outdir] + protos
    try:
        subprocess.check_call(cmd)
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit("ERROR: protoc failed (%s). Install protobuf-compiler." % e)
    sys.path.insert(0, outdir)
    import metrics_service_pb2  # noqa: E402
    return metrics_service_pb2


PB = load_protobuf()

# OTLP enum values (from metrics.proto)
AGG_TEMPORALITY = {0: "UNSPECIFIED", 1: "DELTA", 2: "CUMULATIVE"}

stats = {"requests": 0, "data_points": 0, "errors": 0}
stats_lock = threading.Lock()


def attrs_to_dict(kv_list):
    out = {}
    for kv in kv_list:
        v = kv.value
        if v.HasField("string_value"):
            out[kv.key] = v.string_value
        elif v.HasField("int_value"):
            out[kv.key] = v.int_value
        elif v.HasField("double_value"):
            out[kv.key] = v.double_value
        elif v.HasField("bool_value"):
            out[kv.key] = v.bool_value
        else:
            out[kv.key] = None
    return out


def fmt_ts(ns):
    if ns == 0:
        return "0 (unset)"
    secs = ns / 1e9
    try:
        dt = datetime.datetime.fromtimestamp(secs, datetime.timezone.utc)
        return "%d (%s.%03dZ)" % (ns, dt.strftime("%Y-%m-%d %H:%M:%S"), int((secs * 1000) % 1000))
    except (OverflowError, OSError, ValueError):
        return str(ns)


def decode_request(body):
    """Decode raw protobuf into a structured python dict and run assertions."""
    req = PB.ExportMetricsServiceRequest()
    req.ParseFromString(body)

    problems = []
    n_points = 0
    resources = []
    for rm in req.resource_metrics:
        res_attrs = attrs_to_dict(rm.resource.attributes)
        # Driver always sets these three resource attributes (metric_export.cpp sendBatch)
        for required in ("service.name", "host.name", "host.id"):
            if required not in res_attrs:
                problems.append("resource missing attribute '%s'" % required)
        scopes = []
        for sm in rm.scope_metrics:
            metrics = []
            for m in sm.metrics:
                kind = None
                points = []
                if m.HasField("gauge"):
                    kind = "gauge"
                    dps = m.gauge.data_points
                    monotonic = None
                    temporality = None
                elif m.HasField("sum"):
                    kind = "sum"
                    dps = m.sum.data_points
                    monotonic = m.sum.is_monotonic
                    temporality = AGG_TEMPORALITY.get(m.sum.aggregation_temporality)
                    if m.sum.aggregation_temporality == 0:
                        problems.append("metric '%s' sum has UNSPECIFIED temporality" % m.name)
                else:
                    kind = "OTHER(%s)" % m.WhichOneof("data")
                    dps = []
                    monotonic = None
                    temporality = None
                if not m.name:
                    problems.append("metric with empty name")
                for dp in dps:
                    n_points += 1
                    if dp.HasField("as_int"):
                        value = dp.as_int
                    elif dp.HasField("as_double"):
                        value = dp.as_double
                    else:
                        value = None
                        problems.append("metric '%s' data point has no numeric value" % m.name)
                    dpa = attrs_to_dict(dp.attributes)
                    if "netxms.dci.id" not in dpa:
                        problems.append("metric '%s' data point missing netxms.dci.id" % m.name)
                    # start_time_unix_nano expectations (§5a):
                    #  - DELTA sum: present and < time (except the first sample, which is legitimately 0)
                    #  - cumulative sum / gauge: must be 0 (unset)
                    is_delta = (kind == "sum" and temporality == "DELTA")
                    if is_delta:
                        if dp.start_time_unix_nano != 0 and dp.start_time_unix_nano >= dp.time_unix_nano:
                            problems.append("metric '%s' DELTA sum start_time >= time (start=%d end=%d)" %
                                            (m.name, dp.start_time_unix_nano, dp.time_unix_nano))
                    elif dp.start_time_unix_nano != 0:
                        label = "CUMULATIVE sum" if kind == "sum" else kind
                        problems.append("metric '%s' %s data point has unexpected start_time_unix_nano=%d "
                                        "(should be unset)" % (m.name, label, dp.start_time_unix_nano))
                    points.append({
                        "value": value,
                        "time_unix_nano": dp.time_unix_nano,
                        "start_time_unix_nano": dp.start_time_unix_nano,
                        "attributes": dpa,
                    })
                metrics.append({
                    "name": m.name,
                    "description": m.description,
                    "unit": m.unit,
                    "kind": kind,
                    "monotonic": monotonic,
                    "temporality": temporality,
                    "data_points": points,
                })
            scopes.append({"scope": sm.scope.name, "metrics": metrics})
        resources.append({"resource_attributes": res_attrs, "scopes": scopes})
    return {"resources": resources, "data_point_count": n_points, "problems": problems}


def pretty_print(decoded, args):
    print("=" * 78)
    print("Request #%d  |  %d resource(s)  |  %d data point(s)" %
          (stats["requests"], len(decoded["resources"]), decoded["data_point_count"]))
    for r in decoded["resources"]:
        ra = r["resource_attributes"]
        print("  Resource: service.name=%r host.name=%r host.id=%r" %
              (ra.get("service.name"), ra.get("host.name"), ra.get("host.id")))
        for sc in r["scopes"]:
            print("    Scope: %s" % sc["scope"])
            for m in sc["metrics"]:
                hdr = "      %s [%s" % (m["name"], m["kind"])
                if m["kind"] == "sum":
                    hdr += ", monotonic=%s, %s" % (m["monotonic"], m["temporality"])
                hdr += "]"
                if m["unit"]:
                    hdr += " unit=%r" % m["unit"]
                if m["description"]:
                    hdr += " desc=%r" % m["description"]
                print(hdr)
                for dp in m["data_points"]:
                    extra = {k: v for k, v in dp["attributes"].items()}
                    line = "         value=%s  ts=%s" % (dp["value"], fmt_ts(dp["time_unix_nano"]))
                    if dp.get("start_time_unix_nano"):
                        line += "  start=%s" % fmt_ts(dp["start_time_unix_nano"])
                    print(line)
                    print("         attrs=%s" % json.dumps(extra, sort_keys=True))
    if decoded["problems"]:
        print("  !! ASSERTION PROBLEMS:")
        for p in decoded["problems"]:
            print("     - %s" % p)
    print("=" * 78)
    sys.stdout.flush()


def make_handler(args):
    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *a):
            pass  # silence default logging; we print our own

        def _check_headers(self):
            problems = []
            ctype = self.headers.get("Content-Type", "")
            if "application/x-protobuf" not in ctype:
                problems.append("unexpected Content-Type: %r" % ctype)
            if args.auth:
                got = self.headers.get("Authorization", "")
                if got != args.auth:
                    problems.append("Authorization mismatch: got %r want %r" % (got, args.auth))
            for h in args.expect_header or []:
                name, _, value = h.partition(":")
                got = self.headers.get(name.strip())
                if got is None or got.strip() != value.strip():
                    problems.append("header %r mismatch: got %r want %r" % (name.strip(), got, value.strip()))
            return problems

        def do_POST(self):
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length) if length else b""

            with stats_lock:
                stats["requests"] += 1
                req_no = stats["requests"]

            header_problems = self._check_headers()

            try:
                decoded = decode_request(body)
            except Exception as e:  # noqa: BLE001
                with stats_lock:
                    stats["errors"] += 1
                print("Request #%d: FAILED TO DECODE protobuf (%d bytes): %s" % (req_no, len(body), e))
                self._reply(400, b"decode error")
                return

            decoded["problems"] = header_problems + decoded["problems"]
            with stats_lock:
                stats["data_points"] += decoded["data_point_count"]
                if decoded["problems"]:
                    stats["errors"] += 1

            if args.json:
                print(json.dumps({"request": req_no, **decoded}))
                sys.stdout.flush()
            else:
                pretty_print(decoded, args)

            self._respond(req_no, decoded)

        def _respond(self, req_no, decoded):
            mode = args.mode
            if mode == "slow":
                import time
                time.sleep(args.delay)
                self._reply_ok()
            elif mode == "http-error":
                self._reply(args.status, b"injected error")
            elif mode == "drop-after":
                if req_no > args.count:
                    self._reply(503, b"simulated outage")
                else:
                    self._reply_ok()
            elif mode == "partial":
                resp = PB.ExportMetricsServiceResponse()
                resp.partial_success.rejected_data_points = args.reject
                resp.partial_success.error_message = args.message
                self._reply(200, resp.SerializeToString(),
                            content_type="application/x-protobuf")
            elif mode == "empty-ok":
                self._reply(200, b"")
            else:  # ok
                self._reply_ok()

        def _reply_ok(self):
            resp = PB.ExportMetricsServiceResponse()  # empty = full success
            self._reply(200, resp.SerializeToString(), content_type="application/x-protobuf")

        def _reply(self, status, body, content_type="text/plain"):
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            if body:
                self.wfile.write(body)

    return Handler


def main():
    ap = argparse.ArgumentParser(description="Mock OTLP collector for NetXMS OTLP metric export testing (issue #3304)")
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=4318)
    ap.add_argument("--mode", default="ok",
                    choices=["ok", "partial", "http-error", "drop-after", "slow", "empty-ok"],
                    help="response behaviour to inject")
    ap.add_argument("--status", type=int, default=503, help="HTTP status for http-error mode")
    ap.add_argument("--reject", type=int, default=1, help="rejected_data_points for partial mode")
    ap.add_argument("--message", default="partial success test", help="error_message for partial mode")
    ap.add_argument("--count", type=int, default=2, dest="count",
                    help="for drop-after: accept this many requests, then fail")
    ap.add_argument("--delay", type=float, default=40, help="seconds to stall in slow mode")
    ap.add_argument("--auth", help="assert this exact Authorization header value")
    ap.add_argument("--expect-header", action="append", help="assert 'Name: value' header present (repeatable)")
    ap.add_argument("--json", action="store_true", help="emit one JSON object per request instead of pretty text")
    # 'drop-after' takes its count as a positional too, for convenience: --mode drop-after 2
    args, extra = ap.parse_known_args()
    if args.mode == "drop-after" and extra:
        try:
            args.count = int(extra[0])
        except ValueError:
            pass

    server = ThreadingHTTPServer((args.host, args.port), make_handler(args))
    print("Mock OTLP collector listening on http://%s:%d/  (mode=%s)" % (args.host, args.port, args.mode))
    print("Point netxmsd.conf [OpenTelemetry/MetricExport] Endpoint at  http://<host>:%d/v1/metrics" % args.port)
    print("Press Ctrl-C to stop.\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down. Totals: %d requests, %d data points, %d requests with problems." %
              (stats["requests"], stats["data_points"], stats["errors"]))
        server.shutdown()


if __name__ == "__main__":
    main()
