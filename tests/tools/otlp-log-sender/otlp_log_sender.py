#!/usr/bin/env python3
#
# NetXMS - Network Management System
# Copyright (C) 2026 Raden Solutions
#
# OTLP log sender / generator for testing the NetXMS OpenTelemetry log
# ingestion pipeline (issue #3292).
#
# This is the INGESTION-side counterpart to ../otlp-mock-collector (which tests
# the metric EXPORT path). It builds OTLP/HTTP protobuf ExportLogsServiceRequest
# payloads and POSTs them to the server's OTLP backend endpoint
# (POST /otlp-backend/v1/logs), exercising:
#   - resource -> node matching (host.name / host.ip / custom attribute rules)
#   - severity number -> NetXMS severity band mapping
#   - body value types, attribute flattening, trace/span ids, timestamps
#   - partial_success.rejected_log_records accounting for unmatched resources
#   - malformed payload / auth / content-type error handling
#
# The protobuf Python bindings are generated on the fly with protoc from the
# vendored .proto files in src/server/otlp/ so the wire format always matches
# exactly what the server parses (no pip dependency on opentelemetry-proto).
#
# Authentication: the endpoint requires a WebAPI Bearer token. By default the
# tool logs in via POST /v1/login with --user/--password and reuses the issued
# token. Pass --token to supply one directly (and skip login).
#
# Usage:
#   ./otlp_log_sender.py --url http://127.0.0.1:8000 --host-name node1 \
#       --body "disk failure on /dev/sda" --severity-number 17
#   ./otlp_log_sender.py --scenario all --host-name node1 --host-ip 10.0.0.1
#
import argparse
import json
import os
import ssl
import subprocess
import sys
import tempfile
import time
import urllib.request
import urllib.error

# --------------------------------------------------------------------------
# Generate and import protobuf bindings from the vendored .proto files
# --------------------------------------------------------------------------
def load_protobuf():
    here = os.path.dirname(os.path.abspath(__file__))
    proto_dir = os.environ.get(
        "OTLP_PROTO_DIR",
        os.path.normpath(os.path.join(here, "..", "..", "..", "src", "server", "otlp")))
    protos = ["common.proto", "resource.proto", "logs.proto", "logs_service.proto"]
    for p in protos:
        if not os.path.isfile(os.path.join(proto_dir, p)):
            sys.exit("ERROR: cannot find %s in %s (set OTLP_PROTO_DIR)" % (p, proto_dir))
    outdir = tempfile.mkdtemp(prefix="otlp_log_pb_")
    cmd = ["protoc", "--proto_path=" + proto_dir, "--python_out=" + outdir] + protos
    try:
        subprocess.check_call(cmd)
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit("ERROR: protoc failed (%s). Install protobuf-compiler." % e)
    sys.path.insert(0, outdir)
    import logs_service_pb2     # noqa: E402
    import logs_pb2             # noqa: E402
    import common_pb2           # noqa: E402
    import resource_pb2         # noqa: E402
    return logs_service_pb2, logs_pb2, common_pb2, resource_pb2


SVC_PB, LOGS_PB, COMMON_PB, RESOURCE_PB = load_protobuf()

# OTLP severity number -> NetXMS severity band the server maps it to
# (see MapOtelSeverity in src/server/otlp/logs.cpp). For human-readable output.
def severity_band(n):
    if n >= 21:
        return "CRITICAL"
    if n >= 17:
        return "MAJOR"
    if n >= 13:
        return "WARNING"
    return "NORMAL"


# --------------------------------------------------------------------------
# Protobuf builders
# --------------------------------------------------------------------------
def set_any_value(av, value, value_type):
    """Populate an opentelemetry AnyValue from a python value."""
    if value_type == "string":
        av.string_value = str(value)
    elif value_type == "int":
        av.int_value = int(value)
    elif value_type == "double":
        av.double_value = float(value)
    elif value_type == "bool":
        av.bool_value = bool(value) if isinstance(value, bool) else str(value).lower() in ("1", "true", "yes")
    elif value_type == "kvlist":
        # Nested kvlist: server AnyValueToString() does not flatten these,
        # so the resulting body/attribute is stored empty. Useful negative test.
        av.kvlist_value.values.add()
    elif value_type == "none":
        pass  # leave AnyValue unset
    else:
        av.string_value = str(value)


