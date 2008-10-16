/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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

Network *g_pEntireNet = NULL;
ServiceRoot *g_pServiceRoot = NULL;
TemplateRoot *g_pTemplateRoot = NULL;

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

const char *g_szClassName[]={ "Generic", "Subnet", "Node", "Interface",
                              "Network", "Container", "Zone", "ServiceRoot",
                              "Template", "TemplateGroup", "TemplateRoot",
                              "NetworkService", "VPNConnector", "Condition",
                              "Cluster" };


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

   while(1)
   {
      pInfo = (TEMPLATE_UPDATE_INFO *)g_pTemplateUpdateQueue->GetOrBlock();
      if (pInfo == INVALID_POINTER_VALUE)
         break;

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
                     ((Node *)pNode)->UnbindFromTemplate(pInfo->pTemplate->Id(), pInfo->bRemoveDCI);
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
			pInfo->pTemplate->DecRefCount();
         free(pInfo);
      }
      else
      {
         g_pTemplateUpdateQueue->Put(pInfo);    // Requeue
         ThreadSleepMs(500);
      }
   }

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
   for(i = 0; i < g_dwNodeAddrIndexSize; i++)
   {
      ((Node *)g_pNodeIndexByAddr[i].pObject)->UpdateDCICache();
      ThreadSleepMs(50);  // Give a chance to other threads to do something with database
   }
   RWLockUnlock(g_rwlockNodeIndex);
   DbgPrintf(1, _T("Finished caching of DCI values"));
   return THREAD_OK;
}


//
// Initialize objects infrastructure
//

