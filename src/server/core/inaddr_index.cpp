/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: inaddr_index.cpp
**
**/

#include "nxcore.h"
#include <uthash.h>

/**
 * Entry
 */
struct InetAddressIndexEntry
{
   UT_hash_handle hh;
   BYTE key[18];
   InetAddress addr;
   NetObj *object;
};

/**
 * Constructor
 */
InetAddressIndex::InetAddressIndex()
{
   m_root = NULL;
   m_lock = RWLockCreate();
}

/**
 * Destructor
 */
InetAddressIndex::~InetAddressIndex()
{
   InetAddressIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_root, entry, tmp)
   {
      HASH_DEL(m_root, entry);
      free(entry);
   }
   RWLockDestroy(m_lock);
}

/**
 * Put object into index
 *
 * @param addr IP address
 * @param object object
 * @return true if existing object was replaced
 */
bool InetAddressIndex::put(const InetAddress& addr, NetObj *object)
{
   if (!addr.isValid())
      return false;

   bool replace = true;

   BYTE key[18];
   addr.buildHashKey(key);

   RWLockWriteLock(m_lock, INFINITE);

   InetAddressIndexEntry *entry;
   HASH_FIND(hh, m_root, key, 18, entry);
   if (entry == NULL)
   {
      entry = (InetAddressIndexEntry *)malloc(sizeof(InetAddressIndexEntry));
      memcpy(entry->key, key, 18);
      entry->addr = addr;
      HASH_ADD_KEYPTR(hh, m_root, entry->key, 18, entry);
      replace = false;
   }
   entry->object = object;

   RWLockUnlock(m_lock);
   return replace;
}

/**
 * Put object into index
 *
 * @param addrList IP address list
 * @param object object
 * @return true if existing object was replaced
 */
bool InetAddressIndex::put(const InetAddressList *addrList, NetObj *object)
{
   bool replaced = false;
   for(int i = 0; i < addrList->size(); i++)
   {
      if (put(addrList->get(i), object))
         replaced = true;
   }
   return replaced;
}

/**
 * Remove object from index
 */
void InetAddressIndex::remove(const InetAddress& addr)
{
   if (!addr.isValid())
      return;

   BYTE key[18];
   addr.buildHashKey(key);

   RWLockWriteLock(m_lock, INFINITE);

   InetAddressIndexEntry *entry;
   HASH_FIND(hh, m_root, key, 18, entry);
   if (entry != NULL)
   {
      HASH_DEL(m_root, entry);
      free(entry);
   }
   RWLockUnlock(m_lock);
}

/**
 * Get object by IP address
 */
NetObj *InetAddressIndex::get(const InetAddress& addr)
{
   if (!addr.isValid())
      return NULL;

   NetObj *object = NULL;

   BYTE key[18];
   addr.buildHashKey(key);

   RWLockReadLock(m_lock, INFINITE);

   InetAddressIndexEntry *entry;
   HASH_FIND(hh, m_root, key, 18, entry);
   if (entry != NULL)
   {
      object = entry->object;
   }
   RWLockUnlock(m_lock);
   return object;
}

/**
 * Find object using comparator
 */
NetObj *InetAddressIndex::find(bool (*comparator)(NetObj *, void *), void *data)
{
   NetObj *object = NULL;

   RWLockReadLock(m_lock, INFINITE);

   InetAddressIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_root, entry, tmp)
   {
      if (comparator(entry->object, data))
      {
         object = entry->object;
         break;
      }
   }

   RWLockUnlock(m_lock);
   return object;
}

/**
 * Get index size
 */
int InetAddressIndex::size()
{
   RWLockReadLock(m_lock, INFINITE);
   int s = HASH_COUNT(m_root);
   RWLockUnlock(m_lock);
   return s;
}

/**
 * Get all objects
 */
ObjectArray<NetObj> *InetAddressIndex::getObjects(bool updateRefCount, bool (*filter)(NetObj *, void *), void *userData)
{
   ObjectArray<NetObj> *objects = new ObjectArray<NetObj>();

   RWLockReadLock(m_lock, INFINITE);
   InetAddressIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_root, entry, tmp)
   {
      if ((filter == NULL) || filter(entry->object, userData))
      {
         if (updateRefCount)
            entry->object->incRefCount();
         objects->add(entry->object);
      }
   }
   RWLockUnlock(m_lock);
   return objects;
}

/**
 * Execute given callback for each object
 */
void InetAddressIndex::forEach(void (*callback)(const InetAddress& addr, NetObj *, void *), void *data)
{
   RWLockReadLock(m_lock, INFINITE);
   InetAddressIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_root, entry, tmp)
   {
      callback(entry->addr, entry->object, data);
   }
   RWLockUnlock(m_lock);
}
