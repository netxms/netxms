/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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

#include "nxcore.h"


//
// NetObj class constructor
//

NetObj::NetObj()
{
   m_dwRefCount = 0;
   m_mutexData = MutexCreate();
   m_mutexRefCount = MutexCreate();
   m_mutexACL = MutexCreate();
   m_rwlockParentList = RWLockCreate();
   m_rwlockChildList = RWLockCreate();
   m_iStatus = STATUS_UNKNOWN;
   m_szName[0] = 0;
   m_bIsModified = FALSE;
   m_bIsDeleted = FALSE;
   m_bIsHidden = FALSE;
   m_dwIpAddr = 0;
   m_dwChildCount = 0;
   m_pChildList = NULL;
   m_dwParentCount = 0;
   m_pParentList = NULL;
   m_pAccessList = new AccessList;
   m_bInheritAccessRights = TRUE;
   m_dwImageId = IMG_DEFAULT;    // Default image
   m_pPollRequestor = NULL;
   m_iStatusAlgorithm = SA_DEFAULT;
}


//
// NetObj class destructor
//

NetObj::~NetObj()
{
   MutexDestroy(m_mutexData);
   MutexDestroy(m_mutexRefCount);
   MutexDestroy(m_mutexACL);
   RWLockDestroy(m_rwlockParentList);
   RWLockDestroy(m_rwlockChildList);
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

BOOL NetObj::SaveToDB(DB_HANDLE hdb)
{
   return FALSE;     // Abstract objects cannot be saved to database
}


//
// Delete object from database
//

BOOL NetObj::DeleteFromDB(void)
{
   char szQuery[256];

   // Delete ACL
   sprintf(szQuery, "DELETE FROM acl WHERE object_id=%d", m_dwId);
   QueueSQLRequest(szQuery);
   sprintf(szQuery, "DELETE FROM object_properties WHERE object_id=%d", m_dwId);
   QueueSQLRequest(szQuery);
   return TRUE;
}


//
// Load common object properties from database
//

BOOL NetObj::LoadCommonProperties(void)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bResult = FALSE;

   // Load access options
   _sntprintf(szQuery, 256, _T("SELECT name,status,is_deleted,image_id,"
                               "inherit_access_rights,last_modified,status_alg "
                               "FROM object_properties WHERE object_id=%ld"), m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         _tcsncpy(m_szName, DBGetField(hResult, 0, 0), MAX_OBJECT_NAME);
         m_iStatus = DBGetFieldLong(hResult, 0, 1);
         m_bIsDeleted = DBGetFieldLong(hResult, 0, 2) ? TRUE : FALSE;
         m_dwImageId = DBGetFieldULong(hResult, 0, 3);
         m_bInheritAccessRights = DBGetFieldLong(hResult, 0, 4) ? TRUE : FALSE;
         m_dwTimeStamp = DBGetFieldULong(hResult, 0, 5);
         m_iStatusAlgorithm = DBGetFieldLong(hResult, 0, 6);
         bResult = TRUE;
      }
      DBFreeResult(hResult);
   }
   return bResult;
}


//
// Save common object properties to database
//

