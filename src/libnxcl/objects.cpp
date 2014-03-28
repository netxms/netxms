/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: objects.cpp
**
**/

#include "libnxcl.h"


//
// Destroy object
//

void DestroyObject(NXC_OBJECT *pObject)
{
   DebugPrintf(_T("DestroyObject(id:%d, name:\"%s\")"), pObject->dwId, pObject->szName);
   switch(pObject->iClass)
   {
		case OBJECT_NODE:
			safe_free(pObject->node.pszAuthName);
			safe_free(pObject->node.pszAuthPassword);
			safe_free(pObject->node.pszPrivPassword);
			safe_free(pObject->node.pszSnmpObjectId);
			break;
      case OBJECT_NETWORKSERVICE:
         safe_free(pObject->netsrv.pszRequest);
         safe_free(pObject->netsrv.pszResponse);
         break;
      case OBJECT_VPNCONNECTOR:
         safe_free(pObject->vpnc.pLocalNetList);
         safe_free(pObject->vpnc.pRemoteNetList);
         break;
      case OBJECT_CONDITION:
         safe_free(pObject->cond.pszScript);
         safe_free(pObject->cond.pDCIList);
         break;
      case OBJECT_CLUSTER:
         safe_free(pObject->cluster.pSyncNetList);
			safe_free(pObject->cluster.pResourceList);
         break;
		case OBJECT_TEMPLATE:
			safe_free(pObject->dct.pszAutoApplyFilter);
			break;
		case OBJECT_CONTAINER:
			safe_free(pObject->container.pszAutoBindFilter);
			break;
   }
   safe_free(pObject->pdwChildList);
   safe_free(pObject->pdwParentList);
   safe_free(pObject->pAccessList);
   safe_free(pObject->pszComments);
	safe_free(pObject->pdwTrustedNodes);
	delete pObject->pCustomAttrs;
   free(pObject);
}


//
// Perform binary search on index
// Returns INVALID_INDEX if key not found or position of appropriate network object
// We assume that pIndex == NULL will not be passed
//

static UINT32 SearchIndex(INDEX *pIndex, UINT32 dwIndexSize, UINT32 dwKey)
{
   UINT32 dwFirst, dwLast, dwMid;

   if (dwIndexSize == 0)
      return INVALID_INDEX;

   dwFirst = 0;
   dwLast = dwIndexSize - 1;

   if ((dwKey < pIndex[0].dwKey) || (dwKey > pIndex[dwLast].dwKey))
      return INVALID_INDEX;

   while(dwFirst < dwLast)
   {
      dwMid = (dwFirst + dwLast) / 2;
      if (dwKey == pIndex[dwMid].dwKey)
         return dwMid;
      if (dwKey < pIndex[dwMid].dwKey)
         dwLast = dwMid - 1;
      else
         dwFirst = dwMid + 1;
   }

   if (dwKey == pIndex[dwLast].dwKey)
      return dwLast;

   return INVALID_INDEX;
}


//
// Index comparision callback for qsort()
//

static int IndexCompare(const void *pArg1, const void *pArg2)
{
   return ((INDEX *)pArg1)->dwKey < ((INDEX *)pArg2)->dwKey ? -1 :
            (((INDEX *)pArg1)->dwKey > ((INDEX *)pArg2)->dwKey ? 1 : 0);
}


//
// Add object to list
//

void NXCL_Session::addObject(NXC_OBJECT *pObject, BOOL bSortIndex)
{
   DebugPrintf(_T("AddObject(id:%d, name:\"%s\")"), pObject->dwId, pObject->szName);
   lockObjectIndex();
   m_pIndexById = (INDEX *)realloc(m_pIndexById, sizeof(INDEX) * (m_dwNumObjects + 1));
   m_pIndexById[m_dwNumObjects].dwKey = pObject->dwId;
   m_pIndexById[m_dwNumObjects].pObject = pObject;
   m_dwNumObjects++;
   if (bSortIndex)
      qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
   unlockObjectIndex();
}


//
// Replace object's data in list
//

static void ReplaceObject(NXC_OBJECT *pObject, NXC_OBJECT *pNewObject)
{
   DebugPrintf(_T("ReplaceObject(id:%d, name:\"%s\")"), pObject->dwId, pObject->szName);
   switch(pObject->iClass)
   {
		case OBJECT_NODE:
			safe_free(pObject->node.pszAuthName);
			safe_free(pObject->node.pszAuthPassword);
			safe_free(pObject->node.pszPrivPassword);
			safe_free(pObject->node.pszSnmpObjectId);
			break;
      case OBJECT_NETWORKSERVICE:
         safe_free(pObject->netsrv.pszRequest);
         safe_free(pObject->netsrv.pszResponse);
         break;
      case OBJECT_VPNCONNECTOR:
         safe_free(pObject->vpnc.pLocalNetList);
         safe_free(pObject->vpnc.pRemoteNetList);
         break;
      case OBJECT_CONDITION:
         safe_free(pObject->cond.pszScript);
         safe_free(pObject->cond.pDCIList);
         break;
      case OBJECT_CLUSTER:
         safe_free(pObject->cluster.pSyncNetList);
			safe_free(pObject->cluster.pResourceList);
         break;
		case OBJECT_TEMPLATE:
			safe_free(pObject->dct.pszAutoApplyFilter);
			break;
		case OBJECT_CONTAINER:
			safe_free(pObject->container.pszAutoBindFilter);
			break;
   }
   safe_free(pObject->pdwChildList);
   safe_free(pObject->pdwParentList);
   safe_free(pObject->pAccessList);
   safe_free(pObject->pszComments);
	safe_free(pObject->pdwTrustedNodes);
	delete pObject->pCustomAttrs;
   memcpy(pObject, pNewObject, sizeof(NXC_OBJECT));
   free(pNewObject);
}


//
// Create new object from message
//

