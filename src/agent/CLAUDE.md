# Agent Component (nxagentd)

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for C++ development guidelines, build commands, and contribution workflow.

## Overview

The NetXMS agent (`nxagentd`) is a monitoring agent deployed on target systems. It collects metrics, executes actions, and communicates with the server.

## Directory Structure

```
src/agent/
├── core/           # Agent core (60+ files)
├── install/        # Installation scripts
├── libnxagent/     # Agent library (also used by server)
├── libnxappc/      # Application connectivity library
├── libnxsde/       # Subagent development library
├── libnxtux/       # Tuxedo integration library
├── nxsagent/       # Session agent
├── nxuseragent/    # User agent (Windows)
├── smbios/         # SMBIOS data collection
├── subagents/      # Platform-specific subagents (47+)
└── tools/          # Agent tools
```

## Build Commands

```bash
# Build agent only (after configure)
./configure --prefix=/opt/netxms --with-agent --enable-debug
make -j$(nproc)

# Run in debug mode (foreground, debug level 6)
/opt/netxms/bin/nxagentd -D6

# Run with specific config
/opt/netxms/bin/nxagentd -c /path/to/nxagentd.conf -D6
```

## Key Files

| File | Description |
|------|-------------|
| `core/nxagentd.cpp` | Agent main entry point |
| `core/session.cpp` | Server session handling |
| `core/config.cpp` | Configuration management |
| `core/localdb.cpp` | Local database for caching |
| `core/tunnel.cpp` | Tunnel connections to server |
| `libnxagent/subagent.cpp` | Subagent loading and management |

## Subagents

Subagents provide platform-specific and technology-specific monitoring capabilities.

### Platform Subagents

| Directory | Platform | Description |
|-----------|----------|-------------|
| `linux/` | Linux | Process, disk, network, memory |
| `winnt/` | Windows | WMI, performance counters, services |
| `aix/` | AIX | System metrics, LPARs |
| `sunos/` | Solaris | Zones, SMF services |
| `freebsd/` | FreeBSD | System metrics |
| `darwin/` | macOS | System metrics |
| `netbsd/` | NetBSD | System metrics |
| `openbsd/` | OpenBSD | System metrics |
| `minix/` | MINIX | System metrics |

### Technology Subagents

| Directory | Technology | Description |
|-----------|------------|-------------|
| `mysql/` | MySQL | Database monitoring |
| `pgsql/` | PostgreSQL | Database monitoring |
| `oracle/` | Oracle | Database monitoring |
| `db2/` | DB2 | Database monitoring |
| `informix/` | Informix | Database monitoring |
| `mongodb/` | MongoDB | Database monitoring |
| `redis/` | Redis | Cache monitoring |
| `java/` | Java | JVM via JMX |
| `jmx/` | JMX | Generic JMX monitoring |
| `python/` | Python | Python script execution |
| `ssh/` | SSH | Remote command execution |
| `prometheus/` | Prometheus | Prometheus metric scraping |

### Protocol Subagents

| Directory | Protocol | Description |
|-----------|----------|-------------|
| `mqtt/` | MQTT | MQTT broker monitoring |
| `opcua/` | OPC UA | Industrial protocol |
| `ping/` | ICMP | Ping functionality |
| `netsvc/` | Network | TCP/HTTP service checks |

### Hardware Subagents

| Directory | Hardware | Description |
|-----------|----------|-------------|
| `ups/` | UPS | UPS monitoring via NUT/APC |
| `gps/` | GPS | GPS receiver monitoring |
| `ds18x20/` | Sensors | DS18x20 temperature sensors |
| `lmsensors/` | Sensors | Linux hardware sensors |
| `rpi/` | Raspberry Pi | GPIO and sensor monitoring |

### Other Subagents

| Directory | Purpose | Description |
|-----------|---------|-------------|
| `asterisk/` | VoIP | Asterisk PBX monitoring |
| `bind9/` | DNS | BIND DNS server monitoring |
| `dbquery/` | Database | Custom SQL queries |
| `filemgr/` | Files | File management operations |
| `logwatch/` | Logs | Log file monitoring |
| `tuxedo/` | Tuxedo | Oracle Tuxedo monitoring |
| `vmgr/` | VMs | Virtual machine management |
| `xen/` | Xen | Xen hypervisor monitoring |
| `devemu/` | Testing | Device emulator for testing |

## Creating a New Subagent

### Basic Structure

```cpp
// mysubagent.h
#include <nms_common.h>
#include <nms_agent.h>

// Metric handler
LONG H_MyMetric(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

// mysubagent.cpp
#include "mysubagent.h"

static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("MySubagent.Metric(*)"), H_MyMetric, nullptr, DCI_DT_INT, _T("Description") }
};

LONG H_MyMetric(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR buffer[256];
   if (!AgentGetParameterArg(param, 1, buffer, 256))
      return SYSINFO_RC_UNSUPPORTED;

   // Get metric value
   ret_int(value, 42);
   return SYSINFO_RC_SUCCESS;
}

// Entry point
DECLARE_SUBAGENT_ENTRY_POINT(MYSUBAGENT)
{
   *ppInfo = &s_info;
   return true;
}

static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("MYSUBAGENT"),
   NETXMS_VERSION_STRING,
   nullptr,                    // init
   nullptr,                    // shutdown
   nullptr,                    // command handler
   nullptr,                    // notify
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   s_parameters,
   0,                          // list count
   nullptr,                    // lists
   0,                          // table count
   nullptr,                    // tables
   0,                          // action count
   nullptr,                    // actions
   0,                          // push parameter count
   nullptr                     // push parameters
};
```

### Metric Return Functions

```cpp
ret_string(value, _T("string value"));
ret_int(value, 42);
ret_int64(value, 9223372036854775807LL);
ret_uint(value, 42);
ret_uint64(value, 18446744073709551615ULL);
ret_double(value, 3.14159);
ret_mbstring(value, "multibyte string");
```

### Return Codes

| Code | Meaning |
|------|---------|
| `SYSINFO_RC_SUCCESS` | Value retrieved successfully |
| `SYSINFO_RC_UNSUPPORTED` | Metric not supported |
| `SYSINFO_RC_ERROR` | Error retrieving value |
| `SYSINFO_RC_NO_SUCH_INSTANCE` | Instance not found |

## Agent Configuration

Configuration file: `/etc/nxagentd.conf` or specified via `-c`

```ini
# Server connection
MasterServers = 192.168.1.1
ServerConnection = 192.168.1.1

# Subagent loading
SubAgent = linux.nsm
SubAgent = mysql.nsm

# External metrics
ExternalParameter = Custom.Metric : /path/to/script.sh

# Local database
EnableLocalDatabase = yes
```

## Debugging

```bash
# Run with maximum debug output
/opt/netxms/bin/nxagentd -D9

# Common debug tags (check subagent headers for exact tags)
nxlog_debug_tag(_T("linux"), level, ...)      # Linux subagent
nxlog_debug_tag(_T("mysql"), level, ...)      # MySQL subagent
nxlog_debug_tag(_T("session"), level, ...)    # Server sessions
nxlog_debug_tag(_T("tunnel"), level, ...)     # Tunnel connections
```

## libnxagent

The `libnxagent/` library provides:
- Subagent loading and management
- Metric parsing and parameter extraction
- Communication protocols
- Used by both agent and server (for metric parsing)

## Related Components

- [libnetxms](../libnetxms/CLAUDE.md) - Core library
- [libnxsl](../libnxsl/CLAUDE.md) - NXSL scripting (for action scripts)
- [Server](../server/CLAUDE.md) - Server that collects from agents
