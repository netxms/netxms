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

static CONDITION m_hCondLoadFinished;
static NXC_EVENT_TEMPLATE **m_ppEventTemplates = NULL;
static DWORD m_dwNumTemplates = 0;
static BOOL m_bEventDBOpened = FALSE;


//
// Add template to list
//

static AddEventTemplate(NXC_EVENT_TEMPLATE *pEventTemplate)
{
   m_ppEventTemplates = (NXC_EVENT_TEMPLATE **)MemReAlloc(m_ppEventTemplates, 
      sizeof(NXC_EVENT_TEMPLATE *) * (m_dwNumTemplates + 1));
   m_ppEventTemplates[m_dwNumTemplates] = pEventTemplate;
   m_dwNumTemplates++;
}


//
// Process record from network
//

void ProcessEventDBRecord(CSCPMessage *pMsg)
{
   NXC_EVENT_TEMPLATE *pEventTemplate;

   switch(pMsg->GetCode())
   {
      case CMD_EVENT_DB_EOF:
         if (g_dwState == STATE_LOAD_EVENT_DB)
            ConditionSet(m_hCondLoadFinished);
         break;
      case CMD_EVENT_DB_RECORD:
         // Allocate new event template structure and fill it with values from message
         pEventTemplate = (NXC_EVENT_TEMPLATE *)MemAlloc(sizeof(NXC_EVENT_TEMPLATE));
         pEventTemplate->dwCode = pMsg->GetVariableLong(VID_EVENT_ID);
         pEventTemplate->dwSeverity = pMsg->GetVariableLong(VID_SEVERITY);
         pEventTemplate->dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pMsg->GetVariableStr(VID_NAME, pEventTemplate->szName, MAX_EVENT_NAME);
         pEventTemplate->pszMessage = pMsg->GetVariableStr(VID_MESSAGE, NULL, 0);
         pEventTemplate->pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION, NULL, 0);
         AddEventTemplate(pEventTemplate);
         break;
      default:
         break;
   }
}


//
// Open event configuration database
//

DWORD OpenEventDB(DWORD dwRqId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode;

   ChangeState(STATE_LOAD_EVENT_DB);
   m_hCondLoadFinished = ConditionCreate();

   msg.SetCode(CMD_OPEN_EVENT_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   if (dwRetCode == RCC_SUCCESS)
   {
      // Wait for object list end or for disconnection
      while(g_dwState != STATE_DISCONNECTED)
      {
         if (ConditionWait(m_hCondLoadFinished, 500))
            break;
      }
      m_bEventDBOpened = TRUE;
   }

   ConditionDestroy(m_hCondLoadFinished);
   if (g_dwState != STATE_DISCONNECTED)
      ChangeState(STATE_IDLE);

   return dwRetCode;
}


//
// Close event configuration database
//

DWORD CloseEventDB(DWORD dwRqId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode;

   msg.SetCode(CMD_CLOSE_EVENT_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);
   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
   else
      dwRetCode = RCC_TIMEOUT;
   return dwRetCode;
}


//
// Get pointer to event templates list
//

BOOL EXPORTABLE NXCGetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, DWORD *pdwNumRecords)
{
   if (!m_bEventDBOpened)
      return FALSE;

   *pppTemplateList = m_ppEventTemplates;
   *pdwNumRecords = m_dwNumTemplates;
   return TRUE;
}