static NXC_OBJECT *NewObjectFromMsg(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject;
   UINT32 i, dwId1, dwId2, dwCount;
	WORD methods;

   // Allocate memory for new object structure
   pObject = (NXC_OBJECT *)malloc(sizeof(NXC_OBJECT));
   memset(pObject, 0, sizeof(NXC_OBJECT));

   // Common attributes
   pObject->dwId = pMsg->GetVariableLong(VID_OBJECT_ID);
   pObject->iClass = pMsg->GetVariableShort(VID_OBJECT_CLASS);
   pMsg->GetVariableStr(VID_OBJECT_NAME, pObject->szName, MAX_OBJECT_NAME);
   pObject->iStatus = pMsg->GetVariableShort(VID_OBJECT_STATUS);
   pObject->dwIpAddr = pMsg->GetVariableLong(VID_IP_ADDRESS);
   pObject->bIsDeleted = pMsg->GetVariableShort(VID_IS_DELETED);
   pObject->iStatusCalcAlg = pMsg->getFieldAsInt16(VID_STATUS_CALCULATION_ALG);
   pObject->iStatusPropAlg = pMsg->getFieldAsInt16(VID_STATUS_PROPAGATION_ALG);
   pObject->iFixedStatus = pMsg->getFieldAsInt16(VID_FIXED_STATUS);
   pObject->iStatusShift = pMsg->getFieldAsInt16(VID_STATUS_SHIFT);
   pObject->iStatusTrans[0] = pMsg->getFieldAsInt16(VID_STATUS_TRANSLATION_1);
   pObject->iStatusTrans[1] = pMsg->getFieldAsInt16(VID_STATUS_TRANSLATION_2);
   pObject->iStatusTrans[2] = pMsg->getFieldAsInt16(VID_STATUS_TRANSLATION_3);
   pObject->iStatusTrans[3] = pMsg->getFieldAsInt16(VID_STATUS_TRANSLATION_4);
   pObject->iStatusSingleTh = pMsg->getFieldAsInt16(VID_STATUS_SINGLE_THRESHOLD);
   pObject->iStatusThresholds[0] = pMsg->getFieldAsInt16(VID_STATUS_THRESHOLD_1);
   pObject->iStatusThresholds[1] = pMsg->getFieldAsInt16(VID_STATUS_THRESHOLD_2);
   pObject->iStatusThresholds[2] = pMsg->getFieldAsInt16(VID_STATUS_THRESHOLD_3);
   pObject->iStatusThresholds[3] = pMsg->getFieldAsInt16(VID_STATUS_THRESHOLD_4);
   pObject->pszComments = pMsg->GetVariableStr(VID_COMMENTS);

	pObject->geolocation.type = (int)pMsg->GetVariableShort(VID_GEOLOCATION_TYPE);
	pObject->geolocation.latitude = pMsg->getFieldAsDouble(VID_LATITUDE);
	pObject->geolocation.longitude = pMsg->getFieldAsDouble(VID_LONGITUDE);

	pObject->dwNumTrustedNodes = pMsg->GetVariableLong(VID_NUM_TRUSTED_NODES);
	if (pObject->dwNumTrustedNodes > 0)
	{
		pObject->pdwTrustedNodes = (UINT32 *)malloc(sizeof(UINT32) * pObject->dwNumTrustedNodes);
		pMsg->getFieldAsInt32Array(VID_TRUSTED_NODES, pObject->dwNumTrustedNodes, pObject->pdwTrustedNodes);
	}

	// Custom attributes
	pObject->pCustomAttrs = new StringMap;
	dwCount = pMsg->GetVariableLong(VID_NUM_CUSTOM_ATTRIBUTES);
	for(i = 0, dwId1 = VID_CUSTOM_ATTRIBUTES_BASE; i < dwCount; i++, dwId1 += 2)
	{
		pObject->pCustomAttrs->setPreallocated(pMsg->GetVariableStr(dwId1), pMsg->GetVariableStr(dwId1 + 1));
	}

   // Parents
   pObject->dwNumParents = pMsg->GetVariableLong(VID_PARENT_CNT);
   pObject->pdwParentList = (UINT32 *)malloc(sizeof(UINT32) * pObject->dwNumParents);
   for(i = 0, dwId1 = VID_PARENT_ID_BASE; i < pObject->dwNumParents; i++, dwId1++)
      pObject->pdwParentList[i] = pMsg->GetVariableLong(dwId1);

   // Childs
   pObject->dwNumChilds = pMsg->GetVariableLong(VID_CHILD_CNT);
   pObject->pdwChildList = (UINT32 *)malloc(sizeof(UINT32) * pObject->dwNumChilds);
   for(i = 0, dwId1 = VID_CHILD_ID_BASE; i < pObject->dwNumChilds; i++, dwId1++)
      pObject->pdwChildList[i] = pMsg->GetVariableLong(dwId1);

   // Access control
   pObject->bInheritRights = pMsg->GetVariableShort(VID_INHERIT_RIGHTS);
   pObject->dwAclSize = pMsg->GetVariableLong(VID_ACL_SIZE);
   pObject->pAccessList = (NXC_ACL_ENTRY *)malloc(sizeof(NXC_ACL_ENTRY) * pObject->dwAclSize);
   for(i = 0, dwId1 = VID_ACL_USER_BASE, dwId2 = VID_ACL_RIGHTS_BASE; 
       i < pObject->dwAclSize; i++, dwId1++, dwId2++)
   {
      pObject->pAccessList[i].dwUserId = pMsg->GetVariableLong(dwId1);
      pObject->pAccessList[i].dwAccessRights = pMsg->GetVariableLong(dwId2);
   }

   // Class-specific attributes
   switch(pObject->iClass)
   {
      case OBJECT_INTERFACE:
			pObject->iface.dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pObject->iface.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         pObject->iface.dwIfIndex = pMsg->GetVariableLong(VID_IF_INDEX);
         pObject->iface.dwIfType = pMsg->GetVariableLong(VID_IF_TYPE);
         pObject->iface.dwSlot = pMsg->GetVariableLong(VID_IF_SLOT);
         pObject->iface.dwPort = pMsg->GetVariableLong(VID_IF_PORT);
         pMsg->GetVariableBinary(VID_MAC_ADDR, pObject->iface.bMacAddr, MAC_ADDR_LENGTH);
			pObject->iface.wRequiredPollCount = pMsg->GetVariableShort(VID_REQUIRED_POLLS);
			pObject->iface.adminState = (BYTE)pMsg->GetVariableShort(VID_ADMIN_STATE);
			pObject->iface.operState = (BYTE)pMsg->GetVariableShort(VID_OPER_STATE);
         break;
      case OBJECT_NODE:
			pMsg->GetVariableStr(VID_PRIMARY_NAME, pObject->node.szPrimaryName, MAX_DNS_NAME);
         pObject->node.dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pObject->node.dwRuntimeFlags = pMsg->GetVariableLong(VID_RUNTIME_FLAGS);
         pObject->node.dwNodeType = pMsg->GetVariableLong(VID_NODE_TYPE);
         pObject->node.dwPollerNode = pMsg->GetVariableLong(VID_POLLER_NODE_ID);
         pObject->node.dwProxyNode = pMsg->GetVariableLong(VID_AGENT_PROXY);
         pObject->node.dwSNMPProxy = pMsg->GetVariableLong(VID_SNMP_PROXY);
         pObject->node.dwZoneId = pMsg->GetVariableLong(VID_ZONE_ID);
         pObject->node.wAgentPort = pMsg->GetVariableShort(VID_AGENT_PORT);
         pObject->node.wAuthMethod = pMsg->GetVariableShort(VID_AUTH_METHOD);
         pMsg->GetVariableStr(VID_SHARED_SECRET, pObject->node.szSharedSecret, MAX_SECRET_LENGTH);
         pObject->node.pszAuthName = pMsg->GetVariableStr(VID_SNMP_AUTH_OBJECT);
         pObject->node.pszAuthPassword = pMsg->GetVariableStr(VID_SNMP_AUTH_PASSWORD);
         pObject->node.pszPrivPassword = pMsg->GetVariableStr(VID_SNMP_PRIV_PASSWORD);
			methods = pMsg->GetVariableShort(VID_SNMP_USM_METHODS);
			pObject->node.wSnmpAuthMethod = methods & 0xFF;
			pObject->node.wSnmpPrivMethod = methods >> 8;
         pObject->node.pszSnmpObjectId = pMsg->GetVariableStr(VID_SNMP_OID);
         pObject->node.nSNMPVersion = (BYTE)pMsg->GetVariableShort(VID_SNMP_VERSION);
			pObject->node.wSnmpPort = pMsg->GetVariableShort(VID_SNMP_PORT);
         pMsg->GetVariableStr(VID_AGENT_VERSION, pObject->node.szAgentVersion, MAX_AGENT_VERSION_LEN);
         pMsg->GetVariableStr(VID_PLATFORM_NAME, pObject->node.szPlatformName, MAX_PLATFORM_NAME_LEN);
			pObject->node.wRequiredPollCount = pMsg->GetVariableShort(VID_REQUIRED_POLLS);
         pMsg->GetVariableStr(VID_SYS_DESCRIPTION, pObject->node.szSysDescription, MAX_DB_STRING);
			pObject->node.nUseIfXTable = (BYTE)pMsg->GetVariableShort(VID_USE_IFXTABLE);
         pMsg->GetVariableBinary(VID_BRIDGE_BASE_ADDRESS, pObject->node.bridgeBaseAddress, MAC_ADDR_LENGTH);
         break;
      case OBJECT_SUBNET:
         pObject->subnet.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         pObject->subnet.dwZoneId = pMsg->GetVariableLong(VID_ZONE_ID);
         break;
      case OBJECT_CONTAINER:
         pObject->container.dwCategory = pMsg->GetVariableLong(VID_CATEGORY);
			pObject->container.dwFlags = pMsg->GetVariableLong(VID_FLAGS);
			pObject->container.pszAutoBindFilter = pMsg->GetVariableStr(VID_AUTOBIND_FILTER);
         break;
      case OBJECT_TEMPLATE:
         pObject->dct.dwVersion = pMsg->GetVariableLong(VID_TEMPLATE_VERSION);
			pObject->dct.dwFlags = pMsg->GetVariableLong(VID_FLAGS);
			pObject->dct.pszAutoApplyFilter = pMsg->GetVariableStr(VID_AUTOBIND_FILTER);
         break;
      case OBJECT_NETWORKSERVICE:
         pObject->netsrv.iServiceType = (int)pMsg->GetVariableShort(VID_SERVICE_TYPE);
         pObject->netsrv.wProto = pMsg->GetVariableShort(VID_IP_PROTO);
         pObject->netsrv.wPort = pMsg->GetVariableShort(VID_IP_PORT);
         pObject->netsrv.dwPollerNode = pMsg->GetVariableLong(VID_POLLER_NODE_ID);
         pObject->netsrv.pszRequest = pMsg->GetVariableStr(VID_SERVICE_REQUEST);
         pObject->netsrv.pszResponse = pMsg->GetVariableStr(VID_SERVICE_RESPONSE);
			pObject->netsrv.wRequiredPollCount = pMsg->GetVariableShort(VID_REQUIRED_POLLS);
         break;
      case OBJECT_ZONE:
         pObject->zone.dwZoneId = pMsg->GetVariableLong(VID_ZONE_ID);
         pObject->zone.dwAgentProxy = pMsg->GetVariableLong(VID_AGENT_PROXY);
         pObject->zone.dwSnmpProxy = pMsg->GetVariableLong(VID_SNMP_PROXY);
         pObject->zone.dwIcmpProxy = pMsg->GetVariableLong(VID_ICMP_PROXY);
         break;
      case OBJECT_VPNCONNECTOR:
         pObject->vpnc.dwPeerGateway = pMsg->GetVariableLong(VID_PEER_GATEWAY);
         pObject->vpnc.dwNumLocalNets = pMsg->GetVariableLong(VID_NUM_LOCAL_NETS);
         pObject->vpnc.pLocalNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * pObject->vpnc.dwNumLocalNets);
         for(i = 0, dwId1 = VID_VPN_NETWORK_BASE; i < pObject->vpnc.dwNumLocalNets; i++)
         {
            pObject->vpnc.pLocalNetList[i].dwAddr = pMsg->GetVariableLong(dwId1++);
            pObject->vpnc.pLocalNetList[i].dwMask = pMsg->GetVariableLong(dwId1++);
         }
         pObject->vpnc.dwNumRemoteNets = pMsg->GetVariableLong(VID_NUM_REMOTE_NETS);
         pObject->vpnc.pRemoteNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * pObject->vpnc.dwNumRemoteNets);
         for(i = 0; i < pObject->vpnc.dwNumRemoteNets; i++)
         {
            pObject->vpnc.pRemoteNetList[i].dwAddr = pMsg->GetVariableLong(dwId1++);
            pObject->vpnc.pRemoteNetList[i].dwMask = pMsg->GetVariableLong(dwId1++);
         }
         break;
      case OBJECT_CONDITION:
         pObject->cond.dwActivationEvent = pMsg->GetVariableLong(VID_ACTIVATION_EVENT);
         pObject->cond.dwDeactivationEvent = pMsg->GetVariableLong(VID_DEACTIVATION_EVENT);
         pObject->cond.dwSourceObject = pMsg->GetVariableLong(VID_SOURCE_OBJECT);
         pObject->cond.pszScript = pMsg->GetVariableStr(VID_SCRIPT);
         pObject->cond.wActiveStatus = pMsg->GetVariableShort(VID_ACTIVE_STATUS);
         pObject->cond.wInactiveStatus = pMsg->GetVariableShort(VID_INACTIVE_STATUS);
         pObject->cond.dwNumDCI = pMsg->GetVariableLong(VID_NUM_ITEMS);
         pObject->cond.pDCIList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * pObject->cond.dwNumDCI);
         for(i = 0, dwId1 = VID_DCI_LIST_BASE; i < pObject->cond.dwNumDCI; i++)
         {
            pObject->cond.pDCIList[i].id = pMsg->GetVariableLong(dwId1++);
            pObject->cond.pDCIList[i].nodeId = pMsg->GetVariableLong(dwId1++);
            pObject->cond.pDCIList[i].function = pMsg->GetVariableShort(dwId1++);
            pObject->cond.pDCIList[i].polls = pMsg->GetVariableShort(dwId1++);
            dwId1 += 6;
         }
         break;
      case OBJECT_CLUSTER:
			pObject->cluster.dwZoneId = pMsg->GetVariableLong(VID_ZONE_ID);
         pObject->cluster.dwClusterType = pMsg->GetVariableLong(VID_CLUSTER_TYPE);
         pObject->cluster.dwNumSyncNets = pMsg->GetVariableLong(VID_NUM_SYNC_SUBNETS);
         pObject->cluster.pSyncNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * pObject->cluster.dwNumSyncNets);
			pMsg->getFieldAsInt32Array(VID_SYNC_SUBNETS, pObject->cluster.dwNumSyncNets * 2, (UINT32 *)pObject->cluster.pSyncNetList);
			pObject->cluster.dwNumResources = pMsg->GetVariableLong(VID_NUM_RESOURCES);
			if (pObject->cluster.dwNumResources > 0)
			{
				pObject->cluster.pResourceList = (CLUSTER_RESOURCE *)malloc(sizeof(CLUSTER_RESOURCE) * pObject->cluster.dwNumResources);
				for(i = 0, dwId1 = VID_RESOURCE_LIST_BASE; i < pObject->cluster.dwNumResources; i++, dwId1 += 6)
				{
					pObject->cluster.pResourceList[i].dwId = pMsg->GetVariableLong(dwId1++);
					pMsg->GetVariableStr(dwId1++, pObject->cluster.pResourceList[i].szName, MAX_DB_STRING);
					pObject->cluster.pResourceList[i].dwIpAddr = pMsg->GetVariableLong(dwId1++);
					pObject->cluster.pResourceList[i].dwCurrOwner = pMsg->GetVariableLong(dwId1++);
				}
			}
         break;
      default:
         break;
   }

   return pObject;
}


