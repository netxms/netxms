/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#include <nms_users.h>

/**
 * Update from NXCP message
 */
void AccessList::updateFromMessage(const NXCPMessage& msg)
{
   LockGuard lockGuard(m_mutex);

   m_size = 0;
   int count = msg.getFieldAsUInt32(VID_ACL_SIZE);
   if (count > 0)
   {
      if (count > m_allocated)
      {
         m_allocated = count;
         m_elements = MemReallocArray(m_elements, m_allocated);
      }

      for(int i = 0; i < count; i++)
      {
         uint32_t userId = msg.getFieldAsUInt32(VID_ACL_USER_BASE + i);

         bool found = false;
         for(int j = 0; j < m_size; j++)
            if (m_elements[j].userId == userId)    // Object already exist in list
            {
               m_elements[j].accessRights = msg.getFieldAsUInt32(VID_ACL_RIGHTS_BASE + i);
               found = true;
               break;
            }

         if (!found)
         {
            m_elements[m_size].userId = userId;
            m_elements[m_size].accessRights = msg.getFieldAsUInt32(VID_ACL_RIGHTS_BASE + i);
            m_size++;
         }
      }
   }
   else
   {
      m_allocated = 0;
      MemFreeAndNull(m_elements);
   }
}

/**
 * Add element to list
 */
bool AccessList::addElement(uint32_t userId, uint32_t accessRights)
{
   LockGuard lockGuard(m_mutex);

   for(int i = 0; i < m_size; i++)
      if (m_elements[i].userId == userId)    // Object already exist in list
      {
         if (m_elements[i].accessRights == accessRights)
            return false;
         m_elements[i].accessRights = accessRights;
         return true;
      }

   if (m_size == m_allocated)
   {
      m_allocated += 16;
      m_elements = MemReallocArray(m_elements, m_allocated);
   }
   m_elements[m_size].userId = userId;
   m_elements[m_size].accessRights = accessRights;
   m_size++;
   return true;
}

/**
 * Delete element from list
 */
bool AccessList::deleteElement(uint32_t userId)
{
   LockGuard lockGuard(m_mutex);
   bool deleted = false;
   for(int i = 0; i < m_size; i++)
      if (m_elements[i].userId == userId)
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
bool AccessList::getUserRights(uint32_t userId, uint32_t *accessRights) const
{
   // We can safely read size without lock here. It is guaranteed (on architectures we support)
   // that 32 bit integer read will be atomic. So the only potential problem would be
   // when size is already updated, but data is not. But in that case mutex lock will cause
   // calling thread to wait for update completion.
   if (m_size == 0)
   {
      *accessRights = 0;
      return false;
   }

   m_mutex.lock();
   int size = m_size;
   ACL_ELEMENT *elements = static_cast<ACL_ELEMENT*>(MemAllocLocal(sizeof(ACL_ELEMENT) * size));
   memcpy(elements, m_elements, sizeof(ACL_ELEMENT) * size);
   m_mutex.unlock();

   bool found = false;

   // Check for explicit rights
   for(int i = 0; i < size; i++)
      if (elements[i].userId == userId)
      {
         *accessRights = elements[i].accessRights;
         found = true;
         break;
      }

   if (!found)
   {
      *accessRights = 0;   // Default: no access
      for(int i = 0; i < size; i++)
         if (elements[i].userId & GROUP_FLAG)
         {
            if (CheckUserMembership(userId, elements[i].userId))
            {
               *accessRights |= elements[i].accessRights;
               found = true;
            }
         }
   }

   MemFreeLocal(elements);
   return found;
}

/**
 * Enumerate all elements
 */
void AccessList::enumerateElements(void (*handler)(uint32_t, uint32_t, void *), void *context) const
{
   LockGuard lockGuard(m_mutex);
   for(int i = 0; i < m_size; i++)
      handler(m_elements[i].userId, m_elements[i].accessRights, context);
}

/**
 * Fill NXCP message with ACL's data
 */
void AccessList::fillMessage(NXCPMessage *msg) const
{
   LockGuard lockGuard(m_mutex);

   msg->setField(VID_ACL_SIZE, m_size);

   uint32_t id1, id2;
   int i;
   for(i = 0, id1 = VID_ACL_USER_BASE, id2 = VID_ACL_RIGHTS_BASE; i < m_size; i++, id1++, id2++)
   {
      msg->setField(id1, m_elements[i].userId);
      msg->setField(id2, m_elements[i].accessRights);
   }
}

/**
 * Serialize as JSON
 */
json_t *AccessList::toJson() const
{
   LockGuard lockGuard(m_mutex);
   json_t *root = json_array();
   for(int i = 0; i < m_size; i++)
   {
      json_t *e = json_object();
      json_object_set_new(e, "userId", json_integer(m_elements[i].userId));
      json_object_set_new(e, "access", json_integer(m_elements[i].accessRights));
      json_array_append_new(root, e);
   }
   return root;
}

/**
 * Delete all elements
 */
void AccessList::deleteAll()
{
   LockGuard lockGuard(m_mutex);
   m_size = 0;
   m_allocated = 0;
   MemFreeAndNull(m_elements);
}
