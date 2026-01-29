# Windows Build System Design: Makefile.w32 Approach

**Document Version**: 1.0
**Date**: 2026-01-20
**Author**: NetXMS Development Team
**Status**: Design Phase

---

## Executive Summary

This document describes the design for transitioning NetXMS Windows builds from MSBuild/Visual C++ to GNU Make with MinGW-w64 compiler. The approach uses dedicated `Makefile.w32` files alongside existing `Makefile.am` files, providing clean separation between Unix and Windows build systems.

### Current State
- 209 `.vcxproj` files (Visual Studio projects)
- 6 `.sln` solution files
- VC++ toolchain (v141, Visual Studio 2017+)
- Platform targets: Win32 (x86), x64, ARM64
- Build configurations: Debug, Release, "Release - Client Only"

### Target State
- Dedicated `Makefile.w32` files for Windows builds
- MinGW-w64 compiler toolchain (GCC-based, C++11 compatible)
- Independent from Unix autoconf/automake system
- Maintain all platform targets (x86, x64, ARM64)

---

## Benefits of Makefile.w32 Approach

### Advantages Over configure.ac Integration

1. **Simplicity**
   - No autoconf/automake complexity on Windows
   - Only requires MinGW + GNU Make
   - Direct makefile editing vs. configure script regeneration

2. **Clean Separation**
   - Windows builds independent from Unix builds
   - Changes to one platform don't affect the other
   - Platform-specific code isolated

3. **Performance**
   - Faster builds (no configure overhead)
   - Direct compilation without libtool wrapper overhead
   - Faster iteration during development

4. **Ease of Use**
   - Single configuration file (`config.mingw`)
   - Explicit compiler flags (no hidden autoconf magic)
   - Easier debugging (direct makefiles vs. generated ones)

5. **Existing Pattern**
   - Already used in `sql/Makefile.w32`
   - Proven approach in the codebase

### Comparison Table

| Aspect | Makefile.w32 | configure.ac Integration |
|--------|--------------|-------------------------|
| **Complexity** | Low | High |
| **Setup** | Edit config.mingw | Run configure with 20+ options |
| **Build time** | Fast (direct make) | Slower (configure + make) |
| **Maintenance** | Simple, isolated | Changes affect both platforms |
| **Learning curve** | Basic make knowledge | Autoconf/automake expertise required |
| **Dependencies** | MinGW + make | MinGW + autotools (m4, autoconf, automake, libtool) |
| **Flexibility** | Full control | Limited by autoconf macros |
| **Platform separation** | Clean separation | Shared configuration |
| **Error diagnosis** | Easy (direct makefile) | Hard (generated makefiles) |

---

## Architecture Overview

### Build Structure

```
NetXMS/
├── Makefile.am           # Unix/Linux builds (existing, unchanged)
├── Makefile.w32          # Windows top-level (NEW)
├── config.mingw          # Windows build configuration (NEW)
├── config.mingw.template # Template for user customization (NEW)
├── configure.ac          # Unix only (existing, unchanged)
├── build/
│   ├── rules.mingw       # Common build rules (NEW)
│   └── targets.mingw     # Standard target templates (NEW)
├── src/
│   ├── libnetxms/
│   │   ├── Makefile.am   # Unix
│   │   ├── Makefile.w32  # Windows (NEW)
│   │   └── libnetxms.vcxproj  # DEPRECATED
│   ├── agent/
│   │   ├── core/
│   │   │   ├── Makefile.am
│   │   │   └── Makefile.w32  # Windows (NEW)
│   │   └── subagents/
│   │       ├── winnt/
│   │       │   └── Makefile.w32  # Windows-only component
│   │       ├── ping/
│   │       │   ├── Makefile.am   # Unix
│   │       │   └── Makefile.w32  # Windows
│   └── ...
└── netxms.sln            # DEPRECATED
```

### Component Count
- **209 Makefile.w32 files** to create (one per component)
- **903 C++ source files** to compile
- **~2.5GB** source code
- **17 Windows-specific agent subagents**

---