//
// Process object information received from server
//

void NXCL_Session::processObjectUpdate(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject, *pNewObject;

   switch(pMsg->GetCode())
   {
      case CMD_OBJECT_LIST_END:
         if (!(m_dwFlags & NXC_SF_HAS_OBJECT_CACHE))
         {
            lockObjectIndex();
            qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
            unlockObjectIndex();
         }
         CompleteSync(SYNC_OBJECTS, RCC_SUCCESS);
         break;
      case CMD_OBJECT:
         // Create new object from message and add it to list
         pNewObject = NewObjectFromMsg(pMsg);
         if (m_dwFlags & NXC_SF_HAS_OBJECT_CACHE)
         {
            // We already have some objects loaded from cache file
            pObject = findObjectById(pNewObject->dwId, TRUE);
            if (pObject == NULL)
            {
               addObject(pNewObject, TRUE);
            }
            else
            {
               ReplaceObject(pObject, pNewObject);
            }
         }
         else
         {
            // No cache file, all objects are new
            addObject(pNewObject, FALSE);
         }
         break;
      case CMD_OBJECT_UPDATE:
         pNewObject = NewObjectFromMsg(pMsg);
         pObject = findObjectById(pNewObject->dwId, TRUE);
         if (pObject == NULL)
         {
            addObject(pNewObject, TRUE);
            pObject = pNewObject;
         }
         else
         {
            ReplaceObject(pObject, pNewObject);
         }
         callEventHandler(NXC_EVENT_OBJECT_CHANGED, pObject->dwId, pObject);
         break;
      default:
         break;
   }
}


//
// Synchronize objects with the server
// This function is NOT REENTRANT
//

UINT32 NXCL_Session::syncObjects(const TCHAR *pszCacheFile, BOOL bSyncComments)
{
   CSCPMessage msg;
   UINT32 dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync(SYNC_OBJECTS);

   destroyAllObjects();

   m_dwFlags &= ~NXC_SF_HAS_OBJECT_CACHE;
   if (pszCacheFile != NULL)
      loadObjectsFromCache(pszCacheFile);

   msg.SetCode(CMD_GET_OBJECTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_TIMESTAMP, m_dwTimeStamp);
   msg.SetVariable(VID_SYNC_COMMENTS, (WORD)bSyncComments);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   // If request was successful, wait for object list end or for disconnection
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = WaitForSync(SYNC_OBJECTS, INFINITE);
   else
      UnlockSyncOp(SYNC_OBJECTS);

   return dwRetCode;
}


//
// Wrappers for NXCL_Session::SyncObjects()
//

UINT32 LIBNXCL_EXPORTABLE NXCSyncObjects(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->syncObjects(NULL, FALSE);
}

UINT32 LIBNXCL_EXPORTABLE NXCSyncObjectsEx(NXC_SESSION hSession, const TCHAR *pszCacheFile,
                                          BOOL bSyncComments)
{
   return ((NXCL_Session *)hSession)->syncObjects(pszCacheFile, bSyncComments);
}


//
// Synchronize object set
//

UINT32 LIBNXCL_EXPORTABLE NXCSyncObjectSet(NXC_SESSION hSession, UINT32 *idList, UINT32 length, bool syncComments, WORD flags)
{
   UINT32 dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   CSCPMessage msg;
   msg.SetCode(CMD_GET_SELECTED_OBJECTS);
   msg.SetId(dwRqId);
	msg.SetVariable(VID_SYNC_COMMENTS, (WORD)(syncComments ? 1 : 0));
	msg.SetVariable(VID_FLAGS, (WORD)(flags | OBJECT_SYNC_SEND_UPDATES));	// C library requres objects to go in update messages
   msg.SetVariable(VID_NUM_OBJECTS, length);
	msg.setFieldInt32Array(VID_OBJECT_LIST, length, idList);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   UINT32 rcc = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
	if ((rcc == RCC_SUCCESS) && (flags & OBJECT_SYNC_DUAL_CONFIRM))
	{
		rcc = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
	}
	return rcc;
}


//
// Synchronize single object
// Simple wrapper for NXCSyncObjectSet which syncs comments and waits for sync completion
//

UINT32 LIBNXCL_EXPORTABLE NXCSyncSingleObject(NXC_SESSION hSession, UINT32 objectId)
{
	return NXCSyncObjectSet(hSession, &objectId, 1, true, OBJECT_SYNC_DUAL_CONFIRM);
}


//
// Find object by ID
//

