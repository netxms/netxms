# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NetXMS is an enterprise-grade, open-source network and infrastructure monitoring solution. It's a distributed client-server system written primarily in C++ (server, agent, libraries) and Java (management console, client libraries).

A full-stack feature can touch server-side C++, the Java client library, the SWT UI, the RWT (web) UI, NXSL scripting bindings, the WebAPI layer and the database schema — check each.

## Design Principles

Do not introduce unnecessary abstraction layers or wrapper functions. Call existing methods directly unless there is a clear architectural reason for indirection. When in doubt, prefer the simpler approach.

Do not promote a value to a `g_*` global or a config-template field just to surface it to a single one-shot caller — read it into a local variable instead. Globals and template entries are long-lived surface area that only pay off when many callers need the value or it must persist across the run.

When designing APIs: do not pair a value argument with a boolean that means "ignore that argument" — split into separate, well-named methods instead. Do not thread a parameter through call sites when the receiving object can derive the value from its own state. When a downstream consumer needs a value not on the object it receives, add a parameter to the API signature rather than adding a setter and mutating a throwaway copy.

When replacing an in-tree mechanism (loader, registration scheme, plugin format), convert every in-tree user and delete all legacy machinery (old loader, macros, config variables, build glue, DB rows via upgrade) in the same change set. Do not keep a transitional dual path — both paths live in the same tree and release, so a coexistence window only adds review surface and dead code.

When a shared helper performs a lookup that can fail (e.g. resolving an imported object reference), make the helper emit the diagnostic itself — give it the context and descriptor parameters it needs — rather than having every caller wrap it with its own logging. Duplicated wrappers drift.

When hardening ingestion of an NXCP or JSON field, keep the existing `getFieldAs*` / `json_object_get_*` accessor and add the range or sanity check after it. Change the accessor type only when the current one actually loses information the code needs; the out-of-range value is the bug, not the narrow read.

## Quick Start

### C++ Components (Server, Agent, Libraries)

```bash
# First-time setup after clone
./init-source-tree # generate build-tag files and do autoreconf

# Configure with desired options
./configure --prefix=/opt/netxms --with-sqlite --with-server --with-agent --with-tests --enable-debug
# Database options: --with-pgsql --with-mysql --with-oracle --with-mssql

# Build and install (always pass -j; a serial build of this tree takes a very long time)
make -j$(nproc)
make install

# Run tests
./tests/suite/netxms-test-suite

# Run server/agent in debug mode (not as daemon)
/opt/netxms/bin/netxmsd -D6
/opt/netxms/bin/nxagentd -D6

# Run an NXSL script outside the server
/opt/netxms/bin/nxscript script.nxsl
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

# Run integration tests against a running server (tests/integration also depends on
# netxms-mobile-agent, so build it in addition to base and client libraries)
mvn -f src/mobile-agent/java/pom.xml install
mvn -f tests/integration/pom.xml test -Dtest=ScriptTest#testNXSLDataCollectionFunctions
```

Maven must run online for `tests/integration` - offline mode (`-o`) cannot resolve the
`nexus-staging-maven-plugin` build extension. Server address and credentials default to
`127.0.0.1` and `admin`/`netxms` (see `TestConstants.java`), and the tests locate their target
node through `TestHelper.findManagementServer()`, so the server must have a node with the
`NC_IS_LOCAL_MGMT` capability.

## C++ Development Guidelines

Longer reference: [doc/CPP_DEVELOPER_GUIDE.md](doc/CPP_DEVELOPER_GUIDE.md).

### Language Version and Compiler Floor

- **Maximum C++11** — must compile with GCC 4.8+, Clang 3.3+, Visual Studio 2015+ (`_MSC_VER >= 1900`)
- Use `nullptr` instead of `NULL`
- Preprocessor gates that can never be false at these minimums (`__GNUC__ < 4`, `_MSC_VER < 1900` and the like) are dead and may be removed during cleanups. Keep fallbacks for C++14+ features (e.g. `HAVE_STD_MAKE_UNIQUE`).
- Exception: the Windows XP / pre-2003 64-bit atomic block in `include/nxatomic.h` (`!defined(_WIN64) && (_WIN32_WINNT < 0x0502)`) stays — a WinXP agent build may still be needed.