def add_kv(parent_attrs, key, value, value_type="string"):
    kv = parent_attrs.add()
    kv.key = key
    set_any_value(kv.value, value, value_type)


def parse_attr_spec(spec):
    """Parse 'key=value' or 'key:type=value' into (key, value, type)."""
    name, _, value = spec.partition("=")
    value_type = "string"
    if ":" in name:
        name, _, value_type = name.partition(":")
    return name.strip(), value, value_type.strip() or "string"


def hex_to_bytes(s, expected_len, label):
    if not s:
        return b""
    try:
        b = bytes.fromhex(s)
    except ValueError:
        sys.exit("ERROR: %s must be a hex string, got %r" % (label, s))
    return b


def build_resource_logs(req, host_name=None, host_ip=None, service_name=None,
                        resource_attrs=None, scope_name=None, records=None):
    """Append one ResourceLogs (with a single ScopeLogs) carrying the given records."""
    rl = req.resource_logs.add()
    if host_name is not None:
        add_kv(rl.resource.attributes, "host.name", host_name)
    if host_ip is not None:
        add_kv(rl.resource.attributes, "host.ip", host_ip)
    if service_name is not None:
        add_kv(rl.resource.attributes, "service.name", service_name)
    for spec in resource_attrs or []:
        k, v, t = parse_attr_spec(spec)
        add_kv(rl.resource.attributes, k, v, t)

    sl = rl.scope_logs.add()
    if scope_name:
        sl.scope.name = scope_name
    for rec in records or []:
        build_log_record(sl.log_records.add(), rec)
    return rl


def build_log_record(lr, rec):
    if rec.get("time_unix_nano") is not None:
        lr.time_unix_nano = rec["time_unix_nano"]
    if rec.get("observed_time_unix_nano") is not None:
        lr.observed_time_unix_nano = rec["observed_time_unix_nano"]
    if rec.get("severity_number") is not None:
        lr.severity_number = rec["severity_number"]
    if rec.get("severity_text"):
        lr.severity_text = rec["severity_text"]
    if rec.get("body") is not None:
        set_any_value(lr.body, rec["body"], rec.get("body_type", "string"))
    if rec.get("flags") is not None:
        lr.flags = rec["flags"]
    if rec.get("dropped_attributes_count") is not None:
        lr.dropped_attributes_count = rec["dropped_attributes_count"]
    if rec.get("trace_id"):
        lr.trace_id = hex_to_bytes(rec["trace_id"], 16, "trace-id")
    if rec.get("span_id"):
        lr.span_id = hex_to_bytes(rec["span_id"], 8, "span-id")
    for spec in rec.get("attrs", []):
        k, v, t = parse_attr_spec(spec)
        add_kv(lr.attributes, k, v, t)


# --------------------------------------------------------------------------
# HTTP transport
# --------------------------------------------------------------------------
class Client:
    def __init__(self, args):
        self.base = args.url.rstrip("/")
        self.logs_path = args.endpoint
        self.login_path = args.login_endpoint
        self.token = args.token
        self.user = args.user
        self.password = args.password
        self.ctx = None
        if self.base.startswith("https") and args.insecure:
            self.ctx = ssl.create_default_context()
            self.ctx.check_hostname = False
            self.ctx.verify_mode = ssl.CERT_NONE

    def login(self):
        if self.token:
            return self.token
        body = json.dumps({"username": self.user, "password": self.password}).encode("utf-8")
        url = self.base + self.login_path
        req = urllib.request.Request(url, data=body, method="POST")
        req.add_header("Content-Type", "application/json")
        try:
            with urllib.request.urlopen(req, context=self.ctx, timeout=30) as resp:
                data = json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as e:
            sys.exit("ERROR: login failed (HTTP %d): %s" % (e.code, e.read().decode("utf-8", "replace")))
        except urllib.error.URLError as e:
            sys.exit("ERROR: cannot reach %s: %s" % (url, e))
        self.token = data.get("token")
        if not self.token:
            sys.exit("ERROR: login response did not contain a token: %s" % data)
        print("Logged in as %s, token=%s..." % (self.user, self.token[:8]))
        return self.token

    def send_logs(self, payload, content_type="application/x-protobuf", with_auth=True):
        """POST a serialized payload (bytes). Returns (status, response_bytes)."""
        url = self.base + self.logs_path
        req = urllib.request.Request(url, data=payload, method="POST")
        if content_type is not None:
            req.add_header("Content-Type", content_type)
        if with_auth:
            req.add_header("Authorization", "Bearer " + (self.token or ""))
        try:
            with urllib.request.urlopen(req, context=self.ctx, timeout=30) as resp:
                return resp.status, resp.read()
        except urllib.error.HTTPError as e:
            return e.code, e.read()
        except urllib.error.URLError as e:
            sys.exit("ERROR: cannot reach %s: %s" % (url, e))


