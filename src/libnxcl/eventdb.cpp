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
** $module: eventdb.cpp
**
**/

#include "libnxcl.h"


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

void LIBNXCL_EXPORTABLE NXCDeleteEDBRecord(NXC_SESSION hSession, DWORD dwEventCode)
{
   ((NXCL_Session *)hSession)->DeleteEDBRecord(dwEventCode);
}


//
// Process record from network
//

void ProcessEventDBRecord(NXCL_Session *pSession, CSCPMessage *pMsg)
{
   NXC_EVENT_TEMPLATE *pEventTemplate;
   DWORD dwEventCode;

   if (pMsg->GetCode() == CMD_EVENT_DB_RECORD)
   {
      dwEventCode = pMsg->GetVariableLong(VID_EVENT_CODE);
      if (dwEventCode != 0)
      {
         // Allocate new event template structure and fill it with values from message
         pEventTemplate = (NXC_EVENT_TEMPLATE *)malloc(sizeof(NXC_EVENT_TEMPLATE));
         pEventTemplate->dwCode = dwEventCode;
         pEventTemplate->dwSeverity = pMsg->GetVariableLong(VID_SEVERITY);
         pEventTemplate->dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pMsg->GetVariableStr(VID_NAME, pEventTemplate->szName, MAX_EVENT_NAME);
         pEventTemplate->pszMessage = pMsg->GetVariableStr(VID_MESSAGE, NULL, 0);
         pEventTemplate->pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION, NULL, 0);
         pSession->AddEventTemplate(pEventTemplate, FALSE);
      }
      else
      {
         pSession->CompleteSync(RCC_SUCCESS);
      }
   }
}


//
// Load event configuration database
//

DWORD LIBNXCL_EXPORTABLE NXCLoadEventDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->LoadEventDB();
}


//
// Set event information
//

DWORD LIBNXCL_EXPORTABLE NXCSetEventInfo(NXC_SESSION hSession, NXC_EVENT_TEMPLATE *pArg)
{
   CSCPMessage msg;
   DWORD dwRqId;

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

DWORD LIBNXCL_EXPORTABLE NXCDeleteEventTemplate(NXC_SESSION hSession, DWORD dwEventCode)
{
   CSCPMessage msg;
   DWORD dwRqId;

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

DWORD LIBNXCL_EXPORTABLE NXCGenerateEventCode(NXC_SESSION hSession, DWORD *pdwEventCode)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.SetCode(CMD_GENERATE_EVENT_CODE);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   // Wait for reply
   pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwEventCode = pResponce->GetVariableLong(VID_EVENT_CODE);
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
                                      DWORD *pdwNumRecords)
{
   return ((NXCL_Session *)hSession)->GetEventDB(pppTemplateList, pdwNumRecords);
}


//
// Resolve event id to name
//

const TCHAR LIBNXCL_EXPORTABLE *NXCGetEventName(NXC_SESSION hSession, DWORD dwId)
{
   return ((NXCL_Session *)hSession)->GetEventName(dwId);
}


//
// Resolve event id to name using application-provided buffer
//

BOOL LIBNXCL_EXPORTABLE NXCGetEventNameEx(NXC_SESSION hSession, DWORD dwId, 
                                          TCHAR *pszBuffer, DWORD dwBufSize)
{
   return ((NXCL_Session *)hSession)->GetEventNameEx(dwId, pszBuffer, dwBufSize);
}


//
// Get severity for given event id. Will return -1 for unknown id.
//

int LIBNXCL_EXPORTABLE NXCGetEventSeverity(NXC_SESSION hSession, DWORD dwId)
{
   return ((NXCL_Session *)hSession)->GetEventSeverity(dwId);
}


//
// Get text of event template with given ID.
// If there are no template with such ID, empty string will be returned.
//

BOOL LIBNXCL_EXPORTABLE NXCGetEventText(NXC_SESSION hSession, DWORD dwId, TCHAR *pszBuffer, DWORD dwBufSize)
{
   return ((NXCL_Session *)hSession)->GetEventText(dwId, pszBuffer, dwBufSize);
}


//
// Lock/unlock event DB
//

static DWORD DoEventDBLock(NXCL_Session *pSession, BOOL bLock)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = pSession->CreateRqId();

   // Prepare message
   msg.SetCode(bLock ? CMD_LOCK_EVENT_DB : CMD_UNLOCK_EVENT_DB);
   msg.SetId(dwRqId);
   pSession->SendMsg(&msg);
   
   // Wait for reply
   return pSession->WaitForRCC(dwRqId);
}


//
// Lock event DB
//

DWORD LIBNXCL_EXPORTABLE NXCLockEventDB(NXC_SESSION hSession)
{
   return DoEventDBLock((NXCL_Session *)hSession, TRUE);
}


//
// Unlock event DB
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockEventDB(NXC_SESSION hSession)
{
   return DoEventDBLock((NXCL_Session *)hSession, FALSE);
}
