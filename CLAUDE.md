# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NetXMS is an enterprise-grade, open-source network and infrastructure monitoring solution. It's a distributed client-server system written primarily in C++ (server, agent, libraries) and Java (management console, client libraries).

## Scope

Do not go beyond the scope of the request. If you notice a bug or improvement opportunity outside the current task, mention it in a comment but do NOT start fixing it unless explicitly asked.

## Project Architecture

This is a large multi-language project (Java, C/C++, JavaScript, Python). The primary codebase is Java and C++. When implementing full-stack features, always check for impacts across: server-side C++, Java client library, SWT UI, RWT (web) UI, NXSL scripting bindings, WebAPI layer, and database schema.

## Design Principles

Do not introduce unnecessary abstraction layers or wrapper functions. Call existing methods directly unless there is a clear architectural reason for indirection. When in doubt, prefer the simpler approach.

## Component Documentation

This monorepo has component-specific CLAUDE.md files:

| Component | Location | Description |
|-----------|----------|-------------|
| Server | [src/server/CLAUDE.md](src/server/CLAUDE.md) | Central management server (netxmsd), device drivers, WebAPI |
| Agent | [src/agent/CLAUDE.md](src/agent/CLAUDE.md) | Monitoring agent (nxagentd), 47+ subagents |
| Client | [src/client/CLAUDE.md](src/client/CLAUDE.md) | Management console (nxmc), client libraries |
| Core Library | [src/libnetxms/CLAUDE.md](src/libnetxms/CLAUDE.md) | Core C++ utilities, threading, containers |
| NXSL | [src/libnxsl/CLAUDE.md](src/libnxsl/CLAUDE.md) | NetXMS Scripting Language interpreter |
| Database | [src/db/CLAUDE.md](src/db/CLAUDE.md) | Database abstraction layer (libnxdb) |
| SNMP | [src/snmp/CLAUDE.md](src/snmp/CLAUDE.md) | SNMP protocol library and tools |
| Notification Channels | [src/ncdrivers/CLAUDE.md](src/ncdrivers/CLAUDE.md) | Notification channel drivers |

## Quick Start

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
mvn -f src/java-common/netxms-base/pom.xml install
mvn -f src/client/java/netxms-client/pom.xml install

# Build desktop client (standalone)
mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone

# Build web client
mvn -f src/client/nxmc/java/pom.xml clean package -Pweb

# Run standalone client
java -jar src/client/nxmc/java/target/netxms-nxmc-standalone-*.jar

# Run web client in dev mode
mvn -f src/client/nxmc/java/pom.xml jetty:run -Pweb -Dnetxms.build.disablePlatformProfile=true
```

## Architecture

```
                    ┌─────────────────────────────────────────────────────────┐
                    │                    Management Console                   │
                    │              (nxmc - Java SWT/RWT)                      │
                    │              src/client/nxmc/java/                      │
                    └───────────────────────┬─────────────────────────────────┘
                                            │
                    ┌───────────────────────┴─────────────────────────────────┐
                    │                     Server (netxmsd)                    │
                    │                     src/server/                         │
                    │  ┌─────────────┐ ┌─────────────┐ ┌────────────────────┐ │
                    │  │   Drivers   │ │   WebAPI    │ │   Event/Alarm      │ │
                    │  │  (35+ NDD)  │ │   (REST)    │ │   Processing       │ │
                    │  └─────────────┘ └─────────────┘ └────────────────────┘ │
                    └───────────────────────┬─────────────────────────────────┘
                                            │
         ┌──────────────────────────────────┼──────────────────────────────────┐
         │                                  │                                  │
         ▼                                  ▼                                  ▼
┌─────────────────┐              ┌─────────────────────┐              ┌─────────────────┐
│ Agent (nxagentd)│              │   Network Devices   │              │   Database      │
│ src/agent/      │              │   (SNMP/SSH/etc)    │              │ (PostgreSQL/    │
│ 47+ subagents   │              │                     │              │  MySQL/etc)     │
└─────────────────┘              └─────────────────────┘              └─────────────────┘
```

### Core Components

- **netxmsd** (`src/server/`) - Central management server
- **nxagentd** (`src/agent/`) - Monitoring agent
- **nxmc** (`src/client/nxmc/java/`) - Management console (desktop SWT/web RWT)
- **webapi** (`src/server/webapi/`) - REST API

### Key Libraries

- **libnetxms** (`src/libnetxms/`) - Core utility library
- **libnxagent** (`src/agent/libnxagent/`) - Agent-related tools
- **libnxsl** (`src/libnxsl/`) - NXSL scripting language interpreter
- **libnxdb** (`src/db/libnxdb/`) - Database abstraction
- **libnxsnmp** (`src/snmp/libnxsnmp/`) - SNMP protocol support
- **libnxcc** (`src/libnxcc/`) - Cluster communication library

## C++ Development Guidelines

> **Note**: These guidelines apply to all C++ components. See component-specific CLAUDE.md for additional details.

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

## Key Documentation

- [Administration Guide](https://www.netxms.org/documentation/adminguide/)
- [NXSL Scripting](https://www.netxms.org/documentation/nxsl-latest/)
- [C++ Developer Guide](doc/CPP_DEVELOPER_GUIDE.md)
- [Java API](https://www.netxms.org/documentation/javadoc/latest/)
- [Data Dictionary](https://www.netxms.org/documentation/datadictionary-latest/)

## Code Changes

When implementing changes, ensure ALL call sites and consumers are updated. After making a rename or API change, run a project-wide grep for the old name to catch missed references before attempting a build.

## Implementation Discipline

When implementing a plan that spans multiple files, do NOT skip any files listed in the plan. If a file seems less critical, flag it explicitly rather than silently omitting it. Cross-check your completed changes against the plan before declaring done.

## Translations

When translating PO files or doing text transformations, preserve grammatical structure of the target language. Do not do word-by-word substitution. Process in contextual batches and verify grammar, especially noun/adjective ordering and gendered forms.

## Contribution Workflow

- Issue-first approach: create GitHub issue before coding
- All changes require discussion and approval before implementation
- Pull requests must reference related issues