def decode_response(body):
    """Decode ExportLogsServiceResponse; return rejected count and message."""
    if not body:
        return 0, None
    try:
        resp = SVC_PB.ExportLogsServiceResponse()
        resp.ParseFromString(body)
        if resp.HasField("partial_success"):
            return resp.partial_success.rejected_log_records, resp.partial_success.error_message
        return 0, None
    except Exception:  # noqa: BLE001
        return None, "<undecodable response: %r>" % body[:64]


def report(label, status, body, expect_status=200, expect_rejected=None):
    rejected, msg = decode_response(body) if status == 200 else (None, None)
    ok = (status == expect_status)
    detail = "HTTP %d" % status
    if status == 200:
        detail += ", rejected_log_records=%s" % rejected
        if msg:
            detail += " (%s)" % msg
        if expect_rejected is not None and rejected != expect_rejected:
            ok = False
            detail += " [EXPECTED rejected=%d]" % expect_rejected
    elif status != expect_status:
        detail += " (body: %r)" % body[:120]
    print("  [%s] %-28s %s" % ("PASS" if ok else "FAIL", label, detail))
    return ok


# --------------------------------------------------------------------------
# Single send from CLI flags
# --------------------------------------------------------------------------
def cmd_send(client, args):
    client.login()
    req = SVC_PB.ExportLogsServiceRequest()
    records = []
    for i in range(args.count):
        body = args.body
        if body is not None and args.count > 1:
            body = "%s #%d" % (body, i + 1)
        records.append({
            "time_unix_nano": (None if args.no_time else (args.time_unix_nano or int(time.time() * 1e9))),
            "observed_time_unix_nano": args.observed_time_nano,
            "severity_number": args.severity_number,
            "severity_text": args.severity_text,
            "body": body,
            "body_type": args.body_type,
            "flags": args.flags,
            "dropped_attributes_count": args.dropped_attrs,
            "trace_id": args.trace_id,
            "span_id": args.span_id,
            "attrs": args.attr or [],
        })
    build_resource_logs(req, host_name=args.host_name, host_ip=args.host_ip,
                        service_name=args.service_name, resource_attrs=args.resource_attr,
                        scope_name=args.scope_name, records=records)

    payload = req.SerializeToString()
    if args.malformed:
        payload = b"\xde\xad\xbe\xef not a valid protobuf"
        print("Sending MALFORMED payload (%d bytes)" % len(payload))
    else:
        print("Sending %d record(s), severity=%s(%s), %d byte(s)" %
              (args.count, args.severity_number, severity_band(args.severity_number or 0), len(payload)))

    ct = None if args.no_content_type else args.content_type
    for b in range(args.batches):
        status, body = client.send_logs(payload, content_type=ct, with_auth=not args.no_auth)
        report("batch %d" % (b + 1), status, body, expect_status=args.expect_status)
        if b + 1 < args.batches and args.interval:
            time.sleep(args.interval)


# --------------------------------------------------------------------------
# Scenario suite
# --------------------------------------------------------------------------
def scenario_match(client, args):
    print("\n[match] single record to a matchable node (expect rejected=0)")
    req = SVC_PB.ExportLogsServiceRequest()
    build_resource_logs(req, host_name=args.host_name, host_ip=args.host_ip,
                        service_name="auth-service", scope_name="test.sender",
                        records=[{"time_unix_nano": int(time.time() * 1e9),
                                  "severity_number": 9, "severity_text": "INFO",
                                  "body": "scenario match: user login ok"}])
    status, body = client.send_logs(req.SerializeToString())
    report("matched resource", status, body, expect_rejected=0)


