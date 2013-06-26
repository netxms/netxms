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
** File: agentcfg.cpp
**
**/

#include "libnxcl.h"


//
// Get list of available agent configs
//

UINT32 LIBNXCL_EXPORTABLE NXCGetAgentConfigList(NXC_SESSION hSession, UINT32 *pdwNumRecs,
                                               NXC_AGENT_CONFIG_INFO **ppList)
{
   CSCPMessage msg, *pResponse;
   UINT32 i, dwRetCode, dwRqId, dwId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_AGENT_CFG_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumRecs = 0;
   *ppList = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwNumRecs = pResponse->GetVariableLong(VID_NUM_RECORDS);
         *ppList = (NXC_AGENT_CONFIG_INFO *)malloc(sizeof(NXC_AGENT_CONFIG_INFO) * (*pdwNumRecs));
         for(i = 0, dwId = VID_AGENT_CFG_LIST_BASE; i < *pdwNumRecs; i++, dwId += 7)
         {
            (*ppList)[i].dwId = pResponse->GetVariableLong(dwId++);
            pResponse->GetVariableStr(dwId++, (*ppList)[i].szName, MAX_DB_STRING);
            (*ppList)[i].dwSequence = pResponse->GetVariableLong(dwId++);
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
// Open agent's config
//

UINT32 LIBNXCL_EXPORTABLE NXCOpenAgentConfig(NXC_SESSION hSession, UINT32 dwCfgId,
                                            NXC_AGENT_CONFIG *pConfig)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_OPEN_AGENT_CONFIG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_CONFIG_ID, dwCfgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         pConfig->dwId = dwCfgId;
         pConfig->dwSequence = pResponse->GetVariableLong(VID_SEQUENCE_NUMBER);
         pConfig->pszFilter = pResponse->GetVariableStr(VID_FILTER);
         pConfig->pszText = pResponse->GetVariableStr(VID_CONFIG_FILE);
         pResponse->GetVariableStr(VID_NAME, pConfig->szName, MAX_DB_STRING);
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
// Save agent's config
//

UINT32 LIBNXCL_EXPORTABLE NXCSaveAgentConfig(NXC_SESSION hSession, NXC_AGENT_CONFIG *pConfig)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SAVE_AGENT_CONFIG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_CONFIG_ID, pConfig->dwId);
   msg.SetVariable(VID_SEQUENCE_NUMBER, pConfig->dwSequence);
   msg.SetVariable(VID_NAME, pConfig->szName);
   msg.SetVariable(VID_CONFIG_FILE, pConfig->pszText);
   msg.SetVariable(VID_FILTER, pConfig->pszFilter);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         if (pConfig->dwId == 0) // New ID was requested
         {
            pConfig->dwId = pResponse->GetVariableLong(VID_CONFIG_ID);
            pConfig->dwSequence = pResponse->GetVariableLong(VID_SEQUENCE_NUMBER);
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
// Delete agent config
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteAgentConfig(NXC_SESSION hSession, UINT32 dwCfgId)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_AGENT_CONFIG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_CONFIG_ID, dwCfgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Swap sequence numbers of two agent configs
//

UINT32 LIBNXCL_EXPORTABLE NXCSwapAgentConfigs(NXC_SESSION hSession, UINT32 dwCfgId1, UINT32 dwCfgId2)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SWAP_AGENT_CONFIGS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_CONFIG_ID, dwCfgId1);
   msg.SetVariable(VID_CONFIG_ID_2, dwCfgId2);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
