/* 
** PostgreSQL Database Driver
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
** $module: pgsql.cpp
**
**/

#include "pgsqldrv.h"


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
   return TRUE;
}


//
// Connect to database
//

extern "C" DB_HANDLE EXPORT DrvConnect(char *szHost, char *szLogin, char *szPassword, char *szDatabase)
{
	PGconn *pConn;
   
   pConn = PQsetdbLogin(szHost, NULL, NULL, NULL, szDatabase, szLogin, szPassword);

	if (PQstatus(pConn) == CONNECTION_BAD)
      return 0;

   return (DB_HANDLE)pConn;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(DB_HANDLE hConn)
{
   PQfinish((PGconn *)hConn);
}


//
// Perform non-SELECT query
//

extern "C" BOOL EXPORT DrvQuery(DB_HANDLE hConn, char *szQuery)
{
	PGresult	*pResult;

	pResult = PQexec((PGconn *)hConn, szQuery);

	if (pResult == NULL)
	{
		PQclear(pResult);
		return FALSE;
	}

	if (PQresultStatus(pResult) != PGRES_COMMAND_OK)
	{
		PQclear(pResult);
		return FALSE;
	}

	PQclear(pResult);
   return TRUE;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(DB_HANDLE hConn, char *szQuery)
{
	PGresult	*pResult;

	pResult = PQexec((PGconn *)hConn, szQuery);

	if (pResult == NULL)
	{
		PQclear(pResult);
		return NULL;
	}

	if (PQresultStatus(pResult) != PGRES_COMMAND_OK)
	{
		PQclear(pResult);
		return NULL;
	}

   return (DB_RESULT)pResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn)
{
   return PQgetvalue((PGresult *)hResult, iRow, iColumn);
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(DB_RESULT hResult)
{
   return PQntuples((PGresult *)hResult);
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT hResult)
{
   PQclear((PGresult *)hResult);
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

