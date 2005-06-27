/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: objects.cpp
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

DWORD g_dwMgmtNode = 0;
INDEX *g_pIndexById = NULL;
DWORD g_dwIdIndexSize = 0;
INDEX *g_pSubnetIndexByAddr = NULL;
DWORD g_dwSubnetAddrIndexSize = 0;
INDEX *g_pNodeIndexByAddr = NULL;
DWORD g_dwNodeAddrIndexSize = 0;
INDEX *g_pInterfaceIndexByAddr = NULL;
DWORD g_dwInterfaceAddrIndexSize = 0;
INDEX *g_pZoneIndexByGUID = NULL;
DWORD g_dwZoneGUIDIndexSize = 0;

RWLOCK g_rwlockIdIndex;
RWLOCK g_rwlockNodeIndex;
RWLOCK g_rwlockSubnetIndex;
RWLOCK g_rwlockInterfaceIndex;
RWLOCK g_rwlockZoneIndex;

DWORD g_dwNumCategories = 0;
CONTAINER_CATEGORY *g_pContainerCatList = NULL;

Queue *g_pTemplateUpdateQueue = NULL;

char *g_pszStatusName[] = { "Normal", "Warning", "Minor", "Major", "Critical",
                            "Unknown", "Unmanaged", "Disabled", "Testing" };
char *g_szClassName[]={ "Generic", "Subnet", "Node", "Interface",
                        "Network", "Container", "Zone", "ServiceRoot",
                        "Template", "TemplateGroup", "TemplateRoot", "NetworkService" };


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
                  bLock1 = pInfo->pTemplate->LockDCIList(0x7FFFFFFF);
                  bLock2 = ((Node *)pNode)->LockDCIList(0x7FFFFFFF);
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
                  if (((Node *)pNode)->LockDCIList(0x7FFFFFFF))
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

   DbgPrintf(AF_DEBUG_DC, _T("Started caching of DCI values"));
   RWLockReadLock(g_rwlockNodeIndex, INFINITE);
   for(i = 0; i < g_dwNodeAddrIndexSize; i++)
   {
      ((Node *)g_pNodeIndexByAddr[i].pObject)->UpdateDCICache();
      ThreadSleepMs(100);  // Give a chance to other threads to do something with database
   }
   RWLockUnlock(g_rwlockNodeIndex);
   DbgPrintf(AF_DEBUG_DC, _T("Finished caching of DCI values"));
   return THREAD_OK;
}


//
// Initialize objects infrastructure
//

