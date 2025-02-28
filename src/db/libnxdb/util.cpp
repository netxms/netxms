/*
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: util.cpp
**
**/

#include "libnxdb.h"

/**
 * Utility query tracer
 */
static void (*s_queryTracer)(const TCHAR *query, bool failure, const TCHAR *errorMessage);

/**
 * Set tracer callback for utility functions
 */
void LIBNXDB_EXPORTABLE DBSetUtilityQueryTracer(void (*queryTracer)(const TCHAR *, bool, const TCHAR *))
{
   s_queryTracer = queryTracer;
}

/**
 * Execute utility query
 */
static bool ExecuteQuery(DB_HANDLE hdb, const TCHAR *query)
{
   if (s_queryTracer != nullptr)
      s_queryTracer(query, false, nullptr);
   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   bool success = DBQueryEx(hdb, query, errorText);
   if (!success && (s_queryTracer != nullptr))
      s_queryTracer(query, true, errorText);
   return success;
}

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, void *id, int cType, int sqlType, int allocType)
{
   bool exist = false;

   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT %s FROM %s WHERE %s=?"), idColumn, table, idColumn);

   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, sqlType, cType, id, allocType);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         exist = (DBGetNumRows(hResult) > 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   return exist;
}

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, uint32_t id)
{
   return IsDatabaseRecordExist(hdb, table, idColumn, &id, DB_CTYPE_UINT32, DB_SQLTYPE_INTEGER, DB_BIND_TRANSIENT);
}

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, uint64_t id)
{
   return IsDatabaseRecordExist(hdb, table, idColumn, &id, DB_CTYPE_UINT64, DB_SQLTYPE_BIGINT, DB_BIND_TRANSIENT);
}

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const uuid& id)
{
   return IsDatabaseRecordExist(hdb, table, idColumn, const_cast<TCHAR*>(id.toString().cstr()), DB_CTYPE_STRING, DB_SQLTYPE_VARCHAR, DB_BIND_TRANSIENT);
}

/**
 * Check if given record exists in database
 */
bool LIBNXDB_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const TCHAR *id)
{
   return IsDatabaseRecordExist(hdb, table, idColumn, const_cast<TCHAR*>(id), DB_CTYPE_STRING, DB_SQLTYPE_VARCHAR, DB_BIND_STATIC);
}

/**
 * Get database schema version
 * Will return false if version cannot be determined
 */
bool LIBNXDB_EXPORTABLE DBGetSchemaVersion(DB_HANDLE conn, int32_t *major, int32_t *minor)
{
	DB_RESULT hResult;

	*major = -1;
	*minor = -1;

   // Read legacy schema version from 'metadata' table, where it should
   // be stored starting from schema version 87
   // We ignore SQL error in this case, because table 'metadata'
   // may not exist in old schema versions
	int legacy = 0;
   hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='SchemaVersion'"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         legacy = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }

   // If database schema version is less than 87, version number
   // will be stored in 'config' table
   if (legacy == 0)
   {
      hResult = DBSelect(conn, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            legacy = DBGetFieldLong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
   }

   if (legacy == 0)
      return false;  // Cannot determine legacy version or schema is invalid

   if (legacy < 700)
   {
      // not upgraded to new major.minor versioning system
      *major = 0;
      *minor = legacy;
      return true;
   }

   hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='SchemaVersionMajor'"));
   if (hResult == nullptr)
      return false;  // DB error

   if (DBGetNumRows(hResult) > 0)
      *major = DBGetFieldLong(hResult, 0, 0);
   DBFreeResult(hResult);

   hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='SchemaVersionMinor'"));
   if (hResult == nullptr)
      return false;  // DB error

   if (DBGetNumRows(hResult) > 0)
      *minor = DBGetFieldLong(hResult, 0, 0);
   DBFreeResult(hResult);

   // Either version at -1 means schema is incorrect
   return ((*major != -1) && (*minor != -1));
}

/**
 * Custom syntax reader
 */
static bool (*s_syntaxReader)(DB_HANDLE, TCHAR *) = nullptr;

/**
 * Set custom syntax reader
 */
void LIBNXDB_EXPORTABLE DBSetSyntaxReader(bool (*reader)(DB_HANDLE, TCHAR *))
{
   s_syntaxReader = reader;
}

/**
 * Get database syntax
 */
int LIBNXDB_EXPORTABLE DBGetSyntax(DB_HANDLE conn, const TCHAR *fallback)
{
	TCHAR syntaxId[256] = _T("");
	bool read = false;

	if (s_syntaxReader != nullptr)
	{
	   read = s_syntaxReader(conn, syntaxId);
	}

	// Get syntax from metadata table
	if (!read)
	{
      DB_RESULT hResult = DBSelect(conn, _T("SELECT var_value FROM metadata WHERE var_name='Syntax'"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            DBGetField(hResult, 0, 0, syntaxId, sizeof(syntaxId) / sizeof(TCHAR));
            read = true;
         }
         DBFreeResult(hResult);
      }
	}

	// If database schema version is less than 87, syntax
	// will be stored in 'config' table, so try it
	if (!read)
	{
		DB_RESULT hResult = DBSelect(conn, _T("SELECT var_value FROM config WHERE var_name='DBSyntax'"));
		if (hResult != nullptr)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, syntaxId, sizeof(syntaxId) / sizeof(TCHAR));
            read = true;
			}
			DBFreeResult(hResult);
		}
	}

	// Use fallback if cannot read syntax from database
	if (!read)
      _tcslcpy(syntaxId, (fallback != nullptr) ? fallback : _T("UNKNOWN"), 256);

   int syntax;
   if (!_tcscmp(syntaxId, _T("MYSQL")))
   {
      syntax = DB_SYNTAX_MYSQL;
   }
   else if (!_tcscmp(syntaxId, _T("PGSQL")))
   {
      syntax = DB_SYNTAX_PGSQL;
   }
   else if (!_tcscmp(syntaxId, _T("MSSQL")))
   {
      syntax = DB_SYNTAX_MSSQL;
   }
   else if (!_tcscmp(syntaxId, _T("ORACLE")))
   {
      syntax = DB_SYNTAX_ORACLE;
   }
   else if (!_tcscmp(syntaxId, _T("SQLITE")))
   {
      syntax = DB_SYNTAX_SQLITE;
   }
   else if (!_tcscmp(syntaxId, _T("DB2")))
   {
      syntax = DB_SYNTAX_DB2;
   }
   else if (!_tcscmp(syntaxId, _T("TSDB")))
   {
      syntax = DB_SYNTAX_TSDB;
   }
   else
   {
		syntax = DB_SYNTAX_UNKNOWN;
   }

	return syntax;
}

/**
 * Get list of tables
 */
StringList LIBNXDB_EXPORTABLE *DBGetTableList(DB_HANDLE hdb)
{
   const TCHAR *query;
   switch(DBGetSyntax(hdb))
   {
      case DB_SYNTAX_DB2:
         query = _T("SELECT lower(name) FROM sysibm.systables WHERE type='T'");
         break;
      case DB_SYNTAX_INFORMIX:
         query = _T("SELECT lower(tabname) FROM informix.systables WHERE tabtype='T'");
         break;
      case DB_SYNTAX_MSSQL:
         query = _T("SELECT lower(name) FROM sysobjects WHERE xtype='U'");
         break;
      case DB_SYNTAX_MYSQL:
         query = _T("SELECT lower(table_name) FROM information_schema.tables WHERE table_type='BASE TABLE' AND table_schema=database()");
         break;
      case DB_SYNTAX_ORACLE:
         query = _T("SELECT lower(table_name) FROM user_tables");
         break;
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         query = _T("SELECT lower(table_name) FROM information_schema.tables WHERE table_catalog=current_database() AND table_schema=current_schema()");
         break;
      case DB_SYNTAX_SQLITE:
         query = _T("SELECT lower(name) FROM sqlite_master WHERE type='table'");
         break;
      default:    // Unsupported DB engine
         return nullptr;
   }

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return nullptr;

   StringList *tables = new StringList();
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      TCHAR name[256];
      DBGetField(hResult, i, 0, name, 256);
      tables->add(name);
   }
   DBFreeResult(hResult);
   return tables;
}

/**
 * Rename table
 */
