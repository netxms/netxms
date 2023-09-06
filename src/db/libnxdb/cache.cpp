/*
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2008-2023 Raden Solutions
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
** File: cache.cpp
**
**/

#include "libnxdb.h"

#define DEBUG_TAG _T("db.cache")

/**
 * Open in memory database
 */
DB_HANDLE LIBNXDB_EXPORTABLE DBOpenInMemoryDatabase()
{
   DB_DRIVER drv = DBLoadDriver(_T("sqlite.ddr"), nullptr, nullptr, nullptr);
   if (drv == nullptr)
      return nullptr;

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_HANDLE hdb = DBConnect(drv, nullptr, _T(":memory:"), nullptr, nullptr, nullptr, errorText);
   if (hdb == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Cannot open in-memory database: %s"), errorText);
      DBUnloadDriver(drv);
   }

   DBQuery(hdb, _T("PRAGMA page_size=65536"));
   return hdb;
}

/**
 * Close in-memory database
 */
void LIBNXDB_EXPORTABLE DBCloseInMemoryDatabase(DB_HANDLE hdb)
{
   DB_DRIVER drv = hdb->m_driver;
   DBDisconnect(hdb);
   DBUnloadDriver(drv);
}

/**
 * Cache table
 */
bool LIBNXDB_EXPORTABLE DBCacheTable(DB_HANDLE cacheDB, DB_HANDLE sourceDB, const TCHAR *table, const TCHAR *indexColumn,
         const TCHAR *columns, const TCHAR * const *intColumns)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Caching table \"%s\""), table);

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT %s FROM %s"), columns, table);

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbufferedEx(sourceDB, query, errorText);
   if (hResult == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot read table %s for caching: %s"), table, errorText);
      return false;
   }

   StringBuffer createStatement = _T("CREATE TABLE ");
   createStatement.append(table);
   createStatement.append(_T(" ("));

   StringBuffer insertStatement = _T("INSERT INTO ");
   insertStatement.append(table);
   insertStatement.append(_T(" ("));

   int numColumns = DBGetColumnCount(hResult);
   for(int i = 0; i < numColumns; i++)
   {
      TCHAR name[256];
      if (!DBGetColumnName(hResult, i, name, 256))
      {
         DBFreeResult(hResult);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot get name of column %d of table %s"), i, table);
         return false;
      }
      if (i > 0)
      {
         createStatement.append(_T(", "));
         insertStatement.append(_T(", "));
      }
      createStatement.append(name);
      bool isInteger = false;
      if (intColumns != nullptr)
      {
         for(int c = 0; intColumns[c] != nullptr; c++)
         {
            if (!_tcsicmp(intColumns[c], name))
            {
               isInteger = true;
               break;
            }
         }
      }
      createStatement.append(isInteger ? _T(" integer") : _T(" varchar"));
      insertStatement.append(name);
   }
   if (indexColumn != nullptr)
   {
      createStatement.append(_T(", PRIMARY KEY("));
      createStatement.append(indexColumn);
      createStatement.append(_T(")) WITHOUT ROWID"));
   }
   else
   {
      createStatement.append(_T(')'));
   }

   if (!DBQueryEx(cacheDB, createStatement, errorText))
   {
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot create table %s in cache database: %s"), table, errorText);
      return false;
   }

   insertStatement.append(_T(") VALUES ("));
   for(int i = 0; i < numColumns; i++)
      insertStatement.append(_T("?,"));
   insertStatement.shrink();
   insertStatement.append(_T(')'));

   DB_STATEMENT hInsertStmt = DBPrepareEx(cacheDB, insertStatement, true, errorText);
   if (hInsertStmt == nullptr)
   {
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare insert statement for table %s in cache database: %s"), table, errorText);
      return false;
   }

   DBBegin(cacheDB);

   while(DBFetch(hResult))
   {
      for(int i = 0; i < numColumns; i++)
         DBBind(hInsertStmt, i + 1, DB_SQLTYPE_VARCHAR, DBGetField(hResult, i, nullptr, 0), DB_BIND_DYNAMIC);
      if (!DBExecuteEx(hInsertStmt, errorText))
      {
         DBRollback(cacheDB);
         DBFreeStatement(hInsertStmt);
         DBFreeResult(hResult);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot execute insert statement for table %s in cache database: %s"), table, errorText);
         return false;
      }
   }

   DBCommit(cacheDB);
   DBFreeStatement(hInsertStmt);
   DBFreeResult(hResult);
   return true;
}
