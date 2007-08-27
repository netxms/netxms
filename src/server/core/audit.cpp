/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: audit.cpp
**
**/

#include "nxcore.h"


//
// Static data
//

static DWORD m_dwRecordId = 1;


//
// Initalize audit log
//

void InitAuditLog(void)
{
	DB_RESULT hResult;

   hResult = DBSelect(g_hCoreDB, "SELECT max(record_id) FROM audit_log");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwRecordId = DBGetFieldULong(hResult, 0, 0) + 1;
      DBFreeResult(hResult);
   }
}


//
// Write audit record
//

void NXCORE_EXPORTABLE WriteAuditLog(TCHAR *pszSubsys, BOOL bSuccess, DWORD dwUserId,
												 DWORD dwObjectId, TCHAR *pszText)
{
	TCHAR *pszQuery, *pszEscText;

	pszEscText = EncodeSQLString(pszText);
	pszQuery = (TCHAR *)malloc((_tcslen(pszText) + 256) * sizeof(TCHAR));
	_stprintf(pszQuery, _T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,object_id,message) VALUES(%d,%d,'%s',%d,%d,%d,'%s')"),
		       m_dwRecordId++, time(NULL), pszSubsys, bSuccess ? 1 : 0, 
		       dwUserId, dwObjectId, pszEscText);
	free(pszEscText);
	QueueSQLRequest(pszQuery);
	free(pszQuery);
}