## Phase 1: Infrastructure Preparation

### 1.1 Global Configuration File

**File: `config.mingw.template`**

Template file that users copy and customize for their environment.

**Key sections:**
```makefile
# Build options
DEBUG = 0                    # Set to 1 for debug build
UNICODE = 1                  # Unicode build (always 1)
BUILD_AGENT = 1              # Build agent components
BUILD_CLIENT = 1             # Build client libraries
BUILD_SERVER = 0             # Server (usually not on Windows)

# MinGW toolchain
CC = gcc
CXX = g++
AR = ar
RANLIB = ranlib
WINDRES = windres
STRIP = strip

# Architecture (x86, x64, arm64)
ARCH = x64

# Installation prefix
PREFIX = C:/NetXMS

# SDK paths - user edits these
OPENSSL_ROOT = C:/SDK/OpenSSL/$(ARCH)
PCRE_ROOT = C:/SDK/PCRE/$(ARCH)
CURL_ROOT = C:/SDK/cURL/$(ARCH)
LIBSSH_ROOT = C:/SDK/libssh/$(ARCH)
PGSQL_ROOT = C:/SDK/PostgreSQL/$(ARCH)
MYSQL_ROOT = C:/SDK/mysql/$(ARCH)
MOSQUITTO_ROOT = C:/SDK/libmosquitto/$(ARCH)
PYTHON_ROOT = C:/Program Files/Python37

# Optional features (set to 1 to enable)
WITH_PGSQL = 1
WITH_MYSQL = 1
WITH_MSSQL = 1
WITH_ORACLE = 0
WITH_SQLITE = 1
WITH_PYTHON = 1
WITH_JAVA = 1
WITH_SSH = 1
WITH_MOSQUITTO = 1

# Common compiler flags
COMMON_CFLAGS = -D_WIN32 -DUNICODE -D_UNICODE \
                -Wall -Wno-unknown-pragmas \
                -mthreads -fno-exceptions

# Windows system libraries
WIN_LIBS = -lws2_32 -liphlpapi -lnetapi32 -luserenv \
           -lpsapi -lpdh -lwtsapi32 -lsetupapi \
           -ladvapi32 -lshell32 -lole32 -loleaut32 \
           -lsensapi -lversion
```

### 1.2 Top-Level Makefile

**File: `Makefile.w32`**

Orchestrates the entire build process.

**Key features:**
- Includes `config.mingw` for configuration
- Defines build order (respects dependencies)
- Provides standard targets: `all`, `clean`, `install`, `help`
- Recursively builds subdirectories

**Build order:**
```makefile
SUBDIRS = \
    src/jansson \
    src/libpng \
    src/libnetxms \
    src/libnxsl \
    src/libnxdb \
    src/libnxlp

ifeq ($(BUILD_AGENT),1)
SUBDIRS += \
    src/agent/libnxagent \
    src/agent/core \
    src/agent/subagents \
    src/agent/tools
endif

ifeq ($(BUILD_CLIENT),1)
SUBDIRS += \
    src/client/libnxclient \
    src/client/nxalarm \
    src/client/nxevent \
    src/client/nxpush
endif

SUBDIRS += src/db/dbdrv
SUBDIRS += src/ncdrivers
```

### 1.3 Common Build Rules

**File: `build/rules.mingw`**

Shared pattern rules to avoid duplication across component makefiles.

**Contents:**
```makefile
# Standard suffix rules
.SUFFIXES: .cpp .c .o .rc

.cpp.o:
    $(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
    $(CC) $(CFLAGS) -c $< -o $@

.rc.o:
    $(WINDRES) $(RCFLAGS) -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@

# Standard targets
.PHONY: all clean install depend

# Dependency generation
depend:
    @echo "Generating dependencies..."
    @$(CXX) -MM $(CXXFLAGS) $(SOURCES) > .depend

-include .depend
```

### 1.4 Standard Target Templates

**File: `build/targets.mingw`**

Makefile macros for common target types (DLL, EXE, static library).

