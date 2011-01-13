/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: objects.cpp
**
**/

#include "nxcore.h"


//
// Global data
//

BOOL g_bModificationsLocked = FALSE;

Network NXCORE_EXPORTABLE *g_pEntireNet = NULL;
ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot = NULL;
TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot = NULL;
PolicyRoot NXCORE_EXPORTABLE *g_pPolicyRoot = NULL;
NetworkMapRoot NXCORE_EXPORTABLE *g_pMapRoot = NULL;

DWORD NXCORE_EXPORTABLE g_dwMgmtNode = 0;
INDEX NXCORE_EXPORTABLE *g_pIndexById = NULL;
DWORD NXCORE_EXPORTABLE g_dwIdIndexSize = 0;
INDEX NXCORE_EXPORTABLE *g_pSubnetIndexByAddr = NULL;
DWORD NXCORE_EXPORTABLE g_dwSubnetAddrIndexSize = 0;
INDEX NXCORE_EXPORTABLE *g_pNodeIndexByAddr = NULL;
DWORD NXCORE_EXPORTABLE g_dwNodeAddrIndexSize = 0;
INDEX NXCORE_EXPORTABLE *g_pInterfaceIndexByAddr = NULL;
DWORD NXCORE_EXPORTABLE g_dwInterfaceAddrIndexSize = 0;
INDEX NXCORE_EXPORTABLE *g_pZoneIndexByGUID = NULL;
DWORD NXCORE_EXPORTABLE g_dwZoneGUIDIndexSize = 0;
INDEX NXCORE_EXPORTABLE *g_pConditionIndex = NULL;
DWORD NXCORE_EXPORTABLE g_dwConditionIndexSize = 0;

RWLOCK NXCORE_EXPORTABLE g_rwlockIdIndex;
RWLOCK NXCORE_EXPORTABLE g_rwlockNodeIndex;
RWLOCK NXCORE_EXPORTABLE g_rwlockSubnetIndex;
RWLOCK NXCORE_EXPORTABLE g_rwlockInterfaceIndex;
RWLOCK NXCORE_EXPORTABLE g_rwlockZoneIndex;
RWLOCK NXCORE_EXPORTABLE g_rwlockConditionIndex;

DWORD g_dwNumCategories = 0;
CONTAINER_CATEGORY *g_pContainerCatList = NULL;

Queue *g_pTemplateUpdateQueue = NULL;

const TCHAR *g_szClassName[]={ _T("Generic"), _T("Subnet"), _T("Node"), _T("Interface"),
                               _T("Network"), _T("Container"), _T("Zone"), _T("ServiceRoot"),
                               _T("Template"), _T("TemplateGroup"), _T("TemplateRoot"),
                               _T("NetworkService"), _T("VPNConnector"), _T("Condition"),
                               _T("Cluster"), _T("PolicyGroup"), _T("PolicyRoot"),
                               _T("AgentPolicy"), _T("AgentPolicyConfig"), _T("NetworkMapRoot"),
                               _T("NetworkMapGroup"), _T("NetworkMap") };


//
// Static data
//

static int m_iStatusCalcAlg = SA_CALCULATE_MOST_CRITICAL;
static int m_iStatusPropAlg = SA_PROPAGATE_UNCHANGED;
static int m_iFixedStatus;        // Status if propagation is "Fixed"
static int m_iStatusShift;        // Shift value for "shifted" status propagation
static int m_iStatusTranslation[4];
static int m_iStatusSingleThreshold;
static int m_iStatusThresholds[4];


//
// Thread which apply template updates
//

static THREAD_RESULT THREAD_CALL ApplyTemplateThread(void *pArg)
{
   TEMPLATE_UPDATE_INFO *pInfo;
   NetObj *pNode;
   BOOL bSuccess, bLock1, bLock2;

	DbgPrintf(1, _T("Apply template thread started"));
   while(1)
   {
      pInfo = (TEMPLATE_UPDATE_INFO *)g_pTemplateUpdateQueue->GetOrBlock();
      if (pInfo == INVALID_POINTER_VALUE)
         break;

		DbgPrintf(5, _T("ApplyTemplateThread: template=%d(%s) updateType=%d node=%d removeDci=%d"),
		          pInfo->pTemplate->Id(), pInfo->pTemplate->Name(), pInfo->iUpdateType, pInfo->dwNodeId, pInfo->bRemoveDCI);
      bSuccess = FALSE;
      pNode = FindObjectById(pInfo->dwNodeId);
      if (pNode != NULL)
      {
         if (pNode->Type() == OBJECT_NODE)
         {
            switch(pInfo->iUpdateType)
            {
               case APPLY_TEMPLATE:
                  bLock1 = pInfo->pTemplate->LockDCIList(0x7FFFFFFF, _T("SYSTEM"), NULL);
                  bLock2 = ((Node *)pNode)->LockDCIList(0x7FFFFFFF, _T("SYSTEM"), NULL);
                  if (bLock1 && bLock2)
                  {
                     pInfo->pTemplate->ApplyToNode((Node *)pNode);
                     bSuccess = TRUE;
                  }
                  if (bLock1)
                     pInfo->pTemplate->UnlockDCIList(0x7FFFFFFF);
                  if (bLock2)
                     ((Node *)pNode)->UnlockDCIList(0x7FFFFFFF);
                  break;
               case REMOVE_TEMPLATE:
                  if (((Node *)pNode)->LockDCIList(0x7FFFFFFF, _T("SYSTEM"), NULL))
                  {
                     ((Node *)pNode)->unbindFromTemplate(pInfo->pTemplate->Id(), pInfo->bRemoveDCI);
                     ((Node *)pNode)->UnlockDCIList(0x7FFFFFFF);
                     bSuccess = TRUE;
                  }
                  break;
               default:
                  bSuccess = TRUE;
                  break;
            }
         }
      }

      if (bSuccess)
      {
			DbgPrintf(8, _T("ApplyTemplateThread: success"));
			pInfo->pTemplate->DecRefCount();
         free(pInfo);
      }
      else
      {
			DbgPrintf(8, _T("ApplyTemplateThread: failed"));
         g_pTemplateUpdateQueue->Put(pInfo);    // Requeue
         ThreadSleepMs(500);
      }
   }

	DbgPrintf(1, _T("Apply template thread stopped"));
   return THREAD_OK;
}


