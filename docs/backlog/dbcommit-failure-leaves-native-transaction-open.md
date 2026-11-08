---
worth: yes
where: src/db/libnxdb/session.cpp
added: 2026-11-08
---
# DBCommit failure can leave the native transaction open while libnxdb thinks it is closed

`DBCommit()` decrements `m_transactionLevel` before calling the driver's `Commit`, and does so even when
the commit fails. After a failed commit the level is 0, so a following `DBRollback()` returns false
without sending anything, and later statements run as if in autocommit mode. On SQLite a `COMMIT` that
hits `SQLITE_BUSY` fails but leaves the native transaction open: Codex reproduced COMMIT busy →
`in_transaction` still true → the next UPDATE (nxdbmgr's `RemoveDatabaseLock()`) stays inside that
transaction → closing the connection rolls it back and the database stays marked locked. PostgreSQL is
not affected (a failed `COMMIT` ends the transaction server side).

Surfaced while answering finding 3 of the review on PR #3701 (`nxdbmgr check` rollback after a failed
commit): adding `DBRollback()` in nxdbmgr cannot fix it, the transaction state has to be handled in
libnxdb or the driver (for example, roll back natively when the driver reports a commit failure with a
transaction still open, `sqlite3_get_autocommit()` on SQLite).