bool LIBNXDB_EXPORTABLE DBRenameTable(DB_HANDLE hdb, const TCHAR *oldName, const TCHAR *newName)
{
   int syntax = DBGetSyntax(hdb);

   TCHAR query[1024];
   switch(syntax)
   {
      case DB_SYNTAX_DB2:
      case DB_SYNTAX_INFORMIX:
      case DB_SYNTAX_MYSQL:
         _sntprintf(query, 1024, _T("RENAME TABLE %s TO %s"), oldName, newName);
         break;
      case DB_SYNTAX_ORACLE:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         _sntprintf(query, 1024, _T("ALTER TABLE %s RENAME TO %s"), oldName, newName);
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(query, 1024, _T("EXEC sp_rename '%s','%s'"), oldName, newName);
         break;
      default:    // Unsupported DB engine
         return false;
   }
   return ExecuteQuery(hdb, query);
}

/**
 * SQLite ALTER TABLE operation
 */
enum SQLileAlterOp
{
   ALTER_COLUMN,
   RENAME_COLUMN,
   DROP_COLUMN,
   SET_NOT_NULL,
   REMOVE_NOT_NULL,
   SET_PRIMARY_KEY,
   DROP_PRIMARY_KEY
};

/**
 * Do SQLite schema change operation
 */
