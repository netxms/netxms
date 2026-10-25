# CLAUDE.md — Tests

## Build Gating

Nothing in this tree is built by a default `make`. `tests` is added to
`TOP_LEVEL_MODULES` (in `configure.ac` and the top-level `Makefile`) only when
the tree is configured `--with-tests`. Within it, `include`, `config`, `suite`,
`test-libnetxms` and `test-libnxdb` are unconditional `SUBDIRS` in
`tests/Makefile.am`; everything else comes in through `@TEST_MODULES@` /
`@AGENT_UNIT_TESTS@` (in `tests/Makefile.am` and `tests/agent/unit/Makefile.am`),
populated by `configure.ac`. `integration/` is built by Maven separately;
`nx-2488/` and `tools/` are manual helpers outside the build.

## Windows build

The Windows (MinGW) build has its own wiring, parallel to the autotools one:

- Each test directory has a `Makefile.w32` that sets `TOOL` / `SOURCES` /
  `TOOL_CPPFLAGS` / `TOOL_LIBS` and includes `build/tool-common.mk`. Tests that
  compile sources from outside their own directory (the webhook helpers, the
  subagent sources under `agent/unit/*`) locate them with `vpath` so the objects
  land in the test's own object directory rather than the component's.
- The top-level `Makefile.w32` lists the test directories in `TESTS_DIRS`, gated
  on `BUILD_TESTS` in `config.mingw` (forced off for non-x64 architectures).
- `mingw32-make -f Makefile.w32 test` builds and runs `suite/netxms-test-suite.cmd`,
  the Windows counterpart of `netxms-test-suite.in`. See
  [BUILD_WINDOWS.md](../BUILD_WINDOWS.md) for details.
- There is no install step on Windows: the runner sets `NETXMS_HOME` to
  `out\<arch>\<type>`, and the server build stages the SQL scripts that
  `test-server` needs for `nxdbmgr init` into `out\<arch>\<type>\share\sql`.

Test binaries that resolve or parse IP addresses must call `WSAStartup()` after
`InitNetXMSProcess()` under `#ifdef _WIN32` — `InetAddress::parse()` uses
`WSAStringToAddress()` and silently returns an invalid address until Winsock is
initialized.

## Test framework

There is **no external framework** (no GoogleTest/Catch). Tests are hand-rolled:

- Each `test-*` directory builds one `bin_PROGRAM` whose `main()` calls a
  sequence of `Test*()` functions. Example: `test-libnetxms/test-libnetxms.cpp`
  declares the tests as `extern` and invokes them in order; the individual
  tests live in sibling `.cpp` files (`crypto.cpp`, `queue.cpp`, `tp.cpp`, …).
- `main()` starts with `InitNetXMSProcess(true)`.
- Assertions come from `tests/include/testtools.h`:
  - `StartTest(_T("name"))` … work … `EndTest()` (or `EndTest(ms)` to print timing).
  - `Assert(c)`, `AssertEx(c, msg)`, `AssertTrue/AssertFalse`,
    `AssertNull/AssertNotNull`, `AssertEquals(x, y)` (overloaded for
    int32/int64/uint/size_t/ssize_t and `char*`/`wchar_t*`).
  - A failing assert prints a red `FAIL` with file:line and **exits the
    process non-zero** (via `ptrace(PT_TRACE_ME)` so a debugger can catch it,
    unless ASan is active). There is no "continue after failure" — first
    failure aborts that binary, and the suite runner stops.
- `SetTestFailureHook(fn)` registers a function that `ExitTestProcess()` calls
  before exiting on a failed assert; `test-server` uses it to `FastShutdown()`
  the live server so the database is unlocked and the log flushed.
- Use `_T()` for literals and `TCHAR` throughout, same as production code.

## Building and running

Configure with `--with-tests` (see root Quick Start). Each test binary is
installed to `$BINDIR` and can be run directly (`/opt/netxms/bin/test-libnetxms`).
The suite runner (`suite/netxms-test-suite.in`) invokes each test binary in
turn and `exit 1`s on the first non-zero exit. Optional binaries are guarded
with `[ -x ... ]` so a suite built without a given module still runs.