//
// DCI cache loading thread
//

static THREAD_RESULT THREAD_CALL CacheLoadingThread(void *pArg)
{
   DWORD i;

   DbgPrintf(1, _T("Started caching of DCI values"));
   RWLockReadLock(g_rwlockNodeIndex, INFINITE);
      for(i = 0; i < g_dwIdIndexSize; i++)
		{
			if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NODE)
			{
				((Node *)g_pIndexById[i].pObject)->updateDciCache();
		      ThreadSleepMs(50);  // Give a chance to other threads to do something with database
			}
		}
   RWLockUnlock(g_rwlockNodeIndex);
   DbgPrintf(1, _T("Finished caching of DCI values"));
   return THREAD_OK;
}


//
// Initialize objects infrastructure
//

void ObjectsInit()
{
   // Load default status calculation info
   m_iStatusCalcAlg = ConfigReadInt(_T("StatusCalculationAlgorithm"), SA_CALCULATE_MOST_CRITICAL);
   m_iStatusPropAlg = ConfigReadInt(_T("StatusPropagationAlgorithm"), SA_PROPAGATE_UNCHANGED);
   m_iFixedStatus = ConfigReadInt(_T("FixedStatusValue"), STATUS_NORMAL);
   m_iStatusShift = ConfigReadInt(_T("StatusShift"), 0);
   ConfigReadByteArray(_T("StatusTranslation"), m_iStatusTranslation, 4, STATUS_WARNING);
   m_iStatusSingleThreshold = ConfigReadInt(_T("StatusSingleThreshold"), 75);
   ConfigReadByteArray(_T("StatusThresholds"), m_iStatusThresholds, 4, 50);

   g_pTemplateUpdateQueue = new Queue;

   g_rwlockIdIndex = RWLockCreate();
   g_rwlockNodeIndex = RWLockCreate();
   g_rwlockSubnetIndex = RWLockCreate();
   g_rwlockInterfaceIndex = RWLockCreate();
   g_rwlockZoneIndex = RWLockCreate();
   g_rwlockConditionIndex = RWLockCreate();

   // Create _T("Entire Network") object
   g_pEntireNet = new Network;
   NetObjInsert(g_pEntireNet, FALSE);

   // Create _T("Service Root") object
   g_pServiceRoot = new ServiceRoot;
   NetObjInsert(g_pServiceRoot, FALSE);

   // Create _T("Template Root") object
   g_pTemplateRoot = new TemplateRoot;
   NetObjInsert(g_pTemplateRoot, FALSE);

	// Create _T("Policy Root") object
   g_pPolicyRoot = new PolicyRoot;
   NetObjInsert(g_pPolicyRoot, FALSE);
   
	// Create _T("Network Maps Root") object
   g_pMapRoot = new NetworkMapRoot;
   NetObjInsert(g_pMapRoot, FALSE);
   
	DbgPrintf(1, _T("Built-in objects created"));

   // Start template update applying thread
   ThreadCreate(ApplyTemplateThread, 0, NULL);
}


//
// Function to compare two indexes
//

static int IndexCompare(const void *pArg1, const void *pArg2)
{
   return (((INDEX *)pArg1)->dwKey < ((INDEX *)pArg2)->dwKey) ? -1 :
            ((((INDEX *)pArg1)->dwKey > ((INDEX *)pArg2)->dwKey) ? 1 : 0);
}


//
// Add new object to index
//

void AddObjectToIndex(INDEX **ppIndex, DWORD *pdwIndexSize, DWORD dwKey, void *pObject)
{
   *ppIndex = (INDEX *)realloc(*ppIndex, sizeof(INDEX) * ((*pdwIndexSize) + 1));
   (*ppIndex)[*pdwIndexSize].dwKey = dwKey;
   (*ppIndex)[*pdwIndexSize].pObject = pObject;
   (*pdwIndexSize)++;
   qsort(*ppIndex, *pdwIndexSize, sizeof(INDEX), IndexCompare);
}


//
// Perform binary search on index
// Returns INVALID_INDEX if key not found or position of appropriate network object
// We assume that pIndex == NULL will not be passed
//