**Example:**
```makefile
# DLL target
define BUILD_DLL
$(1): $(2) $(3)
    $(CXX) -o $$@ $$^ $(LDFLAGS) -shared -Wl,--out-implib,$(4) $(LIBS)
    @echo "Built DLL: $(1)"
endef

# EXE target
define BUILD_EXE
$(1): $(2) $(3)
    $(CXX) -o $$@ $$^ $(LDFLAGS) $(LIBS)
    @echo "Built executable: $(1)"
endef
```

### 1.5 Build Scripts

**File: `build_windows.cmd`**

Convenience script for building on Windows.

**Features:**
- Checks for MinGW in PATH
- Generates build tag
- Runs parallel build
- Reports success/failure

**File: `setup_windows_build.cmd`**

Initial setup script.

**Features:**
- Copies `config.mingw.template` to `config.mingw`
- Prompts user to edit SDK paths
- Creates necessary directories

---

## Phase 2: Component Makefile Templates

### 2.1 Shared Library Template

**Pattern for building DLLs (e.g., `libnetxms.dll`)**

**Key elements:**
```makefile
include $(TOPDIR)/config.mingw

TARGET = libnetxms.dll
IMPLIB = libnetxms.dll.a
STATIC_LIB = libnetxms.a

# Source files
SOURCES = array.cpp base32.cpp base64.cpp ...
          dir.cpp dirw.cpp npipe_win32.cpp windll.cpp

# Resource file
RC_SOURCE = libnetxms.rc

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.c=.o)
RC_OBJECT = $(RC_SOURCE:.rc=.o)

# Compiler flags
CFLAGS = $(COMMON_CFLAGS) $(COMMON_INCLUDES) -DLIBNETXMS_EXPORTS
CXXFLAGS = $(CFLAGS) -std=c++11

# Linker flags
LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) -shared \
          -Wl,--out-implib,$(IMPLIB)

# Dependencies
LIBS = $(SSL_LIBS) $(PCRE_LIBS) $(WIN_LIBS)

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RC_OBJECT)
    $(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
```

### 2.2 Executable Template

**Pattern for building EXEs (e.g., `nxagentd.exe`)**

**Key differences from DLL:**
- No import library generation
- Uses `-mconsole` or `-mwindows` subsystem flag
- Links against project libraries

```makefile
TARGET = nxagentd.exe

LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) -mconsole

LIBS = -L$(TOPDIR)/src/agent/libnxagent -lnxagent \
       -L$(TOPDIR)/src/libnetxms -lnetxms \
       -L$(TOPDIR)/src/libnxsl -lnxsl \
       $(SSL_LIBS) $(PCRE_LIBS) $(WIN_LIBS)
```

### 2.3 Subagent (Plugin) Template

**Pattern for building NSM files (agent plugins)**

**Key differences:**
- Target is `.nsm` file
- Shared library without import lib needed
- Uses `-Wl,--subsystem,windows`

```makefile
TARGET = winnt.nsm

LDFLAGS = $(COMMON_LDFLAGS) -shared -Wl,--subsystem,windows

LIBS = -L$(TOPDIR)/src/agent/libnxagent -lnxagent \
       -L$(TOPDIR)/src/libnetxms -lnetxms \
       -lwbemuuid $(WIN_LIBS)
```

### 2.4 Directory Coordinator Template

**Pattern for subdirectory makefiles**

**Example: `src/agent/subagents/Makefile.w32`**

```makefile
include $(TOPDIR)/config.mingw

# Windows-specific subagents (always built)
WIN_SUBAGENTS = winnt winperf wmi wineventsync

# Cross-platform subagents available on Windows
COMMON_SUBAGENTS = filemgr logwatch ping netsvc mqtt ssh

# Optional database subagents
DB_SUBAGENTS =
ifeq ($(WITH_MYSQL),1)
DB_SUBAGENTS += mysql
endif
ifeq ($(WITH_PGSQL),1)
DB_SUBAGENTS += pgsql
endif

SUBDIRS = $(WIN_SUBAGENTS) $(COMMON_SUBAGENTS) $(DB_SUBAGENTS)

.PHONY: all clean install $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
    @$(MAKE) -C $@ -f Makefile.w32 TOPDIR=$(TOPDIR)

clean:
    @for %%d in ($(SUBDIRS)) do @$(MAKE) -C %%d -f Makefile.w32 clean TOPDIR=$(TOPDIR)
```

