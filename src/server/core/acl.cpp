/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: acl.cpp
**
**/

#include "nxcore.h"


//
// Access list constructor
//

AccessList::AccessList()
{
   m_pElements = NULL;
   m_dwNumElements = 0;
   m_hMutex = MutexCreate();
}


//
// Destructor
//

AccessList::~AccessList()
{
   safe_free(m_pElements);
   MutexDestroy(m_hMutex);
}


//
// Add element to list
//

void AccessList::AddElement(DWORD dwUserId, DWORD dwAccessRights)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwNumElements; i++)
      if (m_pElements[i].dwUserId == dwUserId)    // Object already exist in list
      {
         m_pElements[i].dwAccessRights = dwAccessRights;
         break;
      }

   if (i == m_dwNumElements)
   {
      m_pElements = (ACL_ELEMENT *)realloc(m_pElements, sizeof(ACL_ELEMENT) * (m_dwNumElements + 1));
      m_pElements[m_dwNumElements].dwUserId = dwUserId;
      m_pElements[m_dwNumElements].dwAccessRights = dwAccessRights;
      m_dwNumElements++;
   }
   Unlock();
}


//
// Delete element from list
//

BOOL AccessList::DeleteElement(DWORD dwUserId)
{
   DWORD i;
   BOOL bDeleted = FALSE;

   Lock();
   for(i = 0; i < m_dwNumElements; i++)
      if (m_pElements[i].dwUserId == dwUserId)
      {
         m_dwNumElements--;
         memmove(&m_pElements[i], &m_pElements[i + 1], sizeof(ACL_ELEMENT) * (m_dwNumElements - i));
         bDeleted = TRUE;
         break;
      }
   Unlock();
   return bDeleted;
}


//
// Retrieve access rights for specific user object
// Return TRUE on success and stores access rights to specific location
// If user doesn't have explicit rights or via group, returns FALSE
//

BOOL AccessList::GetUserRights(DWORD dwUserId, DWORD *pdwAccessRights)
{
   DWORD i;
   BOOL bFound = FALSE;

   Lock();

   // Check for explicit rights
   for(i = 0; i < m_dwNumElements; i++)
      if (m_pElements[i].dwUserId == dwUserId)
      {
         *pdwAccessRights = m_pElements[i].dwAccessRights;
         bFound = TRUE;
         break;
      }

   if (!bFound)
   {
      *pdwAccessRights = 0;   // Default: no access
      for(i = 0; i < m_dwNumElements; i++)
         if (m_pElements[i].dwUserId & GROUP_FLAG)
         {
            if (CheckUserMembership(dwUserId, m_pElements[i].dwUserId))
            {
               *pdwAccessRights |= m_pElements[i].dwAccessRights;
               bFound = TRUE;
            }
         }
   }

   Unlock();
   return bFound;
}


//
// Enumerate all elements
//

void AccessList::EnumerateElements(void (* pHandler)(DWORD, DWORD, void *), void *pArg)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwNumElements; i++)
      pHandler(m_pElements[i].dwUserId, m_pElements[i].dwAccessRights, pArg);
   Unlock();
}


//
// Fill CSCP message with ACL's data
//

void AccessList::CreateMessage(CSCPMessage *pMsg)
{
   DWORD i, dwId1, dwId2;

   Lock();
   pMsg->SetVariable(VID_ACL_SIZE, m_dwNumElements);
   for(i = 0, dwId1 = VID_ACL_USER_BASE, dwId2 = VID_ACL_RIGHTS_BASE;
       i < m_dwNumElements; i++, dwId1++, dwId2++)
   {
      pMsg->SetVariable(dwId1, m_pElements[i].dwUserId);
      pMsg->SetVariable(dwId2, m_pElements[i].dwAccessRights);
   }
   Unlock();
}


//
// Delete all elements
//

void AccessList::DeleteAll()
{
   Lock();
   m_dwNumElements = 0;
   safe_free(m_pElements);
   m_pElements = NULL;
   Unlock();
}
