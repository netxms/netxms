/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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

void InitAuditLog()
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

   if (pSession->isAuthenticated() && pSession->isSubscribed(NXC_CHANNEL_AUDIT_LOG))
	{
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_AUDIT_RECORD;
      pUpdate->pData = new CSCPMessage((CSCPMessage *)pArg);
      pSession->queueUpdate(pUpdate);
	}
}


//
// Write audit record
//

void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, BOOL isSuccess, DWORD userId,
                                     const TCHAR *workstation, DWORD objectId,
                                     const TCHAR *format, ...)
{
	String text, query;
	va_list args;
	CSCPMessage msg;

	va_start(args, format);
	text.addFormattedString(format, args);
	va_end(args);

	query.addFormattedString(_T("INSERT INTO audit_log (record_id,timestamp,subsystem,success,user_id,workstation,object_id,message) VALUES(%d,") TIME_T_FMT _T(",%s,%d,%d,%s,%d,%s)"),
		       m_dwRecordId++, time(NULL), (const TCHAR *)DBPrepareString(subsys), isSuccess ? 1 : 0, 
		       userId, (const TCHAR *)DBPrepareString(workstation), objectId, (const TCHAR *)DBPrepareString(text));
	QueueSQLRequest(query);

	msg.SetCode(CMD_AUDIT_RECORD);
	msg.SetVariable(VID_SUBSYSTEM, subsys);
	msg.SetVariable(VID_SUCCESS_AUDIT, (WORD)isSuccess);
	msg.SetVariable(VID_USER_ID, userId);
	msg.SetVariable(VID_WORKSTATION, workstation);
	msg.SetVariable(VID_OBJECT_ID, objectId);
	msg.SetVariable(VID_MESSAGE, (const TCHAR *)text);
	EnumerateClientSessions(SendNewRecord, &msg);
}
