#!/usr/bin/env python3
"""
Sample NetXMS agent extension (spawn mode).

Demonstrates the agent extension protocol with stdlib only:
  * hello handshake + token verification
  * list_capabilities
  * get_metric / get_list / get_table
  * execute_action with streamed output
  * ping reply, shutdown handling
  * a background thread that pushes a metric every 10 seconds

Configure the agent with, e.g.:

    Extension = sensors: spawn /usr/bin/python3 /path/to/sensor_extension.py

See ../../doc/AGENT_EXTENSION_PROTOCOL.md for the full protocol spec.
"""

import json
import os
import socket
import sys
import threading
import time

PROTOCOL_VERSION = 1

# --- fake sensor inventory ------------------------------------------------

SENSORS = {
    "sensor1": {"location": "rack-3", "temperature": 23.4},
    "sensor2": {"location": "rack-3", "temperature": 24.1},
    "sensor3": {"location": "rack-7", "temperature": 21.9},
}


def log(level, message):
    """Send a structured log line to the agent (also ends up in the agent log)."""
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


# --- request handlers -----------------------------------------------------

def parse_metric_arg(name):
    """Extract the single argument from 'Foo.Bar(arg)'."""
    a = name.find("(")
    b = name.rfind(")")
    return name[a + 1:b] if (a != -1 and b > a) else None


def handle_get_metric(name):
    if name.startswith("Sensors.Temperature("):
        sid = parse_metric_arg(name)
        if sid not in SENSORS:
            return None  # signals "no such instance"
        return "%.1f" % SENSORS[sid]["temperature"]
    if name == "Sensors.Count":
        return str(len(SENSORS))
    raise KeyError(name)


def handle_get_list(name):
    if name == "Sensors.SensorList":
        return list(SENSORS.keys())
    raise KeyError(name)


def handle_get_table(name):
    if name == "Sensors.Inventory":
        return {
            "columns": [
                {"name": "id", "type": "string", "instance": True},
                {"name": "location", "type": "string"},
                {"name": "temperature", "type": "float"},
            ],
            "rows": [
                [sid, s["location"], "%.1f" % s["temperature"]]
                for sid, s in SENSORS.items()
            ],
        }
    raise KeyError(name)


def handle_execute_action(req_id, name, args):
    if name != "Sensors.Recalibrate":
        return False
    sid = args[0] if args else "all"
    notify("action_output", {"name": name, "request_id": req_id,
                             "chunk": "recalibrating %s...\n" % sid})
    time.sleep(0.2)
    notify("action_output", {"name": name, "request_id": req_id, "chunk": "done\n"})
    return True


CAPABILITIES = {
    "metrics": [
        {"name": "Sensors.Temperature(*)", "type": "float", "description": "Sensor temperature, C"},
        {"name": "Sensors.Count", "type": "uint", "description": "Number of known sensors"},
    ],
    "lists": [
        {"name": "Sensors.SensorList", "description": "All sensor IDs"},
    ],
    "tables": [
        {"name": "Sensors.Inventory", "description": "Sensor inventory"},
    ],
    "actions": [
        {"name": "Sensors.Recalibrate", "description": "Recalibrate a sensor (arg: sensor ID)"},
    ],
}


def dispatch(msg, token):
    method = msg.get("method")
    req_id = msg.get("id")
    params = msg.get("params") or {}

    if method == "hello":
        if params.get("token") != token:
            error(req_id, -32001, "Unauthorized")
            return False  # caller should close the connection
        respond(req_id, {"protocol": PROTOCOL_VERSION, "extension": "sample-python/1.0"})
    elif method == "list_capabilities":
        respond(req_id, CAPABILITIES)
    elif method == "get_metric":
        try:
            v = handle_get_metric(params["name"])
            if v is None:
                error(req_id, -32010, "No such instance")
            else:
                respond(req_id, {"value": v})
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
    elif method == "execute_action":
        ok = handle_execute_action(req_id, params.get("name", ""), params.get("args", []))
        respond(req_id, {}) if ok else error(req_id, -32602, "Unknown action")
    elif method == "ping":
        respond(req_id, {})
    elif method == "shutdown":
        log("info", "shutdown requested by agent")
        _stop.set()
    else:
        if req_id is not None:
            error(req_id, -32601, "Unknown method: %s" % method)
    return True


# --- background pusher ----------------------------------------------------

def push_loop():
    while not _stop.wait(10):
        # Push the first sensor's temperature as a DCI value on our own schedule.
        s = next(iter(SENSORS.values()))
        notify("push_metric", {"name": "Sensors.Temperature(sensor1)",
                               "value": "%.1f" % s["temperature"],
                               "timestamp": int(time.time())})


# --- main -----------------------------------------------------------------

def main():
    port = os.environ.get("NXAGENT_EXTENSION_PORT")
    token = os.environ.get("NXAGENT_EXTENSION_TOKEN", "")
    if not port:
        sys.stderr.write("NXAGENT_EXTENSION_PORT not set; this extension must be launched by nxagentd in spawn mode\n")
        return 1

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", int(port)))
    srv.listen(1)

    conn, _ = srv.accept()
    conn.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

    global _conn
    _conn = conn

    threading.Thread(target=push_loop, daemon=True).start()
    log("info", "sample extension connected")

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
