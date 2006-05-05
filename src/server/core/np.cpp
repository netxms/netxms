/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: np.cpp
**
**/

#include "nxcore.h"


//
// Externals
//

extern Queue g_nodePollerQueue;


//
// Discovery class
//

class NXSL_DiscoveryClass : public NXSL_Class
{
public:
   NXSL_DiscoveryClass();

   virtual NXSL_Value *GetAttr(NXSL_Object *pObject, char *pszAttr);
};


//
// Implementation of discovery class
//

NXSL_DiscoveryClass::NXSL_DiscoveryClass()
                     :NXSL_Class()
{
   strcpy(m_szName, "NewNode");
}

NXSL_Value *NXSL_DiscoveryClass::GetAttr(NXSL_Object *pObject, char *pszAttr)
{
   DISCOVERY_FILTER_DATA *pData;
   NXSL_Value *pValue = NULL;
   char szBuffer[256];

   pData = (DISCOVERY_FILTER_DATA *)pObject->Data();
   if (!strcmp(pszAttr, "ipAddr"))
   {
      IpToStr(pData->dwIpAddr, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!strcmp(pszAttr, "netMask"))
   {
      IpToStr(pData->dwNetMask, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!strcmp(pszAttr, "subnet"))
   {
      IpToStr(pData->dwSubnetAddr, szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!strcmp(pszAttr, "isAgent"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_AGENT) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isSNMP"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_SNMP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isBridge"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isRouter"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_ROUTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isPrinter"))
   {
      pValue = new NXSL_Value((LONG)((pData->dwFlags & NNF_IS_PRINTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "snmpVersion"))
   {
      pValue = new NXSL_Value((LONG)pData->nSNMPVersion);
   }
   else if (!strcmp(pszAttr, "snmpOID"))
   {
      pValue = new NXSL_Value(pData->szObjectId);
   }
   else if (!strcmp(pszAttr, "agentVersion"))
   {
      pValue = new NXSL_Value(pData->szAgentVersion);
   }
   else if (!strcmp(pszAttr, "platformName"))
   {
      pValue = new NXSL_Value(pData->szPlatform);
   }
   return pValue;
}


//
// Discovery class object
//

static NXSL_DiscoveryClass m_nxslDiscoveryClass;


//
// Poll new node for configuration
// Returns pointer to new node object on success or NULL on failure
//

NetObj *PollNewNode(DWORD dwIpAddr, DWORD dwNetMask, DWORD dwFlags, TCHAR *pszName)
{
   Node *pNode;
   char szIpAddr1[32], szIpAddr2[32];

   DbgPrintf(AF_DEBUG_DISCOVERY, "PollNode(%s,%s)",  
             IpToStr(dwIpAddr, szIpAddr1), IpToStr(dwNetMask, szIpAddr2));
   // Check for node existence
   if ((FindNodeByIP(dwIpAddr) != NULL) ||
       (FindSubnetByIP(dwIpAddr) != NULL))
   {
      DbgPrintf(AF_DEBUG_DISCOVERY, "PollNode: Node %s already exist in database", 
                IpToStr(dwIpAddr, szIpAddr1));
      return NULL;
   }

   pNode = new Node(dwIpAddr, 0, dwFlags, 0);
   NetObjInsert(pNode, TRUE);
   pNode->NewNodePoll(dwNetMask);
   if (pszName != NULL)
      pNode->SetName(pszName);
   pNode->Unhide();
   PostEvent(EVENT_NODE_ADDED, pNode->Id(), NULL);

   // DEBUG
   pNode->AddItem(new DCItem(CreateUniqueId(IDG_ITEM), _T("Status"), DS_INTERNAL, DCI_DT_INT, 60, 30, pNode));
   return pNode;
}


//
// Check if newly discovered node should be added
//

static BOOL AcceptNewNode(DWORD dwIpAddr, DWORD dwNetMask)
{
   DISCOVERY_FILTER_DATA data;
   char szCommunityString[MAX_DB_STRING], szFilter[MAX_DB_STRING], szBuffer[256];
   DWORD dwTemp;
   AgentConnection *pAgentConn;
   NXSL_Program *pScript;
   NXSL_Value *pValue;
   BOOL bResult = FALSE;

   if ((FindNodeByIP(dwIpAddr) != NULL) ||
       (FindSubnetByIP(dwIpAddr) != NULL))
      return FALSE;  // Node already exist in database

   // Read configuration
   ConfigReadStr("DefaultCommunityString", szCommunityString, MAX_DB_STRING, "public");
   ConfigReadStr("DiscoveryFilter", szFilter, MAX_DB_STRING, "");
   StrStrip(szFilter);

   // Run filter script
   if ((szFilter[0] == 0) || (!_tcsicmp(szFilter, "none")))
      return TRUE;   // No filtering

   // Initialize new node data
   memset(&data, 0, sizeof(DISCOVERY_FILTER_DATA));
   data.dwIpAddr = dwIpAddr;
   data.dwNetMask = dwNetMask;
   data.dwSubnetAddr = dwIpAddr & dwNetMask;

   // Check SNMP support
   if (SnmpGet(SNMP_VERSION_2C, dwIpAddr, 161, szCommunityString,
               ".1.3.6.1.2.1.1.2.0", NULL, 0, data.szObjectId, MAX_OID_LEN * 4,
               FALSE, FALSE) == SNMP_ERR_SUCCESS)
   {
      data.dwFlags |= NNF_IS_SNMP;
      data.nSNMPVersion = SNMP_VERSION_2C;
   }
   else
   {
      if (SnmpGet(SNMP_VERSION_1, dwIpAddr, 161, szCommunityString,
                  ".1.3.6.1.2.1.1.2.0", NULL, 0, data.szObjectId, MAX_OID_LEN * 4,
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         data.dwFlags |= NNF_IS_SNMP;
         data.nSNMPVersion = SNMP_VERSION_1;
      }
   }

   // Check NetXMS agent support
   pAgentConn = new AgentConnection(htonl(dwIpAddr), AGENT_LISTEN_PORT,
                                    AUTH_NONE, "");
   if (pAgentConn->Connect(g_pServerKey))
   {
      data.dwFlags |= NNF_IS_AGENT;
      pAgentConn->GetParameter("Agent.Version", MAX_AGENT_VERSION_LEN, data.szAgentVersion);
      pAgentConn->GetParameter("System.PlatformName", MAX_PLATFORM_NAME_LEN, data.szPlatform);
   }

   // Check if node is a router
   if (data.dwFlags & NNF_IS_SNMP)
   {
      if (SnmpGet(data.nSNMPVersion, dwIpAddr, 161, szCommunityString,
                  ".1.3.6.1.2.1.4.1.0", NULL, 0, &dwTemp, sizeof(DWORD),
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         if (dwTemp == 1)
            data.dwFlags |= NNF_IS_ROUTER;
      }
   }
   else if (data.dwFlags & NNF_IS_AGENT)
   {
      // Check IP forwarding status
      if (pAgentConn->GetParameter("Net.IP.Forwarding", 16, szBuffer) == ERR_SUCCESS)
      {
         if (_tcstoul(szBuffer, NULL, 10) != 0)
            data.dwFlags |= NNF_IS_ROUTER;
      }
   }

   // Check if node is a bridge
   if (data.dwFlags & NNF_IS_SNMP)
   {
      if (SnmpGet(data.nSNMPVersion, dwIpAddr, 161, szCommunityString,
                  ".1.3.6.1.2.1.17.1.1.0", NULL, 0, szBuffer, 256,
                  FALSE, FALSE) == SNMP_ERR_SUCCESS)
      {
         data.dwFlags |= NNF_IS_BRIDGE;
      }
   }

   // Cleanup
   delete pAgentConn;

   g_pScriptLibrary->Lock();
   pScript = g_pScriptLibrary->FindScript(szFilter);
   if (pScript != NULL)
   {
      DbgPrintf(AF_DEBUG_DISCOVERY, "Running filter script %s", szFilter);
      pValue = new NXSL_Value(new NXSL_Object(&m_nxslDiscoveryClass, &data));
      if (pScript->Run(NULL, 1, &pValue) == 0)
      {
         bResult = (pScript->GetResult()->GetValueAsInt32() != 0) ? TRUE : FALSE;
         DbgPrintf(AF_DEBUG_DISCOVERY, "Filter script result: %d", bResult);
      }
      else
      {
         DbgPrintf(AF_DEBUG_DISCOVERY, "Filter script execution error: %s",
                   pScript->GetErrorText());
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, _T("ssd"), szFilter,
                   pScript->GetErrorText(), 0);
      }
   }
   else
   {
      DbgPrintf(AF_DEBUG_DISCOVERY, "Cannot find filter script %s", szFilter);
   }
   g_pScriptLibrary->Unlock();

   return bResult;
}


//
// Node poller thread (poll new nodes and put them into the database)
//

THREAD_RESULT THREAD_CALL NodePoller(void *arg)
{
   NEW_NODE *pInfo;

   DbgPrintf(AF_DEBUG_DISCOVERY, "Node poller started");

   while(!ShutdownInProgress())
   {
      pInfo = (NEW_NODE *)g_nodePollerQueue.GetOrBlock();
      if (pInfo == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator received

      if (AcceptNewNode(pInfo->dwIpAddr, pInfo->dwNetMask))
         PollNewNode(pInfo->dwIpAddr, pInfo->dwNetMask, pInfo->dwFlags, NULL);
      free(pInfo);
   }
   DbgPrintf(AF_DEBUG_DISCOVERY, "Node poller thread terminated");
   return THREAD_OK;
}
