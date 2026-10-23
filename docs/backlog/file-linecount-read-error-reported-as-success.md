---
worth: yes
where: src/agent/core/sysinfo.cpp:820
added: 2026-10-24
---
# File.LineCount treats a read error as end of file

`H_FileLineCount` loops on `_read() > 0`, so a `-1` stops it exactly like EOF and the handler returns
`SYSINFO_RC_SUCCESS` with whatever it counted. `File.LineCount(/var/log)` on Linux opens the directory
fine, the first read fails with EISDIR, and the metric reports a valid count; an I/O error mid-file
returns a truncated count as success. Fix: after `_close(fd)`, return `SYSINFO_RC_ERROR` when
`bytes < 0`.

Surfaced by both agents of the revmux review of PR #3832 (empty-file fix, issue #3822). That PR makes
it slightly worse: the failing-directory case moves from 1 to 0, which now looks like a genuinely
empty file.