void ObjectsInit(void)
{
   g_pTemplateUpdateQueue = new Queue;

   g_rwlockIdIndex = RWLockCreate();
   g_rwlockNodeIndex = RWLockCreate();
   g_rwlockSubnetIndex = RWLockCreate();
   g_rwlockInterfaceIndex = RWLockCreate();
   g_rwlockZoneIndex = RWLockCreate();

   // Create "Entire Network" object
   g_pEntireNet = new Network;
   NetObjInsert(g_pEntireNet, FALSE);

   // Create "Service Root" object
   g_pServiceRoot = new ServiceRoot;
   NetObjInsert(g_pServiceRoot, FALSE);

   // Create "Template Root" object
   g_pTemplateRoot = new TemplateRoot;
   NetObjInsert(g_pTemplateRoot, FALSE);
   DbgPrintf(AF_DEBUG_MISC, "Built-in objects created");

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

static void AddObjectToIndex(INDEX **ppIndex, DWORD *pdwIndexSize, DWORD dwKey, NetObj *pObject)
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

static DWORD SearchIndex(INDEX *pIndex, DWORD dwIndexSize, DWORD dwKey)
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

static void DeleteObjectFromIndex(INDEX **ppIndex, DWORD *pdwIndexSize, DWORD dwKey)
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
   if (((pObject->IpAddr() != 0) || (pObject->Type() == OBJECT_ZONE)) && (!pObject->IsDeleted()))
   {
      switch(pObject->Type())
      {
         case OBJECT_GENERIC:
         case OBJECT_NETWORK:
         case OBJECT_CONTAINER:
         case OBJECT_SERVICEROOT:
         case OBJECT_NETWORKSERVICE:
            break;
         case OBJECT_SUBNET:
            RWLockWriteLock(g_rwlockSubnetIndex, INFINITE);
            AddObjectToIndex(&g_pSubnetIndexByAddr, &g_dwSubnetAddrIndexSize, pObject->IpAddr(), pObject);
            RWLockUnlock(g_rwlockSubnetIndex);
            if (bNewObject)
               PostEvent(EVENT_SUBNET_ADDED, pObject->Id(), NULL);
            break;
         case OBJECT_NODE:
            RWLockWriteLock(g_rwlockNodeIndex, INFINITE);
            AddObjectToIndex(&g_pNodeIndexByAddr, &g_dwNodeAddrIndexSize, pObject->IpAddr(), pObject);
            RWLockUnlock(g_rwlockNodeIndex);
            break;
         case OBJECT_INTERFACE:
            RWLockWriteLock(g_rwlockInterfaceIndex, INFINITE);
            AddObjectToIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, pObject->IpAddr(), pObject);
            RWLockUnlock(g_rwlockInterfaceIndex);
            break;
         case OBJECT_ZONE:
            RWLockWriteLock(g_rwlockZoneIndex, INFINITE);
            AddObjectToIndex(&g_pZoneIndexByGUID, &g_dwZoneGUIDIndexSize, ((Zone *)pObject)->GUID(), pObject);
            RWLockUnlock(g_rwlockZoneIndex);
            break;
         default:
            WriteLog(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
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
   if (pObject->IpAddr() != 0)
      switch(pObject->Type())
      {
         case OBJECT_GENERIC:
         case OBJECT_NETWORK:
         case OBJECT_CONTAINER:
         case OBJECT_SERVICEROOT:
         case OBJECT_NETWORKSERVICE:
            break;
         case OBJECT_SUBNET:
            RWLockWriteLock(g_rwlockSubnetIndex, INFINITE);
            DeleteObjectFromIndex(&g_pSubnetIndexByAddr, &g_dwSubnetAddrIndexSize, pObject->IpAddr());
            RWLockUnlock(g_rwlockSubnetIndex);
            break;
         case OBJECT_NODE:
            RWLockWriteLock(g_rwlockNodeIndex, INFINITE);
            DeleteObjectFromIndex(&g_pNodeIndexByAddr, &g_dwNodeAddrIndexSize, pObject->IpAddr());
            RWLockUnlock(g_rwlockNodeIndex);
            break;
         case OBJECT_INTERFACE:
            RWLockWriteLock(g_rwlockInterfaceIndex, INFINITE);
            DeleteObjectFromIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, pObject->IpAddr());
            RWLockUnlock(g_rwlockInterfaceIndex);
            break;
         case OBJECT_ZONE:
            RWLockWriteLock(g_rwlockZoneIndex, INFINITE);
            DeleteObjectFromIndex(&g_pZoneIndexByGUID, &g_dwZoneGUIDIndexSize, ((Zone *)pObject)->GUID());
            RWLockUnlock(g_rwlockZoneIndex);
            break;
         default:
            WriteLog(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
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
   pObject = (dwPos == INVALID_INDEX) ? NULL : g_pIndexById[dwPos].pObject;
   RWLockUnlock(g_rwlockIdIndex);
   return pObject;
}


//
// Find object by ID
//

Zone NXCORE_EXPORTABLE *FindZoneByGUID(DWORD dwZoneGUID)
{
   DWORD dwPos;
   NetObj *pObject;

   if (g_pZoneIndexByGUID == NULL)
      return NULL;

   RWLockReadLock(g_rwlockZoneIndex, INFINITE);
   dwPos = SearchIndex(g_pZoneIndexByGUID, g_dwZoneGUIDIndexSize, dwZoneGUID);
   pObject = (dwPos == INVALID_INDEX) ? NULL : g_pZoneIndexByGUID[dwPos].pObject;
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
         dwId = g_pNodeIndexByAddr[i].pObject->Id();
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
   DWORD i, dwNumRows;
   DWORD dwId;
   char szQuery[256];

   // Prevent objects to change it's modification flag
   g_bModificationsLocked = TRUE;

   // Load container categories
   DbgPrintf(AF_DEBUG_MISC, "Loading container categories...");
   hResult = DBSelect(g_hCoreDB, "SELECT category,name,image_id,description FROM container_categories");
   if (hResult != NULL)
   {
      g_dwNumCategories = DBGetNumRows(hResult);
      g_pContainerCatList = (CONTAINER_CATEGORY *)malloc(sizeof(CONTAINER_CATEGORY) * g_dwNumCategories);
      for(i = 0; i < (int)g_dwNumCategories; i++)
      {
         g_pContainerCatList[i].dwCatId = DBGetFieldULong(hResult, i, 0);
         strncpy(g_pContainerCatList[i].szName, DBGetField(hResult, i, 1), MAX_OBJECT_NAME);
         g_pContainerCatList[i].dwImageId = DBGetFieldULong(hResult, i, 2);
         g_pContainerCatList[i].pszDescription = strdup(DBGetField(hResult, i, 3));
      }
      DBFreeResult(hResult);
   }

   // Load built-in object properties
   DbgPrintf(AF_DEBUG_MISC, "Loading built-in object properties...");
   g_pEntireNet->LoadFromDB();
   g_pServiceRoot->LoadFromDB();
   g_pTemplateRoot->LoadFromDB();

   // Load zones
   if (g_dwFlags & AF_ENABLE_ZONING)
   {
      Zone *pZone;

      DbgPrintf(AF_DEBUG_MISC, "Loading zones...");

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
               WriteLog(MSG_ZONE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
            }
         }
         DBFreeResult(hResult);
      }
   }

   // Load subnets
   DbgPrintf(AF_DEBUG_MISC, "Loading subnets...");
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
            WriteLog(MSG_SUBNET_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load nodes
   DbgPrintf(AF_DEBUG_MISC, "Loading nodes...");
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
            WriteLog(MSG_NODE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);

      // Start cache loading thread
      ThreadCreate(CacheLoadingThread, 0, NULL);
   }

   // Load interfaces
   DbgPrintf(AF_DEBUG_MISC, "Loading interfaces...");
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
            WriteLog(MSG_INTERFACE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load network services
   DbgPrintf(AF_DEBUG_MISC, "Loading network services...");
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
            WriteLog(MSG_NETSRV_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load templates
   DbgPrintf(AF_DEBUG_MISC, "Loading templates...");
   hResult = DBSelect(g_hCoreDB, "SELECT id FROM templates");
   if (hResult != 0)
   {
      Template *pTemplate;

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
            WriteLog(MSG_TEMPLATE_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load container objects
   DbgPrintf(AF_DEBUG_MISC, "Loading containers...");
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
            WriteLog(MSG_CONTAINER_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Load template group objects
   DbgPrintf(AF_DEBUG_MISC, "Loading template groups...");
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
            WriteLog(MSG_TG_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "d", dwId);
         }
      }
      DBFreeResult(hResult);
   }

   // Link childs to container and template group objects
   DbgPrintf(AF_DEBUG_MISC, "Linking objects...");
   for(i = 0; i < g_dwIdIndexSize; i++)
      if ((g_pIndexById[i].pObject->Type() == OBJECT_CONTAINER) ||
          (g_pIndexById[i].pObject->Type() == OBJECT_TEMPLATEGROUP))
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
         g_pZoneIndexByGUID[i].pObject->CalculateCompoundStatus();
   }

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
      g_pIndexById[i].pObject->DropUserAccess(dwUserId);
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

   pBuffer = (char *)malloc(128000);
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
      ConsolePrintf(pCtx, "Object ID %d \"%s\"\n"
                          "   Class: %s  Primary IP: %s  Status: %s  IsModified: %d  IsDeleted: %d\n",
                    g_pIndexById[i].pObject->Id(),g_pIndexById[i].pObject->Name(),
                    g_szClassName[g_pIndexById[i].pObject->Type()],
                    IpToStr(g_pIndexById[i].pObject->IpAddr(), pBuffer),
                    g_pszStatusName[g_pIndexById[i].pObject->Status()],
                    g_pIndexById[i].pObject->IsModified(), g_pIndexById[i].pObject->IsDeleted());
      ConsolePrintf(pCtx, "   Parents: <%s>\n   Childs: <%s>\n", 
                    g_pIndexById[i].pObject->ParentList(pBuffer),
                    g_pIndexById[i].pObject->ChildList(&pBuffer[4096]));
      ConsolePrintf(pCtx, "   Last change: %s", g_pIndexById[i].pObject->TimeStampAsText());
      switch(g_pIndexById[i].pObject->Type())
      {
         case OBJECT_NODE:
            ConsolePrintf(pCtx, "   IsSNMP: %d IsAgent: %d IsLocal: %d OID: %s\n",
                          ((Node *)(g_pIndexById[i].pObject))->IsSNMPSupported(),
                          ((Node *)(g_pIndexById[i].pObject))->IsNativeAgent(),
                          ((Node *)(g_pIndexById[i].pObject))->IsLocalManagement(),
                          ((Node *)(g_pIndexById[i].pObject))->ObjectId());
            break;
         case OBJECT_SUBNET:
            ConsolePrintf(pCtx, "   Network mask: %s\n", 
                          IpToStr(((Subnet *)g_pIndexById[i].pObject)->IpNetMask(), pBuffer));
            break;
         case OBJECT_CONTAINER:
            pCat = FindContainerCategory(((Container *)g_pIndexById[i].pObject)->Category());
            ConsolePrintf(pCtx, "   Category: %s\n   Description: %s\n", pCat ? pCat->szName : "<unknown>",
                          ((Container *)g_pIndexById[i].pObject)->Description());
            break;
         case OBJECT_TEMPLATE:
            ConsolePrintf(pCtx, "   Version: %d.%d\n   Description: %s\n", 
                          ((Template *)(g_pIndexById[i].pObject))->VersionMajor(),
                          ((Template *)(g_pIndexById[i].pObject))->VersionMinor(),
                          ((Template *)(g_pIndexById[i].pObject))->Description());
            break;
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
   free(pBuffer);
}


//
// Check is given object class is a valid parent class for other object
// This function is used to check manually created bindings, so i won't
// return TRUE for node -- subnet for example
//

BOOL IsValidParentClass(int iChildClass, int iParentClass)
{
   switch(iParentClass)
   {
      case OBJECT_SERVICEROOT:
      case OBJECT_CONTAINER:
         if ((iChildClass == OBJECT_CONTAINER) || 
             (iChildClass == OBJECT_NODE))
            return TRUE;
         break;
      case OBJECT_TEMPLATEROOT:
      case OBJECT_TEMPLATEGROUP:
         if ((iChildClass == OBJECT_TEMPLATEGROUP) || 
             (iChildClass == OBJECT_TEMPLATE))
            return TRUE;
         break;
      case OBJECT_NODE:
         if (iChildClass == OBJECT_NETWORKSERVICE)
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
   char szQuery[256], szIpAddr[16], szNetMask[16];

   // Write object to deleted objects table
   _sntprintf(szQuery, 256, _T("INSERT INTO deleted_objects (object_id,object_class,name,"
                               "ip_addr,ip_netmask) VALUES (%ld,%ld,'%s','%s','%s')"),
              pObject->Id(), pObject->Type(), pObject->Name(), 
              IpToStr(pObject->IpAddr(), szIpAddr),
              IpToStr(GetObjectNetmask(pObject), szNetMask));
   DBQuery(g_hCoreDB, szQuery);

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
