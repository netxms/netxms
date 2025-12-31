# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NetXMS is an enterprise-grade, open-source network and infrastructure monitoring solution. It's a distributed client-server system written primarily in C++ (server, agent, libraries) and Java (management console, client libraries).

## Build Commands

### C++ Components (Server, Agent, Libraries)

```bash
# First-time setup after clone
./tools/updatetag.pl    # Generate version.h (run once)
./reconf                # Run autoconf/automake (run once)

# Configure with desired options
./configure --prefix=/opt/netxms --with-sqlite --with-server --with-agent --with-tests --enable-debug
# Database options: --with-pgsql --with-mysql --with-oracle --with-mssql

# Build and install
make -j$(nproc)
make install

# Run tests
./tests/suite/netxms-test-suite

# Run server/agent in debug mode (not as daemon)
/opt/netxms/bin/netxmsd -D6
/opt/netxms/bin/nxagentd -D6
```

### Java Components (Management Console)

```bash
# Build base and client libraries (required first)
mvn -f src/java-common/netxms-base/pom.xml install -DskipTests -Dmaven.javadoc.skip=true
mvn -f src/client/java/netxms-client/pom.xml install -DskipTests -Dmaven.javadoc.skip=true

# Build desktop client (standalone)
mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone

# Build web client
mvn -f src/client/nxmc/java/pom.xml clean package -Pweb

# Run standalone client
java -jar src/client/nxmc/java/target/netxms-nxmc-standalone-*.jar

# Run web client in dev mode
mvn -f src/client/nxmc/java/pom.xml jetty:run -Pweb -Dnetxms.build.disablePlatformProfile=true
```

### Eclipse Setup for Java Development

```bash
cd src/java-common/netxms-base && mvn install eclipse:eclipse && cd -
cd src/client/java/netxms-client && mvn install eclipse:eclipse && cd -
cd src/client/nxmc/java && mvn eclipse:eclipse -Pdesktop && cd -
# Import all three projects into Eclipse, run org.netxms.nxmc.Startup
```

## Architecture

### Core Components

- **netxmsd** (`src/server/`) - Central management server, handles all monitoring, event processing, data storage
- **nxagentd** (`src/agent/`) - Monitoring agent deployed on target systems, with 47+ platform-specific subagents
- **nxmc** (`src/client/nxmc/java/`) - Management console (desktop SWT/web RWT)
- **webapi** (`src/server/webapi/`) - REST API

### Key Libraries

- **libnetxms** (`src/libnetxms/`) - Core utility library: String, containers, threading, crypto, network utilities
- **libnxagent** (`src/agent/libnxagent/`) - Agent-related tools (but also used by server for metric parsing)
- **libnxsl** (`src/libnxsl/`) - NXSL scripting language interpreter
- **libnxdb** (`src/libnxdb/`) - Database abstraction (PostgreSQL, MySQL, Oracle, MSSQL, SQLite, etc.)
- **libnxsnmp** (`src/libnxsnmp/`) - SNMP protocol support
- **libnxcc** (`src/libnxcc/`) - Cluster communication library

### Network Device Support

