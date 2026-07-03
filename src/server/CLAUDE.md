# Server Component (netxmsd)

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for C++ development guidelines, build commands, and contribution workflow.

## Exporting Functions for Modules

Server core is a shared library. External modules (e.g., `webapi/`, `hdlink/`, `ntcb/`) link against it. Any function or class defined in `core/` that needs to be called from a module **must** be marked with `NXCORE_EXPORTABLE` in its declaration (in the header under `include/`). For exported variables, use `NXCORE_EXPORTABLE_VAR(v)`. Without this, the symbol will not be visible to modules and linking will fail at runtime.

```cpp
// In include/nms_core.h or other server header
void NXCORE_EXPORTABLE MyFunction();
class NXCORE_EXPORTABLE MyClass { ... };
extern NXCORE_EXPORTABLE_VAR(int) g_myGlobal;
```

## String Handling

Server code is always built in Unicode mode. Use `L"..."` string literals and wide-character functions (`wcsncmp`, `wcslen`, etc.) directly. Do **not** use `_T()` / `TCHAR` / `_tcsncmp` wrappers in new server code — those are remnants from older versions that supported non-Unicode builds. The `_T()` abstraction is only needed in agent code and shared libraries that may be built in either mode.

For formatting into a `wchar_t *` buffer, use `nx_swprintf(buffer, size, L"...", ...)` rather than `swprintf()` / `_snwprintf()`: `%s` means `wchar_t*` on Windows but `char*` on glibc, and `nx_swprintf` normalizes `%s` to `wchar_t*` on both platforms. Same argument order as `swprintf`.

## Server Core Conventions

- **Console commands:** when adding or substantially rewriting a branch in `ProcessConsoleCommand()` (`core/console.cpp`, already >1000 lines), extract the logic into a dedicated `static void HandleXxxCommand(ServerConsole*, const wchar_t *arg)` defined earlier in the file. The branch body should be a one- or two-line call.

- **Object update from JSON (WebAPI `modifyFromJSON*` handlers):** when reading a scalar property into an object field, use the strict `json_object_update_*` helpers in `include/nms_util.h` instead of hand-writing the get/validate/cast block. Each returns `false` only when the property is present with an incompatible type — map that to `RCC_INVALID_ARGUMENT`; absent leaves the field unchanged:
  - `json_object_update_integer(json, "tag", &field)` — any integer field type; null = 0.
  - `json_object_update_boolean(json, "tag", &boolField)`.
  - `json_object_update_flag(json, "tag", BIT, &setFlags, &mask)` — collect bits for one trailing `updateFlags(setFlags, mask)`.
  - `json_object_update_string(json, "tag", wcharBuf, capacityChars)` / `json_object_update_string_utf8(json, "tag", charBuf, capacityBytes)` — fixed buffers; null/empty clear.

  These are strict (fail on wrong type) — distinct from the lenient coercing `json_object_get_*` getters. Keep blocks manual when the field has side effects, needs object-ID validation, targets a setter or `String`/`SharedString` member, or uses null-means-clear semantics — the helpers don't cover those.

## Overview

The NetXMS server (`netxmsd`) is the central management component that handles:
- Network monitoring and data collection
- Event and alarm processing
- Data storage in the database
- Communication with agents and network devices
- REST API for client access

## Directory Structure

```
src/server/
├── core/           # Main server core (190+ files)
├── drivers/        # Network device drivers (35+ vendors)
├── include/        # Server-specific headers
├── libnxsrv/       # Server library (shared with drivers)
├── netxmsd/        # Server executable entry point
├── tools/          # Server tools (nxdbmgr, nxadm, etc.)
├── webapi/         # REST API implementation
├── aitools/        # AI integration tools
├── ncdrivers/      # Notification channel drivers
├── hdlink/         # Help desk integration
├── leef/           # LEEF log exporter
├── ntcb/           # Network topology builder
├── pdsdrv/         # PDS drivers
├── wcc/            # Web connection cache
└── wlcbridge/      # WLC bridge
```

## Build Commands

```bash
# Build server only (after configure)
./configure --prefix=/opt/netxms --with-sqlite --with-server --enable-debug
make -j$(nproc)

# Run in debug mode (foreground, debug level 6)
/opt/netxms/bin/netxmsd -D6

# Database management
/opt/netxms/bin/nxdbmgr check      # Check database integrity
/opt/netxms/bin/nxdbmgr upgrade    # Upgrade database schema
```

## Key Files

| File | Description |
|------|-------------|
| `core/main.cpp` | Server main loop and initialization |
| `core/session.cpp` | Client session handling |
| `core/node.cpp` | Node object implementation |
| `core/interface.cpp` | Interface object implementation |
| `core/dctable.cpp` | Data collection table |
| `core/poll.cpp` | Polling infrastructure |
| `core/events.cpp` | Event processing |
| `core/alarm.cpp` | Alarm management |
| `core/ai_core.cpp` | AI/LLM integration core |

## Network Device Drivers

Network device drivers provide vendor-specific support for switches, routers, and other equipment.

### Driver Location

All drivers are in `drivers/<vendor>/`:
- Each driver has: `<vendor>.h`, `<vendor>.cpp`
- Driver interface: `include/nddrv.h`
- Base class: `libnxsrv/ndd.cpp`

### Currently Supported Vendors

| Vendor | Directory | Description |
|--------|-----------|-------------|
| Cisco | `cisco/` | IOS, NX-OS, Wireless |
| Juniper | `juniper/` | JunOS devices |
| Huawei | `huawei/` | VRP switches/routers |
| HPE | `hpe/` | ProCurve, Aruba |
| MikroTik | `mikrotik/` | RouterOS |
| Extreme | `extreme/` | EXOS switches |
| Fortinet | `fortinet/` | FortiGate firewalls |
| Hirschmann | `hirschmann/` | Industrial switches |
| (30+ more) | ... | See `drivers/` directory |

