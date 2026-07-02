# NetXMS Agent Extension Protocol

This document specifies the protocol used by the NetXMS agent (`nxagentd`) to
talk to **agent extensions** — independent processes that expose metrics, lists,
tables, and actions over a simple, language-agnostic protocol.

Extensions are a lighter-weight, less-coupled alternative to subagents: they run
in their own process, can be written in any language (Python is the primary
target), and communicate over a JSON-RPC 2.0 connection rather than the C ABI or
the NXCP-over-named-pipe path used by `ExternalSubagent`.

A minimal reference implementation is in
[`contrib/extensions/sample-python/`](../contrib/extensions/sample-python/).

## 1. Transport

- **TCP** on the loopback interface (`127.0.0.1` / `::1`).
- The **agent is the client**; the **extension is the server** (listener).
- Wire format: **line-delimited JSON** — one JSON value per line, terminated by
  `\n` (LF). UTF-8 throughout. Lines longer than 1 MiB are rejected and the
  connection is dropped.
- Application protocol: **JSON-RPC 2.0** (requests, responses, notifications).
- v1 is **plaintext on the wire**. TLS is out of scope; use stunnel / an SSH
  tunnel / WireGuard if you need a non-loopback endpoint.

## 2. Lifecycle modes

### spawn mode

The agent launches and supervises the extension process. Before `exec`, the
agent injects two environment variables:

| Variable | Meaning |
|---|---|
| `NXAGENT_EXTENSION_PORT` | TCP port the extension must bind/listen on (loopback) |
| `NXAGENT_EXTENSION_TOKEN` | Auth token the extension must accept in `hello` |

The extension binds `127.0.0.1:$NXAGENT_EXTENSION_PORT`, listens, and accepts
the agent's connection. Anything the extension writes to **stderr/stdout** is
captured and routed into the agent's log under the extension's debug tag
(`extension.<name>`), one line per message — so a stray `print()` is harmless
(it goes to the log, not the protocol).

On crash, the agent restarts the extension with exponential backoff. On agent
shutdown, the agent sends a `shutdown` notification, waits `ShutdownTimeout`
seconds, then `SIGTERM`s the process group, then `SIGKILL`s.

### connect mode

The extension is managed externally (systemd unit, container, embedded in a
larger application). The agent connects to a configured `tcp://host:port` and
reconnects with exponential backoff on disconnect. The auth token is pre-shared
via the agent's config (`Token = file:<path>` is preferred so the same file is
referenced by both sides). Non-loopback endpoints require `AllowRemote = yes`.

## 3. Handshake

Immediately after connecting, the agent issues `hello`:

```json
--> {"jsonrpc":"2.0","id":1,"method":"hello","params":{"protocol":1,"agent":"nxagentd/6.2.0","token":"<token>"}}
<-- {"jsonrpc":"2.0","id":1,"result":{"protocol":1,"extension":"my-ext/1.0"}}
```

- `protocol` (request): minimum protocol version the agent speaks (currently `1`).
- `token`: the auth token. The extension **must** verify it (against
  `NXAGENT_EXTENSION_TOKEN` in spawn mode, or its own copy of the pre-shared
  secret in connect mode). On mismatch, return JSON-RPC error `-32001` and close
  the connection.
- `protocol` (response): the protocol version the extension implements. Must be
  `>=` the agent's requested version, or the agent drops the connection.
- `extension` (response): free-form identification string (logged by the agent).

Then the agent issues `list_capabilities` (see below). Both must succeed before
the agent considers the extension `ready` and starts routing requests.

## 4. Methods (agent → extension, request/response)

### `list_capabilities`

```json
--> {"jsonrpc":"2.0","id":2,"method":"list_capabilities"}
<-- {"jsonrpc":"2.0","id":2,"result":{
      "metrics":[
        {"name":"MyExt.Temperature(*)","type":"float","description":"Sensor temperature, °C"},
        {"name":"MyExt.Status","type":"string","description":"Overall status"}
      ],
      "lists":[
        {"name":"MyExt.Sensors","description":"All sensor IDs"}
      ],
      "tables":[
        {"name":"MyExt.SensorTable","description":"Sensor inventory","columns":[
          {"name":"id","type":"string","instance":true},
          {"name":"location","type":"string"},
          {"name":"temperature","type":"float"}
        ]}
      ],
      "actions":[
        {"name":"MyExt.Reboot","description":"Reboot a sensor"}
      ]
   }}
```

