/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

/**
 * Access list constructor
 */
AccessList::AccessList()
{
   m_size = 0;
   m_allocated = 0;
   m_elements = NULL;
}

/**
 * Destructor
 */
AccessList::~AccessList()
{
   free(m_elements);
}

/**
 * Add element to list
 */
void AccessList::addElement(UINT32 dwUserId, UINT32 dwAccessRights)
{
   int i;

   for(i = 0; i < m_size; i++)
      if (m_elements[i].dwUserId == dwUserId)    // Object already exist in list
      {
         m_elements[i].dwAccessRights = dwAccessRights;
         break;
      }

   if (i == m_size)
   {
      if (m_size == m_allocated)
      {
         m_allocated += 16;
         m_elements = (ACL_ELEMENT *)realloc(m_elements, sizeof(ACL_ELEMENT) * m_allocated);
      }
      m_elements[m_size].dwUserId = dwUserId;
      m_elements[m_size].dwAccessRights = dwAccessRights;
      m_size++;
   }
}

/**
 * Delete element from list
 */
bool AccessList::deleteElement(UINT32 dwUserId)
{
   bool deleted = false;
   for(int i = 0; i < m_size; i++)
      if (m_elements[i].dwUserId == dwUserId)
      {
         m_size--;
         memmove(&m_elements[i], &m_elements[i + 1], sizeof(ACL_ELEMENT) * (m_size - i));
         deleted = true;
         break;
      }
   return deleted;
}

/**
 * Retrieve access rights for specific user object
 * Returns true on success and stores access rights to specific location
 * If user doesn't have explicit rights or via group, returns false
 */
bool AccessList::getUserRights(UINT32 dwUserId, UINT32 *pdwAccessRights)
{
   int i;
   bool found = false;

   // Check for explicit rights
   for(i = 0; i < m_size; i++)
      if (m_elements[i].dwUserId == dwUserId)
      {
         *pdwAccessRights = m_elements[i].dwAccessRights;
         found = true;
         break;
      }

   if (!found)
   {
      *pdwAccessRights = 0;   // Default: no access
      for(i = 0; i < m_size; i++)
         if (m_elements[i].dwUserId & GROUP_FLAG)
         {
            if (CheckUserMembership(dwUserId, m_elements[i].dwUserId))
            {
               *pdwAccessRights |= m_elements[i].dwAccessRights;
               found = true;
            }
         }
   }

   return found;
}

/**
 * Enumerate all elements
 */
void AccessList::enumerateElements(void (* pHandler)(UINT32, UINT32, void *), void *pArg)
{
   for(int i = 0; i < m_size; i++)
      pHandler(m_elements[i].dwUserId, m_elements[i].dwAccessRights, pArg);
}

/**
 * Fill NXCP message with ACL's data
 */
void AccessList::fillMessage(NXCPMessage *pMsg)
{
   pMsg->setField(VID_ACL_SIZE, m_size);

   UINT32 dwId1, dwId2;
   int i;
   for(i = 0, dwId1 = VID_ACL_USER_BASE, dwId2 = VID_ACL_RIGHTS_BASE; i < m_size; i++, dwId1++, dwId2++)
   {
      pMsg->setField(dwId1, m_elements[i].dwUserId);
      pMsg->setField(dwId2, m_elements[i].dwAccessRights);
   }
}

/**
 * Serialize as JSON
 */
json_t *AccessList::toJson()
{
   json_t *root = json_array();
   for(int i = 0; i < m_size; i++)
   {
      json_t *e = json_object();
      json_object_set_new(e, "userId", json_integer(m_elements[i].dwUserId));
      json_object_set_new(e, "access", json_integer(m_elements[i].dwAccessRights));
      json_array_append_new(root, e);
   }
   return root;
}

/**
 * Delete all elements
 */
void AccessList::deleteAll()
{
   m_size = 0;
   m_allocated = 0;
   free(m_elements);
   m_elements = NULL;
}
