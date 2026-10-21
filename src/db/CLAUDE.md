# Database Abstraction Layer (libnxdb)

> Shared guidelines: see [root CLAUDE.md](../../CLAUDE.md). Upgrade procedures and branch backports: [nxdbmgr CLAUDE.md](../server/tools/nxdbmgr/CLAUDE.md).

## Binary Data

There is no dedicated BLOB SQL type. Binary values are stored in text columns
(`varchar(N)` or `SQL_TEXT`/CLOB) and transparently base64-encoded by libnxdb.
Use the dedicated `DBBind` overload and `DBGetFieldBinary` accessors; the caller
picks `DB_SQLTYPE_VARCHAR` or `DB_SQLTYPE_TEXT` to match the underlying column.

```cpp
// Write
DBBind(stmt, 1, DB_SQLTYPE_TEXT, imageData, imageLen, DB_BIND_STATIC);

// Read (dynamic alloc; caller MemFree's)
size_t length;
BYTE *data = DBGetFieldBinary(result, row, col, &length);
// ...
MemFree(data);

// Read into fixed-size buffer (e.g. SHA-256 = 32 bytes)
BYTE hash[32];
size_t actual = DBGetFieldBinary(result, row, col, hash, sizeof(hash));
```

When sizing a `varchar(N)` column for binary content, allocate
`ceil(maxBinaryBytes / 3) * 4` characters for the base64 representation.

## Editing the Schema

Database schema lives in `sql/` at the repository root.

- Edit `sql/schema.in` — **never** the generated `sql/dbschema_*.sql` files (those are produced from `schema.in` and will be overwritten).
- After any schema change, add an upgrade procedure in `src/server/tools/nxdbmgr/upgrade_v<major>.cpp` and bump `DB_SCHEMA_VERSION_MINOR` in `include/netxmsdb.h` — see the nxdbmgr CLAUDE.md for versioning and backport rules.
- A new server **configuration variable** must be added in **both** places: a `CreateConfigParam(...)` call in the upgrade step (covers existing installs) **and** an `INSERT INTO config (...)` row in `sql/setup.in` (covers fresh installs, which never run the upgrade path). Keep `setup.in` rows alphabetical within their section.
- Small enums stored as `CHAR(1)` store the **ASCII digit** of the value (`'0'`, `'1'`, `'2'`…), not letter mnemonics — so `DBGetFieldULong()` returns the value directly and save is `buf[0] = value + '0'`. Match the pattern of `retention_type` / `m_retentionType` in `dcitem.cpp`.
- Size a C/C++ buffer that mirrors a `varchar(N)` column to exactly `N + 1`. The column is the canonical constraint; don't add speculative headroom (propose a schema change instead if the column is too narrow).

### SQL Type Macros — Two Different Systems

There are two separate macro systems for database-portable type names. Do NOT mix them up:

| Context | Macro style | Example | Processed by |
|---------|-------------|---------|--------------|
| `sql/schema.in` (initial schema) | CPP macros | `SQL_INT64`, `SQL_TEXT`, `SQL_BLOB` | C preprocessor (`cpp`), with definitions from `sql/database.in` |
| `nxdbmgr` upgrade scripts (C++ code) | `$SQL:` placeholders | `$SQL:INT64`, `$SQL:TEXT`, `$SQL:BLOB` | Runtime string replacement in `nxdbmgr` |

Using `$SQL:INT64` in `schema.in` will **not** be substituted and will cause SQL syntax errors at database initialization.

### No DEFAULT Values in Schema

Do not use `DEFAULT` clauses in column definitions — neither in `schema.in` nor in `nxdbmgr` upgrade scripts. Oracle handles `DEFAULT` with `NOT NULL` differently from other databases (requires `DEFAULT` before `NOT NULL`), and the server code always provides explicit values on insert. The only exception is the legacy `config` table which predates this rule.
