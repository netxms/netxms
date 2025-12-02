/*
** NetXMS database manager library
** Copyright (C) 2004-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: query.cpp
**
**/

#include "libnxdbmgr.h"

/**
 * Show query
 */
void LIBNXDBMGR_EXPORTABLE ShowQuery(const TCHAR *query)
{
   WriteToTerminalEx(_T("\x1b[1m>>> \x1b[32;1m%s\x1b[0m\n"), query);
}

/**
 * Execute SQL SELECT query and print error message on screen if query failed
 */
DB_RESULT LIBNXDBMGR_EXPORTABLE SQLSelect(const TCHAR *query)
{
   return SQLSelectEx(g_dbHandle, query);
}

/**
 * Execute SQL SELECT query and print error message on screen if query failed
 */
DB_RESULT LIBNXDBMGR_EXPORTABLE SQLSelectEx(DB_HANDLE hdb, const TCHAR *query)
{
   if (g_queryTrace)
      ShowQuery(query);

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_RESULT hResult = DBSelectEx(hdb, query, errorText);
   if (hResult == NULL)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, query);
   return hResult;
}

/**
 * Execute SQL SELECT query via DBSelectUnbuffered and print error message on screen if query failed
 */
DB_UNBUFFERED_RESULT LIBNXDBMGR_EXPORTABLE SQLSelectUnbuffered(const TCHAR *query)
{
   if (g_queryTrace)
      ShowQuery(query);

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(g_dbHandle, query, errorText);
   if (hResult == NULL)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, query);
   return hResult;
}

/**
 * Execute prepared statement and print error message on screen if query failed
 */
bool LIBNXDBMGR_EXPORTABLE SQLExecute(DB_STATEMENT hStmt)
{
   wchar_t errorText[DBDRV_MAX_ERROR_TEXT];

   if (g_queryTrace)
      ShowQuery(DBGetStatementSource(hStmt));

   bool result = DBExecuteEx(hStmt, errorText);
   if (!result)
      WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, DBGetStatementSource(hStmt));
   return result;
}

/**
 * Non-standard SQL data type names
 */
const wchar_t *g_sqlTypes[8][4] =
{
   { L"longtext",     L"text",           L"bigint",     L"longblob"       }, // MySQL
   { L"text",         L"varchar(4000)",  L"bigint",     L"bytea"          }, // PostgreSQL
   { L"varchar(max)", L"varchar(4000)",  L"bigint",     L"varbinary(max)" }, // Microsoft SQL
   { L"clob",         L"varchar(4000)",  L"number(20)", L"blob"           }, // Oracle
   { L"varchar",      L"varchar(4000)",  L"bigint",     L"blob"           }, // SQLite
   { L"long varchar", L"varchar(4000)",  L"bigint",     L"blob"           }, // DB/2
   { L"text",         L"lvarchar(4000)", L"bigint",     L"blob"           }, // Informix
   { L"text",         L"varchar(4000)",  L"bigint",     L"bytea"          }  // TimescaleDB
};

/**
 * Get SQL data type name for current database syntax
 */
const wchar_t LIBNXDBMGR_EXPORTABLE *GetSQLTypeName(int type)
{
   if ((type < 0) || (type >= 4))
      return L"ERROR";
   return g_sqlTypes[g_dbSyntax][type];
}

/**
 * Execute SQL query from formatted string and print error message on screen if query failed
 */
bool LIBNXDBMGR_EXPORTABLE SQLQueryFormatted(const wchar_t *query, ...)
{
   wchar_t formattedQuery[4096];
   va_list args;
   va_start(args, query);
   _vsntprintf(formattedQuery, 4096, query, args);
   va_end(args);
   return SQLQuery(formattedQuery);
}

/**
 * Execute SQL query and print error message on screen if query failed
 */
bool LIBNXDBMGR_EXPORTABLE SQLQuery(const wchar_t *query, bool showOutput)
{
   if (*query == 0)
      return true;

   StringBuffer realQuery(query);
   realQuery.trim();

   if (g_dbSyntax != DB_SYNTAX_UNKNOWN)
   {
      realQuery.replace(L"$SQL:BLOB", g_sqlTypes[g_dbSyntax][SQL_TYPE_BLOB]);
      realQuery.replace(L"$SQL:TEXT", g_sqlTypes[g_dbSyntax][SQL_TYPE_TEXT]);
      realQuery.replace(L"$SQL:TXT4K", g_sqlTypes[g_dbSyntax][SQL_TYPE_TEXT4K]);
      realQuery.replace(L"$SQL:INT64", g_sqlTypes[g_dbSyntax][SQL_TYPE_INT64]);
   }

   if (g_queryTrace)
      ShowQuery(realQuery);

   wchar_t errorText[DBDRV_MAX_ERROR_TEXT];
   bool success;
   if (!wcsnicmp(realQuery, L"SELECT ", 7))
   {
      DB_RESULT hResult = DBSelectEx(g_dbHandle, realQuery, errorText);
      if (hResult != nullptr)
      {
         if (showOutput)
         {
            Table table;
            DBResultToTable(hResult, &table);
            table.writeToTerminal();
         }
         DBFreeResult(hResult);
         success = true;
      }
      else
      {
         success = false;
      }
   }
   else
   {
      success = DBQueryEx(g_dbHandle, realQuery, errorText);
   }
   if (!success)
      WriteToTerminalEx(L"SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n", errorText, (const TCHAR *)realQuery);
   return success;
}

/**
 * Execute SQL batch
 */
bool LIBNXDBMGR_EXPORTABLE SQLBatch(const wchar_t *batchSource)
{
   StringBuffer batch(batchSource);
   if (g_dbSyntax != DB_SYNTAX_UNKNOWN)
   {
      batch.replace(L"$SQL:BLOB", g_sqlTypes[g_dbSyntax][SQL_TYPE_BLOB]);
      batch.replace(L"$SQL:TEXT", g_sqlTypes[g_dbSyntax][SQL_TYPE_TEXT]);
      batch.replace(L"$SQL:TXT4K", g_sqlTypes[g_dbSyntax][SQL_TYPE_TEXT4K]);
      batch.replace(L"$SQL:INT64", g_sqlTypes[g_dbSyntax][SQL_TYPE_INT64]);
   }

   bool success = true;

   TCHAR *pszQuery = batch.getBuffer();
   while(true)
   {
      TCHAR *ptr = _tcschr(pszQuery, _T('\n'));
      if (ptr != nullptr)
         *ptr = 0;
      if (!_tcscmp(pszQuery, _T("<END>")))
         break;

      TCHAR table[128], column[128];
      if (_stscanf(pszQuery, _T("ALTER TABLE %127s DROP COLUMN %127s"), table, column) == 2)
      {
         if (!DBDropColumn(g_dbHandle, table, column))
         {
            WriteToTerminalEx(_T("Cannot drop column \x1b[37;1m%s.%s\x1b[0m\n"), table, column);
            if (!g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
      }
      else
      {
         if (g_queryTrace)
            ShowQuery(pszQuery);

         TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
         if (!DBQueryEx(g_dbHandle, pszQuery, errorText))
         {
            WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, pszQuery);
            if (!g_ignoreErrors)
            {
               success = false;
               break;
            }
         }
      }

      if (ptr == nullptr)
         break;
      ptr++;
      pszQuery = ptr;
   }
   return success;
}
