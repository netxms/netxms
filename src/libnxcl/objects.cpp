/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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

#include "libnxcl.h"


//
// Static data
//

static CONDITION m_hCondSyncFinished;
static DWORD m_dwNumObjects = 0;
static INDEX *m_pIndexById = NULL;


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

static void AddObject(NXC_OBJECT *pObject, BOOL bSortIndex)
{
   DebugPrintf("AddObject(id:%ld, name:\"%s\")", pObject->dwId, pObject->szName);
   m_pIndexById = (INDEX *)MemReAlloc(m_pIndexById, sizeof(INDEX) * (m_dwNumObjects + 1));
   m_pIndexById[m_dwNumObjects].dwKey = pObject->dwId;
   m_pIndexById[m_dwNumObjects].pObject = pObject;
   m_dwNumObjects++;
   if (bSortIndex)
      qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
}


//
// Replace object's data in list
//

static void ReplaceObject(NXC_OBJECT *pObject, NXC_OBJECT *pNewObject)
{
   DebugPrintf("ReplaceObject(id:%ld, name:\"%s\")", pObject->dwId, pObject->szName);
   if (pObject->pdwChildList != NULL)
      MemFree(pObject->pdwChildList);
   if (pObject->pdwParentList != NULL)
      MemFree(pObject->pdwParentList);
   memcpy(pObject, pNewObject, sizeof(NXC_OBJECT));
   MemFree(pNewObject);
}


//
// Destroy object
//

static void DestroyObject(NXC_OBJECT *pObject)
{
   DebugPrintf("DestroyObject(id:%ld, name:\"%s\")", pObject->dwId, pObject->szName);
   if (pObject->pdwChildList != NULL)
      MemFree(pObject->pdwChildList);
   if (pObject->pdwParentList != NULL)
      MemFree(pObject->pdwParentList);
   MemFree(pObject);
}


//
// Create new object from message
//

static NXC_OBJECT *NewObjectFromMsg(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject;
   DWORD i, dwId;

   // Allocate memory for new object structure
   pObject = (NXC_OBJECT *)MemAlloc(sizeof(NXC_OBJECT));
   memset(pObject, 0, sizeof(NXC_OBJECT));

   // Common attributes
   pObject->dwId = pMsg->GetVariableLong(VID_OBJECT_ID);
   pObject->iClass = pMsg->GetVariableShort(VID_OBJECT_CLASS);
   pMsg->GetVariableStr(VID_OBJECT_NAME, pObject->szName, MAX_OBJECT_NAME);
   pObject->iStatus = pMsg->GetVariableShort(VID_OBJECT_STATUS);
   pObject->dwIpAddr = pMsg->GetVariableLong(VID_IP_ADDRESS);
   pObject->bIsDeleted = pMsg->GetVariableShort(VID_IS_DELETED);

   // Parents
   pObject->dwNumParents = pMsg->GetVariableLong(VID_PARENT_CNT);
   pObject->pdwParentList = (DWORD *)MemAlloc(sizeof(DWORD) * pObject->dwNumParents);
   for(i = 0, dwId = VID_PARENT_ID_BASE; i < pObject->dwNumParents; i++, dwId++)
      pObject->pdwParentList[i] = pMsg->GetVariableLong(dwId);

   // Childs
   pObject->dwNumChilds = pMsg->GetVariableLong(VID_CHILD_CNT);
   pObject->pdwChildList = (DWORD *)MemAlloc(sizeof(DWORD) * pObject->dwNumChilds);
   for(i = 0, dwId = VID_CHILD_ID_BASE; i < pObject->dwNumChilds; i++, dwId++)
      pObject->pdwChildList[i] = pMsg->GetVariableLong(dwId);

   // Class-specific attributes
   switch(pObject->iClass)
   {
      case OBJECT_INTERFACE:
         pObject->iface.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         pObject->iface.dwIfIndex = pMsg->GetVariableLong(VID_IF_INDEX);
         pObject->iface.dwIfType = pMsg->GetVariableLong(VID_IF_TYPE);
         break;
      case OBJECT_NODE:
         pObject->node.dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pObject->node.dwDiscoveryFlags = pMsg->GetVariableLong(VID_DISCOVERY_FLAGS);
         pObject->node.wAgentPort = pMsg->GetVariableShort(VID_AGENT_PORT);
         pObject->node.wAuthMethod = pMsg->GetVariableShort(VID_AUTH_METHOD);
         pMsg->GetVariableStr(VID_SHARED_SECRET, pObject->node.szSharedSecret, MAX_SECRET_LENGTH);
         pMsg->GetVariableStr(VID_COMMUNITY_STRING, pObject->node.szCommunityString, MAX_COMMUNITY_LENGTH);
         pMsg->GetVariableStr(VID_SNMP_OID, pObject->node.szObjectId, MAX_OID_LENGTH);
         break;
      case OBJECT_SUBNET:
         pObject->subnet.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         break;
      default:
         break;
   }

   return pObject;
}


