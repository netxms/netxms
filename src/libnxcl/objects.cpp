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
   m_pIndexById = (INDEX *)realloc(m_pIndexById, sizeof(INDEX) * (m_dwNumObjects + 1));
   m_pIndexById[m_dwNumObjects].dwKey = pObject->dwId;
   m_pIndexById[m_dwNumObjects].pObject = pObject;
   m_dwNumObjects++;
   if (bSortIndex)
      qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
}


//
// Create new object from message
//

static NXC_OBJECT *NewObjectFromMsg(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject;
   char *pStr, szBuffer[32];
   DWORD i;

   // Allocate memory for new object structure
   pObject = (NXC_OBJECT *)malloc(sizeof(NXC_OBJECT));
   memset(pObject, 0, sizeof(NXC_OBJECT));

   // Common attributes
   pObject->dwId = pMsg->GetVariableLong("id");
   pObject->iClass = pMsg->GetVariableShort("class");
   pStr = pMsg->GetVariableStr("name");
   strcpy(pObject->szName, pStr);
   LibUtilDestroyObject(pStr);
   pObject->iStatus = pMsg->GetVariableShort("status");
   pObject->dwIpAddr = pMsg->GetVariableLong("ipaddr");

   // Parents
   pObject->dwNumParents = pMsg->GetVariableLong("parent_cnt");
   pObject->pdwParentList = (DWORD *)malloc(sizeof(DWORD) * pObject->dwNumParents);
   for(i = 0; i < pObject->dwNumParents; i++)
   {
      sprintf(szBuffer, "parent.%d", i);
      pObject->pdwParentList[i] = pMsg->GetVariableLong(szBuffer);
   }

   // Class-specific attributes
   switch(pObject->iClass)
   {
      case OBJECT_INTERFACE:
         pObject->iface.dwIpNetMask = pMsg->GetVariableLong("netmask");
         pObject->iface.dwIfIndex = pMsg->GetVariableLong("if_index");
         pObject->iface.dwIfType = pMsg->GetVariableLong("if_type");
         break;
      case OBJECT_NODE:
         pObject->node.dwFlags = pMsg->GetVariableLong("flags");
         pObject->node.dwDiscoveryFlags = pMsg->GetVariableLong("dflags");
         pObject->node.wAgentPort = pMsg->GetVariableShort("agent_port");
         pObject->node.wAuthMethod = pMsg->GetVariableShort("auth_method");
         pStr = pMsg->GetVariableStr("shared_secret");
         strncpy(pObject->node.szSharedSecret, pStr, MAX_SECRET_LENGTH - 1);
         LibUtilDestroyObject(pStr);
         pStr = pMsg->GetVariableStr("community");
         strncpy(pObject->node.szCommunityString, pStr, MAX_COMMUNITY_LENGTH - 1);
         LibUtilDestroyObject(pStr);
         pStr = pMsg->GetVariableStr("oid");
         strncpy(pObject->node.szObjectId, pStr, MAX_OID_LENGTH - 1);
         LibUtilDestroyObject(pStr);
         break;
      case OBJECT_SUBNET:
         pObject->subnet.dwIpNetMask = pMsg->GetVariableLong("netmask");
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
   NXC_OBJECT *pObject;

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
         DebugPrintf("RECV_OBJECT: ID=%d Name=\"%s\" Class=%d", pMsg->GetVariableLong("id"),
                     pMsg->GetVariableStr("name"), pMsg->GetVariableShort("class"));
      
         // Create new object from message and add it to list
         pObject = NewObjectFromMsg(pMsg);
         AddObject(pObject, FALSE);
         break;
      case CMD_OBJECT_UPDATE:
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

NXC_OBJECT EXPORTABLE *NXCFindObjectById(DWORD dwId)
{
   DWORD dwPos;

   dwPos = SearchIndex(m_pIndexById, m_dwNumObjects, dwId);
   return dwPos == INVALID_INDEX ? NULL : m_pIndexById[dwPos].pObject;
}


//
// Enumerate all objects
//

void EXPORTABLE NXCEnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *))
{
   DWORD i;

   for(i = 0; i < m_dwNumObjects; i++)
      if (!pHandler(m_pIndexById[i].pObject))
         break;
}
