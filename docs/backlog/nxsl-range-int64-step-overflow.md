---
worth: yes
where: src/libnxsl/range.cpp:131
added: 2026-10-25
---
# NXSL range() integer step advancement overflows int64

`NXSL_RangeClass::getNext()` computes `current + step` before comparing with `end`, which is signed
int64 overflow (UB) when the sum passes `INT64_MAX`/`INT64_MIN`. Where it wraps, the wrapped value passes
the direction check again: `range(1, 10, 9223372036854775807)` yields out-of-bounds values instead of
stopping after `1`, and `range(9223372036854775806, 9223372036854775807, 2)` iterates ~2^63 times.

Pre-existing (old code did `v += step` the same way); surfaced by revmux on the issue #3810 fix
(task `issue-3810`, round 01-initial). Fix: before adding, stop when the step would reach or cross `end`
(e.g. for `step > 0`, return `nullptr` when `current >= end - step`, computed overflow-safely), mirrored
for `step < 0`. Needs extreme script-supplied bounds and only stalls the offending script.