---

## Phase 3: Implementation Strategy

### 3.1 Build Tiers

Components organized by dependency layers:

**Tier 1 - Core Libraries** (no external dependencies)
- `src/jansson` - JSON library
- `src/libpng` - PNG image library
- `src/libnetxms` - Core utilities

**Tier 2 - Mid-Level Libraries** (depend on Tier 1)
- `src/libnxsl` - NXSL interpreter
- `src/libnxlp` - Log parser
- `src/libnxdb` - Database abstraction

**Tier 3 - Agent Foundation**
- `src/agent/libnxagent` - Agent utilities
- `src/agent/core` - Agent daemon

**Tier 4 - Agent Subagents** (parallel, independent)
- Simple: `filemgr`, `logwatch`, `ping`
- Platform-specific: `winnt`, `winperf`
- Complex: `wmi`, `python`, `java`

**Tier 5 - Tools & Utilities**
- Agent tools: `nxreload`, `nxapush`
- Client tools: `nxalarm`, `nxevent`

**Tier 6 - Database Drivers**
- `sqlite` (bundled)
- `pgsql`, `mysql`, `mssql`, `oracle` (external SDKs)

**Tier 7 - Notification Drivers**
- All drivers in `src/ncdrivers/`

### 3.2 Implementation Order

**Stage 1: Foundation** (Week 1)
1. Create `config.mingw.template` and infrastructure
2. Create top-level `Makefile.w32`
3. Create `build/rules.mingw` and `build/targets.mingw`
4. Create build scripts

**Stage 2: Core Libraries** (Week 2)
5. Tier 1 libraries: `jansson`, `libnetxms`, `libnxsl`
6. Test basic compilation and linking

**Stage 3: Database Layer** (Week 3)
7. `libnxdb` library
8. Database drivers (sqlite, pgsql, mysql)

**Stage 4: Agent Foundation** (Week 4)
9. `libnxagent` library
10. `nxagentd` core
11. Test basic agent startup

**Stage 5: Agent Subagents** (Weeks 5-6)
12. Simple subagents first
13. Windows-specific subagents
14. Complex subagents (WMI, Python, Java)

**Stage 6: Client Libraries** (Week 7)
15. `libnxclient`
16. Client tools

**Stage 7: Notification Drivers** (Week 8)
17. All notification channel drivers

**Stage 8: Testing & Documentation** (Week 9)
18. Comprehensive testing
19. Complete documentation
20. Create troubleshooting guide

### 3.3 Validation Strategy

**For each component:**
1. Verify successful compilation
2. Check symbol exports: `nm -D <library>.dll`
3. Check dependencies: `ldd <binary>.exe`
4. Runtime test: Execute and verify functionality
5. Compare with MSVC-built version

**Test matrix:**
- MinGW x86 (i686-w64-mingw32)
- MinGW x64 (x86_64-w64-mingw32)
- MinGW ARM64 (aarch64-w64-mingw32)
- Debug vs. Release builds
- Static vs. Shared linking

---

## Phase 4: Windows-Specific Considerations

### 4.1 Resource Files

**Windows resource files (`.rc`) handling:**
- Version information
- Icons and bitmaps
- String tables
- Manifest files

**Compilation:**
```makefile
%.o: %.rc
    $(WINDRES) -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@
```

**Build tag integration:**
- Use existing `build/netxms-build-tag.rc`
- Generated by `tools/updatetag.pl`

### 4.2 Windows System Libraries

