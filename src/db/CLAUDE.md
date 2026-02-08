# Database Abstraction Layer (libnxdb)

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for C++ development guidelines.

## Overview

`libnxdb` provides database abstraction for NetXMS, supporting multiple database backends:
- PostgreSQL
- MySQL/MariaDB
- Oracle
- Microsoft SQL Server
- SQLite (for embedded use)
- ODBC (generic)

## Directory Structure

```
src/db/
├── libnxdb/          # Database abstraction library
│   ├── dbcp.cpp      # Connection pool
│   ├── drivers.cpp   # Driver management
│   ├── session.cpp   # Database session
│   ├── util.cpp      # Utilities
│   └── ...
└── dbdrv/            # Database drivers
    ├── db2/          # IBM DB2
    ├── informix/     # Informix
    ├── mariadb/      # MariaDB
    ├── mssql/        # Microsoft SQL Server
    ├── mysql/        # MySQL
    ├── odbc/         # ODBC generic
    ├── oracle/       # Oracle
    ├── pgsql/        # PostgreSQL
    └── sqlite/       # SQLite
```

## Key Classes

| Class | Description |
|-------|-------------|
| `DB_DRIVER` | Database driver handle |
| `DB_HANDLE` | Database connection handle |
| `DB_STATEMENT` | Prepared statement |
| `DB_RESULT` | Query result set |
| `DB_UNBUFFERED_RESULT` | Unbuffered result set |
| `DBConnectionPool` | Connection pool manager |

## Basic Usage

### Initialize and Connect

```cpp
#include <nxdbapi.h>

// Load driver
DB_DRIVER driver = DBLoadDriver(_T("pgsql.ddr"), nullptr, nullptr, nullptr);
if (driver == nullptr)
   return;

// Connect
TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
DB_HANDLE hdb = DBConnect(driver, _T("server"), _T("database"),
                          _T("user"), _T("password"), nullptr, errorText);
if (hdb == nullptr)
{
   _tprintf(_T("Connection failed: %s\n"), errorText);
   return;
}
```

### Simple Queries

```cpp
// Execute non-select query
if (!DBQuery(hdb, _T("UPDATE table SET column = 1")))
{
   // Handle error
}

// Execute select query
DB_RESULT result = DBSelect(hdb, _T("SELECT id, name FROM table"));
if (result != nullptr)
{
   int count = DBGetNumRows(result);
   for (int i = 0; i < count; i++)
   {
      int id = DBGetFieldLong(result, i, 0);
      TCHAR name[256];
      DBGetField(result, i, 1, name, 256);
   }
   DBFreeResult(result);
}
```

### Prepared Statements

```cpp
// Prepare statement
DB_STATEMENT stmt = DBPrepare(hdb, _T("INSERT INTO table(id, name) VALUES (?, ?)"));
if (stmt != nullptr)
{
   // Bind parameters
   DBBind(stmt, 1, DB_SQLTYPE_INTEGER, 42);
   DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, _T("Name"), DB_BIND_STATIC);

   // Execute
   if (!DBExecute(stmt))
   {
      // Handle error
   }

   DBFreeStatement(stmt);
}
```

### Batch Operations

```cpp
DB_STATEMENT stmt = DBPrepare(hdb, _T("INSERT INTO table(id, name) VALUES (?, ?)"));
if (stmt != nullptr)
{
   for (int i = 0; i < 100; i++)
   {
      DBBind(stmt, 1, DB_SQLTYPE_INTEGER, i);
      DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, names[i], DB_BIND_STATIC);
      DBExecute(stmt);
   }
   DBFreeStatement(stmt);
}
```

### Transactions

```cpp
if (DBBegin(hdb))
{
   bool success = true;

   if (!DBQuery(hdb, _T("UPDATE table1 SET x = 1")))
      success = false;

   if (success && !DBQuery(hdb, _T("UPDATE table2 SET y = 2")))
      success = false;

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
}
```

## Connection Pool

```cpp
// Initialize pool
DBConnectionPoolStartup(driver, server, database, user, password,
                        nullptr, 5, 20);  // min, max connections

// Get connection from pool
DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

// Use connection...

// Return to pool
DBConnectionPoolReleaseConnection(hdb);

// Shutdown pool
DBConnectionPoolShutdown();
```

## Parameter Binding Types

| Type | SQL Type |
|------|----------|
| `DB_SQLTYPE_INTEGER` | Integer (32-bit) |
| `DB_SQLTYPE_BIGINT` | Big integer (64-bit) |
| `DB_SQLTYPE_DOUBLE` | Double precision float |
| `DB_SQLTYPE_VARCHAR` | Variable-length string |
| `DB_SQLTYPE_TEXT` | Long text (CLOB) |
| `DB_SQLTYPE_BLOB` | Binary data |

## Bind Flags

| Flag | Description |
|------|-------------|
| `DB_BIND_STATIC` | String is static, don't copy |
| `DB_BIND_DYNAMIC` | String is dynamic, library will free |
| `DB_BIND_TRANSIENT` | String is temporary, copy it |

## Result Field Types

```cpp
// Get field as string
TCHAR buffer[256];
DBGetField(result, row, col, buffer, 256);

// Get field as integer types
int32_t i32 = DBGetFieldLong(result, row, col);
uint32_t u32 = DBGetFieldULong(result, row, col);
int64_t i64 = DBGetFieldInt64(result, row, col);
uint64_t u64 = DBGetFieldUInt64(result, row, col);

// Get field as double
double d = DBGetFieldDouble(result, row, col);

// Get field as IP address
InetAddress addr = DBGetFieldInetAddr(result, row, col);

// Get field as GUID
uuid guid = DBGetFieldGUID(result, row, col);
```

## Schema Management

Database schema is managed in `sql/` directory at repository root.

### Upgrade Procedures

Located in `src/server/tools/nxdbmgr/`:
- Schema upgrades are versioned
- Each version has upgrade SQL and C++ procedures

```cpp
// Example upgrade procedure
static bool H_UpgradeFromV42(int currVersion, int newVersion)
{
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD COLUMN new_field varchar(255)")));
   CHK_EXEC(SetSchemaVersion(newVersion));
   return true;
}
```

## Driver Development

Each database driver implements:

```cpp
// Driver entry point
extern "C" DBDRV_API_CALL DB_DRIVER_ENTRY_POINT

// Required functions
DBConnect, DBDisconnect
DBQuery, DBSelect
DBPrepare, DBBind, DBExecute
DBBegin, DBCommit, DBRollback
// ... etc
```

## Debugging

```bash
# Debug tags
nxlog_debug_tag(_T("db"), level, ...)       # Database operations
nxlog_debug_tag(_T("db.query"), level, ...) # Query logging
nxlog_debug_tag(_T("db.conn"), level, ...)  # Connection events
```

## Documentation

- [Data Dictionary](https://www.netxms.org/documentation/datadictionary-latest/)

## Related Components

- [libnetxms](../libnetxms/CLAUDE.md) - Core library
- [Server](../server/CLAUDE.md) - Primary database user
- [Server Tools](../server/tools/) - nxdbmgr for schema management