void ObjectsInit(void)
{
   // Load default status calculation info
   m_iStatusCalcAlg = ConfigReadInt("StatusCalculationAlgorithm", SA_CALCULATE_MOST_CRITICAL);
   m_iStatusPropAlg = ConfigReadInt("StatusPropagationAlgorithm", SA_PROPAGATE_UNCHANGED);
   m_iFixedStatus = ConfigReadInt("FixedStatusValue", STATUS_NORMAL);
   m_iStatusShift = ConfigReadInt("StatusShift", 0);
   ConfigReadByteArray("StatusTranslation", m_iStatusTranslation, 4, STATUS_WARNING);
   m_iStatusSingleThreshold = ConfigReadInt("StatusSingleThreshold", 75);
   ConfigReadByteArray("StatusThresholds", m_iStatusThresholds, 4, 50);

   g_pTemplateUpdateQueue = new Queue;

   g_rwlockIdIndex = RWLockCreate();
   g_rwlockNodeIndex = RWLockCreate();
   g_rwlockSubnetIndex = RWLockCreate();
   g_rwlockInterfaceIndex = RWLockCreate();
   g_rwlockZoneIndex = RWLockCreate();
   g_rwlockConditionIndex = RWLockCreate();

   // Create "Entire Network" object
   g_pEntireNet = new Network;
   NetObjInsert(g_pEntireNet, FALSE);

   // Create "Service Root" object
   g_pServiceRoot = new ServiceRoot;
   NetObjInsert(g_pServiceRoot, FALSE);

   // Create "Template Root" object
   g_pTemplateRoot = new TemplateRoot;
   NetObjInsert(g_pTemplateRoot, FALSE);
   DbgPrintf(1, "Built-in objects created");

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
      pObject->SetId(CreateUniqueId(IDG_NETWORK_OBJECT));

      // Create table for storing data collection values
      if (pObject->Type() == OBJECT_NODE)
      {
         char szQuery[256], szQueryTemplate[256];
         DWORD i;

         ConfigReadStr("IDataTableCreationCommand", szQueryTemplate, 255, "");
         sprintf(szQuery, szQueryTemplate, pObject->Id());
         DBQuery(g_hCoreDB, szQuery);

         for(i = 0; i < 10; i++)
         {
            sprintf(szQuery, "IDataIndexCreationCommand_%d", i);
            ConfigReadStr(szQuery, szQueryTemplate, 255, "");
            if (szQueryTemplate[0] != 0)
            {
               sprintf(szQuery, szQueryTemplate, pObject->Id(), pObject->Id());
               DBQuery(g_hCoreDB, szQuery);
            }
         }
      }
   }
   RWLockWriteLock(g_rwlockIdIndex, INFINITE);
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
         return ((Interface *)pObject)->IpNetMask();
      case OBJECT_SUBNET:
         return ((Subnet *)pObject)->IpNetMask();
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
   pNode = (dwPos == INVALID_INDEX) ? NULL : (Node *)((Interface *)g_pInterfaceIndexByAddr[dwPos].pObject)->GetParentNode();
   RWLockUnlock(g_rwlockInterfaceIndex);
   return pNode;
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
      if ((dwNodeAddr & ((Subnet *)g_pSubnetIndexByAddr[i].pObject)->IpNetMask()) == 
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

DWORD FindLocalMgmtNode(void)
{
   DWORD i, dwId = 0;

   if (g_pNodeIndexByAddr == NULL)
      return 0;

   RWLockReadLock(g_rwlockNodeIndex, INFINITE);
   for(i = 0; i < g_dwNodeAddrIndexSize; i++)
      if (((Node *)g_pNodeIndexByAddr[i].pObject)->Flags() & NF_IS_LOCAL_MGMT)
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

BOOL LoadObjects(void)
{
   DB_RESULT hResult;
   DWORD i, j, dwNumRows;
   DWORD dwId;
   Template *pTemplate;
   char szQuery[256];

   // Prevent objects to change it's modification flag
   g_bModificationsLocked = TRUE;

   // Load container categories
   DbgPrintf(2, "Loading container categories...");
   hResult = DBSelect(g_hCoreDB, "SELECT category,name,image_id,description FROM container_categories");
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
   DbgPrintf(2, "Loading built-in object properties...");
   g_pEntireNet->LoadFromDB();
   g_pServiceRoot->LoadFromDB();
   g_pTemplateRoot->LoadFromDB();

   // Load zones
   if (g_dwFlags & AF_ENABLE_ZONING)
   {
      Zone *pZone;

      DbgPrintf(2, "Loading zones...");

      // Load (or create) default zone
      pZone = new Zone;
      pZone->CreateFromDB(BUILTIN_OID_ZONE0);
      NetObjInsert(pZone, FALSE);
      g_pEntireNet->AddZone(pZone);

      hResult = DBSelect(g_hCoreDB, "SELECT id FROM zones WHERE id<>4");
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
   DbgPrintf(2, "Loading conditions...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM conditions");
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
   DbgPrintf(2, "Loading subnets...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM subnets");
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

                  pZone = FindZoneByGUID(pSubnet->ZoneGUID());
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
   DbgPrintf(2, "Loading nodes...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM nodes");
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
   DbgPrintf(2, "Loading interfaces...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM interfaces");
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
   DbgPrintf(2, "Loading network services...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM network_services");
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
   DbgPrintf(2, "Loading VPN connectors...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM vpn_connectors");
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
   DbgPrintf(2, "Loading clusters...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM clusters");
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
   DbgPrintf(2, "Loading templates...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM templates");
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
         }
         else     // Object load failed
         {
            delete pTemplate;
            nxlog_write(MSG_TEMPLATE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load container objects
   DbgPrintf(2, "Loading containers...");
   sprintf(szQuery, "SELECT id FROM containers WHERE object_class=%d", OBJECT_CONTAINER);
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
   DbgPrintf(2, "Loading template groups...");
   sprintf(szQuery, "SELECT id FROM containers WHERE object_class=%d", OBJECT_TEMPLATEGROUP);
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

   // Link childs to container and template group objects
   DbgPrintf(2, "Linking objects...");
   for(i = 0; i < g_dwIdIndexSize; i++)
      if ((((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_CONTAINER) ||
          (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_TEMPLATEGROUP))
         ((Container *)g_pIndexById[i].pObject)->LinkChildObjects();

   // Link childs to "Service Root" and "Template Root" objects
   g_pServiceRoot->LinkChildObjects();
   g_pTemplateRoot->LinkChildObjects();

   // Allow objects to change it's modification flag
   g_bModificationsLocked = FALSE;

   // Recalculate status for built-in objects
   g_pEntireNet->CalculateCompoundStatus();
   g_pServiceRoot->CalculateCompoundStatus();
   g_pTemplateRoot->CalculateCompoundStatus();

   // Recalculate status for zone objects
   if (g_dwFlags & AF_ENABLE_ZONING)
   {
      for(i = 0; i < g_dwZoneGUIDIndexSize; i++)
         ((NetObj *)g_pZoneIndexByGUID[i].pObject)->CalculateCompoundStatus();
   }

	// Apply system templates
   DbgPrintf(2, "Applying system templates...");

	pTemplate = FindTemplateByName(_T("@System.Agent"));
	if (pTemplate == NULL)
	{
		pTemplate = new Template(_T("@System.Agent"));
		pTemplate->SetSystemFlag(TRUE);
      NetObjInsert(pTemplate, TRUE);
		g_pTemplateRoot->AddChild(pTemplate);
		pTemplate->AddParent(g_pTemplateRoot);
		pTemplate->Unhide();
	}
	pTemplate->ValidateSystemTemplate();
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
	for(j = 0; j < g_dwIdIndexSize; j++)
		if (((NetObj *)g_pIndexById[j].pObject)->Type() == OBJECT_NODE)
		{
			if (((Node *)g_pIndexById[j].pObject)->IsNativeAgent())
				pTemplate->ApplyToNode((Node *)g_pIndexById[j].pObject);
		}
   RWLockUnlock(g_rwlockIdIndex);

	pTemplate = FindTemplateByName(_T("@System.SNMP"));
	if (pTemplate == NULL)
	{
		pTemplate = new Template(_T("@System.SNMP"));
		pTemplate->SetSystemFlag(TRUE);
      NetObjInsert(pTemplate, TRUE);
		g_pTemplateRoot->AddChild(pTemplate);
		pTemplate->AddParent(g_pTemplateRoot);
		pTemplate->Unhide();
	}
	pTemplate->ValidateSystemTemplate();
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
	for(j = 0; j < g_dwIdIndexSize; j++)
		if (((NetObj *)g_pIndexById[j].pObject)->Type() == OBJECT_NODE)
		{
			if (((Node *)g_pIndexById[j].pObject)->IsSNMPSupported())
				pTemplate->ApplyToNode((Node *)g_pIndexById[j].pObject);
		}
   RWLockUnlock(g_rwlockIdIndex);

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
   char *pBuffer;
   CONTAINER_CATEGORY *pCat;
   NetObj *pObject;

   pBuffer = (char *)malloc(128000);
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
   	pObject = (NetObj *)g_pIndexById[i].pObject;
      ConsolePrintf(pCtx, "Object ID %d \"%s\"\n"
                          "   Class: %s  Primary IP: %s  Status: %s  IsModified: %d  IsDeleted: %d\n",
                    pObject->Id(), pObject->Name(), g_szClassName[pObject->Type()],
                    IpToStr(pObject->IpAddr(), pBuffer),
                    g_szStatusTextSmall[pObject->Status()],
                    pObject->IsModified(), pObject->IsDeleted());
      ConsolePrintf(pCtx, "   Parents: <%s>\n   Childs: <%s>\n", 
                    pObject->ParentList(pBuffer), pObject->ChildList(&pBuffer[4096]));
      ConsolePrintf(pCtx, "   Last change: %s", pObject->TimeStampAsText());
      switch(pObject->Type())
      {
         case OBJECT_NODE:
            ConsolePrintf(pCtx, "   IsSNMP: %d IsAgent: %d IsLocal: %d OID: %s\n",
                          ((Node *)pObject)->IsSNMPSupported(),
                          ((Node *)pObject)->IsNativeAgent(),
                          ((Node *)pObject)->IsLocalManagement(),
                          ((Node *)pObject)->ObjectId());
            break;
         case OBJECT_SUBNET:
            ConsolePrintf(pCtx, "   Network mask: %s\n", 
                          IpToStr(((Subnet *)pObject)->IpNetMask(), pBuffer));
            break;
         case OBJECT_CONTAINER:
            pCat = FindContainerCategory(((Container *)pObject)->Category());
            ConsolePrintf(pCtx, "   Category: %s\n", pCat ? pCat->szName : "<unknown>");
            break;
         case OBJECT_TEMPLATE:
            ConsolePrintf(pCtx, "   Version: %d.%d\n", 
                          ((Template *)(pObject))->VersionMajor(),
                          ((Template *)(pObject))->VersionMinor());
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
                               "ip_addr,ip_netmask) VALUES (%d,%d,'%s','%s','%s')"),
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
