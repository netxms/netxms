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
// Static data
//

static NXC_EVENT_TEMPLATE **m_ppEventTemplates = NULL;
static DWORD m_dwNumTemplates = 0;
static MUTEX m_mutexEventAccess = INVALID_MUTEX_HANDLE;


//
// Destroy event template database
//

static void DestroyEventDB(void)
{
   DWORD i;

   for(i = 0; i < m_dwNumTemplates; i++)
   {
      safe_free(m_ppEventTemplates[i]->pszDescription);
      safe_free(m_ppEventTemplates[i]->pszMessage);
      free(m_ppEventTemplates[i]);
   }
   safe_free(m_ppEventTemplates);
   m_dwNumTemplates = 0;
   m_ppEventTemplates = NULL;
}


//
// Initialize
//

void InitEventDB(void)
{
   m_mutexEventAccess = MutexCreate();
}


//
// Cleanup
//

void ShutdownEventDB(void)
{
   MutexLock(m_mutexEventAccess, INFINITE);
   MutexUnlock(m_mutexEventAccess);
   MutexDestroy(m_mutexEventAccess);
   DestroyEventDB();
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

void LIBNXCL_EXPORTABLE NXCAddEventTemplate(NXC_EVENT_TEMPLATE *pEventTemplate)
{
   m_ppEventTemplates = (NXC_EVENT_TEMPLATE **)realloc(m_ppEventTemplates, 
      sizeof(NXC_EVENT_TEMPLATE *) * (m_dwNumTemplates + 1));
   m_ppEventTemplates[m_dwNumTemplates] = pEventTemplate;
   m_dwNumTemplates++;
}


//
// Delete record from list
//

void LIBNXCL_EXPORTABLE NXCDeleteEDBRecord(DWORD dwEventId)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwEventId)
      {
         m_dwNumTemplates--;
         safe_free(m_ppEventTemplates[i]->pszDescription);
         safe_free(m_ppEventTemplates[i]->pszMessage);
         free(m_ppEventTemplates[i]);
         memmove(&m_ppEventTemplates[i], m_ppEventTemplates[i + 1], 
                 sizeof(NXC_EVENT_TEMPLATE *) * (m_dwNumTemplates - i));
         break;
      }
   MutexUnlock(m_mutexEventAccess);
}


//
// Process record from network
//

void ProcessEventDBRecord(CSCPMessage *pMsg)
{
   NXC_EVENT_TEMPLATE *pEventTemplate;
   DWORD dwEventId;

   if (pMsg->GetCode() == CMD_EVENT_DB_RECORD)
   {
      dwEventId = pMsg->GetVariableLong(VID_EVENT_ID);
      if (dwEventId != 0)
      {
         // Allocate new event template structure and fill it with values from message
         pEventTemplate = (NXC_EVENT_TEMPLATE *)malloc(sizeof(NXC_EVENT_TEMPLATE));
         pEventTemplate->dwCode = dwEventId;
         pEventTemplate->dwSeverity = pMsg->GetVariableLong(VID_SEVERITY);
         pEventTemplate->dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pMsg->GetVariableStr(VID_NAME, pEventTemplate->szName, MAX_EVENT_NAME);
         pEventTemplate->pszMessage = pMsg->GetVariableStr(VID_MESSAGE, NULL, 0);
         pEventTemplate->pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION, NULL, 0);
         NXCAddEventTemplate(pEventTemplate);
      }
      else
      {
         CompleteSync(RCC_SUCCESS);
      }
   }
}


//
// Load event configuration database
//

DWORD LIBNXCL_EXPORTABLE NXCLoadEventDB(void)
{
   CSCPMessage msg;
   DWORD dwRetCode;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;
   PrepareForSync();

   DestroyEventDB();
   MutexLock(m_mutexEventAccess, INFINITE);

   msg.SetCode(CMD_LOAD_EVENT_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   /* TODO: this probably should be recoded as loop with calls to WaitForMessage() */
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = WaitForSync(INFINITE);

   MutexUnlock(m_mutexEventAccess);
   return dwRetCode;
}


//
// Set event information
//

DWORD LIBNXCL_EXPORTABLE NXCSetEventInfo(NXC_EVENT_TEMPLATE *pArg)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   // Prepare message
   msg.SetCode(CMD_SET_EVENT_INFO);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_EVENT_ID, pArg->dwCode);
   msg.SetVariable(VID_SEVERITY, pArg->dwSeverity);
   msg.SetVariable(VID_FLAGS, pArg->dwFlags);
   msg.SetVariable(VID_NAME, pArg->szName);
   msg.SetVariable(VID_MESSAGE, pArg->pszMessage);
   msg.SetVariable(VID_DESCRIPTION, pArg->pszDescription);
   SendMsg(&msg);
   
   // Wait for reply
   return WaitForRCC(dwRqId);
}


//
// Delete event template
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteEventTemplate(DWORD dwEventId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   // Prepare message
   msg.SetCode(CMD_DELETE_EVENT_TEMPLATE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_EVENT_ID, dwEventId);
   SendMsg(&msg);
   
   // Wait for reply
   return WaitForRCC(dwRqId);
}


//
// Generate ID for new event
//

DWORD LIBNXCL_EXPORTABLE NXCGenerateEventId(DWORD *pdwEventId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwRetCode;

   dwRqId = g_dwMsgId++;

   // Prepare message
   msg.SetCode(CMD_GENERATE_EVENT_ID);
   msg.SetId(dwRqId);
   SendMsg(&msg);
   
   // Wait for reply
   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, g_dwCommandTimeout);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwEventId = pResponce->GetVariableLong(VID_EVENT_ID);
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

BOOL LIBNXCL_EXPORTABLE NXCGetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, DWORD *pdwNumRecords)
{
   *pppTemplateList = m_ppEventTemplates;
   *pdwNumRecords = m_dwNumTemplates;
   return TRUE;
}



//
// Resolve event id to name
//

const TCHAR LIBNXCL_EXPORTABLE *NXCGetEventName(DWORD dwId)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         MutexUnlock(m_mutexEventAccess);
         return m_ppEventTemplates[i]->szName;
      }
   MutexUnlock(m_mutexEventAccess);
   return _T("<unknown>");
}


//
// Get severity for given event id. Will return -1 for unknown id.
//

int LIBNXCL_EXPORTABLE NXCGetEventSeverity(DWORD dwId)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         MutexUnlock(m_mutexEventAccess);
         return (int)(m_ppEventTemplates[i]->dwSeverity);
      }
   MutexUnlock(m_mutexEventAccess);
   return -1;
}


//
// Lock/unlock event DB
//

static DWORD DoEventDBLock(BOOL bLock)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   // Prepare message
   msg.SetCode(bLock ? CMD_LOCK_EVENT_DB : CMD_UNLOCK_EVENT_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);
   
   // Wait for reply
   return WaitForRCC(dwRqId);
}


//
// Lock event DB
//

DWORD LIBNXCL_EXPORTABLE NXCLockEventDB(void)
{
   return DoEventDBLock(TRUE);
}


//
// Unlock event DB
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockEventDB(void)
{
   return DoEventDBLock(FALSE);
}