//
// Process object information received from server
//

void ProcessObjectUpdate(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject, *pNewObject;

   switch(pMsg->GetCode())
   {
      case CMD_OBJECT_LIST_END:
         if (g_dwState == STATE_SYNC_OBJECTS)
         {
            qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
            ConditionSet(m_hCondSyncFinished);
         }
         break;
      case CMD_OBJECT:
         DebugPrintf("RECV_OBJECT: ID=%d Name=\"%s\" Class=%d", pMsg->GetVariableLong(VID_OBJECT_ID),
                     pMsg->GetVariableStr(VID_OBJECT_NAME), pMsg->GetVariableShort(VID_OBJECT_CLASS));
      
         // Create new object from message and add it to list
         pObject = NewObjectFromMsg(pMsg);
         AddObject(pObject, FALSE);
         break;
      case CMD_OBJECT_UPDATE:
         pNewObject = NewObjectFromMsg(pMsg);
         pObject = NXCFindObjectById(pNewObject->dwId);
         if (pObject == NULL)
         {
            AddObject(pNewObject, TRUE);
            pObject = pNewObject;
         }
         else
         {
            ReplaceObject(pObject, pNewObject);
         }
         CallEventHandler(NXC_EVENT_OBJECT_CHANGED, pObject->dwId, pObject);
         break;
      default:
         break;
   }
}


//
// Synchronize objects with the server
//

void SyncObjects(void)
{
   CSCPMessage msg;

   ChangeState(STATE_SYNC_OBJECTS);
   m_hCondSyncFinished = ConditionCreate();

   msg.SetCode(CMD_GET_OBJECTS);
   msg.SetId(0);
   SendMsg(&msg);

   // Wait for object list end or for disconnection
   while(g_dwState != STATE_DISCONNECTED)
   {
      if (ConditionWait(m_hCondSyncFinished, 500))
         break;
   }

   ConditionDestroy(m_hCondSyncFinished);
   if (g_dwState != STATE_DISCONNECTED)
      ChangeState(STATE_IDLE);
}


//
// Find object by ID
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectById(DWORD dwId)
{
   DWORD dwPos;

   dwPos = SearchIndex(m_pIndexById, m_dwNumObjects, dwId);
   return dwPos == INVALID_INDEX ? NULL : m_pIndexById[dwPos].pObject;
}


//
// Enumerate all objects
//

void LIBNXCL_EXPORTABLE NXCEnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *))
{
   DWORD i;

   for(i = 0; i < m_dwNumObjects; i++)
      if (!pHandler(m_pIndexById[i].pObject))
         break;
}


//
// Get root ("Entire Network") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetRootObject(void)
{
   if (m_dwNumObjects > 0)
      if (m_pIndexById[0].dwKey == 1)
         return m_pIndexById[0].pObject;
   return NULL;
}
