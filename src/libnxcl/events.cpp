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
void ProcessEventLogRecords(NXCL_Session *pSession, NXCPMessage *pMsg)
{
   UINT32 i, dwNumRecords, dwId;
   NXC_EVENT event;
   int nOrder;

   dwNumRecords = pMsg->getFieldAsUInt32(VID_NUM_RECORDS);
   nOrder = (int)pMsg->getFieldAsUInt16(VID_RECORDS_ORDER);
   DebugPrintf(_T("ProcessEventLogRecords(): %d records in message, in %s order"),
               dwNumRecords, (nOrder == RECORD_ORDER_NORMAL) ? _T("normal") : _T("reversed"));
   for(i = 0, dwId = VID_EVENTLOG_MSG_BASE; i < dwNumRecords; i++)
   {
      event.qwEventId = pMsg->getFieldAsUInt64(dwId++);
      event.dwEventCode = pMsg->getFieldAsUInt32(dwId++);
      event.dwTimeStamp = pMsg->getFieldAsUInt32(dwId++);
      event.dwSourceId = pMsg->getFieldAsUInt32(dwId++);
      event.dwSeverity = pMsg->getFieldAsUInt16(dwId++);
      pMsg->getFieldAsString(dwId++, event.szMessage, MAX_EVENT_MSG_LENGTH);
      pMsg->getFieldAsString(dwId++, event.szUserTag, MAX_USERTAG_LENGTH);
		
		// Skip parameters
		UINT32 count = pMsg->getFieldAsUInt32(dwId++);
		dwId += count;

      // Call client's callback to handle new record
      pSession->callEventHandler(NXC_EVENT_NEW_ELOG_RECORD, nOrder, &event);
   }

   // Notify requestor thread if all messages was received
   if (pMsg->isEndOfSequence())
      pSession->CompleteSync(SYNC_EVENTS, RCC_SUCCESS);
}

/**
 * Synchronize event log
 * This function is NOT REENTRANT
 */
UINT32 LIBNXCL_EXPORTABLE NXCSyncEvents(NXC_SESSION hSession, UINT32 dwMaxRecords)
{
   NXCPMessage msg;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   ((NXCL_Session *)hSession)->PrepareForSync(SYNC_EVENTS);

   msg.setCode(CMD_GET_EVENTS);
   msg.setId(dwRqId);
   msg.setField(VID_MAX_RECORDS, dwMaxRecords);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = ((NXCL_Session *)hSession)->WaitForSync(SYNC_EVENTS, INFINITE);
   else
      ((NXCL_Session *)hSession)->UnlockSyncOp(SYNC_EVENTS);

   return dwRetCode;
}

/**
 * Send event to server. Event can be identified either by code or by name.
 * In latter case event code must be set to zero.
 */
UINT32 LIBNXCL_EXPORTABLE NXCSendEvent(NXC_SESSION hSession, UINT32 dwEventCode, const TCHAR *eventName,
                                      UINT32 dwObjectId, int iNumArgs, TCHAR **pArgList, TCHAR *pszUserTag)
{
   NXCPMessage msg;
   UINT32 dwRqId;
   int i;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_TRAP);
   msg.setId(dwRqId);
   msg.setField(VID_EVENT_CODE, dwEventCode);
   msg.setField(VID_EVENT_NAME, eventName);
   msg.setField(VID_OBJECT_ID, dwObjectId);
	msg.setField(VID_USER_TAG, CHECK_NULL_EX(pszUserTag));
   msg.setField(VID_NUM_ARGS, (WORD)iNumArgs);
   for(i = 0; i < iNumArgs; i++)
      msg.setField(VID_EVENT_ARG_BASE + i, pArgList[i]);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Process syslog records coming from server
//

void ProcessSyslogRecords(NXCL_Session *pSession, NXCPMessage *pMsg)
{
   UINT32 i, dwNumRecords, dwId;
   NXC_SYSLOG_RECORD rec;
   int nOrder;

   dwNumRecords = pMsg->getFieldAsUInt32(VID_NUM_RECORDS);
   nOrder = (int)pMsg->getFieldAsUInt16(VID_RECORDS_ORDER);
   DebugPrintf(_T("ProcessSyslogRecords(): %d records in message, in %s order"),
               dwNumRecords, (nOrder == RECORD_ORDER_NORMAL) ? _T("normal") : _T("reversed"));
   for(i = 0, dwId = VID_SYSLOG_MSG_BASE; i < dwNumRecords; i++)
   {
      rec.qwMsgId = pMsg->getFieldAsUInt64(dwId++);
      rec.dwTimeStamp = pMsg->getFieldAsUInt32(dwId++);
      rec.wFacility = pMsg->getFieldAsUInt16(dwId++);
      rec.wSeverity = pMsg->getFieldAsUInt16(dwId++);
      rec.dwSourceObject = pMsg->getFieldAsUInt32(dwId++);
      pMsg->getFieldAsString(dwId++, rec.szHost, MAX_SYSLOG_HOSTNAME_LEN);
      pMsg->getFieldAsString(dwId++, rec.szTag, MAX_SYSLOG_TAG_LEN);
      rec.pszText = pMsg->getFieldAsString(dwId++);

      // Call client's callback to handle new record
      pSession->callEventHandler(NXC_EVENT_NEW_SYSLOG_RECORD, nOrder, &rec);
      free(rec.pszText);
   }

   // Notify requestor thread if all messages was received
   if (pMsg->isEndOfSequence())
      pSession->CompleteSync(SYNC_SYSLOG, RCC_SUCCESS);
}

/**
 * Synchronize syslog
 * This function is NOT REENTRANT
 */
UINT32 LIBNXCL_EXPORTABLE NXCSyncSyslog(NXC_SESSION hSession, UINT32 dwMaxRecords)
{
   NXCPMessage msg;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   ((NXCL_Session *)hSession)->PrepareForSync(SYNC_SYSLOG);

   msg.setCode(CMD_GET_SYSLOG);
   msg.setId(dwRqId);
   msg.setField(VID_MAX_RECORDS, dwMaxRecords);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = ((NXCL_Session *)hSession)->WaitForSync(SYNC_SYSLOG, INFINITE);
   else
      ((NXCL_Session *)hSession)->UnlockSyncOp(SYNC_SYSLOG);

   return dwRetCode;
}