### Driver Structure

```cpp
// In <vendor>.h
class MyVendorDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;
   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   // ... other overrides
};

// In <vendor>.cpp - Single driver
DECLARE_NDD_ENTRY_POINT(MyVendorDriver);

// Or for multiple drivers in one module
NDD_BEGIN_DRIVER_LIST
NDD_DRIVER(MyVendorDriver1)
NDD_DRIVER(MyVendorDriver2)
NDD_END_DRIVER_LIST
DECLARE_NDD_MODULE_ENTRY_POINT
```

### Key Virtual Methods

| Method | Purpose |
|--------|---------|
| `getName()` | Return driver name (e.g., "CISCO-NEXUS") |
| `getVersion()` | Return driver version (use `NETXMS_VERSION_STRING`) |
| `isPotentialDevice(oid)` | Return priority (0-255) based on sysObjectID match |
| `isDeviceSupported(snmp, oid)` | Confirm device support via SNMP queries |
| `getHardwareInformation(...)` | Fill vendor, model, serial number |
| `getInterfaces(...)` | Get interface list with physical port locations |
| `getVlans(...)` | Get VLAN configuration |
| `getSSHDriverHints(hints)` | Provide SSH CLI interaction parameters |

Do **not** override `getHardwareInformation()` in a new driver when the device supports standard ENTITY-MIB — the generic driver already reads vendor/model/serial from it. Override only for devices that expose hardware info via a proprietary MIB or lack ENTITY-MIB support.

### SSH Driver Hints

For devices supporting interactive SSH sessions:

```cpp
void MyVendorDriver::getSSHDriverHints(SSHDriverHints *hints) const
{
   // Prompt detection regex patterns
   hints->promptPattern = "^[\\w.-]+[>#]\\s*$";        // User/exec mode
   hints->enabledPromptPattern = "^[\\w.-]+#\\s*$";   // Privileged mode

   // Privilege escalation (set to nullptr if not applicable)
   hints->enableCommand = "enable";
   hints->enablePromptPattern = "[Pp]assword:\\s*$";

   // Pagination control
   hints->paginationDisableCmd = "terminal length 0";
   hints->paginationPrompt = "--More--|Press any key";
   hints->paginationContinue = " ";

   // Session management
   hints->exitCommand = "exit";
   hints->commandTimeout = 30000;   // milliseconds
   hints->connectTimeout = 15000;   // milliseconds
}
```

### Implemented SSH Hints by Vendor

| Vendor | Driver Class | Key Differences |
|--------|--------------|-----------------|
| Cisco IOS/IOS-XE | `CiscoDeviceDriver` | `enable`, `terminal length 0` |
| Cisco NX-OS | `CiscoNexusDriver` | Similar to IOS |
| Juniper JunOS | `JuniperDriver` | `user@host>`, no enable, `set cli screen-length 0` |
| MikroTik RouterOS | `MikrotikDriver` | `[user@host] >`, no enable, no pagination |
| Huawei VRP | `HuaweiSWDriver` | `<host>`/`[host]`, `super`, `screen-length 0 temporary` |
| Hirschmann HiOS | `HirschmannHiOSDriver` | IOS-like, `terminal datadump` |
| Extreme EXOS | `ExtremeDriver` | `host.slot #`, no enable, `disable clipaging` |

### Device Detection Priority

`isPotentialDevice()` returns a priority value (0-255):
- **0** - Device not supported
- **1-99** - Low confidence (generic fallback)
- **100-199** - Medium confidence
- **200-255** - High confidence (specific device match)

Higher priority drivers are selected when multiple drivers match.

### Interface Physical Location

```cpp
iface->isPhysicalPort = true;
iface->location.chassis = 1;    // For stacked/chassis devices
iface->location.module = slot;  // Slot/module number
iface->location.port = port;    // Port number within module
```

## Database Changes

When modifying the database schema:

1. Write upgrade procedures in `tools/nxdbmgr/`
2. Update schema in `sql/` folder (root)
3. See [Data Dictionary](https://www.netxms.org/documentation/datadictionary-latest/)

## AI Integration

The AI subsystem (`core/ai_core.cpp`) handles LLM communication for the AI assistant feature.

- Some LLM models (e.g., QwQ) output internal reasoning in `<think>...</think>` tags - these are filtered by `StripThinkingTags()` before presenting to users

## Debugging

```bash
# Run with maximum debug output
/opt/netxms/bin/netxmsd -D9

# Common debug tags
nxlog_debug_tag(_T("ndd"), level, ...)        # Device drivers
nxlog_debug_tag(_T("poll"), level, ...)       # Polling
nxlog_debug_tag(_T("node"), level, ...)       # Node operations
nxlog_debug_tag(_T("db"), level, ...)         # Database
nxlog_debug_tag(_T("session"), level, ...)    # Client sessions
```

## Design Documentation

- SSH Interactive Sessions: `doc/SSH_Interactive_Sessions_Design.md`
- AI Incident Management: `doc/AI_Incident_Management_Integration_Design.md`

## Related Components

- [libnetxms](../libnetxms/CLAUDE.md) - Core library
- [libnxdb](../db/CLAUDE.md) - Database abstraction
- [libnxsnmp](../snmp/CLAUDE.md) - SNMP protocol
- [libnxsl](../libnxsl/CLAUDE.md) - NXSL scripting
- [ncdrivers](ncdrivers/CLAUDE.md) - Notification channels
