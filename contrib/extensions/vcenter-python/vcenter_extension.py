#!/usr/bin/env python3
"""
NetXMS agent extension: VMware vCenter / ESXi monitoring (spawn mode).

Exposes CPU / memory utilization, datastore usage and VM/host inventory from a
vCenter (or standalone ESXi) server to the NetXMS agent. Built on pyVmomi.

Design:
  * vCenter is polled ONCE on a background timer (VC_POLL_INTERVAL, default 60 s)
    into an in-memory cache, using a single PropertyCollector retrieval per
    entity type. All get_metric / get_list / get_table requests are served from
    that cache, so per-DCI polling never touches vCenter.
  * Instance key for VMs, datastores and hosts is the managed-object ID (MOID),
    which is stable across renames. Human-readable names are exposed as a label
    column / list value.

Install:
    pip install pyvmomi

Configure the agent (nxagentd.conf), e.g.:

    [extensions/vcenter]
    Mode = spawn
    Command = /usr/bin/python3 /opt/netxms/extensions/vcenter_extension.py
    Environment = VC_HOST=vc.example.com
    Environment = VC_USER=monitor@vsphere.local
    Environment = VC_PASSWORD=secret
    Environment = VC_POLL_INTERVAL=60
    DebugTag = extension.vcenter

See ../../../doc/AGENT_EXTENSION_PROTOCOL.md for the full protocol spec.
"""

import json
import os
import socket
import ssl
import sys
import threading
import time

try:
    from pyVim.connect import SmartConnect, Disconnect
    from pyVmomi import vim, vmodl
except ImportError:
    sys.stderr.write("pyvmomi is not installed (pip install pyvmomi)\n")
    sys.exit(1)

PROTOCOL_VERSION = 1

# --- configuration (from agent-injected environment) ----------------------

VC_HOST = os.environ.get("VC_HOST", "")
VC_USER = os.environ.get("VC_USER", "")
VC_PASSWORD = os.environ.get("VC_PASSWORD") or os.environ.get("VC_PASS", "")
VC_PORT = int(os.environ.get("VC_PORT", "443"))
VC_POLL_INTERVAL = int(os.environ.get("VC_POLL_INTERVAL", "60"))
VC_VERIFY_TLS = os.environ.get("VC_VERIFY_TLS", "no").lower() in ("1", "yes", "true")


# --- in-memory cache ------------------------------------------------------

class Cache:
    """Snapshot of the last successful vCenter poll. Read under _lock."""

    def __init__(self):
        self.lock = threading.Lock()
        self.vms = {}          # moid -> dict
        self.datastores = {}   # moid -> dict
        self.hosts = {}        # moid -> dict
        self.updated = 0       # unix seconds of last successful poll
        self.error = None      # last poll error message, or None


_cache = Cache()


def log(level, message):
    notify("log", {"level": level, "message": message})


# --- JSON-RPC plumbing ----------------------------------------------------

_conn = None
_conn_lock = threading.Lock()
_stop = threading.Event()


def _send(obj):
    data = (json.dumps(obj, separators=(",", ":")) + "\n").encode("utf-8")
    with _conn_lock:
        if _conn is not None:
            _conn.sendall(data)


def respond(req_id, result):
    _send({"jsonrpc": "2.0", "id": req_id, "result": result})


def error(req_id, code, message):
    _send({"jsonrpc": "2.0", "id": req_id, "error": {"code": code, "message": message}})


def notify(method, params=None):
    msg = {"jsonrpc": "2.0", "method": method}
    if params is not None:
        msg["params"] = params
    _send(msg)


# --- vCenter polling ------------------------------------------------------