static bool SQLiteAlterTable(DB_HANDLE hdb, SQLileAlterOp operation, const TCHAR *table, const TCHAR *column, const TCHAR *operationData)
{
   // Starting from 3.25.0 SQLite supports RENAME COLUMN
   if (operation == RENAME_COLUMN)
   {
      bool useRenameColumn = false;
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT sqlite_version()"));
      if (hResult != nullptr)
      {
         TCHAR version[64] = _T("");
         DBGetField(hResult, 0, 0, version, 64);
         TCHAR *vminor = _tcschr(version, _T('.'));
         if (vminor != nullptr)
         {
            *vminor = 0;
            vminor++;
            TCHAR *vrelease = _tcschr(vminor, _T('.'));
            if (vrelease != nullptr)
            {
               *vrelease = 0;
               int vmajor = _tcstol(version, nullptr, 10);
               if ((vmajor > 3) || ((vmajor == 3) && (_tcstol(vminor, nullptr, 10) >= 25)))
               {
                  useRenameColumn = true;
               }
            }
         }
         DBFreeResult(hResult);
      }
      if (useRenameColumn)
      {
         StringBuffer query(_T("ALTER TABLE "));
         query.append(table);
         query.append(_T(" RENAME COLUMN "));
         query.append(column);
         query.append(_T(" TO "));
         query.append(operationData);
         return ExecuteQuery(hdb, query);
      }
   }

   StringBuffer query(_T("PRAGMA TABLE_INFO('"));
   query.append(table);
   query.append(_T("')"));
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return false;

   int numColumns = DBGetNumRows(hResult);

   // Intermediate buffers for SQLs
   StringBuffer originColumnList;
   StringBuffer targetColumnList;
   StringBuffer createList;

   // TABLE_INFO() columns
   TCHAR tabColName[128], tabColType[64], tabColNull[10], tabColDefault[128];
   for(int i = 0; i < numColumns; i++)
   {
      DBGetField(hResult, i, 1, tabColName, 128);
      DBGetField(hResult, i, 2, tabColType, 64);
      DBGetField(hResult, i, 3, tabColNull, 10);
      DBGetField(hResult, i, 4, tabColDefault, 128);
      if ((operation != DROP_COLUMN) || _tcsicmp(tabColName, column))
      {
         if (!originColumnList.isEmpty())
            originColumnList.append(_T(','));
         originColumnList.append(tabColName);

         if (!targetColumnList.isEmpty())
            targetColumnList.append(_T(','));
         if (!_tcsicmp(tabColName, column) && (operation == RENAME_COLUMN))
         {
            targetColumnList.append(operationData);
         }
         else
         {
            targetColumnList.append(tabColName);
         }

         if (!createList.isEmpty())
            createList.append(_T(','));
         if (!_tcsicmp(tabColName, column) && (operation == RENAME_COLUMN))
         {
            createList.append(operationData);
         }
         else
         {
            createList.append(tabColName);
         }
         createList.append(_T(' '));
         if (!_tcsicmp(tabColName, column) && (operation == ALTER_COLUMN))
         {
            createList.append(operationData);
         }
         else
         {
            createList.append(tabColType);
         }

         if (!_tcsicmp(tabColName, column))
         {
            if ((operation == SET_NOT_NULL) ||
                ((tabColNull[0] == _T('1')) && (operation != REMOVE_NOT_NULL)))
               createList.append(_T(" NOT NULL"));
         }
         else if (tabColNull[0] == _T('1'))
         {
            createList.append(_T(" NOT NULL"));
         }

         if (tabColDefault[0] != 0)
         {
            createList.append(_T(" DEFAULT "));
            createList.append(tabColDefault);
         }
      }
   }
   DBFreeResult(hResult);

   if (originColumnList.isEmpty())
      return false;

   // Primary key
   if (operation == SET_PRIMARY_KEY)
   {
      createList.append(_T(",PRIMARY KEY("));
      createList.append(operationData);
      createList.append(_T(')'));
   }
   else if (operation != DROP_PRIMARY_KEY)
   {
      query = _T("SELECT sql FROM sqlite_master WHERE tbl_name='");
      query.append(table);
      query.append(_T("' AND type='table'"));
      hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         TCHAR *sql = DBGetField(hResult, 0, 0, nullptr, 0);
         if (sql != nullptr)
         {
            _tcsupr(sql);
            TCHAR *p = _tcsstr(sql, _T("PRIMARY KEY"));
            if (p != nullptr)
            {
               TCHAR *s = _tcschr(p, _T(')'));
               if (s != nullptr)
               {
                  createList.append(_T(','));
                  // Replace old primary key name if it is renamed
                  if (operation == RENAME_COLUMN)
                  {
                     *s = 0;
                     StringList columnList = String::split(p + 12, _tcslen(p + 12), _T(","));
                     for (int i = 0; i < columnList.size(); i++)
                     {
                        if (!_tcsicmp(columnList.get(i), column))
                        {
                           columnList.replace(i, operationData);
                           break;
                        }
                     }
                     createList.append(_T("PRIMARY KEY("));
                     createList.appendPreallocated(columnList.join(_T(",")));
                     createList.append(_T(')'));
                  }
                  else
                  {
                     s++;
                     *s = 0;
                     createList.append(p);
                  }
               }
            }
            MemFree(sql);
         }
         DBFreeResult(hResult);
      }
   }

   // Save indexes and constraints
   StringList constraints;
   query = _T("SELECT sql FROM sqlite_master WHERE tbl_name='");
   query.append(table);
   query.append(_T("' AND type<>'table' AND sql<>''"));
   hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         constraints.addPreallocated(DBGetField(hResult, i, 0, nullptr, 0));
      }
      DBFreeResult(hResult);
   }

   bool success = false;

   query = _T("CREATE TABLE ");
   query.append(table);
   query.append(_T("__backup__ ("));
   query.append(createList);
   query.append(_T(')'));
   success = ExecuteQuery(hdb, query);
   if (success)
   {
      query = _T("INSERT INTO ");
      query.append(table);
      query.append(_T("__backup__ ("));
      query.append(targetColumnList);
      query.append(_T(") SELECT "));
      query.append(originColumnList);
      query.append(_T(" FROM "));
      query.append(table);
      success = ExecuteQuery(hdb, query);
   }

   if (success)
   {
      query = _T("DROP TABLE ");
      query.append(table);
      success = ExecuteQuery(hdb, query);
   }

   if (success)
   {
      query = _T("ALTER TABLE ");
      query.append(table);
      query.append(_T("__backup__ RENAME TO "));
      query.append(table);
      success = ExecuteQuery(hdb, query);
   }

   // Restore indexes and constraints
   if (success)
   {
      for(int i = 0; (i < constraints.size()) && success; i++)
         success = ExecuteQuery(hdb, constraints.get(i));
   }

   return success;
}

/**
 * Get column data type for given column (MS SQL and PostgreSQL version)
 */