### Strings

- Server code is always Unicode and uses `L"..."` literals directly (see `src/server/CLAUDE.md`). Agent code and shared libraries that may be built in either mode use `TCHAR` and `_T("...")`.
- Choosing a class for a string member:
  - Set once at construction (ctor arg or move) → `String` (its `operator=` is deleted).
  - Reassigned wholesale but never appended/inserted → `MutableString` (assignable from `String` / `MutableString` / `const TCHAR*`).
  - Built up incrementally (append/insert/format) → `StringBuffer`.
- Other key classes: `StringList`, `StringMap`.
- `wchar_to_utf8` / `ucs2_to_utf8` / `ucs4_to_utf8` / `mb_to_utf8` / `tchar_to_utf8` do not null-terminate when the destination is too small for the next multi-byte sequence. Convert into `bufferSize - 1` and terminate via the return value:

  ```cpp
  size_t bytes = tchar_to_utf8(value, -1, buffer, bufferSize - 1);
  buffer[bytes] = 0;
  ```

### Memory Management

- Use `MemAlloc`, `MemFree`, `MemAllocStruct<T>()`, `MemAllocArray<T>()` instead of malloc/free
- Use `MemCopyStringA` / `MemCopyStringW` / `MemCopyString` (in `nms_common.h`) instead of `strdup` / `wcsdup` / `_tcsdup` — they are nullptr-safe and their results are interchangeable with `MemFree`
- Container classes (ObjectArray, HashMap) manage object ownership via `Ownership::True/False`
- For a scratch buffer that should live on the stack when small and on the heap when large, use `Buffer<T, BUFFER_SIZE>` (`nms_util.h`) instead of the hand-rolled `T local[N]; T *p = (len < N) ? local : MemAllocArrayNoInit<T>(len)` + `MemFree` pattern. `BUFFER_SIZE` is in bytes, `reserve()` does not initialize contents, and `operator T*()` makes it a drop-in `T*`. Its inline storage is `alignas(T)`, which makes it the fix for unaligned-access/SIGBUS bugs on strict-alignment CPUs (SPARC, older ARM/MIPS): `memcpy` bytes from an arbitrary wire/stream offset into the `Buffer`, do the `T`-typed work there, `memcpy` back — never `reinterpret_cast<T*>` into a raw byte stream.

### Threading

- Thread pools (`ThreadPoolExecute()`, `ThreadPoolScheduleRelative()`) and `ThreadCreateEx()` for threads; `Mutex`, `Condition`, `RWLock` with `LockGuard` for RAII locking.
- For trivial accessors to mutex-guarded members, use the `GetAttributeWithLock(attr, mutex)` / `SetAttributeWithLock` templates (`nms_util.h`) as header one-liners instead of hand-written LockGuard getter bodies in the .cpp (pattern: `DCObject` getters in `nms_dcoll.h`). Give the explicit template argument to convert derived types (`MutableString`/`StringBuffer` → `String`). Raw array members (`wchar_t[]`, `char[]`) can't go through the template — argument binding would copy outside the lock; use `{ LockGuard g(m_mutex); return String(m_arr); }` for those.

### Container Classes

`ObjectArray<T>`, `SharedObjectArray<T>`, `IntegerArray<T>`, `HashMap<K,V>` (fixed-size keys), `StringObjectMap<T>`, `StringMap`, `ObjectQueue<T>` (thread-safe).

Prefer NetXMS containers for object collections and ownership management. For maps of plain scalar values (e.g. `nodeId → double`), `std::map` is acceptable and preferred — `HashMap` stores heap-allocated pointers, which forces a pointless per-entry allocation for scalars. `#include <map>` explicitly; it is not pulled in by `nms_util.h`.

### Network and SNMP

- For a MAC address or bridge ID member, use `MacAddress`, not a raw `BYTE[MAC_ADDR_LENGTH]` — it provides `isValid()`, `equals()`, `parse()`, `toString()`, `toJson()`. When persisting, `MacAddressNotation::FLAT_STRING` matches the old `BinToStr` flat-hex format, so converting an existing `BYTE[]` field keeps on-disk values compatible.
- Pass OIDs to `SnmpGet`, `SNMP_Variable`, `CheckSNMPIntegerValue` etc. in binary form — a `uint32_t oid[N]` array, an `SNMP_ObjectId`, or the `{1, 3, 6, ...}` initializer-list overload. Do not build a text OID with `_sntprintf` to splice in a row index; that re-parses the string on every call. For a per-row OID: `uint32_t oid[N] = { ..., 0 }; oid[N-1] = ifIndex;` (example: `getInterfaceState` in `src/server/libnxsrv/ndd.cpp`).