def scenario_unmatched(client, args):
    print("\n[unmatched] resource that matches no node (expect rejected>0, HTTP 200)")
    req = SVC_PB.ExportLogsServiceRequest()
    build_resource_logs(req, host_name="this-host-does-not-exist-xyz",
                        scope_name="test.sender",
                        records=[{"severity_number": 9, "body": "should be rejected"},
                                 {"severity_number": 9, "body": "should be rejected too"}])
    status, body = client.send_logs(req.SerializeToString())
    report("unmatched resource", status, body, expect_rejected=2)


def scenario_mixed(client, args):
    print("\n[mixed] one matched + one unmatched resource in one batch (expect rejected=1)")
    req = SVC_PB.ExportLogsServiceRequest()
    build_resource_logs(req, host_name=args.host_name, scope_name="test.sender",
                        records=[{"severity_number": 9, "body": "matched record"}])
    build_resource_logs(req, host_name="nonexistent-host-abc", scope_name="test.sender",
                        records=[{"severity_number": 9, "body": "unmatched record"}])
    status, body = client.send_logs(req.SerializeToString())
    report("mixed batch", status, body, expect_rejected=1)


def scenario_severity(client, args):
    print("\n[severity] sweep all bands (TRACE..FATAL) -> NetXMS NORMAL/WARNING/MAJOR/CRITICAL")
    req = SVC_PB.ExportLogsServiceRequest()
    records = []
    for n in (1, 5, 9, 13, 17, 21, 24):
        records.append({"time_unix_nano": int(time.time() * 1e9), "severity_number": n,
                        "severity_text": "SEV%d" % n,
                        "body": "severity %d -> %s" % (n, severity_band(n))})
    build_resource_logs(req, host_name=args.host_name, scope_name="test.severity", records=records)
    status, body = client.send_logs(req.SerializeToString())
    report("severity sweep (7 recs)", status, body, expect_rejected=0)
    print("       verify stored severity_number column matches; severity bands per above")


def scenario_body_types(client, args):
    print("\n[body-types] string/int/double/bool stored as text; kvlist/none -> empty body")
    req = SVC_PB.ExportLogsServiceRequest()
    records = [
        {"severity_number": 9, "body": "plain string body", "body_type": "string"},
        {"severity_number": 9, "body": 42, "body_type": "int"},
        {"severity_number": 9, "body": 3.14159, "body_type": "double"},
        {"severity_number": 9, "body": True, "body_type": "bool"},
        {"severity_number": 9, "body": None, "body_type": "kvlist"},  # not flattened -> empty
        {"severity_number": 9},  # no body field at all -> NULL body
    ]
    build_resource_logs(req, host_name=args.host_name, scope_name="test.body", records=records)
    status, body = client.send_logs(req.SerializeToString())
    report("body types (6 recs)", status, body, expect_rejected=0)


def scenario_timestamps(client, args):
    print("\n[timestamps] origin/observed fallback behaviour (ns -> ms)")
    now_ns = int(time.time() * 1e9)
    req = SVC_PB.ExportLogsServiceRequest()
    records = [
        {"time_unix_nano": now_ns, "observed_time_unix_nano": now_ns, "severity_number": 9,
         "body": "both timestamps set"},
        {"time_unix_nano": 0, "observed_time_unix_nano": now_ns, "severity_number": 9,
         "body": "origin 0 -> falls back to observed"},
        {"severity_number": 9, "body": "no timestamps -> origin/observed 0, server sets receive time"},
    ]
    build_resource_logs(req, host_name=args.host_name, scope_name="test.time", records=records)
    status, body = client.send_logs(req.SerializeToString())
    report("timestamp cases (3 recs)", status, body, expect_rejected=0)


def scenario_trace(client, args):
    print("\n[trace] trace_id (16B) / span_id (8B) -> lowercase hex columns")
    req = SVC_PB.ExportLogsServiceRequest()
    records = [
        {"severity_number": 9, "body": "with full trace+span",
         "trace_id": "0123456789abcdef0123456789abcdef", "span_id": "0123456789abcdef"},
        {"severity_number": 9, "body": "no trace ids"},
    ]
    build_resource_logs(req, host_name=args.host_name, scope_name="test.trace", records=records)
    status, body = client.send_logs(req.SerializeToString())
    report("trace/span ids", status, body, expect_rejected=0)