static bool GetColumnDataType_MSSQL_PGSQL(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, TCHAR *definition, size_t len)
{
   bool success = false;
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT data_type,character_maximum_length,numeric_precision,numeric_scale FROM information_schema.columns WHERE table_name='%s' AND column_name='%s'"), table, column);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         TCHAR type[128];
         DBGetField(hResult, 0, 0, type, 128);
         if (!_tcsicmp(type, _T("decimal")) || !_tcsicmp(type, _T("numeric")))
         {
            int p = DBGetFieldLong(hResult, 0, 2);
            if (p > 0)
            {
               TCHAR type[128];
               DBGetField(hResult, 0, 0, type, 128);
               int s = DBGetFieldLong(hResult, 0, 3);
               if (s > 0)
               {
                  _sntprintf(definition, len, _T("%s(%d,%d)"), type, p, s);
               }
               else
               {
                  _sntprintf(definition, len, _T("%s(%d)"), type, p);
               }
            }
            else
            {
               _tcslcpy(definition, type, len);
            }
         }
         else if (!_tcsicmp(type, _T("varchar")) || !_tcsicmp(type, _T("nvarchar")) ||
                  !_tcsicmp(type, _T("char")) || !_tcsicmp(type, _T("nchar")) ||
                  !_tcsicmp(type, _T("character")) || !_tcsicmp(type, _T("character varying")))
         {
            int ch = DBGetFieldLong(hResult, 0, 1);
            if ((ch < INT_MAX) && (ch > 0))
            {
               _sntprintf(definition, len, _T("%s(%d)"), type, ch);
            }
            else
            {
               _tcslcpy(definition, type, len);
            }
         }
         else
         {
            _tcslcpy(definition, type, len);
         }
         success = true;
      }
      DBFreeResult(hResult);
   }
   return success;
}

/**
 * Get column data type for given column (MySQL version)
 */
static bool GetColumnDataType_MYSQL(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, TCHAR *definition, size_t len)
{
   bool success = false;
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT column_type FROM information_schema.columns WHERE table_schema=database() AND table_name='%s' AND column_name='%s'"), table, column);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         DBGetField(hResult, 0, 0, definition, len);
         success = true;
      }
      DBFreeResult(hResult);
   }
   return success;
}

/**
 * Get column data type for given column (SQLite version)
 */
static bool GetColumnDataType_SQLite(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, TCHAR *definition, size_t len)
{
   bool success = false;
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("PRAGMA TABLE_INFO('%s')"), table);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && !success; i++)
      {
         TCHAR curr[256];
         DBGetField(hResult, i, 1, curr, 256);
         if (!_tcsicmp(curr, column))
         {
            DBGetField(hResult, i, 2, definition, len);
            success = true;
         }
      }
      DBFreeResult(hResult);
   }
   return success;
}

/**
 * Get column data type for given column (Oracle version)
 */
static bool GetColumnDataType_Oracle(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, TCHAR *definition, size_t len)
{
   bool success = false;
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT data_type,data_length,data_precision,data_scale FROM user_tab_columns WHERE table_name=UPPER('%s') AND column_name=UPPER('%s')"), table, column);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         TCHAR type[128];
         DBGetField(hResult, 0, 0, type, 128);
         if (!_tcsicmp(type, _T("number")))
         {
            int p = DBGetFieldLong(hResult, 0, 2);
            if (p > 0)
            {
               TCHAR type[128];
               DBGetField(hResult, 0, 0, type, 128);
               int s = DBGetFieldLong(hResult, 0, 3);
               if (s > 0)
               {
                  _sntprintf(definition, len, _T("%s(%d,%d)"), type, p, s);
               }
               else if (p == 22)
               {
                  _tcslcpy(definition, _T("INTEGER"), len);
               }
               else
               {
                  _sntprintf(definition, len, _T("%s(%d)"), type, p);
               }
            }
            else
            {
               _tcslcpy(definition, type, len);
            }
         }
         else if (!_tcsicmp(type, _T("varchar2")) || !_tcsicmp(type, _T("nvarchar2")) ||
                  !_tcsicmp(type, _T("char")) || !_tcsicmp(type, _T("nchar")))
         {
            int ch = DBGetFieldLong(hResult, 0, 1);
            if ((ch < INT_MAX) && (ch > 0))
            {
               _sntprintf(definition, len, _T("%s(%d)"), type, ch);
            }
            else
            {
               _tcslcpy(definition, type, len);
            }
         }
         else
         {
            _tcslcpy(definition, type, len);
         }
         _tcslwr(definition);
         success = true;
      }
      DBFreeResult(hResult);
   }
   return success;
}

/**
 * Get column data type for given column
 */
