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

   hResult = DBSelect(g_hCoreDB, _T("SELECT max(record_id) FROM audit_log"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwRecordId = DBGetFieldULong(hResult, 0, 0) + 1;
      DBFreeResult(hResult);
   }
}


//
// Handler for EnumerateSessions()
//

static void SendNewRecord(ClientSession *pSession, void *pArg)
{
   UPDATE_INFO *pUpdate;

   if (pSession->IsAuthenticated() && pSession->IsSubscribed(NXC_CHANNEL_AUDIT_LOG))
	{
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_AUDIT_RECORD;
      pUpdate->pData = new CSCPMessage((CSCPMessage *)pArg);
      pSession->QueueUpdate(pUpdate);
	}
}


//
// Write audit record
//

void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *pszSubsys, BOOL bSuccess, DWORD dwUserId,
                                     const TCHAR *pszWorkstation, DWORD dwObjectId,
                                     const TCHAR *pszFormat, ...)
{
	TCHAR *pszQuery, *pszText, *pszEscText;
	va_list args;
	size_t nBufSize;
	CSCPMessage msg;

	nBufSize = _tcslen(pszFormat) + NumChars(pszFormat, _T('%')) * 256 + 1;
	pszText = (TCHAR *)malloc(nBufSize * sizeof(TCHAR));
	va_start(args, pszFormat);
	_vsntprintf(pszText, nBufSize, pszFormat, args);
	va_end(args);

	pszEscText = EncodeSQLString(pszText);
	pszQuery = (TCHAR *)malloc((_tcslen(pszText) + 256) * sizeof(TCHAR));
	_stprintf(pszQuery, _T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,workstation,object_id,message) VALUES(%d,%d,'%s',%d,%d,'%s',%d,'%s')"),
		       m_dwRecordId++, time(NULL), pszSubsys, bSuccess ? 1 : 0, 
		       dwUserId, pszWorkstation, dwObjectId, pszEscText);
	free(pszEscText);
	QueueSQLRequest(pszQuery);
	free(pszQuery);

	msg.SetCode(CMD_AUDIT_RECORD);
	msg.SetVariable(VID_SUBSYSTEM, pszSubsys);
	msg.SetVariable(VID_SUCCESS_AUDIT, (WORD)bSuccess);
	msg.SetVariable(VID_USER_ID, dwUserId);
	msg.SetVariable(VID_WORKSTATION, pszWorkstation);
	msg.SetVariable(VID_OBJECT_ID, dwObjectId);
	msg.SetVariable(VID_MESSAGE, pszText);
	EnumerateClientSessions(SendNewRecord, &msg);

	free(pszText);
}
