# Building NetXMS on Windows with MinGW

This document describes how to build NetXMS on Windows using the MinGW-w64 compiler toolchain and GNU Make.

## Overview

NetXMS can now be built on Windows using a `Makefile.w32`-based build system, which provides:
- Clean separation from Unix builds (no autoconf/automake needed on Windows)
- Direct control over build options via `config.mingw`
- Faster builds compared to MSBuild
- Consistent toolchain across all platforms (GCC/MinGW)

## Prerequisites

### 1. MinGW-w64 Toolchain

You need MinGW-w64 (not the older MinGW) for 64-bit support and modern C++11 features.

**Recommended: Install via MSYS2**

1. Download MSYS2 from: https://www.msys2.org/
2. Install MSYS2 (e.g., to `C:\msys64`)
3. Open "MSYS2 MinGW 64-bit" shell
4. Install the toolchain:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-make
   pacman -S mingw-w64-x86_64-binutils
   ```

5. Add MinGW to your Windows PATH:
   ```
   C:\msys64\mingw64\bin
   ```

**Alternative: Standalone MinGW-w64**
- Download from: https://www.mingw-w64.org/downloads/
- Choose a build (e.g., x86_64-posix-seh)
- Add to PATH

### 2. Perl

Required for build tag generation (`tools/updatetag.pl`).

**Option A: Strawberry Perl** (recommended)
- Download from: https://strawberryperl.com/
- Install and add to PATH

**Option B: ActivePerl**
- Download from: https://www.activestate.com/products/perl/

### 3. Required Libraries

NetXMS requires several external libraries. You need to download and build them with MinGW, or download pre-built MinGW versions.

**Mandatory:**
- **OpenSSL** - Encryption and SSL/TLS support
- **PCRE** - Regular expression support
- **zlib** - Compression (usually included with MinGW)

**Optional (for full functionality):**
- **cURL** - HTTP/REST features
- **libssh** - SSH subagent
- **PostgreSQL client library** - PostgreSQL database driver
- **MySQL/MariaDB client** - MySQL database driver
- **libmosquitto** - MQTT subagent
- **Python development headers** - Python subagent
- **Java JDK** - Java subagent (JNI headers)

**Where to get them:**
- MSYS2 packages: `pacman -S mingw-w64-x86_64-<package>`
- Build from source with MinGW
- Some projects provide pre-built MinGW binaries

## Quick Start

### 1. Initial Setup

Run the setup script to create your configuration file:

```cmd
setup_windows_build.cmd
```

This will:
- Create `config.mingw` from template
- Check for required tools
- Open `config.mingw` for editing

### 2. Configure Build

Edit `config.mingw` to match your environment:

```makefile
# Set architecture (x86, x64, or arm64)
ARCH = x64

# Set SDK paths
OPENSSL_ROOT = C:/SDK/OpenSSL/x64
PCRE_ROOT = C:/SDK/PCRE/x64
PGSQL_ROOT = C:/SDK/PostgreSQL/x64
# ... etc

# Enable/disable features
WITH_PGSQL = 1
WITH_MYSQL = 1
WITH_PYTHON = 1
# ... etc
```

**Important:** Use forward slashes (/) in paths, even on Windows.

### 3. Build

Build everything:

```cmd
build_windows.cmd
```

Or use make directly:

```cmd
make -f Makefile.w32 -j8
```

### 4. Install

Install to the prefix defined in `config.mingw` (default: `C:\NetXMS`):

```cmd
make -f Makefile.w32 install
```

## Build Options

### Debug vs Release

Edit `config.mingw`:

```makefile
# Debug build (symbols, no optimization)
DEBUG = 1

# Release build (optimized, no symbols)
DEBUG = 0
```

### Components

Choose which components to build:

```makefile
BUILD_AGENT = 1      # Build agent (nxagentd) and subagents
BUILD_CLIENT = 1     # Build client libraries and tools
BUILD_SERVER = 0     # Server components (rarely needed on Windows)
```

### Database Drivers

Enable/disable database drivers:

```makefile
WITH_SQLITE = 1      # SQLite (bundled)
WITH_PGSQL = 1       # PostgreSQL
WITH_MYSQL = 1       # MySQL/MariaDB
WITH_MSSQL = 1       # Microsoft SQL Server (ODBC)
WITH_ORACLE = 0      # Oracle
```

### Agent Subagents

Enable/disable optional subagents:

```makefile
WITH_PYTHON = 1      # Python subagent
WITH_JAVA = 1        # Java subagent
WITH_SSH = 1         # SSH subagent
WITH_MOSQUITTO = 1   # MQTT subagent
```

## Directory Structure

After building, binaries are organized by architecture and build type:

```
out/
├── x64/
│   ├── Debug/
│   │   ├── bin/        # Executables and DLLs
│   │   ├── lib/        # Import libraries (.dll.a)
│   │   └── obj/        # Object files
│   └── Release/
│       ├── bin/
│       ├── lib/
│       └── obj/
```

## Makefile Targets

```cmd
# Build everything
make -f Makefile.w32