bool LIBNXDB_EXPORTABLE DBGetColumnDataType(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, TCHAR *definition, size_t len)
{
   bool success;
   switch(DBGetSyntax(hdb))
   {
      case DB_SYNTAX_MSSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         success = GetColumnDataType_MSSQL_PGSQL(hdb, table, column, definition, len);
         break;
      case DB_SYNTAX_MYSQL:
         success = GetColumnDataType_MYSQL(hdb, table, column, definition, len);
         break;
      case DB_SYNTAX_ORACLE:
         success = GetColumnDataType_Oracle(hdb, table, column, definition, len);
         break;
      case DB_SYNTAX_SQLITE:
         success = GetColumnDataType_SQLite(hdb, table, column, definition, len);
         break;
      default:
         success = false;
         break;
   }
   return success;
}

/**
 * Drop index from table
 */
bool LIBNXDB_EXPORTABLE DBDropIndex(DB_HANDLE hdb, const TCHAR *table, const TCHAR *index)
{
   TCHAR query[1024];
   switch(DBGetSyntax(hdb))
   {
      case DB_SYNTAX_MSSQL:
         _sntprintf(query, 1024, _T("DROP INDEX %s ON %s"), index, table);
         break;
      case DB_SYNTAX_MYSQL:
         _sntprintf(query, 1024, _T("DROP INDEX `%s` ON `%s`"), index, table);
         break;
      default:
         _sntprintf(query, 1024, _T("DROP INDEX %s"), index);
         break;
   }
   return ExecuteQuery(hdb, query);
}

/**
 * Drop primary key from table
 */
bool LIBNXDB_EXPORTABLE DBDropPrimaryKey(DB_HANDLE hdb, const TCHAR *table)
{
   int syntax = DBGetSyntax(hdb);

   TCHAR query[1024];
   DB_RESULT hResult;
   bool success;

   switch(syntax)
   {
      case DB_SYNTAX_DB2:
      case DB_SYNTAX_INFORMIX:
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_ORACLE:
         _sntprintf(query, 1024, _T("ALTER TABLE %s DROP PRIMARY KEY"), table);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         _sntprintf(query, 1024, _T("SELECT constraint_name FROM information_schema.table_constraints WHERE table_schema='public' AND constraint_type='PRIMARY KEY' AND table_name='%s'"), table);
         hResult = DBSelect(hdb, query);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR objName[512];
               DBGetField(hResult, 0, 0, objName, 512);
               _sntprintf(query, 1024, _T("ALTER TABLE %s DROP CONSTRAINT %s"), table, objName);
               success = ExecuteQuery(hdb, query);
            }
            else
            {
               success = true; // No PK to drop
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(query, 1024, _T("SELECT name FROM sysobjects WHERE xtype='PK' AND parent_obj=OBJECT_ID('%s')"), table);
         hResult = DBSelect(hdb, query);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR objName[512];
               DBGetField(hResult, 0, 0, objName, 512);
               _sntprintf(query, 1024, _T("ALTER TABLE %s DROP CONSTRAINT %s"), table, objName);
               success = ExecuteQuery(hdb, query);
            }
            else
            {
               success = true; // No PK to drop
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         break;
      case DB_SYNTAX_SQLITE:
         success = SQLiteAlterTable(hdb, DROP_PRIMARY_KEY, table, _T(""), _T(""));
         break;
      default:    // Unsupported DB engine
         success = false;
         break;
   }

   if ((syntax == DB_SYNTAX_DB2) && success)
   {
      _sntprintf(query, 1024, _T("CALL Sysproc.admin_cmd('REORG TABLE %s')"), table);
      success = ExecuteQuery(hdb, query);
   }
   return success;
}

/**
 * Add primary key to table. Columns should be passed as comma separated list.
 */
bool LIBNXDB_EXPORTABLE DBAddPrimaryKey(DB_HANDLE hdb, const TCHAR *table, const TCHAR *columns)
{
   int syntax = DBGetSyntax(hdb);

   TCHAR query[1024];
   bool success;
   switch(syntax)
   {
      case DB_SYNTAX_INFORMIX:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ADD CONSTRAINT PRIMARY KEY (%s)"), table, columns);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_DB2:
      case DB_SYNTAX_MSSQL:
      case DB_SYNTAX_ORACLE:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ADD CONSTRAINT pk_%s PRIMARY KEY (%s)"), table, table, columns);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ADD PRIMARY KEY (%s)"), table, columns);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_SQLITE:
         success = SQLiteAlterTable(hdb, SET_PRIMARY_KEY, table, _T(""), columns);
         break;
      default:    // Unsupported DB engine
         success = false;
         break;
   }

   if ((syntax == DB_SYNTAX_DB2) && success)
   {
      _sntprintf(query, 1024, _T("CALL Sysproc.admin_cmd('REORG TABLE %s')"), table);
      success = DBQuery(hdb, query);
   }
   return success;
}

