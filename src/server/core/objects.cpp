/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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

#include "nms_core.h"


//
// Global data
//

INDEX *g_pIndexById = NULL;
DWORD g_dwIdIndexSize = 0;
INDEX *g_pSubnetIndexByAddr = NULL;
DWORD g_dwSubnetAddrIndexSize = 0;
INDEX *g_pNodeIndexByAddr = NULL;
DWORD g_dwNodeAddrIndexSize = 0;
INDEX *g_pInterfaceIndexByAddr = NULL;
DWORD g_dwInterfaceAddrIndexSize = 0;


//
// Static data
//

static MUTEX m_hMutexObjectAccess;
static DWORD dwFreeObjectId = 1;


//
// Initialize objects infrastructure
//

void ObjectsInit(void)
{
   m_hMutexObjectAccess = MutexCreate();
}


//
// Lock write access to all objects
//

void ObjectsGlobalLock(void)
{
   MutexLock(m_hMutexObjectAccess, INFINITE);
}


//
// Unlock write access to all objects
//

void ObjectsGlobalUnlock(void)
{
   MutexUnlock(m_hMutexObjectAccess);
}


//
// Add new object to index
//

static void AddObjectToIndex(INDEX **ppIndex, DWORD *pdwIndexSize, DWORD dwKey, NetObj *pObject)
{
   DWORD dwFirst, dwLast, dwPos;
   INDEX *pIndex = *ppIndex;

   if (pIndex == NULL)
   {
      dwPos = 0;
   }
   else
   {
      dwFirst = 0;
      dwLast = *pdwIndexSize - 1;

      if (dwKey < pIndex[0].dwKey)
         dwPos = 0;
      else if (dwKey > pIndex[dwLast].dwKey)
         dwPos = dwLast + 1;
      else
         while(dwFirst < dwLast)
         {
            dwPos = (dwFirst + dwLast) / 2;
            if (dwPos == 0)
               dwPos++;
            if ((pIndex[dwPos - 1].dwKey < dwKey) && (pIndex[dwPos].dwKey > dwKey))
               break;
            if ((pIndex[dwPos].dwKey < dwKey) && (pIndex[dwPos + 1].dwKey > dwKey))
            {
               dwPos++;
               break;
            }
            if (dwKey < pIndex[dwPos].dwKey)
               dwLast = dwPos;
            else if (dwKey > pIndex[dwPos].dwKey)
               dwFirst = dwPos;
            else
            {
               WriteLog(MSG_DUPLICATE_KEY, EVENTLOG_ERROR_TYPE, "x", dwKey);
               return;
            }
         }
   }

   (*pdwIndexSize)++;
   *ppIndex = (INDEX *)realloc(*ppIndex, sizeof(INDEX) * (*pdwIndexSize));
   pIndex = *ppIndex;
   if (dwPos < (*pdwIndexSize - 1))
      memmove(&pIndex[dwPos + 1], &pIndex[dwPos], sizeof(INDEX) * (*pdwIndexSize - dwPos - 1));
   pIndex[dwPos].dwKey = dwKey;
   pIndex[dwPos].pObject = pObject;
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

void NetObjInsert(NetObj *pObject)
{
   ObjectsGlobalLock();
   pObject->SetId(dwFreeObjectId++);
   AddObjectToIndex(&g_pIndexById, &g_dwIdIndexSize, pObject->Id(), pObject);
   switch(pObject->Type())
   {
      case OBJECT_GENERIC:
         break;
      case OBJECT_SUBNET:
         AddObjectToIndex(&g_pSubnetIndexByAddr, &g_dwSubnetAddrIndexSize, pObject->IpAddr(), pObject);
         break;
      case OBJECT_NODE:
         AddObjectToIndex(&g_pNodeIndexByAddr, &g_dwNodeAddrIndexSize, pObject->IpAddr(), pObject);
         break;
      case OBJECT_INTERFACE:
         AddObjectToIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, pObject->IpAddr(), pObject);
         break;
      default:
         WriteLog(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
         break;
   }
   ObjectsGlobalUnlock();
}


//
// Delete object
//

void NetObjDelete(NetObj *pObject)
{
   ObjectsGlobalLock();
   pObject->Delete();
   DeleteObjectFromIndex(&g_pIndexById, &g_dwIdIndexSize, pObject->Id());
   switch(pObject->Type())
   {
      case OBJECT_GENERIC:
         break;
      case OBJECT_SUBNET:
         DeleteObjectFromIndex(&g_pSubnetIndexByAddr, &g_dwSubnetAddrIndexSize, pObject->IpAddr());
         break;
      case OBJECT_NODE:
         DeleteObjectFromIndex(&g_pNodeIndexByAddr, &g_dwNodeAddrIndexSize, pObject->IpAddr());
         break;
      case OBJECT_INTERFACE:
         DeleteObjectFromIndex(&g_pInterfaceIndexByAddr, &g_dwInterfaceAddrIndexSize, pObject->IpAddr());
         break;
      default:
         WriteLog(MSG_BAD_NETOBJ_TYPE, EVENTLOG_ERROR_TYPE, "d", pObject->Type());
         break;
   }
   ObjectsGlobalUnlock();
}


//
// Find node by IP address
//

Node *FindNodeByIP(DWORD dwAddr)
{
   DWORD dwPos;

   if (g_pInterfaceIndexByAddr == NULL)
      return NULL;

   dwPos = SearchIndex(g_pInterfaceIndexByAddr, g_dwInterfaceAddrIndexSize, dwAddr);
   return (dwPos == INVALID_INDEX) ? NULL : (Node *)g_pInterfaceIndexByAddr[dwPos].pObject->GetParent();
}


//
// Find subnet by IP address
//

Subnet *FindSubnetByIP(DWORD dwAddr)
{
   DWORD dwPos;

   if (g_pSubnetIndexByAddr == NULL)
      return NULL;

   dwPos = SearchIndex(g_pSubnetIndexByAddr, g_dwSubnetAddrIndexSize, dwAddr);
   return (dwPos == INVALID_INDEX) ? NULL : (Subnet *)g_pSubnetIndexByAddr[dwPos].pObject;
}