# Clean build artifacts
make -f Makefile.w32 clean

# Install to PREFIX
make -f Makefile.w32 install

# Show version info
make -f Makefile.w32 version

# Show help
make -f Makefile.w32 help
```

## Troubleshooting

### "gcc not found"

Ensure MinGW's `bin` directory is in your PATH:

```cmd
set PATH=C:\msys64\mingw64\bin;%PATH%
```

Verify:
```cmd
gcc --version
```

### "config.mingw not found"

You need to create `config.mingw` from the template:

```cmd
copy config.mingw.template config.mingw
edit config.mingw
```

### Library not found during linking

Update the SDK path in `config.mingw`:

```makefile
OPENSSL_ROOT = C:/SDK/OpenSSL/x64
```

Ensure the library files exist:
- `C:/SDK/OpenSSL/x64/lib/libssl.a` (or `.dll.a`)
- `C:/SDK/OpenSSL/x64/lib/libcrypto.a`

### Resource compiler errors

Ensure `windres` is in your PATH (included with MinGW binutils).

### "undefined reference to..."

Missing library dependency. Check that:
1. The library is built with MinGW (not MSVC)
2. The library path is in `config.mingw`
3. The library is listed in the component's `Makefile.w32`

### Mixing MinGW and MSVC libraries

**Don't do this!** MinGW and MSVC use different C++ ABIs and cannot be mixed. All libraries must be built with the same toolchain.

Some vendor libraries (Oracle, Informix) are MSVC-only. These may require:
- Creating MinGW import libraries with `dlltool`
- Using C interfaces only (no C++)
- Keeping the MSVC build for those specific components

## Advanced Usage

### Cross-Compilation

To build for a different architecture from the template:

```makefile
ARCH = x86
CROSS_PREFIX = i686-w64-mingw32-
CC = $(CROSS_PREFIX)gcc
CXX = $(CROSS_PREFIX)g++
# ... etc
```

### Parallel Builds

Use `-j` flag for parallel compilation:

```cmd
make -f Makefile.w32 -j8
```

Or set in `config.mingw`:
```makefile
MAKEFLAGS = -j8
```

### Verbose Output

See full compiler commands:

```makefile
VERBOSE = 1
```

Or:
```cmd
make -f Makefile.w32 VERBOSE=1
```

### Building Individual Components

Build just one component:

```cmd
cd src/libnetxms
make -f Makefile.w32 TOPDIR=../..
```

## Differences from MSBuild

| Feature | MSBuild | Makefile.w32 |
|---------|---------|--------------|
| **IDE** | Visual Studio | Any text editor + command line |
| **Configuration** | .vcxproj files | config.mingw |
| **Compiler** | MSVC (cl.exe) | GCC (g++.exe) |
| **Build speed** | Slower | Faster (parallel make) |
| **Debugging** | Visual Studio debugger | GDB |
| **Dependencies** | NuGet, manual SDKs | Manual SDKs |

## Migration from MSBuild

If you're transitioning from the old MSBuild system:

1. Both build systems can coexist (`.vcxproj` files remain)
2. MinGW binaries are **not** ABI-compatible with MSVC binaries
3. You must use one toolchain consistently (don't mix)
4. Most code is platform-agnostic and compiles with both

## Getting Help

- **Design Document**: See `doc/Windows_MinGW_Build_System_Design.md`
- **Component Guide**: Each component has a `Makefile.w32` alongside `Makefile.am`
- **GitHub Issues**: https://github.com/netxms/netxms/issues

## Current Status

**Phase 1: Infrastructure Complete** ✓
- Configuration system (`config.mingw`)
- Top-level `Makefile.w32`
- Common build rules and templates
- Build scripts

**Phase 2: Core Libraries** (In Progress)
- [ ] `src/jansson`
- [ ] `src/libnetxms`
- [ ] `src/libnxsl`
- [ ] Additional libraries...

**Phase 3+**: Agent, client, database drivers, subagents (Planned)

Check `doc/Windows_MinGW_Build_System_Design.md` for the complete implementation plan.

## License

NetXMS is licensed under GNU General Public License version 2.
See the LICENSE file for details.