def _collect(content, vimtype, props):
    """Retrieve `props` for every object of `vimtype` in one PropertyCollector call."""
    view = content.viewManager.CreateContainerView(content.rootFolder, [vimtype], True)
    try:
        traversal = vmodl.query.PropertyCollector.TraversalSpec(
            name="traverseView", path="view", skip=False,
            type=vim.view.ContainerView)
        obj_spec = vmodl.query.PropertyCollector.ObjectSpec(
            obj=view, skip=True, selectSet=[traversal])
        prop_spec = vmodl.query.PropertyCollector.PropertySpec(
            type=vimtype, pathSet=props, all=False)
        filter_spec = vmodl.query.PropertyCollector.FilterSpec(
            objectSet=[obj_spec], propSet=[prop_spec])

        out = []
        for o in content.propertyCollector.RetrieveContents([filter_spec]):
            entry = {"moid": o.obj._moId}
            for p in o.propSet:
                entry[p.name] = p.val
            out.append(entry)
        return out
    finally:
        view.Destroy()


def _perc(used, total):
    return round(used / total * 100.0, 1) if used is not None and total else None


def _mb_to_bytes(mb):
    return mb * 1024 * 1024 if mb is not None else None


def _build_hosts(content):
    hosts = {}
    for h in _collect(content, vim.HostSystem, ["summary"]):
        s = h.get("summary")
        if not s:
            continue
        qs = s.quickStats
        hw = s.hardware
        total_mhz = (hw.cpuMhz * hw.numCpuCores) if hw else None
        total_mem_mb = (hw.memorySize / (1024 * 1024)) if hw and hw.memorySize else None
        hosts[h["moid"]] = {
            "moid": h["moid"],
            "name": s.config.name if s.config else None,
            "cpuUsageMHz": getattr(qs, "overallCpuUsage", None),
            "cpuPerc": _perc(getattr(qs, "overallCpuUsage", None), total_mhz),
            "memUsageBytes": _mb_to_bytes(getattr(qs, "overallMemoryUsage", None)),
            "memPerc": _perc(getattr(qs, "overallMemoryUsage", None), total_mem_mb),
            "connectionState": str(s.runtime.connectionState) if s.runtime else None,
            "numVMs": 0,  # filled in once VMs are known
        }
    return hosts


def _build_vms(content):
    vms = {}
    for v in _collect(content, vim.VirtualMachine, ["summary"]):
        s = v.get("summary")
        if not s:
            continue
        cfg, qs, rt, st = s.config, s.quickStats, s.runtime, s.storage
        host_moid = rt.host._moId if rt and rt.host else None
        power_state = str(rt.powerState) if rt else None
        powered_on = power_state == "poweredOn"

        # A powered-off VM publishes no live CPU/memory quickStats. Report 0 for
        # the usage counters in that case (rather than "no value") so the DCIs
        # stay clean. A powered-on VM with momentarily absent quickStats keeps
        # None, which surfaces as a transient metric error.
        cpu_mhz = getattr(qs, "overallCpuUsage", None)
        guest_mem_mb = getattr(qs, "guestMemoryUsage", None)
        uptime = getattr(qs, "uptimeSeconds", None)
        if not powered_on:
            cpu_mhz = 0
            guest_mem_mb = 0
            uptime = 0

        vms[v["moid"]] = {
            "moid": v["moid"],
            "name": cfg.name if cfg else None,
            "powerState": power_state,
            "cpuUsageMHz": cpu_mhz,
            "cpuPerc": 0.0 if not powered_on else _perc(cpu_mhz, getattr(rt, "maxCpuUsage", None)),
            "memUsageBytes": _mb_to_bytes(guest_mem_mb),
            "memPerc": 0.0 if not powered_on else _perc(guest_mem_mb, cfg.memorySizeMB if cfg else None),
            "uptimeSeconds": uptime,
            "committedBytes": getattr(st, "committed", None),
            "hostMoid": host_moid,
            "guestOS": cfg.guestFullName if cfg else None,
        }
    return vms


def _build_datastores(content):
    datastores = {}
    for d in _collect(content, vim.Datastore, ["summary"]):
        s = d.get("summary")
        if not s:
            continue
        used = (s.capacity - s.freeSpace) if s.capacity is not None and s.freeSpace is not None else None
        datastores[d["moid"]] = {
            "moid": d["moid"],
            "name": s.name,
            "capacityBytes": s.capacity,
            "freeBytes": s.freeSpace,
            "usagePerc": _perc(used, s.capacity),
            "type": s.type,
        }
    return datastores