NXC_OBJECT *NXCL_Session::findObjectById(UINT32 dwId, BOOL bLock)
{
   UINT32 dwPos;
   NXC_OBJECT *pObject;

   if (bLock)
      lockObjectIndex();

   dwPos = SearchIndex(m_pIndexById, m_dwNumObjects, dwId);
   pObject = (dwPos == INVALID_INDEX) ? NULL : m_pIndexById[dwPos].pObject;

   if (bLock)
      unlockObjectIndex();

   return pObject;
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectById(NXC_SESSION hSession, UINT32 dwId)
{
	return (hSession != NULL) ? ((NXCL_Session *)hSession)->findObjectById(dwId, TRUE) : NULL;
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByIdNoLock(NXC_SESSION hSession, UINT32 dwId)
{
	return (hSession != NULL) ? ((NXCL_Session *)hSession)->findObjectById(dwId, FALSE) : NULL;
}


//
// Find object by name
//

NXC_OBJECT *NXCL_Session::findObjectByName(const TCHAR *name, UINT32 dwCurrObject)
{
   NXC_OBJECT *pObject = NULL;
   UINT32 i;

   if (name != NULL)
      if (*name != 0)
      {
         lockObjectIndex();

			if (dwCurrObject != 0)
			{
				pObject = findObjectById(dwCurrObject, FALSE);
				if (pObject != NULL)
				{
	            if (!RegexpMatch(pObject->szName, name, FALSE))
					{
						// Current object doesn't match, start search from the beginning
						dwCurrObject = 0;
					}
				}
				else
				{
					dwCurrObject = 0;
				}
				pObject = NULL;
			}

         for(i = 0; i < m_dwNumObjects; i++)
            if (RegexpMatch(m_pIndexById[i].pObject->szName, name, FALSE))
            {
					if (dwCurrObject == 0)
					{
						pObject = m_pIndexById[i].pObject;
						break;
					}
					else
					{
						if (m_pIndexById[i].dwKey == dwCurrObject)
						{
							dwCurrObject = 0;	// Next match will stop the loop
						}
					}
            }

         unlockObjectIndex();
      }
   return pObject;
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByName(NXC_SESSION hSession, const TCHAR *pszName, UINT32 dwCurrObject)
{
	return (hSession != NULL) ? ((NXCL_Session *)hSession)->findObjectByName(pszName, dwCurrObject) : NULL;
}


//
// Find object by comments
//

NXC_OBJECT *NXCL_Session::findObjectByComments(const TCHAR *comments, UINT32 dwCurrObject)
{
   NXC_OBJECT *pObject = NULL;
   UINT32 i;

   if (comments != NULL)
      if (*comments != 0)
      {
         lockObjectIndex();

			if (dwCurrObject != 0)
			{
				pObject = findObjectById(dwCurrObject, FALSE);
				if (pObject != NULL)
				{
					if (!RegexpMatch(CHECK_NULL_EX(pObject->pszComments), comments, FALSE))
					{
						// Current object doesn't match, start search from the beginning
						dwCurrObject = 0;
					}
				}
				else
				{
					dwCurrObject = 0;
				}
				pObject = NULL;
			}

         for(i = 0; i < m_dwNumObjects; i++)
				if (RegexpMatch(CHECK_NULL_EX(m_pIndexById[i].pObject->pszComments), comments, FALSE))
            {
					if (dwCurrObject == 0)
					{
						pObject = m_pIndexById[i].pObject;
						break;
					}
					else
					{
						if (m_pIndexById[i].dwKey == dwCurrObject)
						{
							dwCurrObject = 0;	// Next match will stop the loop
						}
					}
            }

         unlockObjectIndex();
      }
   return pObject;
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByComments(NXC_SESSION hSession, const TCHAR *comments, UINT32 dwCurrObject)
{
	return (hSession != NULL) ? ((NXCL_Session *)hSession)->findObjectByComments(comments, dwCurrObject) : NULL;
}


//
// Find object by IP address
//

NXC_OBJECT *NXCL_Session::findObjectByIPAddress(UINT32 dwIpAddr, UINT32 dwCurrObject)
{
   NXC_OBJECT *pObject = NULL;
   UINT32 i;

   lockObjectIndex();

	if (dwCurrObject != 0)
	{
		pObject = findObjectById(dwCurrObject, FALSE);
		if (pObject != NULL)
		{
	      if (pObject->dwIpAddr != dwIpAddr)
			{
				// Current object doesn't match, start search from the beginning
				dwCurrObject = 0;
			}
		}
		else
		{
			dwCurrObject = 0;
		}
		pObject = NULL;
	}

   for(i = 0; i < m_dwNumObjects; i++)
      if (m_pIndexById[i].pObject->dwIpAddr == dwIpAddr)
      {
			if (dwCurrObject == 0)
			{
				pObject = m_pIndexById[i].pObject;
				break;
			}
			else
			{
				if (m_pIndexById[i].dwKey == dwCurrObject)
				{
					dwCurrObject = 0;	// Next match will stop the loop
				}
			}
      }

   unlockObjectIndex();
   return pObject;
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByIPAddress(NXC_SESSION hSession, UINT32 dwIpAddr, UINT32 dwCurrObject)
{
	return (hSession != NULL) ? ((NXCL_Session *)hSession)->findObjectByIPAddress(dwIpAddr, dwCurrObject) : NULL;
}


//
// Enumerate all objects
//

void NXCL_Session::EnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *))
{
   UINT32 i;

   lockObjectIndex();
   for(i = 0; i < m_dwNumObjects; i++)
      if (!pHandler(m_pIndexById[i].pObject))
         break;
   unlockObjectIndex();
}

void LIBNXCL_EXPORTABLE NXCEnumerateObjects(NXC_SESSION hSession, BOOL (* pHandler)(NXC_OBJECT *))
{
   ((NXCL_Session *)hSession)->EnumerateObjects(pHandler);
}


//
// Get root object
//

NXC_OBJECT *NXCL_Session::GetRootObject(UINT32 dwId, UINT32 dwIndex)
{
   if (m_dwNumObjects > dwIndex)
      if (m_pIndexById[dwIndex].dwKey == dwId)
         return m_pIndexById[dwIndex].pObject;
   return NULL;
}


//
// Get topology root ("Entire Network") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTopologyRootObject(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->GetRootObject(1, 0);
}


//
// Get service tree root ("All Services") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetServiceRootObject(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->GetRootObject(2, 1);
}


//
// Get template tree root ("Templates") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTemplateRootObject(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->GetRootObject(3, 2);
}


//
// Get pointer to first object on objects' list and entire number of objects
//

void *NXCL_Session::GetObjectIndex(UINT32 *pdwNumObjects)
{
   if (pdwNumObjects != NULL)
      *pdwNumObjects = m_dwNumObjects;
   return m_pIndexById;
}

void LIBNXCL_EXPORTABLE *NXCGetObjectIndex(NXC_SESSION hSession, UINT32 *pdwNumObjects)
{
   return ((NXCL_Session *)hSession)->GetObjectIndex(pdwNumObjects);
}


//
// Lock object index
//

void LIBNXCL_EXPORTABLE NXCLockObjectIndex(NXC_SESSION hSession)
{
   ((NXCL_Session *)hSession)->lockObjectIndex();
}


//
// Unlock object index
//

void LIBNXCL_EXPORTABLE NXCUnlockObjectIndex(NXC_SESSION hSession)
{
   ((NXCL_Session *)hSession)->unlockObjectIndex();
}


//
// Modify object
//

