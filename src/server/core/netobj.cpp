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
** $module: netobj.cpp
**
**/

#include "nms_core.h"


//
// NetObj class constructor
//

NetObj::NetObj()
{
   m_dwRefCount = 0;
   m_hMutex = MutexCreate();
   m_iStatus = STATUS_UNKNOWN;
   m_szName[0] = 0;
   m_bIsModified = FALSE;
   m_bIsDeleted = FALSE;
   m_dwIpAddr = 0;
   m_dwChildCount = 0;
   m_pChildList = NULL;
   m_dwParentCount = 0;
   m_pParentList = NULL;
   m_pAccessList = new AccessList;
   m_bInheritAccessRights = TRUE;
}


//
// NetObj class destructor
//

NetObj::~NetObj()
{
   MutexDestroy(m_hMutex);
   if (m_pChildList != NULL)
      free(m_pChildList);
   if (m_pParentList != NULL)
      free(m_pParentList);
   delete m_pAccessList;
}


//
// Create object from database data
//

BOOL NetObj::CreateFromDB(DWORD dwId)
{
   return FALSE;     // Abstract objects cannot be loaded from database
}


//
// Save object to database
//

BOOL NetObj::SaveToDB(void)
{
   return FALSE;     // Abstract objects cannot be saved to database
}


//
// Delete object from database
//

BOOL NetObj::DeleteFromDB(void)
{
   return FALSE;     // Abstract objects cannot be deleted from database
}


//
// Add reference to the new child object
//

void NetObj::AddChild(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
      {
         Unlock();
         return;     // Already in the child list
      }
   m_pChildList = (NetObj **)realloc(m_pChildList, sizeof(NetObj *) * (m_dwChildCount + 1));
   m_pChildList[m_dwChildCount++] = pObject;
   Modify();
   Unlock();
}


//
// Add reference to parent object
//

void NetObj::AddParent(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
      {
         Unlock();
         return;     // Already in the parents list
      }
   m_pParentList = (NetObj **)realloc(m_pParentList, sizeof(NetObj *) * (m_dwParentCount + 1));
   m_pParentList[m_dwParentCount++] = pObject;
   Modify();
   Unlock();
}


//
// Delete reference to child object
//

void NetObj::DeleteChild(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
         break;

   if (i == m_dwChildCount)   // No such object
   {
      Unlock();
      return;
   }
   m_dwChildCount--;
   if (m_dwChildCount > 0)
   {
      memmove(&m_pChildList[i], &m_pChildList[i + 1], sizeof(NetObj *) * (m_dwChildCount - i));
      m_pChildList = (NetObj **)realloc(m_pChildList, sizeof(NetObj *) * m_dwChildCount);
   }
   else
   {
      free(m_pChildList);
      m_pChildList = NULL;
   }
   Modify();
   Unlock();
}


//
// Delete reference to parent object
//

void NetObj::DeleteParent(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
         break;
   if (i == m_dwParentCount)   // No such object
   {
      Unlock();
      return;
   }
   m_dwParentCount--;
   if (m_dwParentCount > 0)
   {
      memmove(&m_pParentList[i], &m_pParentList[i + 1], sizeof(NetObj *) * (m_dwParentCount - i));
      m_pParentList = (NetObj **)realloc(m_pParentList, sizeof(NetObj *) * m_dwParentCount);
   }
   else
   {
      free(m_pParentList);
      m_pParentList = NULL;
   }
   Modify();
   Unlock();
}


//
// Prepare object for deletion - remove all references, etc.
//

void NetObj::Delete(void)
{
   DWORD i;

   Lock();

   // Remove references to this object from parent objects
   for(i = 0; i < m_dwParentCount; i++)
      m_pParentList[i]->DeleteChild(this);
   free(m_pParentList);
   m_pParentList = NULL;
   m_dwParentCount = 0;

   // Delete references to this object from child objects
   for(i = 0; i < m_dwChildCount; i++)
   {
      m_pChildList[i]->DeleteParent(this);
      if (m_pChildList[i]->IsOrphaned())
         m_pChildList[i]->Delete();
   }
   free(m_pChildList);
   m_pChildList = NULL;
   m_dwChildCount = 0;

   NetObjDeleteFromIndexes(this);

   m_bIsDeleted = TRUE;
   Modify();
   Unlock();
}


//
// Print childs IDs
//