def _make_ssl_context():
    ctx = ssl.create_default_context()
    if not VC_VERIFY_TLS:
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
    return ctx


def poll_once():
    """One full vCenter poll; updates the global cache or records the error."""
    si = SmartConnect(host=VC_HOST, port=VC_PORT, user=VC_USER,
                      pwd=VC_PASSWORD, sslContext=_make_ssl_context())
    try:
        content = si.RetrieveContent()
        hosts = _build_hosts(content)
        vms = _build_vms(content)
        datastores = _build_datastores(content)

        # Resolve host names on VMs and per-host VM counts.
        for vm in vms.values():
            h = hosts.get(vm["hostMoid"])
            vm["hostName"] = h["name"] if h else None
            if h is not None:
                h["numVMs"] += 1

        with _cache.lock:
            _cache.vms = vms
            _cache.datastores = datastores
            _cache.hosts = hosts
            _cache.updated = int(time.time())
            _cache.error = None
    finally:
        Disconnect(si)


def poll_loop():
    # Poll immediately, then every VC_POLL_INTERVAL seconds until shutdown.
    while True:
        try:
            poll_once()
            log("debug", "vcenter poll ok: %d VMs, %d datastores, %d hosts" %
                (len(_cache.vms), len(_cache.datastores), len(_cache.hosts)))
        except Exception as e:  # noqa: BLE001 - keep the poller alive on any failure
            with _cache.lock:
                _cache.error = str(e)
            log("error", "vcenter poll failed: %s" % e)
        if _stop.wait(VC_POLL_INTERVAL):
            return


# --- capabilities ---------------------------------------------------------

CAPABILITIES = {
    "metrics": [
        {"name": "VMware.VM.Name(*)", "type": "string", "description": "VM name by MOID"},
        {"name": "VMware.VM.CPUUsage(*)", "type": "float", "description": "VM CPU utilization, %"},
        {"name": "VMware.VM.CPUUsageMHz(*)", "type": "uint", "description": "VM CPU usage, MHz"},
        {"name": "VMware.VM.MemoryUsage(*)", "type": "float", "description": "VM memory utilization, %"},
        {"name": "VMware.VM.MemoryUsageBytes(*)", "type": "uint64", "description": "VM guest memory usage, bytes"},
        {"name": "VMware.VM.PowerState(*)", "type": "string", "description": "VM power state"},
        {"name": "VMware.VM.UptimeSeconds(*)", "type": "uint", "description": "VM uptime, seconds"},
        {"name": "VMware.VM.StorageCommittedBytes(*)", "type": "uint64", "description": "VM committed storage, bytes"},
        {"name": "VMware.Datastore.Name(*)", "type": "string", "description": "Datastore name by MOID"},
        {"name": "VMware.Datastore.UsagePerc(*)", "type": "float", "description": "Datastore usage, %"},
        {"name": "VMware.Datastore.FreeBytes(*)", "type": "uint64", "description": "Datastore free space, bytes"},
        {"name": "VMware.Datastore.CapacityBytes(*)", "type": "uint64", "description": "Datastore capacity, bytes"},
        {"name": "VMware.Host.Name(*)", "type": "string", "description": "Host name by MOID"},
        {"name": "VMware.Host.CPUUsage(*)", "type": "float", "description": "Host CPU utilization, %"},
        {"name": "VMware.Host.CPUUsageMHz(*)", "type": "uint", "description": "Host CPU usage, MHz"},
        {"name": "VMware.Host.MemoryUsage(*)", "type": "float", "description": "Host memory utilization, %"},
        {"name": "VMware.Host.MemoryUsageBytes(*)", "type": "uint64", "description": "Host memory usage, bytes"},
        {"name": "VMware.Host.NumVMs(*)", "type": "uint", "description": "Number of VMs on host"},
        {"name": "VMware.Host.ConnectionState(*)", "type": "string", "description": "Host connection state"},
    ],
    "lists": [
        {"name": "VMware.VMList", "description": "VM MOIDs (for instance discovery)"},
        {"name": "VMware.DatastoreList", "description": "Datastore MOIDs (for instance discovery)"},
        {"name": "VMware.HostList", "description": "Host MOIDs (for instance discovery)"},
    ],
    "tables": [
        {"name": "VMware.VMs", "description": "Virtual machine inventory", "columns": [
            {"name": "moid", "type": "string", "instance": True},
            {"name": "name", "type": "string"},
            {"name": "powerState", "type": "string"},
            {"name": "cpuPerc", "type": "float"},
            {"name": "memPerc", "type": "float"},
            {"name": "host", "type": "string"},
            {"name": "guestOS", "type": "string"},
        ]},
        {"name": "VMware.Datastores", "description": "Datastore inventory", "columns": [
            {"name": "moid", "type": "string", "instance": True},
            {"name": "name", "type": "string"},
            {"name": "capacityBytes", "type": "uint64"},
            {"name": "freeBytes", "type": "uint64"},
            {"name": "usagePerc", "type": "float"},
            {"name": "type", "type": "string"},
        ]},
        {"name": "VMware.Hosts", "description": "ESXi host inventory", "columns": [
            {"name": "moid", "type": "string", "instance": True},
            {"name": "name", "type": "string"},
            {"name": "cpuPerc", "type": "float"},
            {"name": "memPerc", "type": "float"},
            {"name": "numVMs", "type": "uint"},
            {"name": "connectionState", "type": "string"},
        ]},
    ],
}