UINT32 LIBNXCL_EXPORTABLE NXCModifyObject(NXC_SESSION hSession, NXC_OBJECT_UPDATE *pUpdate)
{
   CSCPMessage msg;
   UINT32 dwRqId, i, dwId1, dwId2;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_MODIFY_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, pUpdate->dwObjectId);
   if (pUpdate->qwFlags & OBJ_UPDATE_NAME)
      msg.SetVariable(VID_OBJECT_NAME, pUpdate->pszName);
   if (pUpdate->qwFlags & OBJ_UPDATE_PRIMARY_NAME)
      msg.SetVariable(VID_PRIMARY_NAME, pUpdate->pszPrimaryName);
   if (pUpdate->qwFlags & OBJ_UPDATE_AGENT_PORT)
      msg.SetVariable(VID_AGENT_PORT, (WORD)pUpdate->iAgentPort);
   if (pUpdate->qwFlags & OBJ_UPDATE_AGENT_AUTH)
      msg.SetVariable(VID_AUTH_METHOD, (WORD)pUpdate->iAuthType);
   if (pUpdate->qwFlags & OBJ_UPDATE_AGENT_SECRET)
      msg.SetVariable(VID_SHARED_SECRET, pUpdate->pszSecret);
   if (pUpdate->qwFlags & OBJ_UPDATE_SNMP_AUTH)
	{
      msg.SetVariable(VID_SNMP_AUTH_OBJECT, pUpdate->pszAuthName);
      msg.SetVariable(VID_SNMP_AUTH_PASSWORD, CHECK_NULL_EX(pUpdate->pszAuthPassword));
      msg.SetVariable(VID_SNMP_PRIV_PASSWORD, CHECK_NULL_EX(pUpdate->pszPrivPassword));
		msg.SetVariable(VID_SNMP_USM_METHODS, (WORD)(pUpdate->wSnmpAuthMethod | (pUpdate->wSnmpPrivMethod << 8)));
	}
   if (pUpdate->qwFlags & OBJ_UPDATE_SNMP_VERSION)
      msg.SetVariable(VID_SNMP_VERSION, pUpdate->wSNMPVersion);
   if (pUpdate->qwFlags & OBJ_UPDATE_CHECK_REQUEST)
      msg.SetVariable(VID_SERVICE_REQUEST, pUpdate->pszRequest);
   if (pUpdate->qwFlags & OBJ_UPDATE_CHECK_RESPONSE)
      msg.SetVariable(VID_SERVICE_RESPONSE, pUpdate->pszResponse);
   if (pUpdate->qwFlags & OBJ_UPDATE_IP_PROTO)
      msg.SetVariable(VID_IP_PROTO, pUpdate->wProto);
   if (pUpdate->qwFlags & OBJ_UPDATE_IP_PORT)
      msg.SetVariable(VID_IP_PORT, pUpdate->wPort);
   if (pUpdate->qwFlags & OBJ_UPDATE_SERVICE_TYPE)
      msg.SetVariable(VID_SERVICE_TYPE, (WORD)pUpdate->iServiceType);
   if (pUpdate->qwFlags & OBJ_UPDATE_POLLER_NODE)
      msg.SetVariable(VID_POLLER_NODE_ID, pUpdate->dwPollerNode);
   if (pUpdate->qwFlags & OBJ_UPDATE_PROXY_NODE)
      msg.SetVariable(VID_AGENT_PROXY, pUpdate->dwProxyNode);
   if (pUpdate->qwFlags & OBJ_UPDATE_SNMP_PROXY)
      msg.SetVariable(VID_SNMP_PROXY, pUpdate->dwSNMPProxy);
   if (pUpdate->qwFlags & OBJ_UPDATE_IP_ADDR)
      msg.SetVariable(VID_IP_ADDRESS, pUpdate->dwIpAddr);
   if (pUpdate->qwFlags & OBJ_UPDATE_PEER_GATEWAY)
      msg.SetVariable(VID_PEER_GATEWAY, pUpdate->dwPeerGateway);
   if (pUpdate->qwFlags & OBJ_UPDATE_FLAGS)
      msg.SetVariable(VID_FLAGS, pUpdate->dwObjectFlags);
   if (pUpdate->qwFlags & OBJ_UPDATE_ACT_EVENT)
      msg.SetVariable(VID_ACTIVATION_EVENT, pUpdate->dwActivationEvent);
   if (pUpdate->qwFlags & OBJ_UPDATE_DEACT_EVENT)
      msg.SetVariable(VID_DEACTIVATION_EVENT, pUpdate->dwDeactivationEvent);
   if (pUpdate->qwFlags & OBJ_UPDATE_SOURCE_OBJECT)
      msg.SetVariable(VID_SOURCE_OBJECT, pUpdate->dwSourceObject);
   if (pUpdate->qwFlags & OBJ_UPDATE_ACTIVE_STATUS)
      msg.SetVariable(VID_ACTIVE_STATUS, (WORD)pUpdate->nActiveStatus);
   if (pUpdate->qwFlags & OBJ_UPDATE_INACTIVE_STATUS)
      msg.SetVariable(VID_INACTIVE_STATUS, (WORD)pUpdate->nInactiveStatus);
   if (pUpdate->qwFlags & OBJ_UPDATE_SCRIPT)
      msg.SetVariable(VID_SCRIPT, pUpdate->pszScript);
   if (pUpdate->qwFlags & OBJ_UPDATE_CLUSTER_TYPE)
      msg.SetVariable(VID_CLUSTER_TYPE, pUpdate->dwClusterType);
   if (pUpdate->qwFlags & OBJ_UPDATE_REQUIRED_POLLS)
      msg.SetVariable(VID_REQUIRED_POLLS, pUpdate->wRequiredPollCount);
   if (pUpdate->qwFlags & OBJ_UPDATE_USE_IFXTABLE)
      msg.SetVariable(VID_USE_IFXTABLE, (WORD)pUpdate->nUseIfXTable);
   if (pUpdate->qwFlags & OBJ_UPDATE_STATUS_ALG)
   {
      msg.SetVariable(VID_STATUS_CALCULATION_ALG, (WORD)pUpdate->iStatusCalcAlg);
      msg.SetVariable(VID_STATUS_PROPAGATION_ALG, (WORD)pUpdate->iStatusPropAlg);
      msg.SetVariable(VID_FIXED_STATUS, (WORD)pUpdate->iFixedStatus);
      msg.SetVariable(VID_STATUS_SHIFT, (WORD)pUpdate->iStatusShift);
      msg.SetVariable(VID_STATUS_TRANSLATION_1, (WORD)pUpdate->iStatusTrans[0]);
      msg.SetVariable(VID_STATUS_TRANSLATION_2, (WORD)pUpdate->iStatusTrans[1]);
      msg.SetVariable(VID_STATUS_TRANSLATION_3, (WORD)pUpdate->iStatusTrans[2]);
      msg.SetVariable(VID_STATUS_TRANSLATION_4, (WORD)pUpdate->iStatusTrans[3]);
      msg.SetVariable(VID_STATUS_SINGLE_THRESHOLD, (WORD)pUpdate->iStatusSingleTh);
      msg.SetVariable(VID_STATUS_THRESHOLD_1, (WORD)pUpdate->iStatusThresholds[0]);
      msg.SetVariable(VID_STATUS_THRESHOLD_2, (WORD)pUpdate->iStatusThresholds[1]);
      msg.SetVariable(VID_STATUS_THRESHOLD_3, (WORD)pUpdate->iStatusThresholds[2]);
      msg.SetVariable(VID_STATUS_THRESHOLD_4, (WORD)pUpdate->iStatusThresholds[3]);
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_NETWORK_LIST)
   {
      msg.SetVariable(VID_NUM_LOCAL_NETS, pUpdate->dwNumLocalNets);
      msg.SetVariable(VID_NUM_REMOTE_NETS, pUpdate->dwNumRemoteNets);
      for(i = 0, dwId1 = VID_VPN_NETWORK_BASE; i < pUpdate->dwNumLocalNets; i++)
      {
         msg.SetVariable(dwId1++, pUpdate->pLocalNetList[i].dwAddr);
         msg.SetVariable(dwId1++, pUpdate->pLocalNetList[i].dwMask);
      }
      for(i = 0; i < pUpdate->dwNumRemoteNets; i++)
      {
         msg.SetVariable(dwId1++, pUpdate->pRemoteNetList[i].dwAddr);
         msg.SetVariable(dwId1++, pUpdate->pRemoteNetList[i].dwMask);
      }
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_ACL)
   {
      msg.SetVariable(VID_ACL_SIZE, pUpdate->dwAclSize);
      msg.SetVariable(VID_INHERIT_RIGHTS, (WORD)pUpdate->bInheritRights);
      for(i = 0, dwId1 = VID_ACL_USER_BASE, dwId2 = VID_ACL_RIGHTS_BASE;
          i < pUpdate->dwAclSize; i++, dwId1++, dwId2++)
      {
         msg.SetVariable(dwId1, pUpdate->pAccessList[i].dwUserId);
         msg.SetVariable(dwId2, pUpdate->pAccessList[i].dwAccessRights);
      }
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_DCI_LIST)
   {
      msg.SetVariable(VID_NUM_ITEMS, pUpdate->dwNumDCI);
      for(i = 0, dwId1 = VID_DCI_LIST_BASE; i < pUpdate->dwNumDCI; i++)
      {
         msg.SetVariable(dwId1++, pUpdate->pDCIList[i].id);
         msg.SetVariable(dwId1++, pUpdate->pDCIList[i].nodeId);
         msg.SetVariable(dwId1++, (WORD)pUpdate->pDCIList[i].function);
         msg.SetVariable(dwId1++, (WORD)pUpdate->pDCIList[i].polls);
         dwId1 += 6;
      }
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_SYNC_NETS)
   {
      msg.SetVariable(VID_NUM_SYNC_SUBNETS, pUpdate->dwNumSyncNets);
		msg.setFieldInt32Array(VID_SYNC_SUBNETS, pUpdate->dwNumSyncNets * 2, (UINT32 *)pUpdate->pSyncNetList);
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_RESOURCES)
   {
      msg.SetVariable(VID_NUM_RESOURCES, pUpdate->dwNumResources);
      for(i = 0, dwId1 = VID_RESOURCE_LIST_BASE; i < pUpdate->dwNumResources; i++, dwId1 += 7)
      {
         msg.SetVariable(dwId1++, pUpdate->pResourceList[i].dwId);
         msg.SetVariable(dwId1++, pUpdate->pResourceList[i].szName);
         msg.SetVariable(dwId1++, pUpdate->pResourceList[i].dwIpAddr);
      }
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_TRUSTED_NODES)
   {
      msg.SetVariable(VID_NUM_TRUSTED_NODES, pUpdate->dwNumTrustedNodes);
		msg.setFieldInt32Array(VID_TRUSTED_NODES, pUpdate->dwNumTrustedNodes, pUpdate->pdwTrustedNodes);
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_CUSTOM_ATTRS)
   {
      msg.SetVariable(VID_NUM_CUSTOM_ATTRIBUTES, pUpdate->pCustomAttrs->getSize());
      for(i = 0, dwId1 = VID_CUSTOM_ATTRIBUTES_BASE; i < pUpdate->pCustomAttrs->getSize(); i++)
      {
         msg.SetVariable(dwId1++, pUpdate->pCustomAttrs->getKeyByIndex(i));
         msg.SetVariable(dwId1++, pUpdate->pCustomAttrs->getValueByIndex(i));
      }
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_AUTOBIND)
   {
		msg.SetVariable(VID_AUTOBIND_FILTER, CHECK_NULL_EX(pUpdate->pszAutoBindFilter));
   }
   if (pUpdate->qwFlags & OBJ_UPDATE_GEOLOCATION)
   {
		msg.SetVariable(VID_GEOLOCATION_TYPE, (WORD)pUpdate->geolocation.type);
		msg.SetVariable(VID_LATITUDE, pUpdate->geolocation.latitude);
		msg.SetVariable(VID_LONGITUDE, pUpdate->geolocation.longitude);
	}
   if (pUpdate->qwFlags & OBJ_UPDATE_SNMP_PORT)
      msg.SetVariable(VID_SNMP_PORT, pUpdate->wSnmpPort);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Set object's mamagement status
//

UINT32 LIBNXCL_EXPORTABLE NXCSetObjectMgmtStatus(NXC_SESSION hSession, UINT32 dwObjectId, 
                                                BOOL bIsManaged)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_SET_OBJECT_MGMT_STATUS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_MGMT_STATUS, (WORD)bIsManaged);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Create new object
//