All four arrays are optional. Metric / column `type` is one of:
`int` / `int32`, `uint` / `uint32`, `int64`, `uint64`, `float` / `double`,
`counter32`, `counter64`, `string` (the default if omitted).

Capabilities are fixed for the life of one connection. To change them, send a
`capabilities_changed` notification (§6) and the agent re-issues
`list_capabilities`.

### `get_metric`

```json
--> {"jsonrpc":"2.0","id":17,"method":"get_metric","params":{"name":"MyExt.Temperature(sensor1)"}}
<-- {"jsonrpc":"2.0","id":17,"result":{"value":"23.4"}}
```

`name` is the **full metric name including any `(arg)`** — parsing arguments out
of `Name(arg1,arg2)` is the extension's job (this mirrors the native agent
metric contract). `value` is always a **JSON string**, regardless of the
declared type — this avoids float-precision loss on large counters and lets the
extension format the value exactly as it wants.

### `get_list`

```json
--> {"jsonrpc":"2.0","id":18,"method":"get_list","params":{"name":"MyExt.Sensors"}}
<-- {"jsonrpc":"2.0","id":18,"result":{"values":["sensor1","sensor2","sensor3"]}}
```

### `get_table`

```json
--> {"jsonrpc":"2.0","id":19,"method":"get_table","params":{"name":"MyExt.SensorTable"}}
<-- {"jsonrpc":"2.0","id":19,"result":{
      "columns":[{"name":"id","type":"string","instance":true},
                 {"name":"location","type":"string"},
                 {"name":"temperature","type":"float"}],
      "rows":[["sensor1","rack-3","23.4"],
              ["sensor2","rack-3","24.1"]]
    }}
```