**Required Windows libraries:**
```makefile
WIN_LIBS = -lws2_32      # Winsock 2
           -liphlpapi    # IP Helper API
           -lnetapi32    # Network Management
           -luserenv     # User Environment
           -lpsapi       # Process Status API
           -lpdh         # Performance Data Helper
           -lwtsapi32    # Windows Terminal Services
           -lsetupapi    # Setup API
           -ladvapi32    # Advanced Windows API
           -lshell32     # Shell API
           -lole32       # OLE32
           -loleaut32    # OLE Automation
           -lsensapi     # System Event Notification
           -lversion     # Version Info
           -lwbemuuid    # WMI (for WMI subagent)
```

### 4.3 Platform-Specific Source Files

**Windows-specific implementations:**
- `dir.cpp`, `dirw.cpp` - Directory operations
- `npipe_win32.cpp` - Named pipes
- `windll.cpp` - DLL helpers

**Conditional compilation:**
- Most platform selection via `#ifdef _WIN32` in source
- Makefile.w32 includes Windows-specific sources
- Makefile.am includes Unix-specific sources

### 4.4 Unicode Handling

**Windows Unicode requirements:**
- Define `_WIN32`, `UNICODE`, `_UNICODE`
- Use `TCHAR` abstraction
- String literals use `_T()` macro
- Becomes `WCHAR` on Windows builds

### 4.5 Threading

**MinGW threading considerations:**
- Use `-mthreads` compiler flag
- Links against pthread emulation
- Windows native thread API underneath

---

## Phase 5: Migration from MSBuild

### 5.1 Parallel Build Period

**Keep both build systems temporarily:**
```
src/libnetxms/
├── libnetxms.vcxproj    # Mark as DEPRECATED
├── Makefile.am          # Unix builds (unchanged)
└── Makefile.w32         # Windows builds (new primary)
```

**Deprecation strategy:**
1. Add comments to `.vcxproj` files
2. Update documentation to reference Makefile.w32
3. Keep for 2-3 release cycles
4. Monitor for issues

### 5.2 CI/CD Updates

**Build pipeline changes:**
- Add MinGW build jobs
- Run parallel builds (MSBuild vs. MinGW)
- Compare outputs for validation
- Automated testing for both

### 5.3 Final Migration

**After validation period:**
1. Remove all `.vcxproj` files
2. Remove all `.sln` files
3. Remove `build/msbuild/` directory
4. Update `EXTRA_DIST` in Makefile.am
5. Update release notes

---

## Phase 6: Known Challenges & Mitigation

### 6.1 WMI Subagent

**Challenge:**
- Heavy COM/WMI usage
- MinGW COM support limited vs. MSVC
- Uses `IWbemServices`, `IWbemLocator`, etc.

**Mitigation:**
- Test thoroughly with MinGW COM headers
- May need C-style WMI interfaces instead of C++
- Fallback: Keep MSVC build for this component only
- Alternative: Rewrite using Windows API direct calls

### 6.2 Third-Party Binary SDKs

**Challenge:**
- Oracle, Informix, DB2, Tuxedo provide MSVC libraries
- ABI compatibility with MinGW uncertain

**Mitigation:**
- MinGW can usually link against MSVC DLLs
- Create import libraries if needed: `dlltool`
- Test each database driver thoroughly
- Document compatibility matrix

### 6.3 Windows Driver Components

**Challenge:**
- Components using WINDDK (Windows Driver Kit)
- Kernel-mode code requires specific toolchain

**Mitigation:**
- WINDDK components likely stay with MSVC/WDK
- Most NetXMS code is user-mode (not affected)
- Document requirements clearly

### 6.4 Resource Compiler Differences

**Challenge:**
- `windres` syntax differs from MSVC `rc.exe`
- Some resource features not supported

**Mitigation:**
- Test all `.rc` files with windres
- Simplify resources if needed
- Use UTF-8 or UTF-16LE encoding consistently

### 6.5 Debugging

**Challenge:**
- MinGW debug symbols differ from MSVC PDB format
- Debugging tools may be less mature

**Mitigation:**
- Use GDB for debugging (included with MinGW)
- Generate DWARF debug symbols (`-g`)
- Consider Dr. MinGW for crash dumps

---

## Phase 7: Documentation

### 7.1 Build Documentation

**File: `BUILD_WINDOWS.md`**

