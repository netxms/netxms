# Building NetXMS on Windows with MinGW

This document describes how to build NetXMS on Windows using the llvm-mingw compiler toolchain and GNU Make.

## Overview

The Windows build uses a `Makefile.w32`-based build system driven by `mingw32-make`. This is now the **only** supported way to build NetXMS on Windows — the old MSVC/Visual Studio project files (`.vcxproj`, `.sln`) have been removed. The MinGW build provides:

- Clean separation from Unix builds (no autoconf/automake needed on Windows)
- Direct control over build options via `config.mingw`
- A consistent GCC/Clang-compatible toolchain across all platforms
- Multi-architecture output (x64, x86, arm64) from a single tree

Binaries produced by this build are **not** ABI-compatible with the old MSVC output; all libraries in a build must use the same toolchain.

## Prerequisites

### 1. llvm-mingw Toolchain

The build targets the **llvm-mingw** toolchain (Clang + LLD + libc++), which ships separate per-architecture cross toolchains:

| Architecture | Target triple |
|--------------|---------------|
| x64 | `x86_64-w64-mingw32` |
| x86 | `i686-w64-mingw32` |
| arm64 | `aarch64-w64-mingw32` |

1. Download a release from: https://github.com/mstorsjo/llvm-mingw/releases
2. Extract it (e.g. to `C:\llvm-mingw`)
3. Add its `bin` directory to your Windows PATH

You also need GNU Make as `mingw32-make` (bundled with most MinGW distributions, or install `mingw-w64-x86_64-make` via MSYS2). All build commands in this document use `mingw32-make`.

### 2. Perl

Required for build tag generation (`tools/updatetag.pl`).

- **Strawberry Perl** (recommended): https://strawberryperl.com/
- **ActivePerl**: https://www.activestate.com/products/perl/

### 3. Required Libraries

External libraries must be built for (or downloaded as) llvm-mingw import libraries. Point `config.mingw` at their install locations (see below).

**Mandatory:**
- **zlib** – compression
- **OpenSSL** – encryption and SSL/TLS
- **PCRE** – regular expressions
- **libexpat** – XML parsing

**Required for the server build (`BUILD_SERVER=1`):**
- **Protocol Buffers (protobuf)** – used by OTLP and the Prometheus subagent
- **libmicrohttpd** – embedded HTTP server

**Optional (per feature):**
- **cURL** – HTTP/REST features
- **libssh** – SSH subagent
- **libmosquitto** – MQTT subagent
- **Java JDK** – Java subagent (JNI headers)
- **PostgreSQL / MySQL / MariaDB / Oracle / Informix / DB2 client libraries** – corresponding database drivers

The template expects SDKs under a common root (`C:/SDK-llvm-mingw/<name>`), with per-architecture libraries in `lib/<arch>`. Adjust the paths in `config.mingw` to match your layout.

## Quick Start

### 1. Initial Setup

Run the setup script to create your configuration file:

```cmd
setup_windows_build.cmd
```

This copies `config.mingw.template` to `config.mingw` and prompts you to edit it.

### 2. Configure Build

Edit `config.mingw` to match your environment:

```makefile
# Target architecture (x64, x86, or arm64)
ARCH ?= x64

# Components to build
BUILD_AGENT = 1
BUILD_CLIENT = 1
BUILD_SERVER = 0

# SDK root and per-SDK paths
SDK_ROOT = C:/SDK-llvm-mingw
OPENSSL_ROOT = $(SDK_ROOT)/openssl
PCRE_ROOT = $(SDK_ROOT)/pcre
# ... etc
```

**Important:** Use forward slashes (`/`) in paths, even on Windows.

### 3. Build

Build everything with the convenience wrapper:

```cmd
build_windows.cmd
```

Or invoke `mingw32-make` directly:

```cmd
mingw32-make -f Makefile.w32 -j8
```

### 4. Install

Install to the prefix defined in `config.mingw` (default: `C:\NetXMS`):

```cmd
mingw32-make -f Makefile.w32 install
```

## Build Options

### Debug vs Release

Edit `config.mingw`:

```makefile
DEBUG = 1    # Debug build (symbols, no optimization)
DEBUG = 0    # Release build (optimized, symbols stripped)
```

### Components

```makefile
BUILD_AGENT = 1      # Agent (nxagentd) and subagents
BUILD_CLIENT = 1     # Client libraries and tools
BUILD_SERVER = 1     # Server (netxmsd, core, modules, drivers)
```

> **Note:** The full server and client tiers are supported for **x64 only**. For `x86` and `arm64` the build forces `BUILD_CLIENT=0`/`BUILD_SERVER=0` and builds the **agent (plus its tools and database drivers) only**, regardless of these settings.

### Database Drivers

```makefile
WITH_SQLITE = 1      # SQLite (bundled)
WITH_PGSQL = 1       # PostgreSQL
WITH_MYSQL = 1       # MySQL/MariaDB
WITH_MSSQL = 1       # Microsoft SQL Server (ODBC)
WITH_ODBC = 1        # ODBC (Windows built-in)
WITH_ORACLE = 0      # Oracle
WITH_INFORMIX = 0    # Informix
WITH_DB2 = 0         # IBM DB2
```

### Agent Subagents

```makefile
WITH_JAVA = 1        # Java subagent
WITH_SSH = 1         # SSH subagent
WITH_MOSQUITTO = 1   # MQTT subagent
```

## Multi-Architecture Builds

Build for a single architecture by setting `ARCH` on the command line (no need to edit `config.mingw`):

```cmd
mingw32-make -f Makefile.w32 ARCH=arm64
```

Build all architectures in one go:

```cmd
mingw32-make -f Makefile.w32 all-arch
```

The set defaults to `x64 x86 arm64` and can be overridden:

```cmd
mingw32-make -f Makefile.w32 all-arch ALL_ARCHS="x64 arm64"
```

## Directory Structure

Binaries are organized by architecture and build type:

```
out/
├── x64/
│   ├── Debug/
│   │   ├── bin/        # Executables and DLLs
│   │   ├── lib/        # Import libraries (.dll.a)
│   │   └── obj/        # Object files (per-component subtree)
│   └── Release/
│       ├── bin/
│       ├── lib/
│       └── obj/
├── x86/
│   └── ...
└── arm64/
    └── ...
```

Object files are written under `out/<arch>/<type>/obj`, keeping each architecture's build fully independent.

## Makefile Targets

```cmd
mingw32-make -f Makefile.w32              # Build everything for ARCH
mingw32-make -f Makefile.w32 all-arch     # Build all architectures
mingw32-make -f Makefile.w32 clean        # Remove build artifacts
mingw32-make -f Makefile.w32 install      # Install to PREFIX
mingw32-make -f Makefile.w32 test         # Build and run the unit test suite
mingw32-make -f Makefile.w32 version      # Show version info
mingw32-make -f Makefile.w32 help         # Show help
```

## Unit Tests

Unit tests are the MinGW equivalent of the autotools `--with-tests` switch: they
are excluded from a normal build and enabled with

```makefile
BUILD_TESTS = 1
```

in `config.mingw`. Tests are built for **x64 only** — as with the client and
server tiers, `BUILD_TESTS` is forced off for `x86` and `arm64`.

```cmd
mingw32-make -f Makefile.w32 test
```

builds any missing test binaries and runs `tests\suite\netxms-test-suite.cmd`,
which executes each test in turn and stops at the first failure. Test
executables land in `out\<arch>\<type>\bin` next to the in-tree DLLs; the
`test` target passes the SDK directories holding the third-party DLLs (OpenSSL,
PCRE, zlib, expat, cURL, libmodbus) to the runner through
`NETXMS_TEST_DLL_PATH`, built from the SDK roots in `config.mingw`.

To run the suite without make, put those SDK `bin\<arch>` directories on `PATH`
(or set `NETXMS_TEST_DLL_PATH` yourself) and call the runner from the top of the
source tree:

```cmd
tests\suite\netxms-test-suite.cmd Release x64
```

Individual binaries can also be run directly, e.g.
`out\x64\Release\bin\test-libnetxms.exe`. `test-libnxsl` needs the directory
holding its `*.nxsl` scripts as an argument (`.\tests\test-libnxsl`).

`test-libnxdb` is built but not part of the suite — it connects to live
MySQL/Oracle instances and must be started manually.

## Troubleshooting

### "gcc not found" / toolchain not on PATH

Ensure the llvm-mingw `bin` directory is on your PATH:

```cmd
set PATH=C:\llvm-mingw\bin;%PATH%
x86_64-w64-mingw32-gcc --version
```

### "config.mingw not found"

Create it from the template:

```cmd
copy config.mingw.template config.mingw
```

### Library not found during linking

Update the SDK path in `config.mingw` and verify the per-architecture library files exist, e.g.:

- `C:/SDK-llvm-mingw/openssl/lib/x64/libssl.a` (or `.dll.a`)
- `C:/SDK-llvm-mingw/openssl/lib/x64/libcrypto.a`

Some vendor SDKs (PostgreSQL, MySQL, Oracle, DB2, ...) ship MSVC-style `*.lib` import libraries. Link those by exact file name, e.g. `-l:libpq.lib` (the `config.mingw` entries for the database drivers already do this).

### Resource compiler errors

Ensure `windres` (from the llvm-mingw toolchain) is on your PATH.

### "undefined reference to..."

A missing library dependency. Check that:
1. The library was built with llvm-mingw (not MSVC)
2. Its path is set in `config.mingw`
3. It is listed in the component's `Makefile.w32`

### Mixing MinGW and MSVC libraries

**Don't.** MinGW/Clang and MSVC use different C++ ABIs and cannot be mixed. All libraries in a build must use the same toolchain. The Tuxedo subagent is excluded from the Windows build because its SDK is MSVC-only and its headers conflict with the MinGW CRT.

## Relation to the Former MSVC Build

NetXMS was previously built on Windows with Visual Studio / MSBuild. Those project files (`.vcxproj`, `.vcxproj.filters`, `netxms.sln`) and the associated MSBuild property sheets and `msvc_setenv_*.cmd` scripts have been **removed** — the MinGW build described here is the sole Windows build system. On Unix, the autoconf/automake build (`Makefile.am`) is unaffected.

## Component Layout

Each buildable component has a `Makefile.w32` alongside its `Makefile.am`. Shared rules live in `build/` and in per-tier `*-common.mk` files:

| Tier | Common rules file | Module type |
|------|-------------------|-------------|
| Agent subagents | `src/agent/subagents/subagent-common.mk` | `.nsm` |
| Agent tools / console tools | `build/tool-common.mk` | `.exe` |
| Database drivers | `src/db/dbdrv/driver-common.mk` | `.ddr` |
| Network device drivers | `src/server/drivers/ndd-common.mk` | `.ndd` |
| Notification channel drivers | `src/server/ncdrivers/ncdrv-common.mk` | `.ncd` |
| Server modules | `src/server/module-common.mk` | `.nxm` |
| PDS drivers | `src/server/pdsdrv/pdsdrv-common.mk` | `.pdsd` |

## Getting Help

- **Design Document**: `doc/Windows_MinGW_Build_System_Design.md`
- **GitHub Issues**: https://github.com/netxms/netxms/issues

## License

NetXMS is licensed under GNU General Public License version 2. See the LICENSE file for details.
</content>
