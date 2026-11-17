---
worth: yes
where: src/db/libnxdb/session.cpp
added: 2026-11-08
---
# DBCommit failure can leave the native transaction open while libnxdb thinks it is closed

`DBCommit()` decrements `m_transactionLevel` before calling the driver's outermost `Commit`, even when
the commit fails. After failure the level is 0, so a following `DBRollback()` returns false without
calling the driver. The library has released its transaction locks, but the native transaction may
still be open. Subsequent statements can therefore remain inside the failed transaction while the
library treats it as closed.

A native SQLite deferred-constraint test reproduced a failed `COMMIT` with `in_transaction` still
true: the next UPDATE setting `DBLockStatus` to `UNLOCKED` remained inside that transaction, and
rolling it back restored the locked value. This synthetic test demonstrates native transaction
behavior; it does not require such a constraint in the NetXMS schema. A previous native SQLite busy
commit test reproduced the same state. Whether a failed commit ends the transaction depends on the
backend and error; a lost connection can also leave the durable commit outcome unknown.

Surfaced while answering finding 3 of both reviews on PR #3701 (`nxdbmgr check` rollback after a
failed commit). Adding a caller `DBRollback()` cannot fix it. The recovery policy belongs in libnxdb
and its drivers, before the failed outer commit releases the transaction lock.

Driver audit, checked on 2026-11-17:

| Driver | Behavior when the native commit fails |
|--------|--------------------------------------|
| [MSSQL](../../src/db/dbdrv/mssql/mssql.cpp), [ODBC](../../src/db/dbdrv/odbc/odbc.cpp), [DB2](../../src/db/dbdrv/db2/db2.cpp), [Informix](../../src/db/dbdrv/informix/informix.cpp) | `Commit` calls `SQLEndTran(SQL_COMMIT)`, then unconditionally enables `SQL_AUTOCOMMIT_ON`, before returning the commit status. It also ignores the autocommit-setting result. Recovery must resolve the failed transaction before enabling autocommit; a library rollback after the driver returns is too late to enforce that ordering. |
| [Oracle](../../src/db/dbdrv/oracle/oracle.cpp) | `Commit` resets native `nTransLevel` only after successful `OCITransCommit`. On failure the native level remains positive, although libnxdb has already dropped its own level to 0. Native `Rollback` can still attempt `OCITransRollback`. |
| [SQLite](../../src/db/dbdrv/sqlite/sqlite.cpp), [MySQL](../../src/db/dbdrv/mysql/mysql.cpp) | `Commit` and `Rollback` send SQL `COMMIT` and `ROLLBACK` through `QueryInternal`; there is no failed-commit cleanup in `Commit`. |
| [PostgreSQL](../../src/db/dbdrv/pgsql/pgsql.cpp) | `Commit` sends SQL `COMMIT` through `UnsafeQuery` and returns its status. It does not inspect the resulting native transaction state or perform recovery. |

The intended contract is: a failed outermost commit attempts native rollback before returning false
and releasing the transaction mutex, resolving the native transaction and autocommit state before
the connection can be reused. Preserve nested transaction levels and mutex accounting. Do not retry
the commit or turn cleanup success into commit success. If rollback fails or the connection's state
cannot be confirmed, treat the connection as unusable until it has been recovered or replaced;
issuing an unlock UPDATE on it is not established to be safe. Reconnecting cannot determine whether
an ambiguous commit was durable.

A library-only rollback patch does not provide uniform recovery: the four ODBC-family drivers must
also change their autocommit ordering. Verify failed outer commit and failed rollback paths with a
deterministic driver test, including rollback-before-unlock/autocommit ordering, no native commit for
an inner nesting level, and refusal to reuse an unresolved connection. A native SQLite transaction
left open by a failed commit is a useful integration case (its state can be inspected with
`sqlite3_get_autocommit()`).