In an `--enable-sanitizer` build the runner starts `test-server` through
`nx-run-asan-binary -ndd test-server` (detected via `ldd | grep libasan`), the
same wrapper `netxmsd-asan` uses. The wrapper preloads `libasan`, the SQLite
database driver and the network device drivers so that LeakSanitizer can
symbolize allocations made in libraries the server unloads at shutdown;
without it those show up as false leaks and fail the run. Run a sanitizer
build of `test-server` by hand the same way, not directly.

## Adding a new unit test

For a **new test binary** five places must stay in sync — updating the
Makefile alone is not enough:

1. Create `tests/<name>/Makefile.am` defining the `bin_PROGRAM`, its
   `_SOURCES`, `_CPPFLAGS` (add `-I@top_srcdir@/tests/include`), and `_LDADD`
   (link the library under test + `@top_srcdir@/src/libnetxms/libnetxms.la` +
   `@EXEC_LIBS@`; add the internal-jansson `if USE_INTERNAL_JANSSON` block if
   you use JSON).
2. Wire it into `configure.ac`: append the dir to `TEST_MODULES` (server/lib
   test) or the subagent name to `AGENT_UNIT_TESTS` (agent test), **and** add
   the `Makefile` to the `AC_CONFIG_FILES` list near the other
   `tests/*/Makefile` entries.
3. Add an executable-guarded stanza to `suite/netxms-test-suite.in` so the
   runner picks it up.
4. Create `tests/<name>/Makefile.w32` (see "Windows build" above) and add the
   directory to `TESTS_DIRS` in the top-level `Makefile.w32`.
5. Add a `call :RunTest <binary>` line to `suite/netxms-test-suite.cmd`.

## Server core tests

Do not create a new standalone binary for server core code. There are two homes:

- **`test-libnxcore/`** links the real `libnxcore` (plus the same library set as
  `netxmsd`) and is for functions and classes that work without an initialized
  server. The test is in the wrong place if it needs the DB connection pool, the
  configuration cache, the user database, or loaded objects — such code hangs
  (`DBConnectionPoolAcquireConnection()` retries forever) or fails instead of
  being unit-testable, and stubbing core symbols is not an option because the
  library is linked, not compiled in. Add a `TestXxx()` entry function in a new
  `.cpp`, list it in both Makefiles, and call it from `test-libnxcore.cpp`.
- **`test-server/`** is a launcher that replaces `netxmsd`: it writes a
  `netxmsd.conf` into a scratch directory under `$TMPDIR`, creates a SQLite
  database with `nxdbmgr init` (found via `GetNetXMSDirectory(nxDirBin)`, so
  the tree must be installed), moves the client, tunnel and mobile listeners to
  ports 147xx and disables the SNMP trap receiver, local admin interface and
  web API, then calls `LoadConfig()` / `Initialize()`, runs the test functions
  inside the live server, and calls `Shutdown()`. Tests create the objects they
  need through the normal object API after initialization rather than through
  SQL. `-D <level>` sets the server debug level (log goes to the work
  directory), `-k` keeps the work directory after a successful run; it is always
  kept after a failure.

Both binaries need `@MICROHTTPD_CPPFLAGS@` / `-I$(MICROHTTPD_ROOT)/include`
because `nms_core.h` reaches `netxms-webapi.h`, which includes `<microhttpd.h>`.

For a **new test case in an existing binary**: add the `Test*()` function (new
`.cpp` if substantial, listed in that dir's `_SOURCES`), declare it in the
runner's `main()` file, and call it from `main()`.

Agent subagent unit tests compile the subagent's own `.cpp` files directly into
the test binary (see `agent/unit/entsoe/Makefile.am`) rather than linking a
built subagent — keep the `_SOURCES` list current when the subagent gains files.

## Conventions

- Any test that links objects into parent/child relations must unlink or delete
  them (`deleteObject()`) before returning. Parent/child lists are
  `SharedObjectArray` on both sides, so every linked pair is a `shared_ptr`
  cycle; objects still linked at exit are reported by LeakSanitizer as indirect
  leaks and break the `--enable-sanitizer` build (issue #3732). `CleanupObjects()`
  does not break these cycles, and LSan suppressions cannot target them without
  also masking real object leaks.
- When fixing a library bug, add or extend a case in the matching `test-lib*`
  binary.