# --- request handlers -----------------------------------------------------

class NoInstance(Exception):
    """Requested instance (MOID) is not present in the cache."""


def parse_metric_arg(name):
    """Extract the single argument from 'Foo.Bar(arg)'."""
    a = name.find("(")
    b = name.rfind(")")
    return name[a + 1:b] if (a != -1 and b > a) else None


# metric prefix -> (cache attribute, field, formatter)
def _fmt(v):
    return None if v is None else str(v)


_METRIC_MAP = {
    "VMware.VM.Name(": ("vms", "name"),
    "VMware.VM.CPUUsage(": ("vms", "cpuPerc"),
    "VMware.VM.CPUUsageMHz(": ("vms", "cpuUsageMHz"),
    "VMware.VM.MemoryUsage(": ("vms", "memPerc"),
    "VMware.VM.MemoryUsageBytes(": ("vms", "memUsageBytes"),
    "VMware.VM.PowerState(": ("vms", "powerState"),
    "VMware.VM.UptimeSeconds(": ("vms", "uptimeSeconds"),
    "VMware.VM.StorageCommittedBytes(": ("vms", "committedBytes"),
    "VMware.Datastore.Name(": ("datastores", "name"),
    "VMware.Datastore.UsagePerc(": ("datastores", "usagePerc"),
    "VMware.Datastore.FreeBytes(": ("datastores", "freeBytes"),
    "VMware.Datastore.CapacityBytes(": ("datastores", "capacityBytes"),
    "VMware.Host.Name(": ("hosts", "name"),
    "VMware.Host.CPUUsage(": ("hosts", "cpuPerc"),
    "VMware.Host.CPUUsageMHz(": ("hosts", "cpuUsageMHz"),
    "VMware.Host.MemoryUsage(": ("hosts", "memPerc"),
    "VMware.Host.MemoryUsageBytes(": ("hosts", "memUsageBytes"),
    "VMware.Host.NumVMs(": ("hosts", "numVMs"),
    "VMware.Host.ConnectionState(": ("hosts", "connectionState"),
}


def handle_get_metric(name):
    for prefix, (attr, field) in _METRIC_MAP.items():
        if name.startswith(prefix):
            moid = parse_metric_arg(name)
            with _cache.lock:
                collection = getattr(_cache, attr)
                entry = collection.get(moid)
            if entry is None:
                raise NoInstance()
            return _fmt(entry.get(field))
    raise KeyError(name)


def handle_get_list(name):
    with _cache.lock:
        if name == "VMware.VMList":
            return list(_cache.vms.keys())
        if name == "VMware.DatastoreList":
            return list(_cache.datastores.keys())
        if name == "VMware.HostList":
            return list(_cache.hosts.keys())
    raise KeyError(name)