DWORD SearchIndex(INDEX *pIndex, DWORD dwIndexSize, DWORD dwKey)
{
   DWORD dwFirst, dwLast, dwMid;

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
// Delete object from index
//

void DeleteObjectFromIndex(INDEX **ppIndex, DWORD *pdwIndexSize, DWORD dwKey)
{
   DWORD dwPos;
   INDEX *pIndex = *ppIndex;

   dwPos = SearchIndex(pIndex, *pdwIndexSize, dwKey);
   if (dwPos != INVALID_INDEX)
   {
      (*pdwIndexSize)--;
      memmove(&pIndex[dwPos], &pIndex[dwPos + 1], sizeof(INDEX) * (*pdwIndexSize - dwPos));
   }
}


//
// Insert new object into network
//

void NetObjInsert(NetObj *pObject, BOOL bNewObject)
{
   if (bNewObject)   
   {
      // Assign unique ID to new object
      pObject->setId(CreateUniqueId(IDG_NETWORK_OBJECT));
		pObject->generateGuid();

      // Create table for storing data collection values
      if (pObject->Type() == OBJECT_NODE)
      {
         TCHAR szQuery[256], szQueryTemplate[256];
         DWORD i;

         MetaDataReadStr(_T("IDataTableCreationCommand"), szQueryTemplate, 255, _T(""));
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->Id());
         DBQuery(g_hCoreDB, szQuery);

         for(i = 0; i < 10; i++)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("IDataIndexCreationCommand_%d"), i);
            MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
            if (szQueryTemplate[0] != 0)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), szQueryTemplate, pObject->Id(), pObject->Id());
               DBQuery(g_hCoreDB, szQuery);
            }
         }
      }
   }
	RWLockPreemptiveWriteLock(g_rwlockIdIndex, 1000, 500);
   AddObjectToIndex(&g_pIndexById, &g_dwIdIndexSize, pObject->Id(), pObject);
   RWLockUnlock(g_rwlockIdIndex);
   if (!pObject->IsDeleted())
   {
      switch(pObject->Type())
      {
         case OBJECT_GENERIC:
         case OBJECT_NETWORK:
         case OBJECT_CONTAINER:
         case OBJECT_SERVICEROOT:
         case OBJECT_NETWORKSERVICE:
         case OBJECT_TEMPLATE:
         case OBJECT_TEMPLATEGROUP:
         case OBJECT_TEMPLATEROOT:
			case OBJECT_CLUSTER:
			case OBJECT_VPNCONNECTOR:
			case OBJECT_POLICYGROUP:
			case OBJECT_POLICYROOT:
			case OBJECT_AGENTPOLICY:
			case OBJECT_AGENTPOLICY_CONFIG:
			case OBJECT_NETWORKMAPROOT:
			case OBJECT_NETWORKMAPGROUP:
			case OBJECT_NETWORKMAP:
            break;
         case OBJECT_SUBNET:
            if (pObject->IpAddr() != 0)
            {
               RWLockWriteLock(g_rwlockSubnetIndex, INFINITE);
               AddObjectToIndex(&g_pSubnetIndexByAddr, &g_dwSubnetAddrIndexSize, pObject->IpAddr(), pObject);
               RWLockUnlock(g_rwlockSubnetIndex);
               if (bNewObject)
                  PostEvent(EVENT_SUBNET_ADDED, pObject->Id(), NULL);
            }
            break;
         case OBJECT_NODE:
            if (pObject->IpAddr() != 0)
            {
               RWLockWriteLock(g_rwlockNodeIndex, INFINITE);
               AddObjectToIndex(&g_pNodeIndexByAddr, &g_dwNodeAddrIndexSize, pObject->IpAddr(), pObject);
               RWLockUnlock(g_rwlockNodeIndex);
            }
            break;
         case OBJECT_INTERFACE:
            if (pObject->IpAddr() != 0)
            {
               RWLockWriteLock(g_rwlockInterfaceIndex, INFINITE);
               AddObjectToIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, pObject->IpAddr(), pObject);
               RWLockUnlock(g_rwlockInterfaceIndex);
            }
            break;
         case OBJECT_ZONE:
            RWLockWriteLock(g_rwlockZoneIndex, INFINITE);
            AddObjectToIndex(&g_pZoneIndexByGUID, &g_dwZoneGUIDIndexSize, ((Zone *)pObject)->GUID(), pObject);
            RWLockUnlock(g_rwlockZoneIndex);
            break;
         case OBJECT_CONDITION:
            RWLockWriteLock(g_rwlockConditionIndex, INFINITE);
            AddObjectToIndex(&g_pConditionIndex, &g_dwConditionIndexSize, pObject->Id(), pObject);
            RWLockUnlock(g_rwlockConditionIndex);
            break;
         default:
            nxlog_write(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
            break;
      }
   }
}


//
// Delete object from indexes
// If object has an IP address, this function will delete it from
// appropriate index. Normally this function should be called from
// NetObj::Delete() method.
//

void NetObjDeleteFromIndexes(NetObj *pObject)
{
   switch(pObject->Type())
   {
      case OBJECT_GENERIC:
      case OBJECT_NETWORK:
      case OBJECT_CONTAINER:
      case OBJECT_SERVICEROOT:
      case OBJECT_NETWORKSERVICE:
      case OBJECT_TEMPLATE:
      case OBJECT_TEMPLATEGROUP:
      case OBJECT_TEMPLATEROOT:
		case OBJECT_CLUSTER:
		case OBJECT_VPNCONNECTOR:
		case OBJECT_POLICYGROUP:
		case OBJECT_POLICYROOT:
		case OBJECT_AGENTPOLICY:
		case OBJECT_AGENTPOLICY_CONFIG:
		case OBJECT_NETWORKMAPROOT:
		case OBJECT_NETWORKMAPGROUP:
		case OBJECT_NETWORKMAP:
         break;
      case OBJECT_SUBNET:
         if (pObject->IpAddr() != 0)
         {
            RWLockWriteLock(g_rwlockSubnetIndex, INFINITE);
            DeleteObjectFromIndex(&g_pSubnetIndexByAddr, &g_dwSubnetAddrIndexSize, pObject->IpAddr());
            RWLockUnlock(g_rwlockSubnetIndex);
         }
         break;
      case OBJECT_NODE:
         if (pObject->IpAddr() != 0)
         {
            RWLockWriteLock(g_rwlockNodeIndex, INFINITE);
            DeleteObjectFromIndex(&g_pNodeIndexByAddr, &g_dwNodeAddrIndexSize, pObject->IpAddr());
            RWLockUnlock(g_rwlockNodeIndex);
         }
         break;
      case OBJECT_INTERFACE:
         if (pObject->IpAddr() != 0)
         {
            RWLockWriteLock(g_rwlockInterfaceIndex, INFINITE);
            DeleteObjectFromIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, pObject->IpAddr());
            RWLockUnlock(g_rwlockInterfaceIndex);
         }
         break;
      case OBJECT_ZONE:
         RWLockWriteLock(g_rwlockZoneIndex, INFINITE);
         DeleteObjectFromIndex(&g_pZoneIndexByGUID, &g_dwZoneGUIDIndexSize, ((Zone *)pObject)->GUID());
         RWLockUnlock(g_rwlockZoneIndex);
         break;
      case OBJECT_CONDITION:
         RWLockWriteLock(g_rwlockConditionIndex, INFINITE);
         DeleteObjectFromIndex(&g_pConditionIndex, &g_dwConditionIndexSize, pObject->Id());
         RWLockUnlock(g_rwlockConditionIndex);
         break;
      default:
         nxlog_write(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
         break;
   }
}


