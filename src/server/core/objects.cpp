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

MUTEX g_hMutexObjectAccess;
RWLOCK g_rwlockIdIndex;
RWLOCK g_rwlockNodeIndex;
RWLOCK g_rwlockSubnetIndex;
RWLOCK g_rwlockInterfaceIndex;

DWORD g_dwNumCategories = 0;
CONTAINER_CATEGORY *g_pContainerCatList = NULL;

char *g_pszStatusName[] = { "Normal", "Warning", "Minor", "Major", "Critical",
                            "Unknown", "Unmanaged", "Disabled", "Testing" };
char *g_szClassName[]={ "Generic", "Subnet", "Node", "Interface",
                        "Network", "Container", "Zone", "ServiceRoot",
                        "Template", "TemplateGroup", "TemplateRoot" };


//
// Initialize objects infrastructure
//

void ObjectsInit(void)
{
   g_hMutexObjectAccess = MutexCreate();
   g_rwlockIdIndex = RWLockCreate();
   g_rwlockNodeIndex = RWLockCreate();
   g_rwlockSubnetIndex = RWLockCreate();
   g_rwlockInterfaceIndex = RWLockCreate();

   // Create "Entire Network" object
   g_pEntireNet = new Network;
   NetObjInsert(g_pEntireNet, FALSE);

   // Create "Service Root" object
   g_pServiceRoot = new ServiceRoot;
   NetObjInsert(g_pServiceRoot, FALSE);

   // Create "Template Root" object
   g_pTemplateRoot = new TemplateRoot;
   NetObjInsert(g_pTemplateRoot, FALSE);
}


//
// Lock write access to all objects
//

void ObjectsGlobalLock(void)
{
   MutexLock(g_hMutexObjectAccess, INFINITE);
}


//
// Unlock write access to all objects
//

void ObjectsGlobalUnlock(void)
{
   MutexUnlock(g_hMutexObjectAccess);
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
   ObjectsGlobalLock();
   if (bNewObject)   
   {
      // Assign unique ID to new object
      pObject->SetId(CreateUniqueId(IDG_NETWORK_OBJECT));

      // Create table for storing data collection values
      if (pObject->Type() == OBJECT_NODE)
      {
         char szQuery[256], szQueryTemplate[256];

         ConfigReadStr("IDataTableCreationCommand", szQueryTemplate, 255, "");
         sprintf(szQuery, szQueryTemplate, pObject->Id());
         DBQuery(g_hCoreDB, szQuery);
      }
   }
   RWLockWriteLock(g_rwlockIdIndex, INFINITE);
   AddObjectToIndex(&g_pIndexById, &g_dwIdIndexSize, pObject->Id(), pObject);
   RWLockUnlock(g_rwlockIdIndex);
   if ((pObject->IpAddr() != 0) && (!pObject->IsDeleted()))
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
            if (bNewObject)
               PostEvent(EVENT_NODE_ADDED, pObject->Id(), NULL);
            break;
         case OBJECT_INTERFACE:
            RWLockWriteLock(g_rwlockInterfaceIndex, INFINITE);
            AddObjectToIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, pObject->IpAddr(), pObject);
            RWLockUnlock(g_rwlockInterfaceIndex);
            break;
         default:
            WriteLog(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
            break;
      }
   }
   ObjectsGlobalUnlock();
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
         default:
            WriteLog(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
            break;
      }
}


//
// Get IP netmask for object of any class
//