def handle_get_table(name):
    with _cache.lock:
        if name == "VMware.VMs":
            cols = ["moid", "name", "powerState", "cpuPerc", "memPerc", "hostName", "guestOS"]
            rows = [[_fmt(vm.get(c)) for c in cols] for vm in _cache.vms.values()]
            return _table(CAPABILITIES["tables"][0]["columns"], rows)
        if name == "VMware.Datastores":
            cols = ["moid", "name", "capacityBytes", "freeBytes", "usagePerc", "type"]
            rows = [[_fmt(ds.get(c)) for c in cols] for ds in _cache.datastores.values()]
            return _table(CAPABILITIES["tables"][1]["columns"], rows)
        if name == "VMware.Hosts":
            cols = ["moid", "name", "cpuPerc", "memPerc", "numVMs", "connectionState"]
            rows = [[_fmt(h.get(c)) for c in cols] for h in _cache.hosts.values()]
            return _table(CAPABILITIES["tables"][2]["columns"], rows)
    raise KeyError(name)


def _table(columns, rows):
    return {"columns": columns, "rows": rows}


# --- dispatch -------------------------------------------------------------

def dispatch(msg, token):
    method = msg.get("method")
    req_id = msg.get("id")
    params = msg.get("params") or {}

    if method == "hello":
        if params.get("token") != token:
            error(req_id, -32001, "Unauthorized")
            return False
        respond(req_id, {"protocol": PROTOCOL_VERSION, "extension": "vcenter-python/1.0"})
    elif method == "list_capabilities":
        respond(req_id, CAPABILITIES)
    elif method == "get_metric":
        try:
            value = handle_get_metric(params["name"])
            if value is None:
                # Instance exists but this field has no current value (e.g. a
                # powered-off VM exposes no live CPU/memory quickStats). The
                # protocol requires `value` to be a JSON string, so reporting a
                # transient error here is correct - emitting {"value": null}
                # would make the agent fail the request with an internal error.
                error(req_id, -32011, "Metric value not available")
            else:
                respond(req_id, {"value": value})
        except NoInstance:
            error(req_id, -32010, "No such instance")
        except KeyError:
            error(req_id, -32602, "Unknown metric")
    elif method == "get_list":
        try:
            respond(req_id, {"values": handle_get_list(params["name"])})
        except KeyError:
            error(req_id, -32602, "Unknown list")
    elif method == "get_table":
        try:
            respond(req_id, handle_get_table(params["name"]))
        except KeyError:
            error(req_id, -32602, "Unknown table")
    elif method == "ping":
        respond(req_id, {})
    elif method == "shutdown":
        log("info", "shutdown requested by agent")
        _stop.set()
    else:
        if req_id is not None:
            error(req_id, -32601, "Unknown method: %s" % method)
    return True


# --- main -----------------------------------------------------------------

def main():
    port = os.environ.get("NXAGENT_EXTENSION_PORT")
    token = os.environ.get("NXAGENT_EXTENSION_TOKEN", "")
    if not port:
        sys.stderr.write("NXAGENT_EXTENSION_PORT not set; this extension must be "
                         "launched by nxagentd in spawn mode\n")
        return 1
    if not VC_HOST or not VC_USER:
        sys.stderr.write("VC_HOST and VC_USER must be set in the extension environment\n")
        return 1

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", int(port)))
    srv.listen(1)

    conn, _ = srv.accept()
    conn.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

    global _conn
    _conn = conn

    threading.Thread(target=poll_loop, daemon=True).start()
    log("info", "vcenter extension connected (polling %s every %ds)" % (VC_HOST, VC_POLL_INTERVAL))

    buf = b""
    try:
        while not _stop.is_set():
            chunk = conn.recv(65536)
            if not chunk:
                break
            buf += chunk
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                line = line.strip()
                if not line:
                    continue
                try:
                    msg = json.loads(line.decode("utf-8"))
                except ValueError:
                    continue
                if not dispatch(msg, token):
                    return 0
    finally:
        _stop.set()
        try:
            conn.close()
        except OSError:
            pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
