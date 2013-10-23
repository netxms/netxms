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
** File: eventdb.cpp
**
**/

#include "libnxcl.h"


//
// Fill event template structure from NXCP message
//

static void EventTemplateFromMsg(CSCPMessage *pMsg, NXC_EVENT_TEMPLATE *pEventTemplate)
{
   pEventTemplate->dwCode = pMsg->GetVariableLong(VID_EVENT_CODE);
   pEventTemplate->dwSeverity = pMsg->GetVariableLong(VID_SEVERITY);
   pEventTemplate->dwFlags = pMsg->GetVariableLong(VID_FLAGS);
   pMsg->GetVariableStr(VID_NAME, pEventTemplate->szName, MAX_EVENT_NAME);
   pEventTemplate->pszMessage = pMsg->GetVariableStr(VID_MESSAGE, NULL, 0);
   pEventTemplate->pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION, NULL, 0);
}


//
// Process CMD_EVENT_DB_UPDATE message
//

void ProcessEventDBUpdate(NXCL_Session *pSession, CSCPMessage *pMsg)
{
   NXC_EVENT_TEMPLATE et;
   UINT32 dwCode;

   dwCode = pMsg->GetVariableShort(VID_NOTIFICATION_CODE);
	et.dwCode = pMsg->GetVariableLong(VID_EVENT_CODE);
   if (dwCode != NX_NOTIFY_ETMPL_DELETED)
      EventTemplateFromMsg(pMsg, &et);
   pSession->callEventHandler(NXC_EVENT_NOTIFICATION, dwCode, &et);
}


//
// Duplicate event template
//

NXC_EVENT_TEMPLATE *DuplicateEventTemplate(NXC_EVENT_TEMPLATE *pSrc)
{
   NXC_EVENT_TEMPLATE *pDst;

   pDst = (NXC_EVENT_TEMPLATE *)nx_memdup(pSrc, sizeof(NXC_EVENT_TEMPLATE));
   pDst->pszDescription = _tcsdup(pSrc->pszDescription);
   pDst->pszMessage = _tcsdup(pSrc->pszMessage);
   return pDst;
}


//
// Add template to list
//

void LIBNXCL_EXPORTABLE NXCAddEventTemplate(NXC_SESSION hSession, NXC_EVENT_TEMPLATE *pEventTemplate)
{
   ((NXCL_Session *)hSession)->AddEventTemplate(pEventTemplate, TRUE);
}


//
// Delete record from list
//

void LIBNXCL_EXPORTABLE NXCDeleteEDBRecord(NXC_SESSION hSession, UINT32 dwEventCode)
{
   ((NXCL_Session *)hSession)->DeleteEDBRecord(dwEventCode);
}


//
// Process record from network
//

void ProcessEventDBRecord(NXCL_Session *pSession, CSCPMessage *pMsg)
{
   NXC_EVENT_TEMPLATE *pEventTemplate;
   UINT32 dwEventCode;

   if (pMsg->GetCode() == CMD_EVENT_DB_RECORD)
   {
      dwEventCode = pMsg->GetVariableLong(VID_EVENT_CODE);
		if (!pMsg->isEndOfSequence() && (dwEventCode != 0))
      {
         // Allocate new event template structure and fill it with values from message
         pEventTemplate = (NXC_EVENT_TEMPLATE *)malloc(sizeof(NXC_EVENT_TEMPLATE));
			EventTemplateFromMsg(pMsg, pEventTemplate);
         pSession->AddEventTemplate(pEventTemplate, FALSE);
      }
      else
      {
         pSession->CompleteSync(SYNC_EVENT_DB, RCC_SUCCESS);
      }
   }
}


//
// Load event configuration database
//

UINT32 LIBNXCL_EXPORTABLE NXCLoadEventDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->LoadEventDB();
}


//
// Set event information
//

UINT32 LIBNXCL_EXPORTABLE NXCModifyEventTemplate(NXC_SESSION hSession, NXC_EVENT_TEMPLATE *pArg)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.SetCode(CMD_SET_EVENT_INFO);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_EVENT_CODE, pArg->dwCode);
   msg.SetVariable(VID_SEVERITY, pArg->dwSeverity);
   msg.SetVariable(VID_FLAGS, pArg->dwFlags);
   msg.SetVariable(VID_NAME, pArg->szName);
   msg.SetVariable(VID_MESSAGE, pArg->pszMessage);
   msg.SetVariable(VID_DESCRIPTION, pArg->pszDescription);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   // Wait for reply
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete event template
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteEventTemplate(NXC_SESSION hSession, UINT32 dwEventCode)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.SetCode(CMD_DELETE_EVENT_TEMPLATE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_EVENT_CODE, dwEventCode);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   // Wait for reply
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Generate ID for new event
//

UINT32 LIBNXCL_EXPORTABLE NXCGenerateEventCode(NXC_SESSION hSession, UINT32 *pdwEventCode)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.SetCode(CMD_GENERATE_EVENT_CODE);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   // Wait for reply
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwEventCode = pResponse->GetVariableLong(VID_EVENT_CODE);
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Get pointer to event templates list
//

BOOL LIBNXCL_EXPORTABLE NXCGetEventDB(NXC_SESSION hSession, 
                                      NXC_EVENT_TEMPLATE ***pppTemplateList, 
                                      UINT32 *pdwNumRecords)
{
   return ((NXCL_Session *)hSession)->GetEventDB(pppTemplateList, pdwNumRecords);
}


//
// Resolve event id to name
//

const TCHAR LIBNXCL_EXPORTABLE *NXCGetEventName(NXC_SESSION hSession, UINT32 dwId)
{
   return ((NXCL_Session *)hSession)->GetEventName(dwId);
}


//
// Resolve event id to name using application-provided buffer
//

BOOL LIBNXCL_EXPORTABLE NXCGetEventNameEx(NXC_SESSION hSession, UINT32 dwId, 
                                          TCHAR *pszBuffer, UINT32 dwBufSize)
{
   return ((NXCL_Session *)hSession)->GetEventNameEx(dwId, pszBuffer, dwBufSize);
}


//
// Get severity for given event id. Will return -1 for unknown id.
//

int LIBNXCL_EXPORTABLE NXCGetEventSeverity(NXC_SESSION hSession, UINT32 dwId)
{
   return ((NXCL_Session *)hSession)->GetEventSeverity(dwId);
}


//
// Get text of event template with given ID.
// If there are no template with such ID, empty string will be returned.
//

BOOL LIBNXCL_EXPORTABLE NXCGetEventText(NXC_SESSION hSession, UINT32 dwId, TCHAR *pszBuffer, UINT32 dwBufSize)
{
   return ((NXCL_Session *)hSession)->GetEventText(dwId, pszBuffer, dwBufSize);
}
