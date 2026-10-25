# Database Schema

Upgrade procedures and the multi-branch backport procedure: [src/server/tools/nxdbmgr/CLAUDE.md](../src/server/tools/nxdbmgr/CLAUDE.md).

## Editing the Schema

- Edit `schema.in` — never the generated `dbschema_*.sql` files.
- After any schema change, add an upgrade procedure in `src/server/tools/nxdbmgr/upgrade_v<major>.cpp` and bump `DB_SCHEMA_VERSION_MINOR` in `include/netxmsdb.h`.
- A new server configuration variable goes in both places: a `CreateConfigParam(...)` call in the upgrade step (existing installs) and an `INSERT INTO config (...)` row in `setup.in` (fresh installs, which never run the upgrade path). Keep `setup.in` rows alphabetical within their section.
- Small enums stored as `CHAR(1)` store the ASCII digit of the value (`'0'`, `'1'`, `'2'`…), not letter mnemonics — so `DBGetFieldULong()` returns the value directly and save is `buf[0] = value + '0'`. Match `retention_type` / `m_retentionType` in `dcitem.cpp`.
- Do not use `DEFAULT` clauses in column definitions, neither here nor in nxdbmgr upgrade scripts. Oracle handles `DEFAULT` with `NOT NULL` differently from other databases, and the server code always provides explicit values on insert. The legacy `config` table predates this rule.

## SQL Type Macros

Two separate macro systems, not interchangeable:

| Context | Macro style | Example | Processed by |
|---------|-------------|---------|--------------|
| `schema.in` | CPP macros | `SQL_INT64`, `SQL_TEXT`, `SQL_BLOB` | C preprocessor, definitions from `database.in` |
| nxdbmgr upgrade procedures | `$SQL:` placeholders | `$SQL:INT64`, `$SQL:TEXT`, `$SQL:BLOB` | Runtime string replacement in nxdbmgr |

`$SQL:INT64` in `schema.in` is not substituted and breaks database initialization.

## Binary Data

There is no dedicated BLOB type. Binary values are stored in text columns (`varchar(N)` or `SQL_TEXT`) and transparently base64-encoded by libnxdb. Size a `varchar(N)` column for binary content as `ceil(maxBinaryBytes / 3) * 4`. Access it with the binary `DBBind` overload (`DB_SQLTYPE_VARCHAR` or `DB_SQLTYPE_TEXT` matching the column) and `DBGetFieldBinary`:

```cpp
DBBind(stmt, 1, DB_SQLTYPE_TEXT, imageData, imageLen, DB_BIND_STATIC);

size_t length;
BYTE *data = DBGetFieldBinary(result, row, col, &length);   // caller MemFree's

BYTE hash[32];
size_t actual = DBGetFieldBinary(result, row, col, hash, sizeof(hash));
```