BOOL NetObj::SaveCommonProperties(DB_HANDLE hdb)
{
   TCHAR szQuery[512];
   DB_RESULT hResult;
   BOOL bResult = FALSE;

   // Save access options
   _sntprintf(szQuery, 512, _T("SELECT object_id FROM object_properties WHERE object_id=%ld"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         _sntprintf(szQuery, 512, 
                    _T("UPDATE object_properties SET name='%s',status=%d,"
                       "is_deleted=%d,image_id=%ld,inherit_access_rights=%d,"
                       "last_modified=%ld,status_alg=%d WHERE object_id=%ld"),
                    m_szName, m_iStatus, m_bIsDeleted, m_dwImageId,
                    m_bInheritAccessRights, m_dwTimeStamp, m_iStatusAlgorithm, m_dwId);
      else
         _sntprintf(szQuery, 512, 
                    _T("INSERT INTO object_properties (object_id,name,status,is_deleted,"
                       "image_id,inherit_access_rights,last_modified,status_alg) "
                       "VALUES (%ld,'%s',%d,%d,%ld,%d,%ld,%d)"),
                    m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwImageId,
                    m_bInheritAccessRights, m_dwTimeStamp, m_iStatusAlgorithm);
      DBFreeResult(hResult);
      bResult = DBQuery(hdb, szQuery);
   }
   return bResult;
}


//
// Add reference to the new child object
//

void NetObj::AddChild(NetObj *pObject)
{
   DWORD i;

   LockChildList(TRUE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
      {
         UnlockChildList();
         return;     // Already in the child list
      }
   m_pChildList = (NetObj **)realloc(m_pChildList, sizeof(NetObj *) * (m_dwChildCount + 1));
   m_pChildList[m_dwChildCount++] = pObject;
   UnlockChildList();
   Modify();
}


//
// Add reference to parent object
//

void NetObj::AddParent(NetObj *pObject)
{
   DWORD i;

   LockParentList(TRUE);
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
      {
         UnlockParentList();
         return;     // Already in the parents list
      }
   m_pParentList = (NetObj **)realloc(m_pParentList, sizeof(NetObj *) * (m_dwParentCount + 1));
   m_pParentList[m_dwParentCount++] = pObject;
   UnlockParentList();
   Modify();
}


//
// Delete reference to child object
//

void NetObj::DeleteChild(NetObj *pObject)
{
   DWORD i;

   LockChildList(TRUE);
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
         break;

   if (i == m_dwChildCount)   // No such object
   {
      UnlockChildList();
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
   UnlockChildList();
   Modify();
}


//
// Delete reference to parent object
//

void NetObj::DeleteParent(NetObj *pObject)
{
   DWORD i;

   LockParentList(TRUE);
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
         break;
   if (i == m_dwParentCount)   // No such object
   {
      UnlockParentList();
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
   UnlockParentList();
   Modify();
}


//
// Prepare object for deletion - remove all references, etc.
// bIndexLocked should be TRUE if object index by ID is already locked
// by current thread
//

void NetObj::Delete(BOOL bIndexLocked)
{
   DWORD i;

   DbgPrintf(AF_DEBUG_OBJECTS, "Deleting object %d [%s]", m_dwId, m_szName);

   LockData();

   // Remove references to this object from parent objects
   DbgPrintf(AF_DEBUG_OBJECTS, "NetObj::Delete(): clearing parent list for object %d", m_dwId);
   LockParentList(TRUE);
   for(i = 0; i < m_dwParentCount; i++)
   {
      m_pParentList[i]->DeleteChild(this);
      m_pParentList[i]->CalculateCompoundStatus();
   }
   free(m_pParentList);
   m_pParentList = NULL;
   m_dwParentCount = 0;
   UnlockParentList();

   // Delete references to this object from child objects
   DbgPrintf(AF_DEBUG_OBJECTS, "NetObj::Delete(): clearing child list for object %d", m_dwId);
   LockChildList(TRUE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      m_pChildList[i]->DeleteParent(this);
      if (m_pChildList[i]->IsOrphaned())
         m_pChildList[i]->Delete(bIndexLocked);
   }
   free(m_pChildList);
   m_pChildList = NULL;
   m_dwChildCount = 0;
   UnlockChildList();

   m_bIsDeleted = TRUE;
   Modify();
   UnlockData();

   DbgPrintf(AF_DEBUG_OBJECTS, "NetObj::Delete(): deleting object %d from indexes", m_dwId);
   NetObjDeleteFromIndexes(this);

   // Notify all other objects about object deletion
   DbgPrintf(AF_DEBUG_OBJECTS, "NetObj::Delete(): calling OnObjectDelete(%d)", m_dwId);
   if (!bIndexLocked)
      RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
      if (g_pIndexById[i].dwKey != m_dwId)
         g_pIndexById[i].pObject->OnObjectDelete(m_dwId);
   }
   if (!bIndexLocked)
      RWLockUnlock(g_rwlockIdIndex);

   DbgPrintf(AF_DEBUG_OBJECTS, "Object %d successfully deleted", m_dwId);
}


//
// Default handler for object deletion notification
//

void NetObj::OnObjectDelete(DWORD dwObjectId)
{
}


//
// Print childs IDs
//

const char *NetObj::ChildList(char *szBuffer)
{
   DWORD i;
   char *pBuf = szBuffer;

   *pBuf = 0;
   LockChildList(FALSE);
   for(i = 0, pBuf = szBuffer; i < m_dwChildCount; i++)
   {
      sprintf(pBuf, "%d ", m_pChildList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   UnlockChildList();
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
   LockParentList(FALSE);
   for(i = 0; i < m_dwParentCount; i++)
   {
      sprintf(pBuf, "%d ", m_pParentList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   UnlockParentList();
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
   int iWorstStatus, iCount, iStatusAlg, iOldStatus = m_iStatus;

   if (m_iStatus != STATUS_UNMANAGED)
   {
      LockData();

      iStatusAlg = (m_iStatusAlgorithm == SA_DEFAULT) ? g_iStatusAlgorithm : m_iStatusAlgorithm;

      switch(iStatusAlg)
      {
         case SA_WORST_STATUS:
            LockChildList(FALSE);
            for(i = 0, iCount = 0, iWorstStatus = -1; i < m_dwChildCount; i++)
               if ((m_pChildList[i]->Status() < STATUS_UNKNOWN) &&
                   (m_pChildList[i]->Status() > iWorstStatus))
               {
                  iWorstStatus = m_pChildList[i]->Status();
                  iCount++;
               }
            UnlockChildList();

            if (iCount > 0)
               m_iStatus = iWorstStatus;
            else
               m_iStatus = STATUS_UNKNOWN;
            break;
         default:
            m_iStatus = STATUS_UNKNOWN;
            break;
      }
      UnlockData();

      // Cause parent object(s) to recalculate it's status
      if (iOldStatus != m_iStatus)
      {
         LockParentList(FALSE);
         for(i = 0; i < m_dwParentCount; i++)
            m_pParentList[i]->CalculateCompoundStatus();
         UnlockParentList();
         Modify();   /* LOCK? */
      }
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

   // Load access list
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
// Enumeration parameters structure
//

struct SAVE_PARAM
{
   DB_HANDLE hdb;
   DWORD dwObjectId;
};


//
// Handler for ACL elements enumeration
//

static void EnumerationHandler(DWORD dwUserId, DWORD dwAccessRights, void *pArg)
{
   char szQuery[256];

   sprintf(szQuery, "INSERT INTO acl (object_id,user_id,access_rights) VALUES (%d,%d,%d)",
           ((SAVE_PARAM *)pArg)->dwObjectId, dwUserId, dwAccessRights);
   DBQuery(((SAVE_PARAM *)pArg)->hdb, szQuery);
}


//
// Save ACL to database
//

BOOL NetObj::SaveACLToDB(DB_HANDLE hdb)
{
   char szQuery[256];
   BOOL bSuccess = FALSE;
   SAVE_PARAM sp;

   // Save access list
   LockACL();
   sprintf(szQuery, "DELETE FROM acl WHERE object_id=%d", m_dwId);
   if (DBQuery(hdb, szQuery))
   {
      sp.dwObjectId = m_dwId;
      sp.hdb = hdb;
      m_pAccessList->EnumerateElements(EnumerationHandler, &sp);
      bSuccess = TRUE;
   }
   UnlockACL();
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
   pMsg->SetVariable(VID_IMAGE_ID, m_dwImageId);
   pMsg->SetVariable(VID_STATUS_ALGORITHM, (WORD)m_iStatusAlgorithm);
   m_pAccessList->CreateMessage(pMsg);
}


//
// Handler for EnumerateSessions()
//

static void BroadcastObjectChange(ClientSession *pSession, void *pArg)
{
   if (pSession->IsAuthenticated())
      pSession->OnObjectChange((NetObj *)pArg);
}


//
// Mark object as modified and put on client's notification queue
// We assume that object is locked at the time of function call
//

void NetObj::Modify(void)
{
   if (g_bModificationsLocked)
      return;

   m_bIsModified = TRUE;
   m_dwTimeStamp = time(NULL);

   // Send event to all connected clients
   if (!m_bIsHidden)
      EnumerateClientSessions(BroadcastObjectChange, this);
}


//
// Modify object from message
//

DWORD NetObj::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Change object's name
   if (pRequest->IsVariableExist(VID_OBJECT_NAME))
      pRequest->GetVariableStr(VID_OBJECT_NAME, m_szName, MAX_OBJECT_NAME);

   // Change object's image (icon)
   if (pRequest->IsVariableExist(VID_IMAGE_ID))
      m_dwImageId = pRequest->GetVariableLong(VID_IMAGE_ID);

   // Change object's status calculation algorithm
   if (pRequest->IsVariableExist(VID_STATUS_ALGORITHM))
      m_iStatusAlgorithm = (int)pRequest->GetVariableShort(VID_STATUS_ALGORITHM);

   // Change object's ACL
   if (pRequest->IsVariableExist(VID_ACL_SIZE))
   {
      DWORD i, dwNumElements;

      LockACL();
      dwNumElements = pRequest->GetVariableLong(VID_ACL_SIZE);
      m_bInheritAccessRights = pRequest->GetVariableShort(VID_INHERIT_RIGHTS);
      m_pAccessList->DeleteAll();
      for(i = 0; i < dwNumElements; i++)
         m_pAccessList->AddElement(pRequest->GetVariableLong(VID_ACL_USER_BASE + i),
                                   pRequest->GetVariableLong(VID_ACL_RIGHTS_BASE +i));
      UnlockACL();
   }

   Modify();
   UnlockData();

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

   LockACL();

   // Check if have direct right assignment
   if (!m_pAccessList->GetUserRights(dwUserId, &dwRights))
   {
      // We don't. If this object inherit rights from parents, get them
      if (m_bInheritAccessRights)
      {
         DWORD i;

         LockParentList(FALSE);
         for(i = 0, dwRights = 0; i < m_dwParentCount; i++)
            dwRights |= m_pParentList[i]->GetUserRights(dwUserId);
         UnlockParentList();
      }
   }

   UnlockACL();
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
   LockACL();
   if (m_pAccessList->DeleteElement(dwUserId))
      Modify();
   UnlockACL();
}


//
// Set object's management status
//

void NetObj::SetMgmtStatus(BOOL bIsManaged)
{
   DWORD i;
   int iOldStatus;

   LockData();

   if ((bIsManaged && (m_iStatus != STATUS_UNMANAGED)) ||
       ((!bIsManaged) && (m_iStatus == STATUS_UNMANAGED)))
   {
      UnlockData();
      return;  // Status is already correct
   }

   iOldStatus = m_iStatus;
   m_iStatus = (bIsManaged ? STATUS_UNKNOWN : STATUS_UNMANAGED);

   // Generate event if current object is a node
   if (Type() == OBJECT_NODE)
      PostEvent(bIsManaged ? EVENT_NODE_UNKNOWN : EVENT_NODE_UNMANAGED, m_dwId, "d", iOldStatus);

   // Change status for child objects also
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      m_pChildList[i]->SetMgmtStatus(bIsManaged);
   UnlockChildList();

   // Cause parent object(s) to recalculate it's status
   LockParentList(FALSE);
   for(i = 0; i < m_dwParentCount; i++)
      m_pParentList[i]->CalculateCompoundStatus();
   UnlockParentList();

   Modify();
   UnlockData();
}


//
// Check if given object is an our child (possibly indirect, i.e child of child)
//

BOOL NetObj::IsChild(DWORD dwObjectId)
{
   DWORD i;
   BOOL bResult = FALSE;

   // Check for our own ID (object ID should never change, so we may not lock object's data)
   if (m_dwId == dwObjectId)
      bResult = TRUE;

   // First, walk through our own child list
   if (!bResult)
   {
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->Id() == dwObjectId)
         {
            bResult = TRUE;
            break;
         }
      UnlockChildList();
   }

   // If given object is not in child list, check if it is indirect child
   if (!bResult)
   {
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
         if (m_pChildList[i]->IsChild(dwObjectId))
         {
            bResult = TRUE;
            break;
         }
      UnlockChildList();
   }

   return bResult;
}


//
// Send message to client, who requests poll, if any
// This method is used by Node and Interface class objects
//

void NetObj::SendPollerMsg(DWORD dwRqId, TCHAR *pszFormat, ...)
{
   if (m_pPollRequestor != NULL)
   {
      va_list args;
      TCHAR szBuffer[1024];

      va_start(args, pszFormat);
      _vsntprintf(szBuffer, 1024, pszFormat, args);
      va_end(args);
      m_pPollRequestor->SendPollerMsg(dwRqId, szBuffer);
   }
}


//
// Add child node objects (direct and indirect childs) to list
//

void NetObj::AddChildNodesToList(DWORD *pdwNumNodes, Node ***pppNodeList, DWORD dwUserId)
{
   DWORD i, j;

   LockChildList(FALSE);

   // Walk through our own child list
   for(i = 0; i < m_dwChildCount; i++)
   {
      if (m_pChildList[i]->Type() == OBJECT_NODE)
      {
         // Check if this node already in the list
         for(j = 0; j < *pdwNumNodes; j++)
            if ((*pppNodeList)[j]->Id() == m_pChildList[i]->Id())
               break;
         if (j == *pdwNumNodes)
         {
            m_pChildList[i]->IncRefCount();
            *pppNodeList = (Node **)realloc(*pppNodeList, sizeof(Node *) * (*pdwNumNodes + 1));
            (*pppNodeList)[*pdwNumNodes] = (Node *)m_pChildList[i];
            (*pdwNumNodes)++;
         }
      }
      else
      {
         if (m_pChildList[i]->CheckAccessRights(dwUserId, OBJECT_ACCESS_READ))
            m_pChildList[i]->AddChildNodesToList(pdwNumNodes, pppNodeList, dwUserId);
      }
   }

   UnlockChildList();
}


//
// Hide object and all its childs
//

void NetObj::Hide(void)
{
   DWORD i;

   LockData();
   LockChildList(FALSE);

   for(i = 0; i < m_dwChildCount; i++)
      m_pChildList[i]->Hide();
   m_bIsHidden = TRUE;

   UnlockChildList();
   UnlockData();
}


//
// Unhide object and all its childs
//

void NetObj::Unhide(void)
{
   DWORD i;

   LockData();

   m_bIsHidden = FALSE;
   EnumerateClientSessions(BroadcastObjectChange, this);

   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
      m_pChildList[i]->Unhide();

   UnlockChildList();
   UnlockData();
}