//
// Get IP netmask for object of any class
//

static DWORD GetObjectNetmask(NetObj *pObject)
{
   switch(pObject->Type())
   {
      case OBJECT_INTERFACE:
         return ((Interface *)pObject)->getIpNetMask();
      case OBJECT_SUBNET:
         return ((Subnet *)pObject)->getIpNetMask();
      default:
         return 0;
   }
}


//
// Find node by IP address
//

Node NXCORE_EXPORTABLE *FindNodeByIP(DWORD dwAddr)
{
   DWORD dwPos;
   Node *pNode;

   if ((g_pInterfaceIndexByAddr == NULL) || (dwAddr == 0))
      return NULL;

   RWLockReadLock(g_rwlockInterfaceIndex, INFINITE);
   dwPos = SearchIndex(g_pInterfaceIndexByAddr, g_dwInterfaceAddrIndexSize, dwAddr);
   pNode = (dwPos == INVALID_INDEX) ? NULL : ((Interface *)g_pInterfaceIndexByAddr[dwPos].pObject)->getParentNode();
   RWLockUnlock(g_rwlockInterfaceIndex);
   return pNode;
}


//
// Find node by MAC address
//

Node NXCORE_EXPORTABLE *FindNodeByMAC(const BYTE *macAddr)
{
	Interface *pInterface = FindInterfaceByMAC(macAddr);
	return (pInterface != NULL) ? pInterface->getParentNode() : NULL;
}


//
// Find interface by MAC address
//

