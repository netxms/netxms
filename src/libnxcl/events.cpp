/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: events.cpp
**
**/

#include "libnxcl.h"


//
// Process events coming from server
//

void ProcessEvent(NXCL_Session *pSession, CSCPMessage *pMsg, CSCP_MESSAGE *pRawMsg)
{
   NXC_EVENT *pEvent;
   WORD wCode;
#ifdef UNICODE
   WCHAR *pSrc, *pDst;
#endif

   wCode = (pMsg != NULL) ? pMsg->GetCode() : pRawMsg->wCode;

   switch(wCode)
   {
      case CMD_EVENT_LIST_END:
         pSession->CompleteSync(RCC_SUCCESS);
         break;
      case CMD_EVENT:
         if (pRawMsg != NULL)    // We should receive events as raw data
         {
            // Allocate new event structure and fill it with values from message
            pEvent = (NXC_EVENT *)malloc(sizeof(NXC_EVENT));
            pEvent->dwEventCode = ntohl(((NXC_EVENT *)pRawMsg->df)->dwEventCode);
            pEvent->qwEventId = ntohq(((NXC_EVENT *)pRawMsg->df)->qwEventId);
            pEvent->dwSeverity = ntohl(((NXC_EVENT *)pRawMsg->df)->dwSeverity);
            pEvent->dwSourceId = ntohl(((NXC_EVENT *)pRawMsg->df)->dwSourceId);
            pEvent->dwTimeStamp = ntohl(((NXC_EVENT *)pRawMsg->df)->dwTimeStamp);

            // Convert bytes in message characters to host byte order
            // and than to single-byte if we building non-unicode library
#ifdef UNICODE
#if WORDS_BIGENDIAN
            memcpy(pEvent->szMessage, ((NXC_EVENT *)pRawMsg->df)->szMessage,
                   MAX_EVENT_MSG_LENGTH * sizeof(WCHAR));
#else
            for(pSrc = ((NXC_EVENT *)pRawMsg->df)->szMessage, 
                pDst = pEvent->szMessage; *pSrc != 0; pSrc++, pDst++)
               *pDst = ntohs(*pSrc);
#endif
#else
            SwapWideString((WCHAR *)((NXC_EVENT *)pRawMsg->df)->szMessage);
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                                (WCHAR *)((NXC_EVENT *)pRawMsg->df)->szMessage, -1,
                                pEvent->szMessage, MAX_EVENT_MSG_LENGTH, NULL, NULL);
            pEvent->szMessage[MAX_EVENT_MSG_LENGTH - 1] = 0;
#endif

            // Call client's callback to handle new record
            // It's up to client to destroy allocated event structure
            pSession->CallEventHandler(NXC_EVENT_NEW_ELOG_RECORD, 0, pEvent);
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

DWORD LIBNXCL_EXPORTABLE NXCSyncEvents(NXC_SESSION hSession)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   ((NXCL_Session *)hSession)->PrepareForSync();

   msg.SetCode(CMD_GET_EVENTS);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = ((NXCL_Session *)hSession)->WaitForSync(INFINITE);

   return dwRetCode;
}


//
// Send event to server
//

DWORD LIBNXCL_EXPORTABLE NXCSendEvent(NXC_SESSION hSession, DWORD dwEventCode, 
                                      DWORD dwObjectId, int iNumArgs, TCHAR **pArgList)
{
   CSCPMessage msg;
   DWORD dwRqId;
   int i;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_TRAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_EVENT_CODE, dwEventCode);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_NUM_ARGS, (WORD)iNumArgs);
   for(i = 0; i < iNumArgs; i++)
      msg.SetVariable(VID_EVENT_ARG_BASE + i, pArgList[i]);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
