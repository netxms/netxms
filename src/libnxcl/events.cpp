/* $Id: events.cpp,v 1.22 2008-01-28 20:23:46 victor Exp $ */
/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: events.cpp
**
**/

#include "libnxcl.h"


//
// Process events coming from server
//

void ProcessEvent(NXCL_Session *pSession, CSCPMessage *pMsg, CSCP_MESSAGE *pRawMsg)
{
   WORD wCode;
#ifdef UNICODE
   WCHAR *pSrc, *pDst;
#endif

   wCode = (pMsg != NULL) ? pMsg->GetCode() : pRawMsg->wCode;

   switch(wCode)
   {
      case CMD_EVENT_LIST_END:
         pSession->CompleteSync(SYNC_EVENTS, RCC_SUCCESS);
         break;
      case CMD_EVENT:
         if (pRawMsg != NULL)    // We should receive events as raw data
         {
            NXC_EVENT event;

            // Fill event structure with values from message
            event.dwEventCode = ntohl(((NXC_EVENT *)pRawMsg->df)->dwEventCode);
            event.qwEventId = ntohq(((NXC_EVENT *)pRawMsg->df)->qwEventId);
            event.dwSeverity = ntohl(((NXC_EVENT *)pRawMsg->df)->dwSeverity);
            event.dwSourceId = ntohl(((NXC_EVENT *)pRawMsg->df)->dwSourceId);
            event.dwTimeStamp = ntohl(((NXC_EVENT *)pRawMsg->df)->dwTimeStamp);

            // Convert bytes in message characters to host byte order
            // and than to single-byte if we building non-unicode library
#ifdef UNICODE
/*#if WORDS_BIGENDIAN
            memcpy(event.szMessage, ((NXC_EVENT *)pRawMsg->df)->szMessage,
                   MAX_EVENT_MSG_LENGTH * sizeof(WCHAR));
#else
            for(pSrc = ((NXC_EVENT *)pRawMsg->df)->szMessage, 
                pDst = event.szMessage; *pSrc != 0; pSrc++, pDst++)
               *pDst = ntohs(*pSrc);
            *pDst = ntohs(*pSrc);
#endif*/
event.szMessage[0] = 0;
#else
            SwapWideString((UCS2CHAR *)((NXC_EVENT *)pRawMsg->df)->szMessage);
            ucs2_to_mb((UCS2CHAR *)((NXC_EVENT *)pRawMsg->df)->szMessage, -1,
                       event.szMessage, MAX_EVENT_MSG_LENGTH);
            event.szMessage[MAX_EVENT_MSG_LENGTH - 1] = 0;
#endif

            // Call client's callback to handle new record
            pSession->CallEventHandler(NXC_EVENT_NEW_ELOG_RECORD, 
                                       (pRawMsg->wFlags & MF_REVERSE_ORDER) ? 
                                               RECORD_ORDER_REVERSED : RECORD_ORDER_NORMAL,
                                       &event);
         }
         break;
      default:
         break;
   }
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
      pSession->CallEventHandler(NXC_EVENT_NEW_SYSLOG_RECORD, nOrder, &rec);
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