Comprehensive guide covering:
- Prerequisites (MinGW installation)
- Dependency setup
- Configuration (`config.mingw`)
- Build process
- Installation
- Troubleshooting

### 7.2 Developer Documentation

**Updates to existing docs:**
- `doc/CPP_DEVELOPER_GUIDE.md` - Add Windows/MinGW section
- `CLAUDE.md` - Add Windows build quick start
- Component-specific `CLAUDE.md` files - Add Windows notes

### 7.3 Migration Guide

**File: `doc/MSBuild_to_MinGW_Migration.md`**

Guide for developers transitioning from MSBuild:
- Comparison of build systems
- Common issues and solutions
- Tool equivalents (CL.exe → gcc, LINK.exe → ld)
- Debugging differences

---

## Phase 8: Quality Assurance

### 8.1 Testing Strategy

**Unit Tests:**
- Run existing test suite: `tests/suite/netxms-test-suite.exe`
- Compare results with MSVC build

**Integration Tests:**
- Full agent deployment
- Subagent loading and execution
- Database driver connectivity
- Client library functionality

**Performance Tests:**
- Compare runtime performance (MinGW vs. MSVC)
- Memory usage comparison
- Startup time comparison

### 8.2 Validation Criteria

**Build Success:**
- All 209 components compile without errors
- All libraries link successfully
- All executables run

**Functional Parity:**
- Agent can start and connect to server
- All subagents load and provide metrics
- Database drivers connect and query
- Client tools execute commands

**Binary Compatibility:**
- DLL exports match MSVC version
- No ABI breaks for external consumers

---

## Phase 9: Long-Term Maintenance

### 9.1 Ongoing Maintenance

**Adding new components:**
1. Create Makefile.w32 alongside Makefile.am
2. Add to parent directory's SUBDIRS
3. Test on all platforms

**Updating dependencies:**
1. Update SDK paths in config.mingw.template
2. Document version requirements
3. Test compatibility

### 9.2 Version Control

**Git considerations:**
- Add `config.mingw` to `.gitignore`
- Commit `config.mingw.template` as template
- Track all `Makefile.w32` files
- Mark `.vcxproj` as deprecated (eventually remove)

---

## Appendix A: File Inventory

### New Files to Create

**Configuration:**
- `config.mingw.template` - Configuration template
- `config.mingw` - User configuration (gitignored)

**Top-level build:**
- `Makefile.w32` - Top-level makefile

**Build infrastructure:**
- `build/rules.mingw` - Common build rules
- `build/targets.mingw` - Target templates

**Scripts:**
- `build_windows.cmd` - Build convenience script
- `setup_windows_build.cmd` - Setup script

**Documentation:**
- `BUILD_WINDOWS.md` - Windows build guide
- `doc/Windows_MinGW_Build_System_Design.md` - This document
- `doc/MSBuild_to_MinGW_Migration.md` - Migration guide

**Component Makefiles:**
- 209 `Makefile.w32` files (one per component)

### Modified Files

**Documentation:**
- `CLAUDE.md` - Add Windows build section
- `doc/CPP_DEVELOPER_GUIDE.md` - Add MinGW section

**Build files:**
- Component `Makefile.am` files - Mark vcxproj in EXTRA_DIST as deprecated

---

## Appendix B: Dependencies Reference

### Required MinGW Packages

**Core toolchain:**
- `mingw-w64-gcc` - C/C++ compiler
- `mingw-w64-binutils` - Linker, assembler, etc.
- `make` - GNU Make

**Build tools:**
- `perl` - For updatetag.pl and other scripts
- `pkg-config` - Optional, for library detection

### Required External Libraries

**Mandatory:**
- OpenSSL (libssl, libcrypto)
- PCRE (regular expressions)
- zlib (compression)

**Optional:**
- cURL (for HTTP/REST features)
- libssh (for SSH subagent)
- PostgreSQL client library
- MySQL/MariaDB client library
- MQTT (libmosquitto)
- Python development headers
- Java JDK (for JNI)

### Windows SDK Components

