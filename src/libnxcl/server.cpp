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
** File: server.cpp
**
**/

#include "libnxcl.h"


//
// Get list of configuration variables
//

UINT32 LIBNXCL_EXPORTABLE NXCGetServerVariables(NXC_SESSION hSession, 
                                               NXC_SERVER_VARIABLE **ppVarList, 
                                               UINT32 *pdwNumVars)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   *pdwNumVars = 0;
   *ppVarList = NULL;

   // Build request message
   msg.setCode(CMD_GET_CONFIG_VARLIST);
   msg.setId(dwRqId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 30000);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwNumVars = pResponse->getFieldAsUInt32(VID_NUM_VARIABLES);
         *ppVarList = (NXC_SERVER_VARIABLE *)malloc(sizeof(NXC_SERVER_VARIABLE) * (*pdwNumVars));

         for(i = 0, dwId = VID_VARLIST_BASE; i < *pdwNumVars; i++)
         {
            pResponse->getFieldAsString(dwId++, (*ppVarList)[i].szName, MAX_OBJECT_NAME);
            pResponse->getFieldAsString(dwId++, (*ppVarList)[i].szValue, MAX_DB_STRING);
            (*ppVarList)[i].bNeedRestart = pResponse->getFieldAsUInt16(dwId++) ? TRUE : FALSE;
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

UINT32 LIBNXCL_EXPORTABLE NXCSetServerVariable(NXC_SESSION hSession, const TCHAR *pszVarName,
                                              const TCHAR *pszValue)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SET_CONFIG_VARIABLE);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, pszVarName);
   msg.setField(VID_VALUE, pszValue);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete server's variable
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteServerVariable(NXC_SESSION hSession, const TCHAR *pszVarName)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_CONFIG_VARIABLE);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, pszVarName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get server statistics
//

UINT32 LIBNXCL_EXPORTABLE NXCGetServerStats(NXC_SESSION hSession, NXC_SERVER_STATS *pStats)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode;

   memset(pStats, 0, sizeof(NXC_SERVER_STATS));
   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_SERVER_STATS);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         pStats->dwNumAlarms = pResponse->getFieldAsUInt32(VID_NUM_ALARMS);
         pResponse->getFieldAsInt32Array(VID_ALARMS_BY_SEVERITY, 5, pStats->dwAlarmsBySeverity);
         pStats->dwNumClientSessions = pResponse->getFieldAsUInt32(VID_NUM_SESSIONS);
         pStats->dwNumDCI = pResponse->getFieldAsUInt32(VID_NUM_ITEMS);
         pStats->dwNumNodes = pResponse->getFieldAsUInt32(VID_NUM_NODES);
         pStats->dwNumObjects = pResponse->getFieldAsUInt32(VID_NUM_OBJECTS);
         pStats->dwServerProcessVMSize = pResponse->getFieldAsUInt32(VID_NETXMSD_PROCESS_VMSIZE);
         pStats->dwServerProcessWorkSet = pResponse->getFieldAsUInt32(VID_NETXMSD_PROCESS_WKSET);
         pStats->dwServerUptime = pResponse->getFieldAsUInt32(VID_SERVER_UPTIME);
         pResponse->getFieldAsString(VID_SERVER_VERSION, pStats->szServerVersion, MAX_DB_STRING);
			pStats->dwQSizeConditionPoller = pResponse->getFieldAsUInt32(VID_QSIZE_CONDITION_POLLER);
			pStats->dwQSizeConfPoller = pResponse->getFieldAsUInt32(VID_QSIZE_CONF_POLLER);
			pStats->dwQSizeDBWriter = pResponse->getFieldAsUInt32(VID_QSIZE_DBWRITER);
			pStats->dwQSizeDCIPoller = pResponse->getFieldAsUInt32(VID_QSIZE_DCI_POLLER);
			pStats->dwQSizeDiscovery = pResponse->getFieldAsUInt32(VID_QSIZE_DISCOVERY);
			pStats->dwQSizeEvents = pResponse->getFieldAsUInt32(VID_QSIZE_EVENT);
			pStats->dwQSizeNodePoller = pResponse->getFieldAsUInt32(VID_QSIZE_NODE_POLLER);
			pStats->dwQSizeRoutePoller = pResponse->getFieldAsUInt32(VID_QSIZE_ROUTE_POLLER);
			pStats->dwQSizeStatusPoller = pResponse->getFieldAsUInt32(VID_QSIZE_STATUS_POLLER);
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
// Get address list
//

UINT32 LIBNXCL_EXPORTABLE NXCGetAddrList(NXC_SESSION hSession, UINT32 dwListType,
                                        UINT32 *pdwAddrCount, NXC_ADDR_ENTRY **ppAddrList)
{
   UINT32 i, dwRetCode, dwRqId, dwId;
   NXCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_ADDR_LIST);
   msg.setId(dwRqId);
   msg.setField(VID_ADDR_LIST_TYPE, dwListType);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwAddrCount = pResponse->getFieldAsUInt32(VID_NUM_RECORDS);
         *ppAddrList = (NXC_ADDR_ENTRY *)malloc(sizeof(NXC_ADDR_ENTRY) * (*pdwAddrCount));
         for(i = 0, dwId = VID_ADDR_LIST_BASE; i < *pdwAddrCount; i++, dwId += 7)
         {
            (*ppAddrList)[i].dwType = pResponse->getFieldAsUInt32(dwId++);
            (*ppAddrList)[i].dwAddr1 = pResponse->getFieldAsUInt32(dwId++);
            (*ppAddrList)[i].dwAddr2 = pResponse->getFieldAsUInt32(dwId++);
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

UINT32 LIBNXCL_EXPORTABLE NXCSetAddrList(NXC_SESSION hSession, UINT32 dwListType,
                                        UINT32 dwAddrCount, NXC_ADDR_ENTRY *pAddrList)
{
   UINT32 i, dwRqId, dwId;
   NXCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SET_ADDR_LIST);
   msg.setId(dwRqId);
   msg.setField(VID_ADDR_LIST_TYPE, dwListType);
   msg.setField(VID_NUM_RECORDS, dwAddrCount);
   for(i = 0, dwId = VID_ADDR_LIST_BASE; i < dwAddrCount; i++, dwId += 7)
   {
      msg.setField(dwId++, pAddrList[i].dwType);
      msg.setField(dwId++, pAddrList[i].dwAddr1);
      msg.setField(dwId++, pAddrList[i].dwAddr2);
   }  
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Reset server's component
//

UINT32 LIBNXCL_EXPORTABLE NXCResetServerComponent(NXC_SESSION hSession, UINT32 dwComponent)
{
   UINT32 dwRqId;
   NXCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_RESET_COMPONENT);
   msg.setId(dwRqId);
   msg.setField(VID_COMPONENT_ID, dwComponent);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get value of CLOB config variable
//

UINT32 LIBNXCL_EXPORTABLE NXCGetServerConfigCLOB(NXC_SESSION hSession, const TCHAR *name, TCHAR **value)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
	*value = NULL;

   // Build request message
   msg.setCode(CMD_CONFIG_GET_CLOB);
   msg.setId(dwRqId);
	msg.setField(VID_NAME, name);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
			*value = pResponse->getFieldAsString(VID_VALUE);
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
// Set value of CLOB config variable
//

UINT32 LIBNXCL_EXPORTABLE NXCSetServerConfigCLOB(NXC_SESSION hSession, const TCHAR *name, const TCHAR *value)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.setCode(CMD_CONFIG_SET_CLOB);
   msg.setId(dwRqId);
	msg.setField(VID_NAME, name);
	msg.setField(VID_VALUE, value);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
