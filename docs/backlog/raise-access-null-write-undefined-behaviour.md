---
worth: later
where: src/server/core/console.cpp
added: 2026-11-17
---
# `raise access` crashes the server through undefined behaviour

The `raise access` console command writes through a plain null pointer (`char *p = nullptr; *p = 0;`). That is undefined behaviour and an optimizing compiler may drop or transform it. In practice GCC at -O2 turns it into a trap (SIGILL instead of SIGSEGV), so the command still crashes, just not through the access-violation path. A `volatile` null write is still UB; `raise(SIGSEGV)` on UNIX and `RaiseException(EXCEPTION_ACCESS_VIOLATION, ...)` on Windows would be defined.

Unknown that settles it: whether any crash-handler or dump test depends on this command producing an access violation specifically.

Found while documenting netxms/netxms-doc#83 in netxms-doc-antora.