**Optional (Windows-provided):**
- MS SQL Server SDK
- Oracle Instant Client
- IBM Informix SDK
- IBM DB2 SDK
- Oracle Tuxedo SDK
- IBM MQ SDK
- Windows Driver Development Kit (WINDDK)

---

## Appendix C: Comparison with Alternative Approaches

### Approach 1: Makefile.w32 (This Design)

**Pros:**
- Simple, direct makefiles
- Clean platform separation
- Fast builds
- Easy maintenance

**Cons:**
- Need to maintain two build systems
- 209 makefiles to create initially

### Approach 2: Integrate Windows into configure.ac

**Pros:**
- Single unified build system
- Consistent approach across platforms

**Cons:**
- Complex autoconf/automake setup
- Slower builds (configure overhead)
- Windows changes affect Unix builds
- Requires full autotools on Windows
- Generated makefiles harder to debug

### Approach 3: CMake

**Pros:**
- Modern build system
- Cross-platform
- Good IDE integration

**Cons:**
- Requires rewriting entire build system
- Large scope change
- Learning curve for team
- Would affect Unix builds too

### Decision: Makefile.w32 Selected

**Rationale:**
- Minimal disruption to existing Unix builds
- Simplest approach for Windows-only changes
- Existing pattern in codebase (`sql/Makefile.w32`)
- Fastest time to implementation
- Easiest to maintain and debug

---

## Appendix D: Example Component Makefile

### Complete Example: src/libnxsl/Makefile.w32

```makefile
#
# libnxsl Windows Makefile
# NetXMS Scripting Language Interpreter
#

include $(TOPDIR)/config.mingw

TARGET = libnxsl.dll
IMPLIB = libnxsl.dll.a

SOURCES = \
    array.cpp class.cpp compiler.cpp env.cpp \
    expression.cpp function.cpp hashmap.cpp instructions.cpp \
    lexer.cpp main.cpp nxslapi.cpp parser.cpp program.cpp \
    stack.cpp string.cpp table.cpp util.cpp value.cpp \
    variable.cpp vm.cpp

RC_SOURCE = libnxsl.rc

OBJECTS = $(SOURCES:.cpp=.o)
RC_OBJECT = $(RC_SOURCE:.rc=.o)

CXXFLAGS = $(COMMON_CFLAGS) $(COMMON_INCLUDES) \
           -DLIBNXSL_EXPORTS \
           -std=c++11

LDFLAGS = $(COMMON_LDFLAGS) $(COMMON_LIBDIRS) \
          -shared -Wl,--out-implib,$(IMPLIB)

LIBS = -L$(TOPDIR)/src/libnetxms -lnetxms \
       $(PCRE_LIBS) $(WIN_LIBS)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RC_OBJECT)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "Built $@"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.rc
	$(WINDRES) -I$(TOPDIR)/include -I$(TOPDIR)/build $< -o $@

clean:
	rm -f $(OBJECTS) $(RC_OBJECT) $(TARGET) $(IMPLIB) *.dll *.a

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/lib
	cp $(TARGET) $(PREFIX)/bin/
	cp $(IMPLIB) $(PREFIX)/lib/

.PHONY: depend
depend:
	$(CXX) -MM $(CXXFLAGS) $(SOURCES) > .depend

-include .depend
```

---

## Appendix E: Glossary

**Terms:**

- **MinGW** - Minimalist GNU for Windows; provides GCC compiler and GNU tools for Windows
- **MinGW-w64** - Improved fork of MinGW with 64-bit support
- **MSVC** - Microsoft Visual C++ compiler
- **MSBuild** - Microsoft's build engine (used by Visual Studio)
- **DLL** - Dynamic Link Library (Windows shared library)
- **Import Library (.dll.a)** - Library file for linking against DLL
- **windres** - Windows Resource Compiler (MinGW version)
- **TCHAR** - Platform-independent character type
- **NSM** - NetXMS Subagent Module (agent plugin)
- **DDR** - Database DRiver (database plugin)

---

## Document Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-20 | NetXMS Team | Initial design document |

---

**End of Document**
