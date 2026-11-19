---
worth: yes
where: src/tools/nxcsum/nxcsum.cpp
added: 2026-11-19
---
# nxcsum crashes when the file argument is missing

`main()` checks `argc < 2` but then passes `argv[2]` to `PrintHash()`, so `nxcsum sha256` (valid method, no
file) dereferences `argv[2]` (null) instead of printing usage and exiting 1. Check `argc < 3`.
The admin guide (`reference/cli-tools.adoc`, `nxcsum`, netxms-doc-antora) documents exit status 1 only for a
missing or unsupported method.
