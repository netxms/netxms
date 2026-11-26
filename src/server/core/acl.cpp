/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
               m_elements.get(j)->accessRights = msg.getFieldAsUInt64(VID_ACL_RIGHTS_BASE + i);
               found = true;
               marks[j] = 1;
               break;
            }

         if (!found)
         {
            ACL_ELEMENT *e = m_elements.addPlaceholder();
            e->userId = userId;
            e->accessRights = msg.getFieldAsUInt64(VID_ACL_RIGHTS_BASE + i);
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
   clearCacheLocked();
}

/**
 * Update from JSON array of { userId, access } elements. Replaces the entire
 * directly assigned access list; the inherited-rights cache is dropped.
 */
void AccessList::updateFromJson(json_t *json)
{
   LockGuard lockGuard(m_mutex);

   m_elements.clear();
   clearCacheLocked();

   if (json_is_array(json))
   {
      size_t i;
      json_t *e;
      json_array_foreach(json, i, e)
      {
         ACL_ELEMENT *element = m_elements.addPlaceholder();
         element->userId = json_object_get_uint32(e, "userId", 0);
         element->accessRights = json_object_get_uint64(e, "access", 0);
      }
   }
}

/**
 * Add element to list
 */
static bool SetRights(StructArray<ACL_ELEMENT>& list, uint32_t userId, uint64_t accessRights)
{

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
 * Add or update a direct assignment
 */
bool AccessList::addElement(uint32_t userId, uint64_t accessRights)
{
   LockGuard lockGuard(m_mutex);
   if (!SetRights(m_elements, userId, accessRights))
      return false;
   clearCacheLocked();
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
         clearCacheLocked();
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
bool AccessList::getUserRights(uint32_t userId, uint64_t *accessRights, CacheVersion *version) const
{
   // Version and element copy are taken together; membership checks below run without the lock
   Buffer<ACL_ELEMENT, 128> elements;
   int size;
   {
      LockGuard lockGuard(m_mutex);
      version->revision = m_revision;
      version->generation = GetObjectRightsGeneration();
      size = m_elements.size();
      if (size > 0)
         elements.set(m_elements.getBuffer(), size);
   }
   if (size == 0)
   {
      *accessRights = 0;
      return false;
   }

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

   return found;
}

/**
 * Retrieve cached access rights for specific user object
 * Returns true on success and stores access rights to specific location
 */
bool AccessList::getCachedUserRights(uint32_t userId, const CacheVersion& version, uint64_t *accessRights) const
{
   LockGuard lockGuard(m_mutex);
   if ((m_cacheGeneration != version.generation) || (m_revision != version.revision))
      return false;

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
 * Publish inherited rights only if neither membership nor this ACL was invalidated during the parent walk.
 */
void AccessList::cacheUserRights(uint32_t userId, uint64_t rights, const CacheVersion& version) const
{
   LockGuard lockGuard(m_mutex);
   if ((m_revision != version.revision) || (GetObjectRightsGeneration() != version.generation))
      return;

   if (m_cacheGeneration != version.generation)
   {
      m_cache.clear();
      m_cacheGeneration = version.generation;
   }
   SetRights(m_cache, userId, rights);
}

/**
 * Enumerate all elements
 */
void AccessList::enumerateElements(void (*handler)(uint32_t, uint64_t, void *), void *context) const
{
   m_mutex.lock();
   for(int i = 0; i < m_elements.size(); i++)
      handler(m_elements.get(i)->userId, m_elements.get(i)->accessRights, context);
   m_mutex.unlock();
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
