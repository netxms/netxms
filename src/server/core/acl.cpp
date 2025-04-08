/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

   int count = msg.getFieldAsUInt32(VID_ACL_SIZE);
   if (count > 0)
   {
      int originalSize = m_elements.size();
      BYTE *marks = static_cast<BYTE*>(MemAllocLocal(originalSize));
      memset(marks, 0, originalSize);

      for(int i = 0; i < count; i++)
      {
         uint32_t userId = msg.getFieldAsUInt32(VID_ACL_USER_BASE + i);

         bool found = false;
         for(int j = 0; j < m_elements.size(); j++)
            if (m_elements.get(j)->userId == userId)    // Object already exist in list
            {
               m_elements.get(j)->accessRights = msg.getFieldAsUInt32(VID_ACL_RIGHTS_BASE + i);
               found = true;
               marks[j] = 1;
               break;
            }

         if (!found)
         {
            ACL_ELEMENT *e = m_elements.addPlaceholder();
            e->userId = userId;
            e->accessRights = msg.getFieldAsUInt32(VID_ACL_RIGHTS_BASE + i);
         }
      }

      // Remove missing elements
      for(int i = originalSize - 1; i >= 0; i--)
         if (!marks[i])
            m_elements.remove(i);

      MemFreeLocal(marks);
   }
   else
   {
      m_elements.clear();
   }
   m_cache.clear();
}

/**
 * Add element to list
 */
bool AccessList::addElement(uint32_t userId, uint32_t accessRights, bool cached)
{
   StructArray<ACL_ELEMENT>& list = cached ? m_cache : m_elements;

   LockGuard lockGuard(m_mutex);

   for(int i = 0; i < list.size(); i++)
   {
      ACL_ELEMENT *e = list.get(i);
      if (e->userId == userId)    // Object already exist in list
      {
         if (e->accessRights == accessRights)
            return false;
         e->accessRights = accessRights;
         return true;
      }
   }

   ACL_ELEMENT *e = list.addPlaceholder();
   e->userId = userId;
   e->accessRights = accessRights;
   return true;
}

/**
 * Delete element from list
 */
bool AccessList::deleteElement(uint32_t userId)
{
   LockGuard lockGuard(m_mutex);
   bool deleted = false;
   for(int i = 0; i < m_elements.size(); i++)
      if (m_elements.get(i)->userId == userId)
      {
         m_elements.remove(i);
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
   if (m_elements.isEmpty())
   {
      *accessRights = 0;
      return false;
   }

   m_mutex.lock();
   int size = m_elements.size();
   ACL_ELEMENT *elements = static_cast<ACL_ELEMENT*>(MemAllocLocal(sizeof(ACL_ELEMENT) * size));
   memcpy(elements, m_elements.getBuffer(), sizeof(ACL_ELEMENT) * size);
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
 * Retrieve cached access rights for specific user object
 * Returns true on success and stores access rights to specific location
 */
bool AccessList::getCachedUserRights(uint32_t userId, uint32_t *accessRights) const
{
   // We can safely read size without lock here. It is guaranteed (on architectures we support)
   // that 32 bit integer read will be atomic. So the only potential problem would be
   // when size is already updated, but data is not. But in that case mutex lock will cause
   // calling thread to wait for update completion.
   if (m_cache.isEmpty())
      return false;

   LockGuard lockGuard(m_mutex);

   for(int i = 0; i < m_cache.size(); i++)
   {
      ACL_ELEMENT *e = m_cache.get(i);
      if (e->userId == userId)
      {
         *accessRights = e->accessRights;
         return true;
      }
   }

   return false;
}

/**
 * Enumerate all elements
 */
void AccessList::enumerateElements(void (*handler)(uint32_t, uint32_t, void *), void *context) const
{
   LockGuard lockGuard(m_mutex);
   for(int i = 0; i < m_elements.size(); i++)
      handler(m_elements.get(i)->userId, m_elements.get(i)->accessRights, context);
}

/**
 * Fill NXCP message with ACL's data
 */
void AccessList::fillMessage(NXCPMessage *msg) const
{
   LockGuard lockGuard(m_mutex);

   msg->setField(VID_ACL_SIZE, m_elements.size());

   uint32_t id1, id2;
   int i;
   for(i = 0, id1 = VID_ACL_USER_BASE, id2 = VID_ACL_RIGHTS_BASE; i < m_elements.size(); i++, id1++, id2++)
   {
      msg->setField(id1, m_elements.get(i)->userId);
      msg->setField(id2, m_elements.get(i)->accessRights);
   }
}

/**
 * Serialize as JSON
 */
json_t *AccessList::toJson() const
{
   LockGuard lockGuard(m_mutex);
   json_t *root = json_array();
   for(int i = 0; i < m_elements.size(); i++)
   {
      json_t *e = json_object();
      json_object_set_new(e, "userId", json_integer(m_elements.get(i)->userId));
      json_object_set_new(e, "access", json_integer(m_elements.get(i)->accessRights));
      json_array_append_new(root, e);
   }
   return root;
}
