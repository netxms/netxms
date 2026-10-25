# Agent Component (nxagentd)

## Creating a New Subagent

### Where Does It Belong?

Before bundling an optional-dependency capability into an existing subagent, check the underlying library/tool's cross-platform availability:

- Linux-only with no extra deps → fold into the `linux` subagent.
- Cross-platform (Linux + BSD + macOS + …) → create a **dedicated** subagent (the `lldpd.nsm` / `ssh.nsm` / `mqtt.nsm` / `modbus.nsm` pattern), one per optional capability.
- The core agent is rarely the right home — it forces link dependencies into the base binary and breaks the one-subagent-per-optional-capability convention.

### Structure

Copy a small existing subagent as the starting point (`bind9/`, `devemu/`) — metric handlers return `SYSINFO_RC_*` and fill the value with the `ret_*` helpers; the `NETXMS_SUBAGENT_INFO` layout is defined in `include/nms_agent.h`.

## Agent Configuration

Configuration uses **hierarchical bracket sections**: `[Section/Subsection/Instance]`. The trailing path element is the instance identifier and can supply an object's name directly — e.g. `[ENTSOE/Zone/LV]` defines zone "LV" with no separate `Name =` key. Use one such block per instance (zone/database/target) for multi-instance subagent config. The legacy asterisk-prefixed `*Section` syntax is deprecated — do not use it in new subagent/config designs even though older subagents still show it.

A plain `Password` key already accepts an encrypted value — `DecryptPassword()` autodetects it. `EncryptedPassword` is a deprecated alias kept only where it already existed (some DB subagents); do not add it to new or unified `NX_CFG_TEMPLATE` tables.

## Agent Extensions

Agent extensions are independent processes that expose metrics, lists, tables,
and actions to the agent over **JSON-RPC 2.0 on a loopback TCP port** — a
language-agnostic, less-coupled alternative to subagents (Python is the primary
target). The agent is the client; the extension is the listener. There is no Python subagent; Python integrations use this protocol. Implementation: `core/extension*.cpp`.

- **Protocol spec:** [`doc/AGENT_EXTENSION_PROTOCOL.md`](../../doc/AGENT_EXTENSION_PROTOCOL.md)
- **Reference implementation:** [`contrib/extensions/sample-python/`](../../contrib/extensions/sample-python/)
- **Config:** see the "Agent extensions" section in `contrib/nxagentd.conf-dist`
- **Debug tags:** `extension` (common), `extension.<name>` (per-extension)
- **Dispatch precedence:** native subagents → `ExternalSubagent` (named pipe/NXCP) → `AgentExtension` (TCP/JSON-RPC)
