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

static DWORD m_dwNumObjects = 0;
static INDEX *m_pIndexById = NULL;
static MUTEX m_mutexIndexAccess;


//
// Initialize object handling module
//

void ObjectsInit(void)
{
   m_mutexIndexAccess = MutexCreate();
}


//
// Destroy object
//

static void DestroyObject(NXC_OBJECT *pObject)
{
   DebugPrintf(_T("DestroyObject(id:%ld, name:\"%s\")"), pObject->dwId, pObject->szName);
   switch(pObject->iClass)
   {
      case OBJECT_CONTAINER:
         safe_free(pObject->container.pszDescription);
         break;
      case OBJECT_NODE:
         safe_free(pObject->node.pszDescription);
         break;
      case OBJECT_TEMPLATE:
         safe_free(pObject->dct.pszDescription);
         break;
   }
   safe_free(pObject->pdwChildList);
   safe_free(pObject->pdwParentList);
   safe_free(pObject->pAccessList);
   free(pObject);
}


//
// Destroy all objects
//

void DestroyAllObjects(void)
{
   DWORD i;

   MutexLock(m_mutexIndexAccess, INFINITE);
   for(i = 0; i < m_dwNumObjects; i++)
      DestroyObject(m_pIndexById[i].pObject);
   m_dwNumObjects = 0;
   free(m_pIndexById);
   m_pIndexById = NULL;
   MutexUnlock(m_mutexIndexAccess);
}


//
// Perform binary search on index
// Returns INVALID_INDEX if key not found or position of appropriate network object
// We assume that pIndex == NULL will not be passed
//

static DWORD SearchIndex(INDEX *pIndex, DWORD dwIndexSize, DWORD dwKey)
{
   DWORD dwFirst, dwLast, dwMid;

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

static void AddObject(NXC_OBJECT *pObject, BOOL bSortIndex)
{
   DebugPrintf(_T("AddObject(id:%ld, name:\"%s\")"), pObject->dwId, pObject->szName);
   NXCLockObjectIndex();
   m_pIndexById = (INDEX *)realloc(m_pIndexById, sizeof(INDEX) * (m_dwNumObjects + 1));
   m_pIndexById[m_dwNumObjects].dwKey = pObject->dwId;
   m_pIndexById[m_dwNumObjects].pObject = pObject;
   m_dwNumObjects++;
   if (bSortIndex)
      qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
   NXCUnlockObjectIndex();
}


//
// Replace object's data in list
//

static void ReplaceObject(NXC_OBJECT *pObject, NXC_OBJECT *pNewObject)
{
   DebugPrintf(_T("ReplaceObject(id:%ld, name:\"%s\")"), pObject->dwId, pObject->szName);
   if (pObject->iClass == OBJECT_CONTAINER)
      safe_free(pObject->container.pszDescription);
   safe_free(pObject->pdwChildList);
   safe_free(pObject->pdwParentList);
   safe_free(pObject->pAccessList);
   memcpy(pObject, pNewObject, sizeof(NXC_OBJECT));
   free(pNewObject);
}


//
// Create new object from message
//

static NXC_OBJECT *NewObjectFromMsg(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject;
   DWORD i, dwId1, dwId2;

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
   pObject->dwImage = pMsg->GetVariableLong(VID_IMAGE_ID);

   // Parents
   pObject->dwNumParents = pMsg->GetVariableLong(VID_PARENT_CNT);
   pObject->pdwParentList = (DWORD *)malloc(sizeof(DWORD) * pObject->dwNumParents);
   for(i = 0, dwId1 = VID_PARENT_ID_BASE; i < pObject->dwNumParents; i++, dwId1++)
      pObject->pdwParentList[i] = pMsg->GetVariableLong(dwId1);

   // Childs
   pObject->dwNumChilds = pMsg->GetVariableLong(VID_CHILD_CNT);
   pObject->pdwChildList = (DWORD *)malloc(sizeof(DWORD) * pObject->dwNumChilds);
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
         pObject->iface.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         pObject->iface.dwIfIndex = pMsg->GetVariableLong(VID_IF_INDEX);
         pObject->iface.dwIfType = pMsg->GetVariableLong(VID_IF_TYPE);
         pMsg->GetVariableBinary(VID_MAC_ADDR, pObject->iface.bMacAddr, MAC_ADDR_LENGTH);
         break;
      case OBJECT_NODE:
         pObject->node.dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pObject->node.dwDiscoveryFlags = pMsg->GetVariableLong(VID_DISCOVERY_FLAGS);
         pObject->node.dwNodeType = pMsg->GetVariableLong(VID_NODE_TYPE);
         pObject->node.wAgentPort = pMsg->GetVariableShort(VID_AGENT_PORT);
         pObject->node.wAuthMethod = pMsg->GetVariableShort(VID_AUTH_METHOD);
         pMsg->GetVariableStr(VID_SHARED_SECRET, pObject->node.szSharedSecret, MAX_SECRET_LENGTH);
         pMsg->GetVariableStr(VID_COMMUNITY_STRING, pObject->node.szCommunityString, MAX_COMMUNITY_LENGTH);
         pMsg->GetVariableStr(VID_SNMP_OID, pObject->node.szObjectId, MAX_OID_LENGTH);
         pObject->node.pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION);
         break;
      case OBJECT_SUBNET:
         pObject->subnet.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         break;
      case OBJECT_CONTAINER:
         pObject->container.dwCategory = pMsg->GetVariableLong(VID_CATEGORY);
         pObject->container.pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION);
         break;
      case OBJECT_TEMPLATE:
         pObject->dct.dwVersion = pMsg->GetVariableLong(VID_TEMPLATE_VERSION);
         pObject->dct.pszDescription = pMsg->GetVariableStr(VID_DESCRIPTION);
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
         NXCLockObjectIndex();
         qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
         NXCUnlockObjectIndex();
         CompleteSync(RCC_SUCCESS);
         break;
      case CMD_OBJECT:
         DebugPrintf(_T("RECV_OBJECT: ID=%d Name=\"%s\" Class=%d"), pMsg->GetVariableLong(VID_OBJECT_ID),
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
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCSyncObjects(void)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;
   PrepareForSync();

   DestroyAllObjects();

   msg.SetCode(CMD_GET_OBJECTS);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   // If request was successful, wait for object list end or for disconnection
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = WaitForSync(INFINITE);

   return dwRetCode;
}


//
// Find object by ID
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectById(DWORD dwId)
{
   DWORD dwPos;
   NXC_OBJECT *pObject;

   NXCLockObjectIndex();
   dwPos = SearchIndex(m_pIndexById, m_dwNumObjects, dwId);
   pObject = (dwPos == INVALID_INDEX) ? NULL : m_pIndexById[dwPos].pObject;
   NXCUnlockObjectIndex();
   return pObject;
}


//
// Enumerate all objects
//

void LIBNXCL_EXPORTABLE NXCEnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *))
{
   DWORD i;

   NXCLockObjectIndex();
   for(i = 0; i < m_dwNumObjects; i++)
      if (!pHandler(m_pIndexById[i].pObject))
         break;
   NXCUnlockObjectIndex();
}


//
// Get topology root ("Entire Network") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTopologyRootObject(void)
{
   if (m_dwNumObjects > 0)
      if (m_pIndexById[0].dwKey == 1)
         return m_pIndexById[0].pObject;
   return NULL;
}


//
// Get service tree root ("All Services") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetServiceRootObject(void)
{
   if (m_dwNumObjects > 1)
      if (m_pIndexById[1].dwKey == 2)
         return m_pIndexById[1].pObject;
   return NULL;
}


//
// Get template tree root ("Templates") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTemplateRootObject(void)
{
   if (m_dwNumObjects > 2)
      if (m_pIndexById[2].dwKey == 3)
         return m_pIndexById[2].pObject;
   return NULL;
}


//
// Get pointer to first object on objects' list and entire number of objects
//

void LIBNXCL_EXPORTABLE *NXCGetObjectIndex(DWORD *pdwNumObjects)
{
   if (pdwNumObjects != NULL)
      *pdwNumObjects = m_dwNumObjects;
   return m_pIndexById;
}


//
// Lock object index
//

void LIBNXCL_EXPORTABLE NXCLockObjectIndex(void)
{
   MutexLock(m_mutexIndexAccess, INFINITE);
}


//
// Unlock object index
//

void LIBNXCL_EXPORTABLE NXCUnlockObjectIndex(void)
{
   MutexUnlock(m_mutexIndexAccess);
}


//
// Find object by name
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByName(TCHAR *pszName)
{
   NXC_OBJECT *pObject = NULL;
   DWORD i;

   if (pszName != NULL)
      if (*pszName != 0)
      {
         NXCLockObjectIndex();

         for(i = 0; i < m_dwNumObjects; i++)
            if (MatchString(pszName, m_pIndexById[i].pObject->szName, FALSE))
            {
               pObject = m_pIndexById[i].pObject;
               break;
            }

         NXCUnlockObjectIndex();
      }
   return pObject;
}