UINT32 LIBNXCL_EXPORTABLE NXCCreateObject(NXC_SESSION hSession, 
                                         NXC_OBJECT_CREATE_INFO *pCreateInfo, 
                                         UINT32 *pdwObjectId)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_CREATE_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARENT_ID, pCreateInfo->dwParentId);
   msg.SetVariable(VID_OBJECT_CLASS, (WORD)pCreateInfo->iClass);
   msg.SetVariable(VID_OBJECT_NAME, pCreateInfo->pszName);
	if (pCreateInfo->pszComments != NULL)
	   msg.SetVariable(VID_COMMENTS, pCreateInfo->pszComments);
   switch(pCreateInfo->iClass)
   {
      case OBJECT_NODE:
			if (pCreateInfo->cs.node.pszPrimaryName != NULL)
				msg.SetVariable(VID_PRIMARY_NAME, pCreateInfo->cs.node.pszPrimaryName);
         msg.SetVariable(VID_IP_ADDRESS, pCreateInfo->cs.node.dwIpAddr);
         msg.SetVariable(VID_IP_NETMASK, pCreateInfo->cs.node.dwNetMask);
         msg.SetVariable(VID_CREATION_FLAGS, pCreateInfo->cs.node.dwCreationFlags);
         msg.SetVariable(VID_AGENT_PROXY, pCreateInfo->cs.node.dwProxyNode);
         msg.SetVariable(VID_SNMP_PROXY, pCreateInfo->cs.node.dwSNMPProxy);
         break;
      case OBJECT_CONTAINER:
         msg.SetVariable(VID_CATEGORY, pCreateInfo->cs.container.dwCategory);
         break;
      case OBJECT_NETWORKSERVICE:
         msg.SetVariable(VID_SERVICE_TYPE, (WORD)pCreateInfo->cs.netsrv.iServiceType);
         msg.SetVariable(VID_IP_PROTO, pCreateInfo->cs.netsrv.wProto);
         msg.SetVariable(VID_IP_PORT, pCreateInfo->cs.netsrv.wPort);
         msg.SetVariable(VID_SERVICE_REQUEST, pCreateInfo->cs.netsrv.pszRequest);
         msg.SetVariable(VID_SERVICE_RESPONSE, pCreateInfo->cs.netsrv.pszResponse);
			msg.SetVariable(VID_CREATE_STATUS_DCI, (WORD)(pCreateInfo->cs.netsrv.createStatusDci ? 1 : 0));
         break;
      default:
         break;
   }

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response. Creating node object can include polling,
   // which can take a minute or even more in worst cases
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 300000);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwObjectId = pResponse->GetVariableLong(VID_OBJECT_ID);
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
// Bind/unbind objects
//

static UINT32 ChangeObjectBinding(NXCL_Session *pSession, UINT32 dwParentObject,
                                 UINT32 dwChildObject, BOOL bBind, BOOL bRemoveDCI)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = pSession->CreateRqId();

   // Build request message
   msg.SetCode(bBind ? CMD_BIND_OBJECT : CMD_UNBIND_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARENT_ID, dwParentObject);
   msg.SetVariable(VID_CHILD_ID, dwChildObject);
   if (!bBind)
      msg.SetVariable(VID_REMOVE_DCI, (WORD)bRemoveDCI);

   // Send request
   pSession->SendMsg(&msg);

   // Wait for reply
   return pSession->WaitForRCC(dwRqId);
}


//
// Bind object
//

UINT32 LIBNXCL_EXPORTABLE NXCBindObject(NXC_SESSION hSession, UINT32 dwParentObject, 
                                       UINT32 dwChildObject)
{
   return ChangeObjectBinding((NXCL_Session *)hSession, dwParentObject,
                              dwChildObject, TRUE, FALSE);
}


//
// Unbind object
//

UINT32 LIBNXCL_EXPORTABLE NXCUnbindObject(NXC_SESSION hSession, UINT32 dwParentObject,
                                         UINT32 dwChildObject)
{
   return ChangeObjectBinding((NXCL_Session *)hSession, dwParentObject, 
                              dwChildObject, FALSE, FALSE);
}


//
// Remove template from node
//

UINT32 LIBNXCL_EXPORTABLE NXCRemoveTemplate(NXC_SESSION hSession, UINT32 dwTemplateId,
                                           UINT32 dwNodeId, BOOL bRemoveDCI)
{
   return ChangeObjectBinding((NXCL_Session *)hSession, dwTemplateId, 
                              dwNodeId, FALSE, bRemoveDCI);
}


//
// Add cluster node
//

UINT32 LIBNXCL_EXPORTABLE NXCAddClusterNode(NXC_SESSION hSession, UINT32 clusterId, UINT32 nodeId)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
	msg.SetCode(CMD_ADD_CLUSTER_NODE);
   msg.SetId(dwRqId);
	msg.SetVariable(VID_PARENT_ID, clusterId);
	msg.SetVariable(VID_CHILD_ID, nodeId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete object
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteObject(NXC_SESSION hSession, UINT32 dwObject)
{
   CSCPMessage msg;
   UINT32 dwRqId, rcc;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_DELETE_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObject);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   rcc = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
	if (rcc == RCC_SUCCESS)
	{
		NXC_OBJECT *object = ((NXCL_Session *)hSession)->findObjectById(dwObject, TRUE);
		if (object != NULL)
		{
			object->bIsDeleted = TRUE;
			((NXCL_Session *)hSession)->callEventHandler(NXC_EVENT_OBJECT_CHANGED, object->dwId, object);
		}
	}
	return rcc;
}


//
// Load container categories
//

UINT32 LIBNXCL_EXPORTABLE NXCLoadCCList(NXC_SESSION hSession, NXC_CC_LIST **ppList)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode = RCC_SUCCESS, dwNumCats = 0, dwCatId = 0;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_CONTAINER_CAT_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *ppList = (NXC_CC_LIST *)malloc(sizeof(NXC_CC_LIST));
   (*ppList)->dwNumElements = 0;
   (*ppList)->pElements = NULL;

   do
   {
      pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_CONTAINER_CAT_DATA, dwRqId);
      if (pResponse != NULL)
      {
         dwCatId = pResponse->GetVariableLong(VID_CATEGORY_ID);
         if (dwCatId != 0)  // 0 is end of list indicator
         {
            (*ppList)->pElements = (NXC_CONTAINER_CATEGORY *)realloc((*ppList)->pElements, 
               sizeof(NXC_CONTAINER_CATEGORY) * ((*ppList)->dwNumElements + 1));
            (*ppList)->pElements[(*ppList)->dwNumElements].dwId = dwCatId;
            pResponse->GetVariableStr(VID_CATEGORY_NAME, 
               (*ppList)->pElements[(*ppList)->dwNumElements].szName, MAX_OBJECT_NAME);
            (*ppList)->pElements[(*ppList)->dwNumElements].pszDescription =
               pResponse->GetVariableStr(VID_DESCRIPTION);
            (*ppList)->dwNumElements++;
         }
         delete pResponse;
      }
      else
      {
         dwRetCode = RCC_TIMEOUT;
         dwCatId = 0;
      }
   }
   while(dwCatId != 0);

   // Destroy results on failure
   if (dwRetCode != RCC_SUCCESS)
   {
      safe_free((*ppList)->pElements);
      free(*ppList);
      *ppList = NULL;
   }

   return dwRetCode;
}


//
// Destroy list of container categories
//

void LIBNXCL_EXPORTABLE NXCDestroyCCList(NXC_CC_LIST *pList)
{
   UINT32 i;

   if (pList == NULL)
      return;

   for(i = 0; i < pList->dwNumElements; i++)
      safe_free(pList->pElements[i].pszDescription);
   safe_free(pList->pElements);
   free(pList);
}


//
// Perform a forced node poll
//

UINT32 LIBNXCL_EXPORTABLE NXCPollNode(NXC_SESSION hSession, UINT32 dwObjectId, int iPollType, 
                                     void (* pfCallback)(TCHAR *, void *), void *pArg)
{
   UINT32 dwRetCode, dwRqId;
   CSCPMessage msg, *pResponse;
   TCHAR *pszMsg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_POLL_NODE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_POLL_TYPE, (WORD)iPollType);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   do
   {
      // Polls can take a long time, so we wait up to 120 seconds for each message
      pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_POLLING_INFO, dwRqId, 120000);
      if (pResponse != NULL)
      {
         dwRetCode = pResponse->GetVariableLong(VID_RCC);
         if ((dwRetCode == RCC_OPERATION_IN_PROGRESS) && (pfCallback != NULL))
         {
            pszMsg = pResponse->GetVariableStr(VID_POLLER_MESSAGE);
            pfCallback(pszMsg, pArg);
            free(pszMsg);
         }
         delete pResponse;
      }
      else
      {
         dwRetCode = RCC_TIMEOUT;
      }
   }
   while(dwRetCode == RCC_OPERATION_IN_PROGRESS);

   return dwRetCode;
}


//
// Wake up node by sending magic packet
//

