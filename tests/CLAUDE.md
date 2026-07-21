# CLAUDE.md — Tests

Guidance for the NetXMS test tree. See the root [CLAUDE.md](../CLAUDE.md) for project-wide rules.

## Overview

`tests/` holds the C++ unit-test suite, agent subagent unit tests, the HA lease
test harness, Java integration tests, and standalone helper tools.

**Nothing in this tree is built by a default `make`.** `tests` is added to
`TOP_LEVEL_MODULES` (in `configure.ac` and the top-level `Makefile`) only when
the tree is configured `--with-tests`; without that switch the whole directory
is never entered. A **second** level of gating then applies *within* the
enabled tests tree via `@TEST_MODULES@` / `@AGENT_UNIT_TESTS@` — see the "Build"
column below.

## Layout

| Directory | What it is | Built when `--with-tests` |
|-----------|-----------|-----------|
| `include/` | `testtools.h` — shared assertion/`StartTest`/`EndTest` macros. No sources, header only. | always (EXTRA_DIST) |
| `config/` | `netxmsd.conf` / `nxagentd.conf` templates used when running tests | always |
| `suite/` | `netxms-test-suite` runner script (generated from `.in`) | always |
| `test-libnetxms/` | Core library unit tests (containers, threading, crypto, NXCP, geolocation, …) | always |
| `test-libnxdb/` | libnxdb tests (incl. Oracle CLOB handling) | always |
| `test-libnxsnmp/` | SNMP library tests | via `@TEST_MODULES@` |
| `test-libnxsl/` | NXSL interpreter tests | via `@TEST_MODULES@` |
| `test-libnxsrv/` | Server library tests (NObject hierarchy, drivers, mock SNMP transport, …) | via `@TEST_MODULES@` |
| `test-ncd-webhook/` | Webhook notification-channel driver tests | via `@TEST_MODULES@` |
| `agent/unit/*` | Per-subagent unit tests: `entsoe`, `openmeteo`, `linux-cpu-usage-collector` | via `@AGENT_UNIT_TESTS@` |
| `ha/` | `ha-node-sim` + Python `harness.py` — adversarial HA lease-manager harness (issue #3364). Compiles the real `src/server/core/halease.cpp` against a shared DB. | via `@TEST_MODULES@` |
| `integration/` | Java/Maven integration tests (`EppScriptTest`, NXSL resource scripts) against a running server | Maven (separate) |
| `nx-2488/` | Manual web-service caching repro (SQL patch, NXSL scripts, trivial web server) | manual (not in build) |
| `tools/` | Standalone Python helpers: `otlp-log-sender`, `otlp-mock-collector` (for OTLP ingestion testing, issue #3292) | manual (not in build) |

Inside the enabled tests tree, `include`, `config`, `suite`, `test-libnetxms`,
and `test-libnxdb` are unconditional `SUBDIRS` in `tests/Makefile.am`; the rest
come in through `@TEST_MODULES@` / `@AGENT_UNIT_TESTS@` (in `tests/Makefile.am`
and `tests/agent/unit/Makefile.am`), populated by `configure.ac`.

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
- Use `_T()` for literals and `TCHAR` throughout, same as production code.

## Building and running

```bash
# Configure with tests, then build the whole tree
./configure --prefix=/opt/netxms --with-server --with-agent --with-tests --enable-debug
make -j$(nproc) && make install

# Run the full suite (each binary is installed to $BINDIR)
./tests/suite/netxms-test-suite         # from build tree
# or, when installed:
/opt/netxms/bin/netxms-test-suite

# Run a single test binary directly
/opt/netxms/bin/test-libnetxms
```

The suite runner (`suite/netxms-test-suite.in`) invokes each test binary in
turn and `exit 1`s on the first non-zero exit. Optional binaries are guarded
with `[ -x ... ]` so a suite built without a given module still runs.

## Adding a new unit test

For a **new test binary** three places must stay in sync — updating the
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

For a **new test case in an existing binary**: add the `Test*()` function (new
`.cpp` if substantial, listed in that dir's `_SOURCES`), declare it in the
runner's `main()` file, and call it from `main()`.

Agent subagent unit tests compile the subagent's own `.cpp` files directly into
the test binary (see `agent/unit/entsoe/Makefile.am`) rather than linking a
built subagent — keep the `_SOURCES` list current when the subagent gains files.

## Conventions

- Follow the root C++ guidelines (C++11 max, 3-space indent, brace on new line).
- Keep tests deterministic and self-contained; tests that need a live server or
  database (integration, HA harness, Oracle CLOB) are separated out and gated.
- When fixing a library bug, prefer adding/extending a case in the matching
  `test-lib*` binary — recent history shows this is the expected pattern
  (e.g. thread-pool stalled-expansion test for #3436, libnxsrv suite for #3442).
