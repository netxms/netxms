/* 
** MySQL Database Driver
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: mysql.cpp
**
**/

#include "mysqldrv.h"


//
// Static data
//

static MUTEX m_hQueryLock;


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
   m_hQueryLock = MutexCreate();
   return TRUE;
}


//
// Unload handler
//

extern "C" void EXPORT DrvUnload(void)
{
   MutexDestroy(m_hQueryLock);
}


//
// Connect to database
//

extern "C" DB_HANDLE EXPORT DrvConnect(char *szHost, char *szLogin, char *szPassword, char *szDatabase)
{
   MYSQL *pConn;

   pConn = mysql_init(NULL);
   if (pConn == NULL)
      return 0;

   if (!mysql_connect(pConn, szHost, szLogin[0] == 0 ? NULL : szLogin,
                      (szPassword[0] == 0 || szLogin[0] == 0) ? NULL : szPassword))
   {
      mysql_close(pConn);
      return 0;
   }

   if (mysql_select_db(pConn, szDatabase) < 0)
   {
      mysql_close(pConn);
      return 0;
   }

   return (DB_HANDLE)pConn;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(DB_HANDLE hConn)
{
   mysql_close((MYSQL *)hConn);
}


//
// Perform non-SELECT query
//

extern "C" BOOL EXPORT DrvQuery(DB_HANDLE hConn, char *szQuery)
{
   BOOL bResult;

   MutexLock(m_hQueryLock, INFINITE);
   bResult = (mysql_query((MYSQL *)hConn, szQuery) == 0);
   MutexUnlock(m_hQueryLock);
   return bResult;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(DB_HANDLE hConn, char *szQuery)
{
   DB_RESULT dwResult = 0;

   MutexLock(m_hQueryLock, INFINITE);
   if (mysql_query((MYSQL *)hConn, szQuery) == 0)
      dwResult = (DB_RESULT)mysql_store_result((MYSQL *)hConn);
   MutexUnlock(m_hQueryLock);
   return dwResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn)
{
   MYSQL_ROW row;

   mysql_data_seek((MYSQL_RES *)hResult, iRow);
   row = mysql_fetch_row((MYSQL_RES *)hResult);
   return (row == NULL) ? NULL : row[iColumn];
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(DB_RESULT hResult)
{
   if (hResult == 0)
      return 0;
   return (int)mysql_num_rows((MYSQL_RES *)hResult);
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT hResult)
{
   mysql_free_result((MYSQL_RES *)hResult);
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif

