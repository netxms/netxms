/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: events.cpp
**
**/

#include "libnxcl.h"

/**
 * Process event log records coming from server
 */
void ProcessEventLogRecords(NXCL_Session *pSession, CSCPMessage *pMsg)
{
   DWORD i, dwNumRecords, dwId;
   NXC_EVENT event;
   int nOrder;

   dwNumRecords = pMsg->GetVariableLong(VID_NUM_RECORDS);
   nOrder = (int)pMsg->GetVariableShort(VID_RECORDS_ORDER);
   DebugPrintf(_T("ProcessEventLogRecords(): %d records in message, in %s order"),
               dwNumRecords, (nOrder == RECORD_ORDER_NORMAL) ? _T("normal") : _T("reversed"));
   for(i = 0, dwId = VID_EVENTLOG_MSG_BASE; i < dwNumRecords; i++)
   {
      event.qwEventId = pMsg->GetVariableInt64(dwId++);
      event.dwEventCode = pMsg->GetVariableLong(dwId++);
      event.dwTimeStamp = pMsg->GetVariableLong(dwId++);
      event.dwSourceId = pMsg->GetVariableLong(dwId++);
      event.dwSeverity = pMsg->GetVariableShort(dwId++);
      pMsg->GetVariableStr(dwId++, event.szMessage, MAX_EVENT_MSG_LENGTH);
      pMsg->GetVariableStr(dwId++, event.szUserTag, MAX_USERTAG_LENGTH);
		
		// Skip parameters
		DWORD count = pMsg->GetVariableLong(dwId++);
		dwId += count;

      // Call client's callback to handle new record
      pSession->callEventHandler(NXC_EVENT_NEW_ELOG_RECORD, nOrder, &event);
   }

   // Notify requestor thread if all messages was received
   if (pMsg->IsEndOfSequence())
      pSession->CompleteSync(SYNC_EVENTS, RCC_SUCCESS);
}


//
// Synchronize event log
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCSyncEvents(NXC_SESSION hSession, DWORD dwMaxRecords)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   ((NXCL_Session *)hSession)->PrepareForSync(SYNC_EVENTS);

   msg.SetCode(CMD_GET_EVENTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MAX_RECORDS, dwMaxRecords);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = ((NXCL_Session *)hSession)->WaitForSync(SYNC_EVENTS, INFINITE);
   else
      ((NXCL_Session *)hSession)->UnlockSyncOp(SYNC_EVENTS);

   return dwRetCode;
}


//
// Send event to server
//

DWORD LIBNXCL_EXPORTABLE NXCSendEvent(NXC_SESSION hSession, DWORD dwEventCode, 
                                      DWORD dwObjectId, int iNumArgs, TCHAR **pArgList,
												  TCHAR *pszUserTag)
{
   CSCPMessage msg;
   DWORD dwRqId;
   int i;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_TRAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_EVENT_CODE, dwEventCode);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
	msg.SetVariable(VID_USER_TAG, CHECK_NULL_EX(pszUserTag));
   msg.SetVariable(VID_NUM_ARGS, (WORD)iNumArgs);
   for(i = 0; i < iNumArgs; i++)
      msg.SetVariable(VID_EVENT_ARG_BASE + i, pArgList[i]);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Process syslog records coming from server
//

void ProcessSyslogRecords(NXCL_Session *pSession, CSCPMessage *pMsg)
{
   DWORD i, dwNumRecords, dwId;
   NXC_SYSLOG_RECORD rec;
   int nOrder;

   dwNumRecords = pMsg->GetVariableLong(VID_NUM_RECORDS);
   nOrder = (int)pMsg->GetVariableShort(VID_RECORDS_ORDER);
   DebugPrintf(_T("ProcessSyslogRecords(): %d records in message, in %s order"),
               dwNumRecords, (nOrder == RECORD_ORDER_NORMAL) ? _T("normal") : _T("reversed"));
   for(i = 0, dwId = VID_SYSLOG_MSG_BASE; i < dwNumRecords; i++)
   {
      rec.qwMsgId = pMsg->GetVariableInt64(dwId++);
      rec.dwTimeStamp = pMsg->GetVariableLong(dwId++);
      rec.wFacility = pMsg->GetVariableShort(dwId++);
      rec.wSeverity = pMsg->GetVariableShort(dwId++);
      rec.dwSourceObject = pMsg->GetVariableLong(dwId++);
      pMsg->GetVariableStr(dwId++, rec.szHost, MAX_SYSLOG_HOSTNAME_LEN);
      pMsg->GetVariableStr(dwId++, rec.szTag, MAX_SYSLOG_TAG_LEN);
      rec.pszText = pMsg->GetVariableStr(dwId++);

      // Call client's callback to handle new record
      pSession->callEventHandler(NXC_EVENT_NEW_SYSLOG_RECORD, nOrder, &rec);
      free(rec.pszText);
   }

   // Notify requestor thread if all messages was received
   if (pMsg->IsEndOfSequence())
      pSession->CompleteSync(SYNC_SYSLOG, RCC_SUCCESS);
}


//
// Synchronize syslog
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCSyncSyslog(NXC_SESSION hSession, DWORD dwMaxRecords)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   ((NXCL_Session *)hSession)->PrepareForSync(SYNC_SYSLOG);

   msg.SetCode(CMD_GET_SYSLOG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MAX_RECORDS, dwMaxRecords);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = ((NXCL_Session *)hSession)->WaitForSync(SYNC_SYSLOG, INFINITE);
   else
      ((NXCL_Session *)hSession)->UnlockSyncOp(SYNC_SYSLOG);

   return dwRetCode;
}