def scenario_attributes(client, args):
    print("\n[attributes] resource+record attrs merged into stored JSON; mixed value types")
    req = SVC_PB.ExportLogsServiceRequest()
    rec = {"severity_number": 9, "body": "record with rich attributes",
           "attrs": ["thread.name=worker-1", "thread.id:int=7", "ratio:double=0.5",
                     "retry:bool=true", "url=https://example.com/x?y=z"]}
    build_resource_logs(req, host_name=args.host_name, service_name="attr-service",
                        resource_attrs=["deployment.environment=prod", "k8s.namespace=default"],
                        scope_name="test.attrs", records=[rec])
    status, body = client.send_logs(req.SerializeToString())
    report("merged attributes", status, body, expect_rejected=0)
    print("       verify stored 'attributes' JSON contains both resource and record keys")


def scenario_malformed(client, args):
    print("\n[malformed] invalid protobuf body -> HTTP 400")
    status, body = client.send_logs(b"\x00\x01\x02 garbage not protobuf")
    report("malformed payload", status, body, expect_status=400)


def scenario_auth(client, args):
    print("\n[auth] missing / bad token -> HTTP 401")
    req = SVC_PB.ExportLogsServiceRequest()
    build_resource_logs(req, host_name=args.host_name, scope_name="test.auth",
                        records=[{"severity_number": 9, "body": "no auth"}])
    payload = req.SerializeToString()
    status, body = client.send_logs(payload, with_auth=False)
    report("no Authorization header", status, body, expect_status=401)
    saved = client.token
    client.token = "INVALIDTOKEN000000000000000000000000"
    status, body = client.send_logs(payload, with_auth=True)
    report("invalid token", status, body, expect_status=401)
    client.token = saved


def scenario_content_type(client, args):
    print("\n[content-type] only application/x-protobuf accepted -> others 415")
    # The OTLP backend routes opt out of the default JSON acceptance
    # (.acceptJson(false) in src/server/otlp/main.cpp), so application/json,
    # text/plain and a missing Content-Type all get 415; only protobuf passes.
    req = SVC_PB.ExportLogsServiceRequest()
    build_resource_logs(req, host_name=args.host_name, scope_name="test.ct",
                        records=[{"severity_number": 9, "body": "content type test"}])
    payload = req.SerializeToString()
    status, body = client.send_logs(payload, content_type="application/json")
    report("Content-Type application/json", status, body, expect_status=415)
    status, body = client.send_logs(payload, content_type="text/plain")
    report("Content-Type text/plain", status, body, expect_status=415)
    status, body = client.send_logs(payload, content_type=None)
    report("missing Content-Type", status, body, expect_status=415)


def scenario_empty(client, args):
    print("\n[empty] empty ExportLogsServiceRequest (0 resources) -> HTTP 400")
    # An empty request serializes to 0 bytes; H_OtlpLogs treats a no-body POST as
    # a parse error and returns 400 ("Invalid protobuf payload"). A 0-resource
    # batch is not a real OTLP case, so 400 is acceptable.
    req = SVC_PB.ExportLogsServiceRequest()
    status, body = client.send_logs(req.SerializeToString())
    report("empty request", status, body, expect_status=400)


def scenario_load(client, args):
    n = args.load_count
    print("\n[load] %d records in one batch (throughput / writer batching)" % n)
    req = SVC_PB.ExportLogsServiceRequest()
    records = [{"time_unix_nano": int(time.time() * 1e9), "severity_number": 9,
                "body": "load record %d" % i, "attrs": ["seq:int=%d" % i]} for i in range(n)]
    build_resource_logs(req, host_name=args.host_name, scope_name="test.load", records=records)
    t0 = time.time()
    status, body = client.send_logs(req.SerializeToString())
    dt = time.time() - t0
    report("load %d recs in %.2fs" % (n, dt), status, body, expect_rejected=0)


SCENARIOS = {
    "match": scenario_match,
    "unmatched": scenario_unmatched,
    "mixed": scenario_mixed,
    "severity": scenario_severity,
    "body-types": scenario_body_types,
    "timestamps": scenario_timestamps,
    "trace": scenario_trace,
    "attributes": scenario_attributes,
    "malformed": scenario_malformed,
    "auth": scenario_auth,
    "content-type": scenario_content_type,
    "empty": scenario_empty,
    "load": scenario_load,
}


def cmd_scenario(client, args):
    client.login()
    names = list(SCENARIOS) if args.scenario == "all" else [args.scenario]
    print("Running scenario(s): %s" % ", ".join(names))
    for name in names:
        SCENARIOS[name](client, args)
    print("\nDone. Check server log (debug tags 'otlp' and 'otel.log', -D6) and the "
          "OpenTelemetry log viewer / otel_log table for stored records.")


