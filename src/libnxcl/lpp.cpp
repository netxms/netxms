/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: lpp.cpp
**
**/

#include "libnxcl.h"


//
// Load list of configured policies
//

DWORD LIBNXCL_EXPORTABLE NXCLoadLPPList(NXC_SESSION hSession, NXC_LPP_LIST **ppList)
{
   DWORD i, dwRqId, dwRetCode, dwId;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_LPP_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppList = (NXC_LPP_LIST *)malloc(sizeof(NXC_LPP_LIST));
         (*ppList)->dwNumEntries = pResponse->GetVariableLong(VID_NUM_RECORDS);
         (*ppList)->pList = (NXC_LPP_INFO *)malloc(sizeof(NXC_LPP_INFO) * (*ppList)->dwNumEntries);
         memset((*ppList)->pList, 0, sizeof(NXC_LPP_INFO) * (*ppList)->dwNumEntries);
         for(i = 0, dwId = VID_LPP_LIST_BASE; i < (*ppList)->dwNumEntries; i++)
         {
            (*ppList)->pList[i].dwId = pResponse->GetVariableLong(dwId++);
            pResponse->GetVariableStr(dwId++, (*ppList)->pList[i].szName, MAX_OBJECT_NAME);
            (*ppList)->pList[i].dwVersion = pResponse->GetVariableLong(dwId++);
            (*ppList)->pList[i].dwFlags = pResponse->GetVariableLong(dwId++);
         }
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Destroy list of log policies
//

void LIBNXCL_EXPORTABLE NXCDestroyLPPList(NXC_LPP_LIST *pList)
{
   if (pList != NULL)
   {
      safe_free(pList->pList);
      free(pList);
   }
}


//
// Request ID for new log processing policy
//

DWORD LIBNXCL_EXPORTABLE NXCRequestNewLPPID(NXC_SESSION hSession, DWORD *pdwId)
{
   DWORD dwRqId, dwRetCode;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_REQUEST_NEW_LPP_ID);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwId = pResponse->GetVariableLong(VID_LPP_ID);
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Open log processing policy for editing
//

DWORD LIBNXCL_EXPORTABLE NXCOpenLPP(NXC_SESSION hSession, DWORD dwPolicyId, NXC_LPP **ppPolicy)
{
   DWORD i, dwId, dwRqId, dwRetCode;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_OPEN_LPP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_LPP_ID, dwPolicyId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppPolicy = (NXC_LPP *)malloc(sizeof(NXC_LPP));
         (*ppPolicy)->dwId = pResponse->GetVariableLong(VID_LPP_ID);
         pResponse->GetVariableStr(VID_NAME, (*ppPolicy)->szName, MAX_OBJECT_NAME);
         (*ppPolicy)->dwVersion = pResponse->GetVariableLong(VID_VERSION);
         (*ppPolicy)->dwFlags = pResponse->GetVariableLong(VID_FLAGS);
         pResponse->GetVariableStr(VID_LOG_FILE, (*ppPolicy)->szLogFile, MAX_DB_STRING);
         (*ppPolicy)->dwNumRules = pResponse->GetVariableLong(VID_NUM_RULES);
         (*ppPolicy)->dwNumNodes = pResponse->GetVariableLong(VID_NUM_NODES);

         (*ppPolicy)->pdwNodeList = (DWORD *)malloc(sizeof(DWORD) * (*ppPolicy)->dwNumNodes);
         for(i = 0, dwId = VID_LPP_NODE_LIST_BASE; i < (*ppPolicy)->dwNumNodes; i++, dwId++)
            (*ppPolicy)->pdwNodeList[i] = pResponse->GetVariableLong(dwId);

         (*ppPolicy)->pRuleList = (NXC_LPP_RULE *)malloc(sizeof(NXC_LPP_RULE) * (*ppPolicy)->dwNumRules);
         for(i = 0, dwId = VID_LPP_RULE_BASE; i < (*ppPolicy)->dwNumRules; i++, dwId += 4)
         {
            (*ppPolicy)->pRuleList[i].dwMsgIdStart = pResponse->GetVariableLong(dwId++);
            (*ppPolicy)->pRuleList[i].dwMsgIdEnd = pResponse->GetVariableLong(dwId++);
            (*ppPolicy)->pRuleList[i].dwSeverity = pResponse->GetVariableLong(dwId++);
            (*ppPolicy)->pRuleList[i].dwEventCode = pResponse->GetVariableLong(dwId++);
            pResponse->GetVariableStr(dwId++, (*ppPolicy)->pRuleList[i].szSourceName, MAX_DB_STRING);
            pResponse->GetVariableStr(dwId++, (*ppPolicy)->pRuleList[i].szMsgText, MAX_DB_STRING);
         }
      }
      else
      {
         if (dwRetCode == RCC_COMPONENT_LOCKED)
         {
            if (pResponse->IsVariableExist(VID_LOCKED_BY))
            {
               TCHAR szBuffer[MAX_LOCKINFO_LEN];

               pResponse->GetVariableStr(VID_LOCKED_BY, szBuffer, MAX_LOCKINFO_LEN);
               ((NXCL_Session *)hSession)->SetLastLock(szBuffer);
            }
            else
            {
               ((NXCL_Session *)hSession)->SetLastLock(_T("<unknown>"));
            }
         }
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Destroy log processing policy
//

void LIBNXCL_EXPORTABLE NXCDestroyLPP(NXC_LPP *pPolicy)
{
   if (pPolicy != NULL)
   {
      safe_free(pPolicy->pdwNodeList);
      safe_free(pPolicy->pRuleList);
      free(pPolicy);
   }
}