Interface NXCORE_EXPORTABLE *FindInterfaceByMAC(const BYTE *macAddr)
{
	if (!memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", 6))
		return NULL;

   Interface *pInterface = NULL;
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
	for(DWORD i = 0; i < g_dwIdIndexSize; i++)
	{
		if ((((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_INTERFACE) &&
		    !memcmp(macAddr, ((Interface *)g_pIndexById[i].pObject)->getMacAddr(), 6))
		{
			pInterface = ((Interface *)g_pIndexById[i].pObject);
			break;
		}
	}
   RWLockUnlock(g_rwlockIdIndex);
   return pInterface;
}


//
// Find node by LLDP ID
//

Node NXCORE_EXPORTABLE *FindNodeByLLDPId(const TCHAR *lldpId)
{
   Node *node = NULL;
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
	for(DWORD i = 0; i < g_dwIdIndexSize; i++)
	{
		if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NODE)
		{
			const TCHAR *id = ((Node *)g_pIndexById[i].pObject)->getLLDPNodeId();
			if ((id != NULL) && !_tcscmp(id, lldpId))
			{
				node = (Node *)g_pIndexById[i].pObject;
				break;
			}
		}
	}
   RWLockUnlock(g_rwlockIdIndex);
   return node;
}


//
// Find subnet by IP address
//

Subnet NXCORE_EXPORTABLE *FindSubnetByIP(DWORD dwAddr)
{
   DWORD dwPos;
   Subnet *pSubnet;

   if ((g_pSubnetIndexByAddr == NULL) || (dwAddr == 0))
      return NULL;

   RWLockReadLock(g_rwlockSubnetIndex, INFINITE);
   dwPos = SearchIndex(g_pSubnetIndexByAddr, g_dwSubnetAddrIndexSize, dwAddr);
   pSubnet = (dwPos == INVALID_INDEX) ? NULL : (Subnet *)g_pSubnetIndexByAddr[dwPos].pObject;
   RWLockUnlock(g_rwlockSubnetIndex);
   return pSubnet;
}


//
// Find subnet for given IP address
//

Subnet NXCORE_EXPORTABLE *FindSubnetForNode(DWORD dwNodeAddr)
{
   DWORD i;
   Subnet *pSubnet = NULL;

   if ((g_pSubnetIndexByAddr == NULL) || (dwNodeAddr == 0))
      return NULL;

   RWLockReadLock(g_rwlockSubnetIndex, INFINITE);
   for(i = 0; i < g_dwSubnetAddrIndexSize; i++)
      if ((dwNodeAddr & ((Subnet *)g_pSubnetIndexByAddr[i].pObject)->getIpNetMask()) == 
               ((Subnet *)g_pSubnetIndexByAddr[i].pObject)->IpAddr())
      {
         pSubnet = (Subnet *)g_pSubnetIndexByAddr[i].pObject;
         break;
      }
   RWLockUnlock(g_rwlockSubnetIndex);
   return pSubnet;
}


//
// Find object by ID
//

NetObj NXCORE_EXPORTABLE *FindObjectById(DWORD dwId)
{
   DWORD dwPos;
   NetObj *pObject;

   if (g_pIndexById == NULL)
      return NULL;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   dwPos = SearchIndex(g_pIndexById, g_dwIdIndexSize, dwId);
   pObject = (dwPos == INVALID_INDEX) ? NULL : (NetObj *)g_pIndexById[dwPos].pObject;
   RWLockUnlock(g_rwlockIdIndex);
   return pObject;
}


//
// Find object by name
//

NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass)
{
   DWORD i;
   NetObj *pObject = NULL;

   if (g_pIndexById == NULL)
      return NULL;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
      if ((!_tcsicmp(((NetObj *)g_pIndexById[i].pObject)->Name(), name)) &&
			 ((objClass == -1) || (objClass == ((NetObj *)g_pIndexById[i].pObject)->Type())))
      {
         pObject = (NetObj *)g_pIndexById[i].pObject;
         break;
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
   return pObject;
}


//
// Find object by GUID
//

NetObj NXCORE_EXPORTABLE *FindObjectByGUID(uuid_t guid, int objClass)
{
   DWORD i;
	uuid_t temp;
   NetObj *pObject = NULL;

   if (g_pIndexById == NULL)
      return NULL;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
		((NetObj *)g_pIndexById[i].pObject)->getGuid(temp);
		if (!uuid_compare(guid, temp) && ((objClass == -1) || (objClass == ((NetObj *)g_pIndexById[i].pObject)->Type())))
      {
         pObject = (NetObj *)g_pIndexById[i].pObject;
         break;
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
   return pObject;
}


//
// Find template object by name
//

Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName)
{
   DWORD i;
   Template *pObject = NULL;

   if (g_pIndexById == NULL)
      return NULL;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
      if ((((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_TEMPLATE) &&
			 (!_tcsicmp(((NetObj *)g_pIndexById[i].pObject)->Name(), pszName)))
      {
         pObject = (Template *)g_pIndexById[i].pObject;
         break;
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
   return pObject;
}


//
// Find zone object by GUID
//

Zone NXCORE_EXPORTABLE *FindZoneByGUID(DWORD dwZoneGUID)
{
   DWORD dwPos;
   NetObj *pObject;

   if (g_pZoneIndexByGUID == NULL)
      return NULL;

   RWLockReadLock(g_rwlockZoneIndex, INFINITE);
   dwPos = SearchIndex(g_pZoneIndexByGUID, g_dwZoneGUIDIndexSize, dwZoneGUID);
   pObject = (dwPos == INVALID_INDEX) ? NULL : (NetObj *)g_pZoneIndexByGUID[dwPos].pObject;
   RWLockUnlock(g_rwlockZoneIndex);
   return (pObject->Type() == OBJECT_ZONE) ? (Zone *)pObject : NULL;
}


//
// Find local management node ID
//

DWORD FindLocalMgmtNode()
{
   DWORD i, dwId = 0;

   if (g_pNodeIndexByAddr == NULL)
      return 0;

   RWLockReadLock(g_rwlockNodeIndex, INFINITE);
   for(i = 0; i < g_dwNodeAddrIndexSize; i++)
      if (((Node *)g_pNodeIndexByAddr[i].pObject)->getFlags() & NF_IS_LOCAL_MGMT)
      {
         dwId = ((Node *)g_pNodeIndexByAddr[i].pObject)->Id();
         break;
      }
   RWLockUnlock(g_rwlockNodeIndex);
   return dwId;
}


//
// Load objects from database at stratup
//

BOOL LoadObjects()
{
   DB_RESULT hResult;
   DWORD i, dwNumRows;
   DWORD dwId;
   Template *pTemplate;
   TCHAR szQuery[256];

   // Prevent objects to change it's modification flag
   g_bModificationsLocked = TRUE;

   // Load container categories
   DbgPrintf(2, _T("Loading container categories..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT category,name,image_id,description FROM container_categories"));
   if (hResult != NULL)
   {
      g_dwNumCategories = DBGetNumRows(hResult);
      g_pContainerCatList = (CONTAINER_CATEGORY *)malloc(sizeof(CONTAINER_CATEGORY) * g_dwNumCategories);
      for(i = 0; i < (int)g_dwNumCategories; i++)
      {
         g_pContainerCatList[i].dwCatId = DBGetFieldULong(hResult, i, 0);
         DBGetField(hResult, i, 1, g_pContainerCatList[i].szName, MAX_OBJECT_NAME);
         g_pContainerCatList[i].dwImageId = DBGetFieldULong(hResult, i, 2);
         g_pContainerCatList[i].pszDescription = DBGetField(hResult, i, 3, NULL, 0);
         DecodeSQLString(g_pContainerCatList[i].pszDescription);
      }
      DBFreeResult(hResult);
   }

   // Load built-in object properties
   DbgPrintf(2, _T("Loading built-in object properties..."));
   g_pEntireNet->LoadFromDB();
   g_pServiceRoot->LoadFromDB();
   g_pTemplateRoot->LoadFromDB();
	g_pPolicyRoot->LoadFromDB();
	g_pMapRoot->LoadFromDB();

   // Load zones
   if (g_dwFlags & AF_ENABLE_ZONING)
   {
      Zone *pZone;

      DbgPrintf(2, _T("Loading zones..."));

      // Load (or create) default zone
      pZone = new Zone;
      pZone->CreateFromDB(BUILTIN_OID_ZONE0);
      NetObjInsert(pZone, FALSE);
      g_pEntireNet->AddZone(pZone);

      hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM zones WHERE id<>4"));
      if (hResult != 0)
      {
         dwNumRows = DBGetNumRows(hResult);
         for(i = 0; i < dwNumRows; i++)
         {
            dwId = DBGetFieldULong(hResult, i, 0);
            pZone = new Zone;
            if (pZone->CreateFromDB(dwId))
            {
               if (!pZone->IsDeleted())
                  g_pEntireNet->AddZone(pZone);
               NetObjInsert(pZone, FALSE);  // Insert into indexes
            }
            else     // Object load failed
            {
               delete pZone;
               nxlog_write(MSG_ZONE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
            }
         }
         DBFreeResult(hResult);
      }
   }

   // Load conditions
   // We should load conditions before nodes because
   // DCI cache size calculation uses information from condition objects
   DbgPrintf(2, _T("Loading conditions..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM conditions"));
   if (hResult != NULL)
   {
      Condition *pCondition;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pCondition = new Condition;
         if (pCondition->CreateFromDB(dwId))
         {
            NetObjInsert(pCondition, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pCondition;
            nxlog_write(MSG_CONDITION_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load subnets
   DbgPrintf(2, _T("Loading subnets..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM subnets"));
   if (hResult != 0)
   {
      Subnet *pSubnet;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pSubnet = new Subnet;
         if (pSubnet->CreateFromDB(dwId))
         {
            if (!pSubnet->IsDeleted())
            {
               if (g_dwFlags & AF_ENABLE_ZONING)
               {
                  Zone *pZone;

                  pZone = FindZoneByGUID(pSubnet->getZoneGUID());
                  if (pZone != NULL)
                     pZone->AddSubnet(pSubnet);
               }
               else
               {
                  g_pEntireNet->AddSubnet(pSubnet);
               }
            }
            NetObjInsert(pSubnet, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pSubnet;
            nxlog_write(MSG_SUBNET_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load nodes
   DbgPrintf(2, _T("Loading nodes..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM nodes"));
   if (hResult != 0)
   {
      Node *pNode;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pNode = new Node;
         if (pNode->CreateFromDB(dwId))
         {
            NetObjInsert(pNode, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pNode;
            nxlog_write(MSG_NODE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);

      // Start cache loading thread
      ThreadCreate(CacheLoadingThread, 0, NULL);
   }

   // Load interfaces
   DbgPrintf(2, _T("Loading interfaces..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM interfaces"));
   if (hResult != 0)
   {
      Interface *pInterface;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pInterface = new Interface;
         if (pInterface->CreateFromDB(dwId))
         {
            NetObjInsert(pInterface, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pInterface;
            nxlog_write(MSG_INTERFACE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load network services
   DbgPrintf(2, _T("Loading network services..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM network_services"));
   if (hResult != 0)
   {
      NetworkService *pService;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pService = new NetworkService;
         if (pService->CreateFromDB(dwId))
         {
            NetObjInsert(pService, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pService;
            nxlog_write(MSG_NETSRV_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load VPN connectors
   DbgPrintf(2, _T("Loading VPN connectors..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM vpn_connectors"));
   if (hResult != NULL)
   {
      VPNConnector *pConnector;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pConnector = new VPNConnector;
         if (pConnector->CreateFromDB(dwId))
         {
            NetObjInsert(pConnector, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pConnector;
            nxlog_write(MSG_VPNC_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load clusters
   DbgPrintf(2, _T("Loading clusters..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM clusters"));
   if (hResult != NULL)
   {
      Cluster *pCluster;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pCluster = new Cluster;
         if (pCluster->CreateFromDB(dwId))
         {
            NetObjInsert(pCluster, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pCluster;
            nxlog_write(MSG_CLUSTER_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load templates
   DbgPrintf(2, _T("Loading templates..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM templates"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pTemplate = new Template;
         if (pTemplate->CreateFromDB(dwId))
         {
            NetObjInsert(pTemplate, FALSE);  // Insert into indexes
				pTemplate->CalculateCompoundStatus();	// Force status change to NORMAL
         }
         else     // Object load failed
         {
            delete pTemplate;
            nxlog_write(MSG_TEMPLATE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load agent policies
   DbgPrintf(2, _T("Loading agent policies..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id,policy_type FROM ap_common"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         AgentPolicy *policy;

			dwId = DBGetFieldULong(hResult, i, 0);
			int type = DBGetFieldLong(hResult, i, 1);
			switch(type)
			{
				case AGENT_POLICY_CONFIG:
					policy = new AgentPolicyConfig();
					break;
				default:
					policy = new AgentPolicy(type);
					break;
			}
         if (policy->CreateFromDB(dwId))
         {
            NetObjInsert(policy, FALSE);  // Insert into indexes
				policy->CalculateCompoundStatus();	// Force status change to NORMAL
         }
         else     // Object load failed
         {
            delete policy;
            nxlog_write(MSG_AGENTPOLICY_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load agent policies
   DbgPrintf(2, _T("Loading network maps..."));
   hResult = DBSelect(g_hCoreDB, _T("SELECT id FROM network_maps"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         NetworkMap *map = new NetworkMap;
         if (map->CreateFromDB(dwId))
         {
            NetObjInsert(map, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete map;
            nxlog_write(MSG_NETMAP_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load container objects
   DbgPrintf(2, _T("Loading containers..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_CONTAINER);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      Container *pContainer;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pContainer = new Container;
         if (pContainer->CreateFromDB(dwId))
         {
            NetObjInsert(pContainer, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pContainer;
            nxlog_write(MSG_CONTAINER_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load template group objects
   DbgPrintf(2, _T("Loading template groups..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_TEMPLATEGROUP);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      TemplateGroup *pGroup;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pGroup = new TemplateGroup;
         if (pGroup->CreateFromDB(dwId))
         {
            NetObjInsert(pGroup, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pGroup;
            nxlog_write(MSG_TG_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load policy group objects
   DbgPrintf(2, _T("Loading policy groups..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_POLICYGROUP);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      PolicyGroup *pGroup;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pGroup = new PolicyGroup;
         if (pGroup->CreateFromDB(dwId))
         {
            NetObjInsert(pGroup, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pGroup;
            nxlog_write(MSG_PG_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load map group objects
   DbgPrintf(2, _T("Loading map groups..."));
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE object_class=%d"), OBJECT_NETWORKMAPGROUP);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      NetworkMapGroup *pGroup;

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         pGroup = new NetworkMapGroup;
         if (pGroup->CreateFromDB(dwId))
         {
            NetObjInsert(pGroup, FALSE);  // Insert into indexes
         }
         else     // Object load failed
         {
            delete pGroup;
            nxlog_write(MSG_MG_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Link childs to container and template group objects
   DbgPrintf(2, _T("Linking objects..."));
   for(i = 0; i < g_dwIdIndexSize; i++)
      if ((((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_CONTAINER) ||
          (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_TEMPLATEGROUP) ||
          (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_POLICYGROUP) ||
          (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NETWORKMAPGROUP))
         ((Container *)g_pIndexById[i].pObject)->LinkChildObjects();

   // Link childs to _T("Service Root" and "Template Root") objects
   g_pServiceRoot->LinkChildObjects();
   g_pTemplateRoot->LinkChildObjects();
   g_pPolicyRoot->LinkChildObjects();
   g_pMapRoot->LinkChildObjects();

   // Allow objects to change it's modification flag
   g_bModificationsLocked = FALSE;

   // Recalculate status for built-in objects
   g_pEntireNet->CalculateCompoundStatus();
   g_pServiceRoot->CalculateCompoundStatus();
   g_pTemplateRoot->CalculateCompoundStatus();
   g_pPolicyRoot->CalculateCompoundStatus();
   g_pMapRoot->CalculateCompoundStatus();

   // Recalculate status for zone objects
   if (g_dwFlags & AF_ENABLE_ZONING)
   {
      for(i = 0; i < g_dwZoneGUIDIndexSize; i++)
         ((NetObj *)g_pZoneIndexByGUID[i].pObject)->CalculateCompoundStatus();
   }

	// Validate system templates and create when needed
   DbgPrintf(2, _T("Validating system templates..."));

	pTemplate = FindTemplateByName(_T("@System.Agent"));
	if (pTemplate == NULL)
	{
		pTemplate = new Template(_T("@System.Agent"));
		pTemplate->SetSystemFlag(TRUE);
      NetObjInsert(pTemplate, TRUE);
		g_pTemplateRoot->AddChild(pTemplate);
		pTemplate->AddParent(g_pTemplateRoot);
		pTemplate->CalculateCompoundStatus();
		pTemplate->Unhide();
	}
	pTemplate->ValidateSystemTemplate();

	/*
	TODO: probably system templates must be removed completely

	pTemplate = FindTemplateByName(_T("@System.SNMP"));
	if (pTemplate == NULL)
	{
		pTemplate = new Template(_T("@System.SNMP"));
		pTemplate->SetSystemFlag(TRUE);
      NetObjInsert(pTemplate, TRUE);
		g_pTemplateRoot->AddChild(pTemplate);
		pTemplate->AddParent(g_pTemplateRoot);
		pTemplate->CalculateCompoundStatus();
		pTemplate->Unhide();
	}
	pTemplate->ValidateSystemTemplate();
	*/

   return TRUE;
}


//
// Delete user or group from all objects' ACLs
//

void DeleteUserFromAllObjects(DWORD dwUserId)
{
   DWORD i;

   // Walk through all objects
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
      ((NetObj *)g_pIndexById[i].pObject)->DropUserAccess(dwUserId);
   RWLockUnlock(g_rwlockIdIndex);
}


//
// Dump objects to console in standalone mode
//

void DumpObjects(CONSOLE_CTX pCtx)
{
   DWORD i;
   TCHAR *pBuffer;
   CONTAINER_CATEGORY *pCat;
   NetObj *pObject;
	time_t t;
	struct tm *ltm;

   pBuffer = (TCHAR *)malloc(128000 * sizeof(TCHAR));
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
   	pObject = (NetObj *)g_pIndexById[i].pObject;
      ConsolePrintf(pCtx, _T("Object ID %d \"%s\"\n")
                          _T("   Class: %s  Primary IP: %s  Status: %s  IsModified: %d  IsDeleted: %d\n"),
                    pObject->Id(), pObject->Name(), g_szClassName[pObject->Type()],
                    IpToStr(pObject->IpAddr(), pBuffer),
                    g_szStatusTextSmall[pObject->Status()],
                    pObject->IsModified(), pObject->IsDeleted());
      ConsolePrintf(pCtx, _T("   Parents: <%s>\n   Childs: <%s>\n"), 
                    pObject->ParentList(pBuffer), pObject->ChildList(&pBuffer[4096]));
		t = pObject->TimeStamp();
		ltm = localtime(&t);
		_tcsftime(pBuffer, 256, _T("%d.%b.%Y %H:%M:%S"), ltm);
      ConsolePrintf(pCtx, _T("   Last change: %s"), pBuffer);
      switch(pObject->Type())
      {
         case OBJECT_NODE:
            ConsolePrintf(pCtx, _T("   IsSNMP: %d IsAgent: %d IsLocal: %d OID: %s\n"),
                          ((Node *)pObject)->isSNMPSupported(),
                          ((Node *)pObject)->isNativeAgent(),
                          ((Node *)pObject)->isLocalManagement(),
                          ((Node *)pObject)->getObjectId());
            break;
         case OBJECT_SUBNET:
            ConsolePrintf(pCtx, _T("   Network mask: %s\n"), 
                          IpToStr(((Subnet *)pObject)->getIpNetMask(), pBuffer));
            break;
         case OBJECT_CONTAINER:
            pCat = FindContainerCategory(((Container *)pObject)->Category());
            ConsolePrintf(pCtx, _T("   Category: %s\n"), pCat ? pCat->szName : _T("<unknown>"));
            break;
         case OBJECT_TEMPLATE:
            ConsolePrintf(pCtx, _T("   Version: %d.%d\n"), 
                          ((Template *)(pObject))->getVersionMajor(),
                          ((Template *)(pObject))->getVersionMinor());
            break;
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
   free(pBuffer);
}


//
// Check is given object class is a valid parent class for other object
// This function is used to check manually created bindings, so it won't
// return TRUE for node -- subnet for example
//

BOOL IsValidParentClass(int iChildClass, int iParentClass)
{
   switch(iParentClass)
   {
      case OBJECT_SERVICEROOT:
      case OBJECT_CONTAINER:
         if ((iChildClass == OBJECT_CONTAINER) || 
             (iChildClass == OBJECT_NODE) ||
             (iChildClass == OBJECT_CLUSTER) ||
             (iChildClass == OBJECT_CONDITION))
            return TRUE;
         break;
      case OBJECT_TEMPLATEROOT:
      case OBJECT_TEMPLATEGROUP:
         if ((iChildClass == OBJECT_TEMPLATEGROUP) || 
             (iChildClass == OBJECT_TEMPLATE))
            return TRUE;
         break;
      case OBJECT_NETWORKMAPROOT:
      case OBJECT_NETWORKMAPGROUP:
         if ((iChildClass == OBJECT_NETWORKMAPGROUP) || 
             (iChildClass == OBJECT_NETWORKMAP))
            return TRUE;
         break;
      case OBJECT_POLICYROOT:
      case OBJECT_POLICYGROUP:
         if ((iChildClass == OBJECT_POLICYGROUP) || 
             (iChildClass == OBJECT_AGENTPOLICY) ||
             (iChildClass == OBJECT_AGENTPOLICY_CONFIG))
            return TRUE;
         break;
      case OBJECT_NODE:
         if ((iChildClass == OBJECT_NETWORKSERVICE) ||
             (iChildClass == OBJECT_VPNCONNECTOR))
            return TRUE;
         break;
      case OBJECT_CLUSTER:
         if (iChildClass == OBJECT_NODE)
            return TRUE;
         break;
      case -1:    // Creating object without parent
         if (iChildClass == OBJECT_NODE)
            return TRUE;   // OK only for nodes, because parent subnet will be created automatically
         break;
   }
   return FALSE;
}


//
// Delete object (final step)
// This function should be called ONLY from syncer thread
// Access to object index by id are write-locked when this function is called
// Object will be removed from index by ID and destroyed.
//

void NetObjDelete(NetObj *pObject)
{
   TCHAR szQuery[256], szIpAddr[16], szNetMask[16];

   // Write object to deleted objects table
   _sntprintf(szQuery, 256, _T("INSERT INTO deleted_objects (object_id,object_class,name,"
                               _T("ip_addr,ip_netmask) VALUES (%d,%d,'%s','%s','%s')")),
              pObject->Id(), pObject->Type(), pObject->Name(), 
              IpToStr(pObject->IpAddr(), szIpAddr),
              IpToStr(GetObjectNetmask(pObject), szNetMask));
   QueueSQLRequest(szQuery);

   // Delete object from index by ID and object itself
   DeleteObjectFromIndex(&g_pIndexById, &g_dwIdIndexSize, pObject->Id());
   delete pObject;
}


//
// Update node index when primary IP address changes
//

void UpdateNodeIndex(DWORD dwOldIpAddr, DWORD dwNewIpAddr, NetObj *pObject)
{
   DWORD dwPos;

   RWLockWriteLock(g_rwlockNodeIndex, INFINITE);
   dwPos = SearchIndex(g_pNodeIndexByAddr, g_dwNodeAddrIndexSize, dwOldIpAddr);
   if (dwPos != INVALID_INDEX)
   {
      g_pNodeIndexByAddr[dwPos].dwKey = dwNewIpAddr;
      qsort(g_pNodeIndexByAddr, g_dwNodeAddrIndexSize, sizeof(INDEX), IndexCompare);
   }
   else
   {
      AddObjectToIndex(&g_pNodeIndexByAddr, &g_dwNodeAddrIndexSize, dwNewIpAddr, pObject);
   }
   RWLockUnlock(g_rwlockNodeIndex);
}


//
// Update interface index when IP address changes
//

void UpdateInterfaceIndex(DWORD dwOldIpAddr, DWORD dwNewIpAddr, NetObj *pObject)
{
   DWORD dwPos;

   RWLockWriteLock(g_rwlockInterfaceIndex, INFINITE);
   dwPos = SearchIndex(g_pInterfaceIndexByAddr, g_dwInterfaceAddrIndexSize, dwOldIpAddr);
   if (dwPos != INVALID_INDEX)
   {
      g_pInterfaceIndexByAddr[dwPos].dwKey = dwNewIpAddr;
      qsort(g_pInterfaceIndexByAddr, g_dwInterfaceAddrIndexSize, sizeof(INDEX), IndexCompare);
   }
   else
   {
      AddObjectToIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, dwNewIpAddr, pObject);
   }
   RWLockUnlock(g_rwlockInterfaceIndex);
}


//
// Calculate propagated status for object using default algorithm
//

int DefaultPropagatedStatus(int iObjectStatus)
{
   int iStatus;

   switch(m_iStatusPropAlg)
   {
      case SA_PROPAGATE_UNCHANGED:
         iStatus = iObjectStatus;
         break;
      case SA_PROPAGATE_FIXED:
         iStatus = ((iObjectStatus > STATUS_NORMAL) && (iObjectStatus < STATUS_UNKNOWN)) ? m_iFixedStatus : iObjectStatus;
         break;
      case SA_PROPAGATE_RELATIVE:
         if ((iObjectStatus > STATUS_NORMAL) && (iObjectStatus < STATUS_UNKNOWN))
         {
            iStatus = iObjectStatus + m_iStatusShift;
            if (iStatus < 0)
               iStatus = 0;
            if (iStatus > STATUS_CRITICAL)
               iStatus = STATUS_CRITICAL;
         }
         else
         {
            iStatus = iObjectStatus;
         }
         break;
      case SA_PROPAGATE_TRANSLATED:
         if ((iObjectStatus > STATUS_NORMAL) && (iObjectStatus < STATUS_UNKNOWN))
         {
            iStatus = m_iStatusTranslation[iObjectStatus - 1];
         }
         else
         {
            iStatus = iObjectStatus;
         }
         break;
      default:
         iStatus = STATUS_UNKNOWN;
         break;
   }
   return iStatus;
}


//
// Get default data for status calculation
//

int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds)
{
   *pnSingleThreshold = m_iStatusSingleThreshold;
   *ppnThresholds = m_iStatusThresholds;
   return m_iStatusCalcAlg;
}


//
// Check if given object is an agent policy object
//

bool IsAgentPolicyObject(NetObj *object)
{
	return (object->Type() == OBJECT_AGENTPOLICY) || (object->Type() == OBJECT_AGENTPOLICY_CONFIG);
}
