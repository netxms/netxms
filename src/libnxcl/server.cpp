/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
** $module: server.cpp
**
**/

#include "libnxcl.h"


//
// Get list of configuration variables
//

DWORD LIBNXCL_EXPORTABLE NXCGetServerVariables(NXC_SESSION hSession, 
                                               NXC_SERVER_VARIABLE **ppVarList, 
                                               DWORD *pdwNumVars)
{
   CSCPMessage msg, *pResponse;
   DWORD i, dwId, dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   *pdwNumVars = 0;
   *ppVarList = NULL;

   // Build request message
   msg.SetCode(CMD_GET_CONFIG_VARLIST);
   msg.SetId(dwRqId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 30000);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwNumVars = pResponse->GetVariableLong(VID_NUM_VARIABLES);
         *ppVarList = (NXC_SERVER_VARIABLE *)malloc(sizeof(NXC_SERVER_VARIABLE) * (*pdwNumVars));

         for(i = 0, dwId = VID_VARLIST_BASE; i < *pdwNumVars; i++)
         {
            pResponse->GetVariableStr(dwId++, (*ppVarList)[i].szName, MAX_OBJECT_NAME);
            pResponse->GetVariableStr(dwId++, (*ppVarList)[i].szValue, MAX_DB_STRING);
            (*ppVarList)[i].bNeedRestart = pResponse->GetVariableShort(dwId++) ? TRUE : FALSE;
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
// Set value of server's variable
//

DWORD LIBNXCL_EXPORTABLE NXCSetServerVariable(NXC_SESSION hSession, TCHAR *pszVarName,
                                              TCHAR *pszValue)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SET_CONFIG_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   msg.SetVariable(VID_VALUE, pszValue);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete server's variable
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteServerVariable(NXC_SESSION hSession, TCHAR *pszVarName)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_CONFIG_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get server statistics
//

DWORD LIBNXCL_EXPORTABLE NXCGetServerStats(NXC_SESSION hSession, NXC_SERVER_STATS *pStats)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwRetCode;

   memset(pStats, 0, sizeof(NXC_SERVER_STATS));
   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_SERVER_STATS);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         pStats->dwNumAlarms = pResponse->GetVariableLong(VID_NUM_ALARMS);
         pResponse->GetVariableInt32Array(VID_ALARMS_BY_SEVERITY, 5, pStats->dwAlarmsBySeverity);
         pStats->dwNumClientSessions = pResponse->GetVariableLong(VID_NUM_SESSIONS);
         pStats->dwNumDCI = pResponse->GetVariableLong(VID_NUM_ITEMS);
         pStats->dwNumNodes = pResponse->GetVariableLong(VID_NUM_NODES);
         pStats->dwNumObjects = pResponse->GetVariableLong(VID_NUM_OBJECTS);
         pStats->dwServerProcessVMSize = pResponse->GetVariableLong(VID_NETXMSD_PROCESS_VMSIZE);
         pStats->dwServerProcessWorkSet = pResponse->GetVariableLong(VID_NETXMSD_PROCESS_WKSET);
         pStats->dwServerUptime = pResponse->GetVariableLong(VID_SERVER_UPTIME);
         pResponse->GetVariableStr(VID_SERVER_VERSION, pStats->szServerVersion, MAX_DB_STRING);
			pStats->dwQSizeConditionPoller = pResponse->GetVariableLong(VID_QSIZE_CONDITION_POLLER);
			pStats->dwQSizeConfPoller = pResponse->GetVariableLong(VID_QSIZE_CONF_POLLER);
			pStats->dwQSizeDBWriter = pResponse->GetVariableLong(VID_QSIZE_DBWRITER);
			pStats->dwQSizeDCIPoller = pResponse->GetVariableLong(VID_QSIZE_DCI_POLLER);
			pStats->dwQSizeDiscovery = pResponse->GetVariableLong(VID_QSIZE_DISCOVERY);
			pStats->dwQSizeEvents = pResponse->GetVariableLong(VID_QSIZE_EVENT);
			pStats->dwQSizeNodePoller = pResponse->GetVariableLong(VID_QSIZE_NODE_POLLER);
			pStats->dwQSizeRoutePoller = pResponse->GetVariableLong(VID_QSIZE_ROUTE_POLLER);
			pStats->dwQSizeStatusPoller = pResponse->GetVariableLong(VID_QSIZE_STATUS_POLLER);
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
// Get module list
//

DWORD LIBNXCL_EXPORTABLE NXCGetServerModuleList(NXC_SESSION hSession,
                                                NXC_SERVER_MODULE_LIST **ppModuleList)
{
   DWORD i, dwRetCode, dwRqId, dwId;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_MODULE_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppModuleList = (NXC_SERVER_MODULE_LIST *)malloc(sizeof(NXC_SERVER_MODULE_LIST));
         (*ppModuleList)->dwNumModules = pResponse->GetVariableLong(VID_NUM_MODULES);
         (*ppModuleList)->pModules = (NXC_SERVER_MODULE_INFO *)malloc(sizeof(NXC_SERVER_MODULE_INFO) * (*ppModuleList)->dwNumModules);
         for(i = 0, dwId = VID_MODULE_LIST_BASE; i < (*ppModuleList)->dwNumModules; i++, dwId += 4)
         {
            (*ppModuleList)->pModules[i].dwModuleId = pResponse->GetVariableLong(dwId++);
            (*ppModuleList)->pModules[i].pszName = pResponse->GetVariableStr(dwId++);
            (*ppModuleList)->pModules[i].pszExecutable = pResponse->GetVariableStr(dwId++);
            (*ppModuleList)->pModules[i].dwFlags = pResponse->GetVariableLong(dwId++);
            (*ppModuleList)->pModules[i].pszDescription = pResponse->GetVariableStr(dwId++);
            (*ppModuleList)->pModules[i].pszLicenseKey = pResponse->GetVariableStr(dwId++);
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
// Destroy module list
//

void LIBNXCL_EXPORTABLE NXCDestroyModuleList(NXC_SERVER_MODULE_LIST *pModuleList)
{
   DWORD i;

   if (pModuleList == NULL)
      return;

   for(i = 0; i < pModuleList->dwNumModules; i++)
   {
      safe_free(pModuleList->pModules[i].pszDescription);
      safe_free(pModuleList->pModules[i].pszExecutable);
      safe_free(pModuleList->pModules[i].pszLicenseKey);
      safe_free(pModuleList->pModules[i].pszName);
   }
   safe_free(pModuleList->pModules);
   free(pModuleList);
}


//
// Get address list
//

DWORD LIBNXCL_EXPORTABLE NXCGetAddrList(NXC_SESSION hSession, DWORD dwListType,
                                        DWORD *pdwAddrCount, NXC_ADDR_ENTRY **ppAddrList)
{
   DWORD i, dwRetCode, dwRqId, dwId;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_ADDR_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ADDR_LIST_TYPE, dwListType);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwAddrCount = pResponse->GetVariableLong(VID_NUM_RECORDS);
         *ppAddrList = (NXC_ADDR_ENTRY *)malloc(sizeof(NXC_ADDR_ENTRY) * (*pdwAddrCount));
         for(i = 0, dwId = VID_ADDR_LIST_BASE; i < *pdwAddrCount; i++, dwId += 7)
         {
            (*ppAddrList)[i].dwType = pResponse->GetVariableLong(dwId++);
            (*ppAddrList)[i].dwAddr1 = pResponse->GetVariableLong(dwId++);
            (*ppAddrList)[i].dwAddr2 = pResponse->GetVariableLong(dwId++);
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
// Set address list
//

DWORD LIBNXCL_EXPORTABLE NXCSetAddrList(NXC_SESSION hSession, DWORD dwListType,
                                        DWORD dwAddrCount, NXC_ADDR_ENTRY *pAddrList)
{
   DWORD i, dwRqId, dwId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SET_ADDR_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ADDR_LIST_TYPE, dwListType);
   msg.SetVariable(VID_NUM_RECORDS, dwAddrCount);
   for(i = 0, dwId = VID_ADDR_LIST_BASE; i < dwAddrCount; i++, dwId += 7)
   {
      msg.SetVariable(dwId++, pAddrList[i].dwType);
      msg.SetVariable(dwId++, pAddrList[i].dwAddr1);
      msg.SetVariable(dwId++, pAddrList[i].dwAddr2);
   }  
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Reset server's component
//

DWORD LIBNXCL_EXPORTABLE NXCResetServerComponent(NXC_SESSION hSession, DWORD dwComponent)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_RESET_COMPONENT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_COMPONENT_ID, dwComponent);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
