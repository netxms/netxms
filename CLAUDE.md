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