- 35+ specialized device drivers in `src/server/drivers/`
- See [Network Device Drivers](#network-device-drivers) section below for implementation details

### Notification Channels

- Notification channel drivers in `src/ncdrivers/`

## C++ Development Guidelines

### Language Version

- **Maximum C++11** - Must compile with GCC 4.8+, Clang 3.3+, Visual Studio 2015+
- Use `nullptr` instead of `NULL`
- Smart pointers (`shared_ptr`, `unique_ptr`) are available
- Lambda expressions and move semantics are used in newer code

### String Handling

- Use `String` class for dynamic strings, `TCHAR` for platform-independent chars
- Use `_T("text")` macro for string literals (becomes `L"text"` in Unicode builds)
- Server-side code is always Unicode; agent code uses TCHAR abstraction
- Key classes: `String`, `StringList`, `StringMap`, `StringBuffer`

### Memory Management

- Use `MemAlloc`, `MemFree`, `MemAllocStruct<T>()`, `MemAllocArray<T>()` instead of malloc/free
- Use smart pointers for complex ownership
- Container classes (ObjectArray, HashMap) manage object ownership via `Ownership::True/False`

### Threading

- Use thread pools: `ThreadPoolExecute()`, `ThreadPoolScheduleRelative()`
- Use `LockGuard` for RAII mutex locking
- Classes: `Mutex`, `Condition`, `RWLock`
- Create threads with `ThreadCreateEx()`

### Container Classes

- `ObjectArray<T>` - Dynamic array of object pointers
- `IntegerArray<T>` - Array of integers
- `HashMap<T>` - Hash map with string keys
- `StringMap` - String-to-string map
- `ObjectQueue<T>` - Thread-safe queue

### Logging

- Use `nxlog_debug_tag(tag, level, format, ...)` for debug output
- Add new debug tags to `doc/internal/debug_tags.txt`
- Log levels: `NXLOG_DEBUG`, `NXLOG_INFO`, `NXLOG_WARNING`, `NXLOG_ERROR`

### Code Style

- 3-space indentation
- Opening brace on new line
- Use PascalCase for functions and classes (`ValidateUser`)
- Use camelCase for class methods (`MyClass::newInstance`)
- Use camelCase with m_ prefix for class members (`int m_objectId`)
- Use camelCase for variables; add g_ prefix for global variables and s_ prefix for static variables
- Modern code avoids Hungarian notation; older code may use it (`dwResult`, `szName`)
- Comments describe current behavior, not change history

## Java Development Guidelines

- 3-space indentation
- Opening brace on new line (except for anonymous class opening bracket)
- Follow standard Java conventions
- Minimize external dependencies
- Desktop client uses SWT, web client uses RWT

## Database Changes

- Write upgrade procedures in `src/server/tools/nxdbmgr/`
- Update schema in `sql/` folder
- See [Data Dictionary](https://www.netxms.org/documentation/datadictionary-latest/)

## Key Documentation

- [Administration Guide](https://www.netxms.org/documentation/adminguide/)
- [NXSL Scripting](https://www.netxms.org/documentation/nxsl-latest/)
- [C++ Developer Guide](doc/CPP_DEVELOPER_GUIDE.md)
- [Java API](https://www.netxms.org/documentation/javadoc/latest/)

## Contribution Workflow

- Issue-first approach: create GitHub issue before coding
- All changes require discussion and approval before implementation
- Pull requests must reference related issues

## Network Device Drivers

Network device drivers provide vendor-specific support for switches, routers, and other network equipment. They are loaded as shared libraries by the server.

### Driver Location

- All drivers are in `src/server/drivers/<vendor>/`
- Each driver has: `<vendor>.h`, `<vendor>.cpp`, and optionally separate files for device variants
- Driver interface defined in `src/server/include/nddrv.h`
- Base class implementation in `src/server/libnxsrv/ndd.cpp`

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

### SSH Driver Hints

For devices supporting interactive SSH sessions, implement `getSSHDriverHints()`:

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

### Implemented SSH Hints

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

Set `InterfaceInfo` fields for physical port mapping:

```cpp
iface->isPhysicalPort = true;
iface->location.chassis = 1;    // For stacked/chassis devices
iface->location.module = slot;  // Slot/module number
iface->location.port = port;    // Port number within module
```

### Debug Logging

Use a driver-specific debug tag:

```cpp
#define DEBUG_TAG _T("ndd.myvendor")
nxlog_debug_tag(DEBUG_TAG, 5, _T("MyVendorDriver: processing %s"), nodeName);
```

### Design Documentation

- SSH Interactive Sessions: `doc/SSH_Interactive_Sessions_Design.md`