const char *NetObj::ChildList(char *szBuffer)
{
   DWORD i;
   char *pBuf = szBuffer;

   *pBuf = 0;
   Lock();
   for(i = 0, pBuf = szBuffer; i < m_dwChildCount; i++)
   {
      sprintf(pBuf, "%d ", m_pChildList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   Unlock();
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}


//
// Print parents IDs
//

const char *NetObj::ParentList(char *szBuffer)
{
   DWORD i;
   char *pBuf = szBuffer;

   *pBuf = 0;
   Lock();
   for(i = 0; i < m_dwParentCount; i++)
   {
      sprintf(pBuf, "%d ", m_pParentList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   Unlock();
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}


//
// Calculate status for compound object based on childs' status
//

void NetObj::CalculateCompoundStatus(void)
{
   DWORD i;
   int iSum, iCount, iOldStatus = m_iStatus;

   /* TODO: probably status calculation algorithm should be changed */
   for(i = 0, iSum = 0, iCount = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Status() < STATUS_UNKNOWN)
      {
         iSum += m_pChildList[i]->Status();
         iCount++;
      }
   if (iCount > 0)
      m_iStatus = iSum / iCount;
   else
      m_iStatus = STATUS_UNKNOWN;

   // Cause parent object(s) to recalculate it's status
   if (iOldStatus != m_iStatus)
   {
      for(i = 0; i < m_dwParentCount; i++)
         m_pParentList[i]->CalculateCompoundStatus();
      Modify();
   }
}


//
// Load ACL from database
//

BOOL NetObj::LoadACLFromDB(void)
{
   char szQuery[256];
   DB_RESULT hResult;
   BOOL bSuccess = FALSE;

   sprintf(szQuery, "SELECT user_id,access_rights FROM acl WHERE object_id=%d", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      int i, iNumRows;

      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
         m_pAccessList->AddElement(DBGetFieldULong(hResult, i, 0), 
                                   DBGetFieldULong(hResult, i, 1));
      DBFreeResult(hResult);
      bSuccess = TRUE;
   }

   return bSuccess;
}


//
// Handler for ACL elements enumeration
//

static void EnumerationHandler(DWORD dwUserId, DWORD dwAccessRights, void *pArg)
{
   char szQuery[256];

   sprintf(szQuery, "INSERT INTO acl (object_id,user_id,access_rights) VALUES (%d,%d,%d)",
           (DWORD)pArg, dwUserId, dwAccessRights);
   DBQuery(g_hCoreDB, szQuery);
}


//
// Save ACL to database
//

BOOL NetObj::SaveACLToDB(void)
{
   char szQuery[256];
   BOOL bSuccess = FALSE;

   sprintf(szQuery, "DELETE FROM acl WHERE object_id=%d", m_dwId);
   if (DBQuery(g_hCoreDB, szQuery))
   {
      m_pAccessList->EnumerateElements(EnumerationHandler, (void *)m_dwId);
      bSuccess = TRUE;
   }

   return bSuccess;
}


//
// Create CSCP message with object's data
//

void NetObj::CreateMessage(CSCPMessage *pMsg)
{
   DWORD i, dwId;

   pMsg->SetVariable(VID_OBJECT_CLASS, (WORD)Type());
   pMsg->SetVariable(VID_OBJECT_ID, m_dwId);
   pMsg->SetVariable(VID_OBJECT_NAME, m_szName);
   pMsg->SetVariable(VID_OBJECT_STATUS, (WORD)m_iStatus);
   pMsg->SetVariable(VID_IP_ADDRESS, m_dwIpAddr);
   pMsg->SetVariable(VID_PARENT_CNT, m_dwParentCount);
   pMsg->SetVariable(VID_CHILD_CNT, m_dwChildCount);
   pMsg->SetVariable(VID_IS_DELETED, (WORD)m_bIsDeleted);
   for(i = 0, dwId = VID_PARENT_ID_BASE; i < m_dwParentCount; i++, dwId++)
      pMsg->SetVariable(dwId, m_pParentList[i]->Id());
   for(i = 0, dwId = VID_CHILD_ID_BASE; i < m_dwChildCount; i++, dwId++)
      pMsg->SetVariable(dwId, m_pChildList[i]->Id());
   pMsg->SetVariable(VID_INHERIT_RIGHTS, (WORD)m_bInheritAccessRights);
   m_pAccessList->CreateMessage(pMsg);
}


//
// Handler for EnumerateSessions()
//

static void BroadcastObjectChange(ClientSession *pSession, void *pArg)
{
   if (pSession->GetState() == STATE_AUTHENTICATED)
      pSession->OnObjectChange((NetObj *)pArg);
}


//
// Mark object as modified and put on client's notification queue
//

void NetObj::Modify(void)
{
   m_bIsModified = TRUE;

   // Send event to all connected clients
   EnumerateClientSessions(BroadcastObjectChange, this);
}


//
// Modify object from message
//

DWORD NetObj::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      Lock();

   // Change object's name
   if (pRequest->IsVariableExist(VID_OBJECT_NAME))
      pRequest->GetVariableStr(VID_OBJECT_NAME, m_szName, MAX_OBJECT_NAME);

   // Change object's ACL
   if (pRequest->IsVariableExist(VID_ACL_SIZE))
   {
      DWORD i, dwNumElements;

      dwNumElements = pRequest->GetVariableLong(VID_ACL_SIZE);
      m_bInheritAccessRights = pRequest->GetVariableShort(VID_INHERIT_RIGHTS);
      m_pAccessList->DeleteAll();
      for(i = 0; i < dwNumElements; i++)
         m_pAccessList->AddElement(pRequest->GetVariableLong(VID_ACL_USER_BASE + i),
                                   pRequest->GetVariableLong(VID_ACL_RIGHTS_BASE +i));
   }

   Unlock();
   Modify();

   return RCC_SUCCESS;
}


//
// Get rights to object for specific user
//

DWORD NetObj::GetUserRights(DWORD dwUserId)
{
   DWORD dwRights;

   // Admin always has all rights to any object
   if (dwUserId == 0)
      return 0xFFFFFFFF;

   Lock();

   // Check if have direct right assignment
   if (!m_pAccessList->GetUserRights(dwUserId, &dwRights))
   {
      // We don't. If this object inherit rights from parents, get them
      if (m_bInheritAccessRights)
      {
         DWORD i;

         for(i = 0, dwRights = 0; i < m_dwParentCount; i++)
            dwRights |= m_pParentList[i]->GetUserRights(dwUserId);
      }
   }

   Unlock();
   return dwRights;
}


//
// Check if given user has specific rights on this object
//

BOOL NetObj::CheckAccessRights(DWORD dwUserId, DWORD dwRequiredRights)
{
   DWORD dwRights = GetUserRights(dwUserId);
   return (dwRights & dwRequiredRights) == dwRequiredRights;
}


//
// Drop all user privileges on current object
//

void NetObj::DropUserAccess(DWORD dwUserId)
{
   if (m_pAccessList->DeleteElement(dwUserId))
      Modify();
}