/**
 * Remove NOT NULL constraint from column
 */
bool LIBNXDB_EXPORTABLE DBRemoveNotNullConstraint(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column)
{
   bool success;
   TCHAR query[1024], type[128];

   switch(DBGetSyntax(hdb))
   {
      case DB_SYNTAX_DB2:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s DROP NOT NULL"), table, column);
         success = DBQuery(hdb, query);
         if (success)
         {
            _sntprintf(query, 1024, _T("CALL Sysproc.admin_cmd('REORG TABLE %s')"), table);
            success = DBQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_MSSQL:
         success = GetColumnDataType_MSSQL_PGSQL(hdb, table, column, type, 128);
         if (success)
         {
            _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s %s NULL"), table, column, type);
            success = DBQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_MYSQL:
         success = GetColumnDataType_MYSQL(hdb, table, column, type, 128);
         if (success)
         {
            _sntprintf(query, 1024, _T("ALTER TABLE %s MODIFY %s %s"), table, column, type);
            success = DBQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_ORACLE:
         _sntprintf(query, 1024, _T("DECLARE already_null EXCEPTION; ")
                                 _T("PRAGMA EXCEPTION_INIT(already_null, -1451); ")
                                 _T("BEGIN EXECUTE IMMEDIATE 'ALTER TABLE %s MODIFY %s null'; ")
                                 _T("EXCEPTION WHEN already_null THEN null; END;"), table, column);
         success = DBQuery(hdb, query);
         break;
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s DROP NOT NULL"), table, column);
         success = DBQuery(hdb, query);
         break;
      case DB_SYNTAX_SQLITE:
         success = SQLiteAlterTable(hdb, REMOVE_NOT_NULL, table, column, _T(""));
         break;
      default:
         success = false;
         break;
   }

   return success;
}

/**
 * Set NOT NULL constraint on column
 */
bool LIBNXDB_EXPORTABLE DBSetNotNullConstraint(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column)
{
   bool success;
   TCHAR query[1024], type[128];

   switch(DBGetSyntax(hdb))
   {
      case DB_SYNTAX_DB2:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s SET NOT NULL"), table, column);
         success = ExecuteQuery(hdb, query);
         if (success)
         {
            _sntprintf(query, 1024, _T("CALL Sysproc.admin_cmd('REORG TABLE %s')"), table);
            success = ExecuteQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_MSSQL:
         success = GetColumnDataType_MSSQL_PGSQL(hdb, table, column, type, 128);
         if (success)
         {
            _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s %s NOT NULL"), table, column, type);
            success = ExecuteQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_MYSQL:
         success = GetColumnDataType_MYSQL(hdb, table, column, type, 128);
         if (success)
         {
            _sntprintf(query, 1024, _T("ALTER TABLE %s MODIFY %s %s NOT NULL"), table, column, type);
            success = ExecuteQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_ORACLE:
         _sntprintf(query, 1024, _T("DECLARE already_not_null EXCEPTION; ")
                                 _T("PRAGMA EXCEPTION_INIT(already_not_null, -1442); ")
                                 _T("BEGIN EXECUTE IMMEDIATE 'ALTER TABLE %s MODIFY %s NOT NULL'; ")
                                 _T("EXCEPTION WHEN already_not_null THEN null; END;"), table, column);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s SET NOT NULL"), table, column);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_SQLITE:
         success = SQLiteAlterTable(hdb, SET_NOT_NULL, table, column, _T(""));
         break;
      default:
         success = false;
         break;
   }

   return success;
}

/**
 * Resize varchar column
 */
bool LIBNXDB_EXPORTABLE DBResizeColumn(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column, int newSize, bool nullable)
{
   int syntax = DBGetSyntax(hdb);

   if (syntax == DB_SYNTAX_SQLITE)
   {
      TCHAR newType[64];
      _sntprintf(newType, 64, _T("varchar(%d)"), newSize);
      return SQLiteAlterTable(hdb, ALTER_COLUMN, table, column, newType);
   }

   TCHAR query[1024];
   switch(syntax)
   {
      case DB_SYNTAX_DB2:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s SET DATA TYPE varchar(%d)"), table, column, newSize);
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s varchar(%d) %s NULL"), table, column, newSize, nullable ? _T("") : _T("NOT"));
         break;
      case DB_SYNTAX_MYSQL:
         _sntprintf(query, 1024, _T("ALTER TABLE %s MODIFY COLUMN %s varchar(%d) %s NULL"), table, column, newSize, nullable ? _T("") : _T("NOT"));
         break;
      case DB_SYNTAX_ORACLE:
         _sntprintf(query, 1024, _T("ALTER TABLE %s MODIFY %s varchar(%d)"), table, column, newSize);
         break;
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         _sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s TYPE varchar(%d)"), table, column, newSize);
         break;
      default:
         query[0] = 0;
         break;
   }

   return (query[0] != 0) ? ExecuteQuery(hdb, query) : true;
}

/**
 * Drop column from the table
 */
bool LIBNXDB_EXPORTABLE DBDropColumn(DB_HANDLE hdb, const TCHAR *table, const TCHAR *column)
{
   int syntax = DBGetSyntax(hdb);

   TCHAR query[1024];
   bool success = false;
   if (syntax != DB_SYNTAX_SQLITE)
   {
      _sntprintf(query, 1024, _T("ALTER TABLE %s DROP COLUMN %s"), table, column);
      success = ExecuteQuery(hdb, query);
      if (syntax == DB_SYNTAX_DB2)
      {
         _sntprintf(query, 1024, _T("CALL Sysproc.admin_cmd('REORG TABLE %s')"), table);
         success = ExecuteQuery(hdb, query);
      }
   }
   else
   {
      success = SQLiteAlterTable(hdb, DROP_COLUMN, table, column, _T(""));
   }

   return success;
}

/**
 * Rename column
 */
bool LIBNXDB_EXPORTABLE DBRenameColumn(DB_HANDLE hdb, const TCHAR *tableName, const TCHAR *oldName, const TCHAR *newName)
{
   bool success;
   TCHAR query[1024], type[128];

   switch(DBGetSyntax(hdb))
   {
      case DB_SYNTAX_DB2:
         _sntprintf(query, 1024, _T("ALTER TABLE %s RENAME COLUMN %s TO %s"), tableName, oldName, newName);
         success = ExecuteQuery(hdb, query);
         if (success)
         {
            _sntprintf(query, 1024, _T("CALL Sysproc.admin_cmd('REORG TABLE %s')"), tableName);
            success = ExecuteQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(query, 1024, _T("EXEC sp_rename '%s.%s', '%s', 'COLUMN'"), tableName, oldName, newName);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_MYSQL:
         success = GetColumnDataType_MYSQL(hdb, tableName, oldName, type, 128);
         if (success)
         {
            _sntprintf(query, 1024, _T("ALTER TABLE %s CHANGE %s %s %s"), tableName, oldName, newName, type);
            success = ExecuteQuery(hdb, query);
         }
         break;
      case DB_SYNTAX_ORACLE:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_TSDB:
         _sntprintf(query, 1024, _T("ALTER TABLE %s RENAME COLUMN %s TO %s"), tableName, oldName, newName);
         success = ExecuteQuery(hdb, query);
         break;
      case DB_SYNTAX_SQLITE:
         success = SQLiteAlterTable(hdb, RENAME_COLUMN, tableName, oldName, newName);
         break;
      default:
         success = false;
         break;
   }

   return success;
}

/**
 * Convert DB_RESULT to table object
 */
void LIBNXDB_EXPORTABLE DBResultToTable(DB_RESULT hResult, Table *table)
{
   int numColumns = DBGetColumnCount(hResult);
   for(int c = 0; c < numColumns; c++)
   {
      TCHAR name[64];
      if (!DBGetColumnName(hResult, c, name, 64))
         _sntprintf(name, 64, _T("COL_%d"), c + 1);
      table->addColumn(name);
   }

   int numRows = DBGetNumRows(hResult);
   for(int r = 0; r < numRows; r++)
   {
      table->addRow();
      for(int c = 0; c < numColumns; c++)
      {
         table->setPreallocated(c, DBGetField(hResult, r, c, nullptr, 0));
      }
   }
}
