/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: direct.cpp
**
**/

#include "dbquery.h"

/**
 * Convert DB_RESULT to table object
 */
void DBResultToTable(DB_RESULT hResult, Table *table)
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
         table->setPreallocated(c, DBGetField(hResult, r, c, NULL, 0));
      }
   }
}

/**
 * Direct query - single value
 */
LONG H_DirectQuery(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   TCHAR dbid[MAX_DBID_LEN], query[256];
   AgentGetParameterArg(param, 1, dbid, MAX_DBID_LEN);
   AgentGetParameterArg(param, 2, query, 256);

   DB_HANDLE hdb = GetConnectionHandle(dbid);
   if (hdb == NULL)
   {
      AgentWriteDebugLog(4, _T("DBQUERY: H_DirectQuery: no connection handle for database %s"), dbid);
      return SYSINFO_RC_ERROR;
   }

   LONG rc = SYSINFO_RC_ERROR;
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      *value = 0;
      DBGetField(hResult, 0, 0, value, MAX_RESULT_LENGTH);
      DBFreeResult(hResult);
      rc = SYSINFO_RC_SUCCESS;
   }
   return rc;
}

/**
 * Direct query - table
 */
LONG H_DirectQueryTable(const TCHAR *param, const TCHAR *arg, Table *value)
{
   TCHAR dbid[MAX_DBID_LEN], query[256];
   AgentGetParameterArg(param, 1, dbid, MAX_DBID_LEN);
   AgentGetParameterArg(param, 2, query, 256);

   DB_HANDLE hdb = GetConnectionHandle(dbid);
   if (hdb == NULL)
   {
      AgentWriteDebugLog(4, _T("DBQUERY: H_DirectQueryTable: no connection handle for database %s"), dbid);
      return SYSINFO_RC_ERROR;
   }

   LONG rc = SYSINFO_RC_ERROR;
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      DBResultToTable(hResult, value);
      DBFreeResult(hResult);
      rc = SYSINFO_RC_SUCCESS;
   }
   return rc;
}