# --------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(
        description="OTLP log sender for NetXMS OpenTelemetry log ingestion testing (issue #3292)")
    # connection
    ap.add_argument("--url", default="http://127.0.0.1:8000",
                    help="WebAPI base URL (default http://127.0.0.1:8000)")
    ap.add_argument("--endpoint", default="/otlp-backend/v1/logs", help="OTLP logs endpoint path")
    ap.add_argument("--login-endpoint", default="/v1/login", help="login endpoint path")
    ap.add_argument("--user", default="admin", help="WebAPI username (default admin)")
    ap.add_argument("--password", default="netxms", help="WebAPI password (default netxms)")
    ap.add_argument("--token", help="use this Bearer token directly (skip login)")
    ap.add_argument("--insecure", action="store_true", help="do not verify TLS certificate (https)")
    # resource / matching
    ap.add_argument("--host-name", help="host.name resource attribute (matches node name / DNS name)")
    ap.add_argument("--host-ip", help="host.ip resource attribute (matches node IP address)")
    ap.add_argument("--service-name", help="service.name resource attribute (-> service_name column)")
    ap.add_argument("--resource-attr", action="append",
                    help="extra resource attribute 'key=value' or 'key:type=value' (repeatable)")
    ap.add_argument("--scope-name", default="otlp.log.sender", help="instrumentation scope name")
    # record
    ap.add_argument("--body", help="log record body text")
    ap.add_argument("--body-type", default="string",
                    choices=["string", "int", "double", "bool", "kvlist", "none"],
                    help="AnyValue type for the body")
    ap.add_argument("--severity-number", type=int, default=9, help="OTLP severity number 1-24 (default 9=INFO)")
    ap.add_argument("--severity-text", default="INFO", help="severity text")
    ap.add_argument("--time-unix-nano", type=int, help="explicit event time (ns); default = now")
    ap.add_argument("--no-time", action="store_true", help="omit time_unix_nano (test observed-time fallback)")
    ap.add_argument("--observed-time-nano", type=int, help="explicit observed time (ns)")
    ap.add_argument("--trace-id", help="trace id as 32-char hex (16 bytes)")
    ap.add_argument("--span-id", help="span id as 16-char hex (8 bytes)")
    ap.add_argument("--flags", type=int, help="LogRecord flags (fixed32)")
    ap.add_argument("--dropped-attrs", type=int, help="dropped_attributes_count")
    ap.add_argument("--attr", action="append",
                    help="record attribute 'key=value' or 'key:type=value' (repeatable)")
    ap.add_argument("--count", type=int, default=1, help="number of log records to put in the scope")
    # transport / batching
    ap.add_argument("--batches", type=int, default=1, help="number of HTTP requests to send")
    ap.add_argument("--interval", type=float, default=0, help="seconds to wait between batches")
    ap.add_argument("--content-type", default="application/x-protobuf", help="Content-Type header to send")
    ap.add_argument("--no-content-type", action="store_true", help="omit Content-Type header")
    ap.add_argument("--no-auth", action="store_true", help="omit Authorization header")
    ap.add_argument("--malformed", action="store_true", help="send an invalid protobuf payload")
    ap.add_argument("--expect-status", type=int, default=200, help="expected HTTP status for PASS/FAIL")
    # scenarios
    ap.add_argument("--scenario", choices=["all"] + list(SCENARIOS),
                    help="run a predefined scenario instead of a single send")
    ap.add_argument("--load-count", type=int, default=500, help="records for the 'load' scenario")

    args = ap.parse_args()
    client = Client(args)

    if args.scenario:
        if not args.host_name and not args.host_ip and args.scenario in ("all", "match", "mixed",
                "severity", "body-types", "timestamps", "trace", "attributes", "auth", "content-type", "load"):
            print("WARNING: --host-name/--host-ip not set; matchable scenarios will report rejected>0.\n"
                  "         Use a host.name equal to an existing node name (or its DNS name) / host.ip = node IP.")
        cmd_scenario(client, args)
    else:
        if args.body is None and not args.malformed:
            args.body = "test log record from otlp_log_sender"
        cmd_send(client, args)


if __name__ == "__main__":
    main()