DWORD GetObjectNetmask(NetObj *pObject)
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
// Delete object (final step)
// This function should be called only by syncer thread when access to objects are locked.
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

   // Delete object from index by ID
   RWLockWriteLock(g_rwlockIdIndex, INFINITE);
   DeleteObjectFromIndex(&g_pIndexById, &g_dwIdIndexSize, pObject->Id());
   RWLockUnlock(g_rwlockIdIndex);
            
   delete pObject;
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
   pNode = (dwPos == INVALID_INDEX) ? NULL : (Node *)g_pInterfaceIndexByAddr[dwPos].pObject->GetParent();
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

   // Load container categories
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
   g_pEntireNet->LoadFromDB();
   g_pServiceRoot->LoadFromDB();
   g_pTemplateRoot->LoadFromDB();

   // Load subnets
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
               g_pEntireNet->AddSubnet(pSubnet);
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
   }

   // Load interfaces
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

   // Load templates
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
   for(i = 0; i < g_dwIdIndexSize; i++)
      if ((g_pIndexById[i].pObject->Type() == OBJECT_CONTAINER) ||
          (g_pIndexById[i].pObject->Type() == OBJECT_TEMPLATEGROUP))
         ((Container *)g_pIndexById[i].pObject)->LinkChildObjects();

   // Link childs to "Service Root" and "Template Root" objects
   g_pServiceRoot->LinkChildObjects();
   g_pTemplateRoot->LinkChildObjects();

   // Recalculate status for built-in objects
   g_pEntireNet->CalculateCompoundStatus();
   g_pServiceRoot->CalculateCompoundStatus();
   g_pTemplateRoot->CalculateCompoundStatus();

   return TRUE;
}


//
// Delete user or group from all objects' ACLs
//

void DeleteUserFromAllObjects(DWORD dwUserId)
{
   DWORD i;

   ObjectsGlobalLock();

   // Walk through all objects
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
      g_pIndexById[i].pObject->DropUserAccess(dwUserId);
   RWLockUnlock(g_rwlockIdIndex);

   ObjectsGlobalUnlock();
}


//
// Dump objects to console in standalone mode
//

void DumpObjects(void)
{
   DWORD i;
   char *pBuffer;
   CONTAINER_CATEGORY *pCat;

   pBuffer = (char *)malloc(128000);
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
      printf("Object ID %d \"%s\"\n"
             "   Class: %s  Primary IP: %s  Status: %s  IsModified: %d  IsDeleted: %d\n",
             g_pIndexById[i].pObject->Id(),g_pIndexById[i].pObject->Name(),
             g_szClassName[g_pIndexById[i].pObject->Type()],
             IpToStr(g_pIndexById[i].pObject->IpAddr(), pBuffer),
             g_pszStatusName[g_pIndexById[i].pObject->Status()],
             g_pIndexById[i].pObject->IsModified(), g_pIndexById[i].pObject->IsDeleted());
      printf("   Parents: <%s>\n   Childs: <%s>\n", 
             g_pIndexById[i].pObject->ParentList(pBuffer),
             g_pIndexById[i].pObject->ChildList(&pBuffer[4096]));
      switch(g_pIndexById[i].pObject->Type())
      {
         case OBJECT_NODE:
            printf("   IsSNMP: %d IsAgent: %d IsLocal: %d OID: %s\n",
                   ((Node *)(g_pIndexById[i].pObject))->IsSNMPSupported(),
                   ((Node *)(g_pIndexById[i].pObject))->IsNativeAgent(),
                   ((Node *)(g_pIndexById[i].pObject))->IsLocalManagement(),
                   ((Node *)(g_pIndexById[i].pObject))->ObjectId());
            break;
         case OBJECT_SUBNET:
            printf("   Network mask: %s\n", 
                   IpToStr(((Subnet *)g_pIndexById[i].pObject)->IpNetMask(), pBuffer));
            break;
         case OBJECT_CONTAINER:
            pCat = FindContainerCategory(((Container *)g_pIndexById[i].pObject)->Category());
            printf("   Category: %s\n   Description: %s\n", pCat ? pCat->szName : "<unknown>",
                   ((Container *)g_pIndexById[i].pObject)->Description());
            break;
         case OBJECT_TEMPLATE:
            printf("   Version: %d.%d\n   Description: %s\n", 
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
      case -1:    // Creating object without parent
         if (iChildClass == OBJECT_NODE)
            return TRUE;   // OK only for nodes, because parent subnet will be created automatically
         break;
   }
   return FALSE;
}
