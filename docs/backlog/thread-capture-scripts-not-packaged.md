---
worth: later
where: tools/capture_nxagentd_threads.sh
added: 2026-11-17
---
# No packaged way to capture agent thread dumps

`tools/capture_netxmsd_threads.sh` and `tools/capture_nxagentd_threads.sh` have no install rule and are not in
`EXTRA_DIST` of `tools/Makefile.am`, so `make dist` tarballs leave them out. For the server this does not
matter: the packaged `nx-collect-server-diag -p` (`src/server/tools/scripts`) runs the same gdb
`thread apply all bt` capture, and the docs should point there. What is missing is an offline way to dump
agent threads, and dbx support (AIX/Solaris), which only the standalone scripts have.

Unknown that settles it: whether agent thread dumps are requested often enough to add an agent mode to
`nx-collect-server-diag` or install the agent script.

Found while documenting netxms/netxms-doc#83 in netxms-doc-antora.
