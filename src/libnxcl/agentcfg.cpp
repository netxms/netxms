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
   NXCPMessage msg, *pResponse;
   UINT32 i, dwRetCode, dwRqId, dwId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_AGENT_CFG_LIST);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumRecs = 0;
   *ppList = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwNumRecs = pResponse->getFieldAsUInt32(VID_NUM_RECORDS);
         *ppList = (NXC_AGENT_CONFIG_INFO *)malloc(sizeof(NXC_AGENT_CONFIG_INFO) * (*pdwNumRecs));
         for(i = 0, dwId = VID_AGENT_CFG_LIST_BASE; i < *pdwNumRecs; i++, dwId += 7)
         {
            (*ppList)[i].dwId = pResponse->getFieldAsUInt32(dwId++);
            pResponse->getFieldAsString(dwId++, (*ppList)[i].szName, MAX_DB_STRING);
            (*ppList)[i].dwSequence = pResponse->getFieldAsUInt32(dwId++);
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
   NXCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_OPEN_AGENT_CONFIG);
   msg.setId(dwRqId);
   msg.setField(VID_CONFIG_ID, dwCfgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         pConfig->dwId = dwCfgId;
         pConfig->dwSequence = pResponse->getFieldAsUInt32(VID_SEQUENCE_NUMBER);
         pConfig->pszFilter = pResponse->getFieldAsString(VID_FILTER);
         pConfig->pszText = pResponse->getFieldAsString(VID_CONFIG_FILE);
         pResponse->getFieldAsString(VID_NAME, pConfig->szName, MAX_DB_STRING);
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
   NXCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SAVE_AGENT_CONFIG);
   msg.setId(dwRqId);
   msg.setField(VID_CONFIG_ID, pConfig->dwId);
   msg.setField(VID_SEQUENCE_NUMBER, pConfig->dwSequence);
   msg.setField(VID_NAME, pConfig->szName);
   msg.setField(VID_CONFIG_FILE, pConfig->pszText);
   msg.setField(VID_FILTER, pConfig->pszFilter);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         if (pConfig->dwId == 0) // New ID was requested
         {
            pConfig->dwId = pResponse->getFieldAsUInt32(VID_CONFIG_ID);
            pConfig->dwSequence = pResponse->getFieldAsUInt32(VID_SEQUENCE_NUMBER);
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
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_AGENT_CONFIG);
   msg.setId(dwRqId);
   msg.setField(VID_CONFIG_ID, dwCfgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Swap sequence numbers of two agent configs
//

UINT32 LIBNXCL_EXPORTABLE NXCSwapAgentConfigs(NXC_SESSION hSession, UINT32 dwCfgId1, UINT32 dwCfgId2)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SWAP_AGENT_CONFIGS);
   msg.setId(dwRqId);
   msg.setField(VID_CONFIG_ID, dwCfgId1);
   msg.setField(VID_CONFIG_ID_2, dwCfgId2);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