### JSON (jansson)

- Never `#include <jansson.h>` directly — include `nms_util.h`, which selects the system or bundled copy.
- Prefer the typed helpers over manual `json_object_get` + `json_is_*` + `json_*_value` chains: `json_object_get_string_t` (allocated `TCHAR*`) / `json_object_get_string_utf8` (no-alloc view) / `json_object_get_string` (`String`), `json_object_get_int32` / `int64` / `uint32` / `uint64` / `boolean` / `time` / `uuid`, and the lenient `json_object_get_array_ex` / `json_object_get_object_ex` for LLM tool arguments that may arrive stringified. Full list near `json_object_get` in `include/nms_util.h`.
- Emit a `uuid` with `guid.toJson()` (`include/uuid.h`).
- New shared helper placement: trivial single-expression helpers go `static inline` in `include/nms_util.h`; anything with a non-trivial body or extra dependencies gets a `LIBNETXMS_EXPORTABLE` declaration there and its body in `src/libnetxms/tools.cpp`.

### Build System

- Before removing an `AC_CHECK_FUNCS` / `AC_CHECK_*` probe from `configure.ac` because no first-party code reads its `HAVE_*` macro, grep the bundled third-party sources too: `src/jansson/jansson_private_config.h` includes the top-level `config.h` (non-Windows) and `hashtable_seed.c` reads `HAVE_GETTIMEOFDAY` / `HAVE_GETPID`; `src/sqlite/sqlite3.c` reads `HAVE_USLEEP`. When the symbol is guaranteed on the relevant platforms, the preferred fix is to hardcode the `HAVE_*` in the bundled library's own private config header (platform-guarded), not to keep the configure probe.

### Logging

- Use `nxlog_debug_tag(tag, level, format, ...)` for debug output
- Add new debug tags to `doc/internal/debug_tags.txt`

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
- Minimize external dependencies
- Desktop client uses SWT, web client uses RWT
- Always add an `import` for any class referenced in code — never write a fully-qualified name inline (e.g. `org.netxms.nxmc.modules.X.Y.method()`). Exceptions: `java.lang.*` and same-package classes.

## CI / Static Analysis

- **Semgrep SAST** runs on every PR (diff-aware, blocks on ERROR severity). Workflow: `.github/workflows/semgrep.yaml`. Custom rules go in `.semgrep.yml`, exclusions in `.semgrepignore`. Inline suppression: `// nosemgrep: <rule-id>` (C/C++, Java) or `# nosemgrep: <rule-id>` (Python), always with the rule-id.
- **Function pointer cast check** blocks on every push and PR. The build runs with GCC's `-Wcast-function-type` and fails if any warning appears. Workflow: `.github/workflows/build_and_test.yaml`

## NXCP Protocol Changes

When adding a new NXCP command code or renaming an existing one, update the `NXCPMessageCodeName()` function in `src/libnetxms/nxcp.cpp` to include the corresponding command code to name mapping. Also mirror the new `CMD_*` constant into `src/java-common/netxms-base/src/main/java/org/netxms/base/NXCPCodes.java` — the Java enum tracks the full command-code namespace even for codes the Java client does not yet use, so fill any adjacent gaps you notice while in the file.

When adding an NXCP field that semantically matches an existing `VID_*` constant (e.g. "ID of a mapping table" → `VID_MAPPING_TABLE_ID`), reuse the existing constant rather than minting a new one. NXCP fields are not typed by ID, so a duplicate just bloats the constant table.

## Translations

Console PO files live in `src/client/nxmc/java/src/main/resources/po/`. Use `msgfmt --statistics` to check completeness and `msgfmt -c` to validate format specifiers. Technical terms (SNMP, SSH, DCI) stay untranslated.

## Commits

Commit subjects reference the tracked issue at the end: `fixed incorrect handling of ... (issue #3781)`.