Each row is an array of cells positionally matching `columns`. Cells are
normally JSON strings; JSON numbers are also accepted. The extension may return
`columns` even when they match the advertised capability (the agent uses the
response's `columns`, not the cached one).

### `execute_action`

```json
--> {"jsonrpc":"2.0","id":20,"method":"execute_action","params":{"name":"MyExt.Reboot","args":["sensor1"]}}
... optional streamed output ...
--> {"jsonrpc":"2.0","method":"action_output","params":{"name":"MyExt.Reboot","request_id":20,"chunk":"rebooting sensor1...\n"}}
--> {"jsonrpc":"2.0","method":"action_output","params":{"name":"MyExt.Reboot","request_id":20,"chunk":"done\n"}}
<-- {"jsonrpc":"2.0","id":20,"result":{}}
```

`args` is a positional array. While the action runs, the extension may emit
`action_output` notifications (§6) carrying `request_id` equal to the
JSON-RPC `id` of this `execute_action` request — the agent forwards each `chunk`
to the server session that requested the action. The final JSON-RPC response
(`result` or `error`) terminates the action.

### `ping`

```json
--> {"jsonrpc":"2.0","id":N,"method":"ping"}
<-- {"jsonrpc":"2.0","id":N,"result":{}}
```

The agent sends `ping` every ~30 s. If three consecutive pings go unanswered
within ~10 s each, the agent drops the connection (and respawns/reconnects). The
extension may also send `ping` to the agent; the agent replies with `{}`.

## 5. Notifications (agent → extension)

| Method | Params | Meaning |
|---|---|---|
| `shutdown` | — | Agent is stopping; exit cleanly |

## 6. Notifications (extension → agent)

JSON-RPC notifications have no `id` and expect no response.

| Method | Params | Meaning |
|---|---|---|
| `push_metric` | `{"name":..., "value":..., "timestamp":<unix-seconds, optional>}` | Push one DCI value (routed like agent push DCI) |
| `push_metrics` | `{"items":[ {push_metric params}, ... ]}` | Batched form |
| `send_event` | `{"event":"<name>"} \| {"code":<int>}` plus `{"args":[...]}` | Generate a NetXMS event/trap; `args` become positional event parameters |
| `log` | `{"level":"error\|warning\|info\|debug", "message":"..."}` | Write a line to the agent log under the extension's debug tag |
| `action_output` | `{"name":..., "request_id":<int>, "chunk":"..."}` | Stream a chunk of action output (see `execute_action`) |
| `capabilities_changed` | — | Agent re-issues `list_capabilities` |
| `shutdown_notice` | `{"reason":"...", optional}` | Extension is exiting cleanly; agent suppresses crash-restart for this cycle |

## 7. Errors

Standard JSON-RPC error objects: `{"code":<int>,"message":"...","data":...}`.
The agent maps them to native metric return codes:

| JSON-RPC error code | Meaning | Agent result |
|---|---|---|
| `-32601` | method not found | unsupported metric |
| `-32602` | invalid params (e.g. unknown metric name) | unsupported metric |
| `-32001` | unauthorized (bad/missing token in `hello`) | connection dropped |
| `-32010` | no such instance | "no such instance" |
| `-32011` | transient metric error | error |
| `-32603` | internal error | error |

## 8. Configuration (agent side)

Flat shorthand (spawn mode only):

```
Extension = temperature: spawn /usr/bin/python3 /opt/myext/temperature.py
```

Full block form (`nxagentd.conf`):

```
*extensions/temperature
Mode = spawn
Command = /usr/bin/python3 /opt/myext/temperature.py
Environment = SENSOR_BUS=/dev/i2c-1
EnvironmentFile = /etc/nxagentd.d/temperature.env
RestartDelay = 5
RestartDelayMax = 300
ShutdownTimeout = 10
DebugTag = extension.temperature

*extensions/legacyapp
Mode = connect
Endpoint = tcp://127.0.0.1:9876
Token = file:/etc/nxagentd.d/legacyapp.token
AllowRemote = no
```

| Key | Mode | Default | Notes |
|---|---|---|---|
| `Mode` | both | — | `spawn` or `connect` (required) |
| `Command` | spawn | — | Required; argv split with shell-like quoting, **not** run through a shell |
| `Environment` | spawn | — | Repeatable `KEY=VALUE`; overlaid on the agent's environment |
| `EnvironmentFile` | spawn | — | File with `KEY=VALUE` lines (`#` comments allowed), re-read at every launch; overrides `Environment` on key collision. Keeps secrets out of the config file and its log/remote dumps |
| `RunAs` | spawn | agent's uid | Drop privileges before exec (Unix; not yet implemented — currently logged and ignored) |
| `RestartDelay` / `RestartDelayMax` | spawn | 5 / 300 (sec) | Exponential backoff on crash |
| `ShutdownTimeout` | spawn | 10 (sec) | Grace period before SIGTERM → SIGKILL |
| `Endpoint` | connect | — | `tcp://host:port` (required) |
| `Token` | connect (required), spawn (auto) | — | `file:<path>` or an inline string |
| `ReconnectDelay` / `ReconnectDelayMax` | connect | 5 / 300 (sec) | Exponential backoff on disconnect |
| `AllowRemote` | connect | `no` | Must be `yes` for a non-loopback `Endpoint` |
| `RequestTimeout` | both | 5 (sec) | Per-request timeout for `get_metric` / `get_list` / `get_table` |
| `ActionTimeout` | both | 60 (sec) | Per-request timeout for `execute_action` |
| `DebugTag` | both | `extension.<name>` | Tag for this extension's log lines and protocol traces |

## 9. Observability

The agent exposes:

- `Agent.Extension.IsConnected(name)` → `1` / `0`
- `Agent.Extension.Uptime(name)` → seconds since the last successful handshake
- `Agent.Extensions` table → `NAME`, `MODE`, `STATE`, `PID`, `PORT`,
  `RESTART_COUNT`, `CONNECTED_SINCE`, `LAST_ERROR`

Protocol traces appear at debug level 7 under the extension's debug tag.

## 10. Implementation checklist for an extension

1. Read `NXAGENT_EXTENSION_PORT` and `NXAGENT_EXTENSION_TOKEN` from the
   environment (spawn mode), bind `127.0.0.1:<port>`, listen, accept one
   connection.
2. Read newline-delimited JSON; for each line, dispatch on `method` (request /
   notification) or `id` (response — only relevant if you send `ping` to the
   agent).
3. Implement `hello` (verify token, return `{"protocol":1,...}`),
   `list_capabilities`, and whichever of `get_metric` / `get_list` /
   `get_table` / `execute_action` you advertise.
4. Reply to `ping` with `{"result":{}}`.
5. Handle the `shutdown` notification by exiting cleanly.
6. Optionally push data with `push_metric` / `send_event` notifications.
7. Send a JSON-RPC error (`-32602`/`-32010`/`-32011`) for bad/unknown/erroring
   metrics rather than crashing.
