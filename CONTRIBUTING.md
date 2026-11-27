# Contributing to NetXMS

## Principles

These shape what gets accepted:

* **No data leaves the user's installation.** NetXMS does not send telemetry, usage statistics or monitoring data to
  the developers or any third party.
* **Resource usage matters.** The same code runs on a small office installation and on servers monitoring tens of
  thousands of nodes. Consider the cost of a change at that scale.
* **Extend through existing plugin points.** Server modules, agent subagents, network device drivers, notification
  channel drivers and database drivers are the preferred way to add functionality without touching the core.
* **Distributed operation.** Proxy agents and zones must keep working when the central server is unreachable.

Rough order of priority when we decide what to work on and review: security fixes, stability and performance of the
core, monitoring capabilities (protocols, device drivers, subagents), user interface, integrations.

## Workflow

Open an [issue](https://github.com/netxms/netxms/issues) before starting non-trivial work, and wait for a
maintainer's response. Describe the problem, the proposed solution and the approach you plan to take. This avoids
duplicated work and pull requests for changes that don't fit the project. Typo fixes and other obvious corrections can
go straight to a pull request.

Issues labeled [good first issue](https://github.com/netxms/netxms/labels/good%20first%20issue) and
[help wanted](https://github.com/netxms/netxms/labels/help%20wanted) are a reasonable starting point.

Pull requests:

* Reference the issue (`Fixes #123`)
* Keep unrelated changes out; one logical change per pull request
* Describe how the change was tested
* By submitting a pull request you agree to license your contribution under the MIT License

Documentation lives in separate repositories:
[Administration Guide](https://github.com/netxms/netxms-doc), [NXSL](https://github.com/netxms/nxsl-doc),
[data dictionary](https://github.com/netxms/netxms-db-doc). Changes to them follow the same process in the
respective repository.

Questions go to the [forum](https://www.netxms.org/forum/index.php) or [Telegram](https://telegram.me/netxms).
Communication in the issue tracker and pull requests is in English.

## Building from source

Dependencies on Debian/Ubuntu:

```bash
sudo apt-get install build-essential automake autoconf libtool flex bison git \
   libssl-dev libjansson-dev libcurl4-openssl-dev libpcre2-dev libexpat1-dev libmicrohttpd-dev \
   libmosquitto-dev libpq-dev libmariadb-dev \
   default-jdk maven gdb valgrind
```

Build server and agent into `/opt/netxms`:

```bash
./init-source-tree   # once after clone: generates build tag files and runs autoreconf
./configure --prefix=/opt/netxms --with-sqlite --with-server --with-agent --with-tests --enable-debug
make -j$(nproc)
make install
```

Database drivers are selected with `--with-pgsql`, `--with-mysql`, `--with-oracle`, `--with-mssql`.
`--enable-sanitizer` builds with AddressSanitizer (requires `libasan`); it can't be combined with valgrind.

Run the test suite, then the server and agent in foreground with debug output:

```bash
./tests/suite/netxms-test-suite
/opt/netxms/bin/netxmsd -D6
/opt/netxms/bin/nxagentd -D6
```

See the [Administration Guide](https://www.netxms.org/documentation/adminguide/installation.html) for database
setup.

Java client libraries and the management console:

```bash
mvn -f src/java-common/netxms-base/pom.xml install -DskipTests
mvn -f src/client/java/netxms-client/pom.xml install -DskipTests

# desktop console
mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone
java -jar src/client/nxmc/java/target/netxms-nxmc-standalone-*.jar

# web console, run in Jetty
mvn -f src/client/nxmc/java/pom.xml jetty:run -Pweb -Dnetxms.build.disablePlatformProfile=true
```

Eclipse formatter profiles for C++ and Java, and Notepad++ NXSL syntax files, are in [docs/ide](docs/ide).

## Code conventions

C/C++:

* C++11 at most; code must compile with GCC 4.8, Clang 3.3 and Visual Studio 2015
* 3-space indentation, opening brace on a new line
* PascalCase for functions and classes, camelCase for methods and variables, `m_` prefix for class members, `g_` for
  globals, `s_` for statics
* `MemAlloc` / `MemFree` family instead of `malloc` / `free`; NetXMS string and container classes instead of raw
  buffers
* `nullptr` instead of `NULL`
* String and character literals contain only ASCII; write other characters as escapes
* Debug output goes through `nxlog_debug_tag`; new tags are registered in `docs/internal/debug_tags.txt`
* Comments describe current behavior, not change history

Java: 3-space indentation, opening brace on a new line, no new external dependencies without discussion. The desktop
console uses SWT and the web console RWT; UI changes must work in both.

Server-client protocol changes must stay compatible with released clients. Database schema changes go through
`nxdbmgr` upgrade procedures.

## Testing

* `tests/suite/netxms-test-suite` runs unit tests for the C++ libraries and server components; it runs in CI on every
  push and pull request
* `tests/integration` contains Java integration tests that run against a live server. They need the Java libraries
  above plus `src/mobile-agent/java` installed, and a server with a node representing the management server itself.
  The server defaults to `127.0.0.1` with `admin`/`netxms`; override with `-DConfigFile=<file>` containing
  `server.address`, `server.login` and `server.password`. The tests set that node's agent port to `agent.port`
  (default 5019), so pass `-Dagent.port=4700` when testing against an agent on the standard port:

  ```bash
  mvn -f src/mobile-agent/java/pom.xml install
  mvn -f tests/integration/pom.xml test -Dtest=ScriptTest -Dagent.port=4700
  ```
* Add tests for new library code where practical

Large parts of the server and the UI have no automated coverage, so describe in the pull request what you tested by
hand and on which platforms.