//
// Modify object
//

DWORD LIBNXCL_EXPORTABLE NXCModifyObject(NXC_OBJECT_UPDATE *pUpdate)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   // Build request message
   msg.SetCode(CMD_MODIFY_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, pUpdate->dwObjectId);
   if (pUpdate->dwFlags & OBJ_UPDATE_NAME)
      msg.SetVariable(VID_OBJECT_NAME, pUpdate->pszName);
   if (pUpdate->dwFlags & OBJ_UPDATE_AGENT_PORT)
      msg.SetVariable(VID_AGENT_PORT, (WORD)pUpdate->iAgentPort);
   if (pUpdate->dwFlags & OBJ_UPDATE_AGENT_AUTH)
      msg.SetVariable(VID_AUTH_METHOD, (WORD)pUpdate->iAuthType);
   if (pUpdate->dwFlags & OBJ_UPDATE_AGENT_SECRET)
      msg.SetVariable(VID_SHARED_SECRET, pUpdate->pszSecret);
   if (pUpdate->dwFlags & OBJ_UPDATE_SNMP_COMMUNITY)
      msg.SetVariable(VID_COMMUNITY_STRING, pUpdate->pszCommunity);
   if (pUpdate->dwFlags & OBJ_UPDATE_IMAGE)
      msg.SetVariable(VID_IMAGE_ID, pUpdate->dwImage);
   if (pUpdate->dwFlags & OBJ_UPDATE_DESCRIPTION)
      msg.SetVariable(VID_DESCRIPTION, pUpdate->pszDescription);
   if (pUpdate->dwFlags & OBJ_UPDATE_ACL)
   {
      DWORD i, dwId1, dwId2;

      msg.SetVariable(VID_ACL_SIZE, pUpdate->dwAclSize);
      msg.SetVariable(VID_INHERIT_RIGHTS, (WORD)pUpdate->bInheritRights);
      for(i = 0, dwId1 = VID_ACL_USER_BASE, dwId2 = VID_ACL_RIGHTS_BASE;
          i < pUpdate->dwAclSize; i++, dwId1++, dwId2++)
      {
         msg.SetVariable(dwId1, pUpdate->pAccessList[i].dwUserId);
         msg.SetVariable(dwId2, pUpdate->pAccessList[i].dwAccessRights);
      }
   }

   // Send request
   SendMsg(&msg);

   // Wait for reply
   return WaitForRCC(dwRqId);
}


//
// Set object's mamagement status
//

DWORD LIBNXCL_EXPORTABLE NXCSetObjectMgmtStatus(DWORD dwObjectId, BOOL bIsManaged)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   // Build request message
   msg.SetCode(CMD_SET_OBJECT_MGMT_STATUS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_MGMT_STATUS, (WORD)bIsManaged);

   // Send request
   SendMsg(&msg);

   // Wait for reply
   return WaitForRCC(dwRqId);
}


//
// Create new object
//

DWORD LIBNXCL_EXPORTABLE NXCCreateObject(NXC_OBJECT_CREATE_INFO *pCreateInfo, DWORD *pdwObjectId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwRetCode;

   dwRqId = g_dwMsgId++;

   // Build request message
   msg.SetCode(CMD_CREATE_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARENT_ID, pCreateInfo->dwParentId);
   msg.SetVariable(VID_OBJECT_CLASS, (WORD)pCreateInfo->iClass);
   msg.SetVariable(VID_OBJECT_NAME, pCreateInfo->pszName);
   switch(pCreateInfo->iClass)
   {
      case OBJECT_NODE:
         msg.SetVariable(VID_IP_ADDRESS, pCreateInfo->cs.node.dwIpAddr);
         msg.SetVariable(VID_IP_NETMASK, pCreateInfo->cs.node.dwNetMask);
         break;
      case OBJECT_CONTAINER:
         msg.SetVariable(VID_CATEGORY, pCreateInfo->cs.container.dwCategory);
         msg.SetVariable(VID_DESCRIPTION, pCreateInfo->cs.container.pszDescription);
         break;
      case OBJECT_TEMPLATEGROUP:
         msg.SetVariable(VID_DESCRIPTION, pCreateInfo->cs.templateGroup.pszDescription);
         break;
      default:
         break;
   }

   // Send request
   SendMsg(&msg);

   // Wait for responce. Creating node object can include polling,
   // which can take a minute or even more in worst cases
   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 120000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwObjectId = pResponce->GetVariableLong(VID_OBJECT_ID);
      }
      delete pResponce;
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

static DWORD ChangeObjectBinding(DWORD dwParentObject, DWORD dwChildObject, BOOL bBind)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   // Build request message
   msg.SetCode(bBind ? CMD_BIND_OBJECT : CMD_UNBIND_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARENT_ID, dwParentObject);
   msg.SetVariable(VID_CHILD_ID, dwChildObject);

   // Send request
   SendMsg(&msg);

   // Wait for reply
   return WaitForRCC(dwRqId);
}


//
// Bind object
//

DWORD LIBNXCL_EXPORTABLE NXCBindObject(DWORD dwParentObject, DWORD dwChildObject)
{
   return ChangeObjectBinding(dwParentObject, dwChildObject, TRUE);
}


//
// Unbind object
//

DWORD LIBNXCL_EXPORTABLE NXCUnbindObject(DWORD dwParentObject, DWORD dwChildObject)
{
   return ChangeObjectBinding(dwParentObject, dwChildObject, FALSE);
}


//
// Delete object
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteObject(DWORD dwObject)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   // Build request message
   msg.SetCode(CMD_DELETE_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObject);

   // Send request
   SendMsg(&msg);

   // Wait for reply
   return WaitForRCC(dwRqId);
}


//
// Load container categories
//

DWORD LIBNXCL_EXPORTABLE NXCLoadCCList(NXC_CC_LIST **ppList)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwRetCode = RCC_SUCCESS, dwNumCats = 0, dwCatId = 0;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_CONTAINER_CAT_LIST);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   *ppList = (NXC_CC_LIST *)malloc(sizeof(NXC_CC_LIST));
   (*ppList)->dwNumElements = 0;
   (*ppList)->pElements = NULL;

   do
   {
      pResponce = WaitForMessage(CMD_CONTAINER_CAT_DATA, dwRqId, g_dwCommandTimeout);
      if (pResponce != NULL)
      {
         dwCatId = pResponce->GetVariableLong(VID_CATEGORY_ID);
         if (dwCatId != 0)  // 0 is end of list indicator
         {
            (*ppList)->pElements = (NXC_CONTAINER_CATEGORY *)realloc((*ppList)->pElements, 
               sizeof(NXC_CONTAINER_CATEGORY) * ((*ppList)->dwNumElements + 1));
            (*ppList)->pElements[(*ppList)->dwNumElements].dwId = dwCatId;
            (*ppList)->pElements[(*ppList)->dwNumElements].dwImageId =
               pResponce->GetVariableLong(VID_IMAGE_ID);
            pResponce->GetVariableStr(VID_CATEGORY_NAME, 
               (*ppList)->pElements[(*ppList)->dwNumElements].szName, MAX_OBJECT_NAME);
            (*ppList)->pElements[(*ppList)->dwNumElements].pszDescription =
               pResponce->GetVariableStr(VID_DESCRIPTION);
            (*ppList)->dwNumElements++;
         }
         delete pResponce;
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
   DWORD i;

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

DWORD LIBNXCL_EXPORTABLE NXCPollNode(DWORD dwObjectId, int iPollType, 
                                     void (* pfCallback)(TCHAR *, void *), void *pArg)
{
   DWORD dwRetCode, dwRqId;
   CSCPMessage msg, *pResponce;
   TCHAR *pszMsg;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_POLL_NODE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_POLL_TYPE, (WORD)iPollType);
   SendMsg(&msg);

   do
   {
      // Polls can take a long time, so we wait up to 120 seconds for each message
      pResponce = WaitForMessage(CMD_POLLING_INFO, dwRqId, 120000);
      if (pResponce != NULL)
      {
         dwRetCode = pResponce->GetVariableLong(VID_RCC);
         if ((dwRetCode == RCC_OPERATION_IN_PROGRESS) && (pfCallback != NULL))
         {
            pszMsg = pResponce->GetVariableStr(VID_POLLER_MESSAGE);
            pfCallback(pszMsg, pArg);
            free(pszMsg);
         }
         delete pResponce;
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

DWORD LIBNXCL_EXPORTABLE NXCWakeUpNode(DWORD dwObjectId)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_WAKEUP_NODE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   SendMsg(&msg);
   return WaitForRCC(dwRqId);
}
