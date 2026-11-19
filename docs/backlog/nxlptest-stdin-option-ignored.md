---
worth: yes
where: src/tools/nxlptest/nxlptest.cpp:101
added: 2026-11-19
---
# nxlptest -i (read standard input) is accepted but does nothing

The help text lists `-i : Use standard input instead of file defined in parser`, and `getopt` accepts
`i`, but the switch has no `case 'i'`, so the option falls through to `default: break` and the parser
still reads the file from its definition (or `-f`). Either implement it or drop it from the help and the
option string.

The admin guide CLI reference (`reference/cli-tools.adoc`, `nxlptest` section, netxms-doc-antora) leaves
`-i` out; add it when it works.