UINT32 LIBNXCL_EXPORTABLE NXCWakeUpNode(NXC_SESSION hSession, UINT32 dwObjectId)
{
   UINT32 dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_WAKEUP_NODE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Retrieve list of supported agent parameters
//

UINT32 LIBNXCL_EXPORTABLE NXCGetSupportedParameters(NXC_SESSION hSession, UINT32 dwNodeId,
                                                   UINT32 *pdwNumParams, 
                                                   NXC_AGENT_PARAM **ppParamList)
{
   CSCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   *pdwNumParams = 0;
   *ppParamList = NULL;

   // Build request message
   msg.SetCode(CMD_GET_PARAMETER_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwNumParams = pResponse->GetVariableLong(VID_NUM_PARAMETERS);
         *ppParamList = (NXC_AGENT_PARAM *)malloc(sizeof(NXC_AGENT_PARAM) * *pdwNumParams);
         for(i = 0, dwId = VID_PARAM_LIST_BASE; i < *pdwNumParams; i++)
         {
            pResponse->GetVariableStr(dwId++, (*ppParamList)[i].szName, MAX_PARAM_NAME);
            pResponse->GetVariableStr(dwId++, (*ppParamList)[i].szDescription, MAX_DB_STRING);
            (*ppParamList)[i].iDataType = (int)pResponse->GetVariableShort(dwId++);
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
// Check if object has default name formed from IP address
//

static BOOL ObjectHasDefaultName(NXC_OBJECT *pObject)
{
   if (pObject->iClass == OBJECT_SUBNET)
   {
      TCHAR szBuffer[64], szIpAddr[32];
      _sntprintf(szBuffer, 64, _T("%s/%d"), IpToStr(pObject->dwIpAddr, szIpAddr),
                 BitsInMask(pObject->subnet.dwIpNetMask));
      return !_tcscmp(szBuffer, pObject->szName);
   }
   else
   {
      return ((pObject->dwIpAddr != 0) &&
              (ntohl(_t_inet_addr(pObject->szName)) == pObject->dwIpAddr));
   }
}


//
// Get object name suitable for comparision
//

void LIBNXCL_EXPORTABLE NXCGetComparableObjectName(NXC_SESSION hSession, UINT32 dwObjectId, TCHAR *pszName)
{
   NXCGetComparableObjectNameEx(((NXCL_Session *)hSession)->findObjectById(dwObjectId, TRUE), pszName);
}


//
// Get object name suitable for comparision
//

void LIBNXCL_EXPORTABLE NXCGetComparableObjectNameEx(NXC_OBJECT *pObject, TCHAR *pszName)
{
   if (pObject != NULL)
   {
      // If object has an IP address as name, we sort as numbers
      // otherwise in alphabetical order
      if (ObjectHasDefaultName(pObject))
      {
         _sntprintf(pszName, MAX_OBJECT_NAME, _T("\x01%03d%03d%03d%03d"),
                    pObject->dwIpAddr >> 24, (pObject->dwIpAddr >> 16) & 255,
                    (pObject->dwIpAddr >> 8) & 255, pObject->dwIpAddr & 255);
      }
      else
      {
         _tcscpy(pszName, pObject->szName);
      }
   }
   else
   {
      *pszName = 0;
   }
}


//
// Save object's cache to file
//

static void WriteStringToCacheFile(FILE *file, const TCHAR *str, bool writeNull)
{
	UINT32 len;

	if (str != NULL)
	{
		len = (UINT32)_tcslen(str);
		fwrite(&len, sizeof(UINT32), 1, file);
		fwrite(str, sizeof(TCHAR), len, file);
	}
	else
	{
		if (writeNull)
		{
			len = 0;
			fwrite(&len, sizeof(UINT32), 1, file);
		}
	}
}

UINT32 LIBNXCL_EXPORTABLE NXCSaveObjectCache(NXC_SESSION hSession, const TCHAR *pszFile)
{
   FILE *hFile;
   OBJECT_CACHE_HEADER hdr;
   UINT32 i, j, dwResult, dwNumObjects, dwSize;
   INDEX *pList;

   hFile = _tfopen(pszFile, _T("wb"));
   if (hFile != NULL)
   {
      ((NXCL_Session *)hSession)->lockObjectIndex();
      pList = (INDEX *)((NXCL_Session *)hSession)->GetObjectIndex(&dwNumObjects);

      // Write cache file header
      hdr.dwMagic = OBJECT_CACHE_MAGIC;
      hdr.dwStructSize = sizeof(NXC_OBJECT);
      hdr.dwTimeStamp = ((NXCL_Session *)hSession)->GetTimeStamp();
      hdr.dwNumObjects = dwNumObjects;
      memcpy(hdr.bsServerId, ((NXCL_Session *)hSession)->m_bsServerId, 8);
      fwrite(&hdr, 1, sizeof(OBJECT_CACHE_HEADER), hFile);

      // Write all objects
      for(i = 0; i < dwNumObjects; i++)
      {
         fwrite(pList[i].pObject, 1, sizeof(NXC_OBJECT), hFile);
         fwrite(pList[i].pObject->pdwChildList, 1, 
                sizeof(UINT32) * pList[i].pObject->dwNumChilds, hFile);
         fwrite(pList[i].pObject->pdwParentList, 1, 
                sizeof(UINT32) * pList[i].pObject->dwNumParents, hFile);
         fwrite(pList[i].pObject->pAccessList, 1, 
                sizeof(NXC_ACL_ENTRY) * pList[i].pObject->dwAclSize, hFile);
         
			WriteStringToCacheFile(hFile, pList[i].pObject->pszComments, true);

			if (pList[i].pObject->dwNumTrustedNodes > 0)
				fwrite(pList[i].pObject->pdwTrustedNodes, pList[i].pObject->dwNumTrustedNodes, sizeof(UINT32), hFile);

			// Custom attributes
			dwSize = pList[i].pObject->pCustomAttrs->getSize();
         fwrite(&dwSize, 1, sizeof(UINT32), hFile);
			for(j = 0; j < pList[i].pObject->pCustomAttrs->getSize(); j++)
			{
				WriteStringToCacheFile(hFile, pList[i].pObject->pCustomAttrs->getKeyByIndex(j), true);
				WriteStringToCacheFile(hFile, pList[i].pObject->pCustomAttrs->getValueByIndex(j), true);
			}

         switch(pList[i].pObject->iClass)
         {
				case OBJECT_NODE:
					WriteStringToCacheFile(hFile, pList[i].pObject->node.pszAuthName, true);
					WriteStringToCacheFile(hFile, pList[i].pObject->node.pszAuthPassword, true);
					WriteStringToCacheFile(hFile, pList[i].pObject->node.pszPrivPassword, true);
					WriteStringToCacheFile(hFile, pList[i].pObject->node.pszSnmpObjectId, true);
					break;
            case OBJECT_NETWORKSERVICE:
					WriteStringToCacheFile(hFile, pList[i].pObject->netsrv.pszRequest, true);
					WriteStringToCacheFile(hFile, pList[i].pObject->netsrv.pszResponse, true);
               break;
            case OBJECT_VPNCONNECTOR:
               fwrite(pList[i].pObject->vpnc.pLocalNetList, 1, 
                      pList[i].pObject->vpnc.dwNumLocalNets * sizeof(IP_NETWORK), hFile);
               fwrite(pList[i].pObject->vpnc.pRemoteNetList, 1, 
                      pList[i].pObject->vpnc.dwNumRemoteNets * sizeof(IP_NETWORK), hFile);
               break;
            case OBJECT_CONDITION:
					WriteStringToCacheFile(hFile, pList[i].pObject->cond.pszScript, true);
               fwrite(pList[i].pObject->cond.pDCIList, 1, 
                      pList[i].pObject->cond.dwNumDCI * sizeof(INPUT_DCI), hFile);
               break;
				case OBJECT_CLUSTER:
					fwrite(pList[i].pObject->cluster.pResourceList, 1,
					       pList[i].pObject->cluster.dwNumResources * sizeof(CLUSTER_RESOURCE), hFile);
					fwrite(pList[i].pObject->cluster.pSyncNetList, 1,
					       pList[i].pObject->cluster.dwNumSyncNets * sizeof(IP_NETWORK), hFile);
					break;
				case OBJECT_TEMPLATE:
					WriteStringToCacheFile(hFile, pList[i].pObject->dct.pszAutoApplyFilter, false);
					break;
				case OBJECT_CONTAINER:
					WriteStringToCacheFile(hFile, pList[i].pObject->container.pszAutoBindFilter, false);
					break;
            default:
               break;
         }
      }

      ((NXCL_Session *)hSession)->unlockObjectIndex();
      fclose(hFile);
      dwResult = RCC_SUCCESS;
   }
   else
   {
      dwResult = RCC_IO_ERROR;
   }

   return dwResult;
}


//
// Load objects from cache file
//

static TCHAR *ReadStringFromCacheFile(FILE *file)
{
	UINT32 len = 0;
	TCHAR *str;

	fread(&len, sizeof(UINT32), 1, file);
	str = (TCHAR *)malloc((len + 1) * sizeof(TCHAR));
	fread(str, sizeof(TCHAR), len, file);
	str[len] = 0;
	return str;
}

void NXCL_Session::loadObjectsFromCache(const TCHAR *pszFile)
{
   FILE *hFile;
   OBJECT_CACHE_HEADER hdr;
   NXC_OBJECT object;
   UINT32 i, j, dwCount, dwSize;
	TCHAR *key, *value;

   hFile = _tfopen(pszFile, _T("rb"));
   if (hFile != NULL)
   {
      // Read header
      DebugPrintf(_T("Checking cache file %s"), pszFile);
      if (fread(&hdr, 1, sizeof(OBJECT_CACHE_HEADER), hFile) == sizeof(OBJECT_CACHE_HEADER))
      {
         if ((hdr.dwMagic == OBJECT_CACHE_MAGIC) &&
             (hdr.dwStructSize == sizeof(NXC_OBJECT)) &&
             (!memcmp(hdr.bsServerId, m_bsServerId, 8)))
         {
            DebugPrintf(_T("Cache file OK, loading objects"));
            m_dwTimeStamp = hdr.dwTimeStamp;
            for(i = 0; i < hdr.dwNumObjects; i++)
            {
               if (fread(&object, 1, sizeof(NXC_OBJECT), hFile) == sizeof(NXC_OBJECT))
               {
                  object.pdwChildList = (UINT32 *)malloc(sizeof(UINT32) * object.dwNumChilds);
                  fread(object.pdwChildList, 1, sizeof(UINT32) * object.dwNumChilds, hFile);
                  
                  object.pdwParentList = (UINT32 *)malloc(sizeof(UINT32) * object.dwNumParents);
                  fread(object.pdwParentList, 1, sizeof(UINT32) * object.dwNumParents, hFile);
                  
                  object.pAccessList = (NXC_ACL_ENTRY *)malloc(sizeof(NXC_ACL_ENTRY) * object.dwAclSize);
                  fread(object.pAccessList, 1, sizeof(NXC_ACL_ENTRY) * object.dwAclSize, hFile);

						object.pszComments = ReadStringFromCacheFile(hFile);

						if (object.dwNumTrustedNodes > 0)
						{
							object.pdwTrustedNodes = (UINT32 *)malloc(sizeof(UINT32) * object.dwNumTrustedNodes);
							fread(object.pdwTrustedNodes, sizeof(UINT32), object.dwNumTrustedNodes, hFile);
						}
						else
						{
							object.pdwTrustedNodes = NULL;
						}

						// Custom attributes
						object.pCustomAttrs = new StringMap;
                  fread(&dwCount, 1, sizeof(UINT32), hFile);
						for(j = 0; j < dwCount; j++)
						{
							key = ReadStringFromCacheFile(hFile);
							value = ReadStringFromCacheFile(hFile);
							object.pCustomAttrs->setPreallocated(key, value);
						}

                  switch(object.iClass)
                  {
							case OBJECT_NODE:
								object.node.pszAuthName = ReadStringFromCacheFile(hFile);
                        object.node.pszAuthPassword = ReadStringFromCacheFile(hFile);
                        object.node.pszPrivPassword = ReadStringFromCacheFile(hFile);
                        object.node.pszSnmpObjectId = ReadStringFromCacheFile(hFile);
								break;
                     case OBJECT_NETWORKSERVICE:
                        object.netsrv.pszRequest = ReadStringFromCacheFile(hFile);
                        object.netsrv.pszResponse = ReadStringFromCacheFile(hFile);
                        break;
                     case OBJECT_VPNCONNECTOR:
                        dwSize = object.vpnc.dwNumLocalNets * sizeof(IP_NETWORK);
                        object.vpnc.pLocalNetList = (IP_NETWORK *)malloc(dwSize);
                        fread(object.vpnc.pLocalNetList, 1, dwSize, hFile);

                        dwSize = object.vpnc.dwNumRemoteNets * sizeof(IP_NETWORK);
                        object.vpnc.pRemoteNetList = (IP_NETWORK *)malloc(dwSize);
                        fread(object.vpnc.pRemoteNetList, 1, dwSize, hFile);
                        break;
                     case OBJECT_CONDITION:
                        object.cond.pszScript = ReadStringFromCacheFile(hFile);

                        dwSize = object.cond.dwNumDCI * sizeof(INPUT_DCI);
                        object.cond.pDCIList = (INPUT_DCI *)malloc(dwSize);
                        fread(object.cond.pDCIList, 1, dwSize, hFile);
                        break;
							case OBJECT_CLUSTER:
								if (object.cluster.dwNumResources > 0)
								{
									object.cluster.pResourceList = (CLUSTER_RESOURCE *)malloc(sizeof(CLUSTER_RESOURCE) * object.cluster.dwNumResources);
									fread(object.cluster.pResourceList, 1, sizeof(CLUSTER_RESOURCE) * object.cluster.dwNumResources, hFile);
								}
								else
								{
									object.cluster.pResourceList = NULL;
								}
								if (object.cluster.dwNumSyncNets > 0)
								{
									object.cluster.pSyncNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * object.cluster.dwNumSyncNets);
									fread(object.cluster.pSyncNetList, 1, sizeof(IP_NETWORK) * object.cluster.dwNumSyncNets, hFile);
								}
								else
								{
									object.cluster.pSyncNetList = NULL;
								}
								break;
							case OBJECT_TEMPLATE:
								if (object.dct.pszAutoApplyFilter != NULL)
									object.dct.pszAutoApplyFilter = ReadStringFromCacheFile(hFile);
								break;
							case OBJECT_CONTAINER:
								if (object.container.pszAutoBindFilter != NULL)
									object.container.pszAutoBindFilter = ReadStringFromCacheFile(hFile);
								break;
                     default:
                        break;
                  }

                  addObject((NXC_OBJECT *)nx_memdup(&object, sizeof(NXC_OBJECT)), FALSE);
               }
            }
            lockObjectIndex();
            qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
            unlockObjectIndex();
            m_dwFlags |= NXC_SF_HAS_OBJECT_CACHE;
         }
      }

      fclose(hFile);
   }
}


//
// Read agent's configuration file
//

UINT32 LIBNXCL_EXPORTABLE NXCGetAgentConfig(NXC_SESSION hSession, UINT32 dwNodeId,
                                           TCHAR **ppszConfig)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   *ppszConfig = NULL;

   // Build request message
   msg.SetCode(CMD_GET_AGENT_CONFIG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 60000);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppszConfig = pResponse->GetVariableStr(VID_CONFIG_FILE);
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
// Update agent's configuration file
//

UINT32 LIBNXCL_EXPORTABLE NXCUpdateAgentConfig(NXC_SESSION hSession, UINT32 dwNodeId,
                                              TCHAR *pszConfig, BOOL bApply)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_UPDATE_AGENT_CONFIG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_CONFIG_FILE, pszConfig);
   msg.SetVariable(VID_APPLY_FLAG, (WORD)bApply);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId, 60000);
}


