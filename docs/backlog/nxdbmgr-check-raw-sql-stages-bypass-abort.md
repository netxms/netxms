---
worth: yes
where: src/server/tools/nxdbmgr/check.cpp
added: 2026-11-08
---
# nxdbmgr check stages still on SQLSelect/SQLQuery bypass the abort-and-rollback rule

Stages converted in PR #3701 use `CheckSelect()` / `CheckQuery()`, whose failure sets `s_checkAborted`
so no later stage runs and the transaction is rolled back. Many older stages still call plain
`SQLSelect` / `SQLQuery` (`grep -n "SQLSelect\|SQLQuery" src/server/tools/nxdbmgr/check.cpp`):
`CheckNodes` and its delete cascade, `CheckClusters`, the business service stages, the action part of
`CheckEPP`, and others. A failed query there makes the stage skip its work silently; on SQLite the
check then completes and may print "Database doesn't contain any errors". On PostgreSQL the poisoned
transaction makes the next converted stage abort, so the failure surfaces, but late.

Surfaced as pre-existing by the revmux review of PR #3701 (round `pr-3701/01-initial`). The fix is
the mechanical conversion of the remaining stages, with care for the ones that deliberately ignore
a failed delete in a cascade. The harness (`tests/nxdbmgr-check/run.sh`) already fails a check run
that prints "SQL query failed", so a broken query is caught there even before the conversion.