//
// Execute action on agent
//

UINT32 LIBNXCL_EXPORTABLE NXCExecuteAction(NXC_SESSION hSession, UINT32 dwObjectId,
                                          TCHAR *pszAction)
{
   UINT32 dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_EXECUTE_ACTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_ACTION_NAME, pszAction);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get object's comments
//

UINT32 LIBNXCL_EXPORTABLE NXCGetObjectComments(NXC_SESSION hSession,
                                              UINT32 dwObjectId, TCHAR **ppszText)
{
   UINT32 dwRqId, dwResult;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_OBJECT_COMMENTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         *ppszText = pResponse->GetVariableStr(VID_COMMENTS);
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Update object's comments
//

UINT32 LIBNXCL_EXPORTABLE NXCUpdateObjectComments(NXC_SESSION hSession,
                                                 UINT32 dwObjectId, TCHAR *pszText)
{
   UINT32 dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_UPDATE_OBJECT_COMMENTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_COMMENTS, pszText);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Actual check for parent-child relation
//

static BOOL IsParent(NXC_SESSION hSession, UINT32 dwParent, UINT32 dwChild)
{
   UINT32 i;
   NXC_OBJECT *pObject;
   BOOL bRet = FALSE;

   pObject = ((NXCL_Session *)hSession)->findObjectById(dwChild, FALSE);
   if (pObject != NULL)
   {
      for(i = 0; i < pObject->dwNumParents; i++)
      {
         if (pObject->pdwParentList[i] == dwParent)
         {
            bRet = TRUE;
            break;
         }
      }

      if (!bRet)
      {
         for(i = 0; i < pObject->dwNumParents; i++)
         {
            if (IsParent(hSession, dwParent, pObject->pdwParentList[i]))
            {
               bRet = TRUE;
               break;
            }
         }
      }
   }
   return bRet;
}


//
// Check if first object is a parent of second object
//

BOOL LIBNXCL_EXPORTABLE NXCIsParent(NXC_SESSION hSession, UINT32 dwParent, UINT32 dwChild)
{
   BOOL bRet;

   ((NXCL_Session *)hSession)->lockObjectIndex();
   bRet = IsParent(hSession, dwParent, dwChild);
   ((NXCL_Session *)hSession)->unlockObjectIndex();
   return bRet;
}


//
// Get list of events used by template's or node's DCIs
//

UINT32 LIBNXCL_EXPORTABLE NXCGetDCIEventsList(NXC_SESSION hSession, UINT32 dwObjectId,
                                             UINT32 **ppdwList, UINT32 *pdwListSize)
{
   UINT32 dwRqId, dwResult;
   CSCPMessage msg, *pResponse;

   *ppdwList = NULL;
   *pdwListSize = 0;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_DCI_EVENTS_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwListSize = pResponse->GetVariableLong(VID_NUM_EVENTS);
         if (*pdwListSize > 0)
         {
            *ppdwList = (UINT32 *)malloc(sizeof(UINT32) * (*pdwListSize));
            pResponse->getFieldAsInt32Array(VID_EVENT_LIST, *pdwListSize, *ppdwList);
         }
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}
