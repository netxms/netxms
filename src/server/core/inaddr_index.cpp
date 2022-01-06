/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
   shared_ptr<NetObj> object;
};

/**
 * Constructor
 */
InetAddressIndex::InetAddressIndex()
{
   m_root = nullptr;
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
      entry->object.~shared_ptr();
      MemFree(entry);
   }
}

/**
 * Put object into index
 *
 * @param addr IP address
 * @param object object
 * @return true if existing object was replaced
 */
bool InetAddressIndex::put(const InetAddress& addr, const shared_ptr<NetObj>& object)
{
   if (!addr.isValidUnicast())
      return false;

   bool replace = true;

   BYTE key[18];
   addr.buildHashKey(key);

   m_lock.writeLock();

   InetAddressIndexEntry *entry;
   HASH_FIND(hh, m_root, key, sizeof(key), entry);
   if (entry == nullptr)
   {
      entry = MemAllocStruct<InetAddressIndexEntry>();
      memcpy(entry->key, key, sizeof(key));
      entry->addr = addr;
      new(&entry->object) shared_ptr<NetObj>();
      HASH_ADD_KEYPTR(hh, m_root, entry->key, sizeof(key), entry);
      replace = false;
   }
   entry->object = object;

   m_lock.unlock();
   return replace;
}

/**
 * Put object into index
 *
 * @param addrList IP address list
 * @param object object
 * @return true if existing object was replaced
 */
bool InetAddressIndex::put(const InetAddressList *addrList, const shared_ptr<NetObj>& object)
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
   if (!addr.isValidUnicast())
      return;

   BYTE key[18];
   addr.buildHashKey(key);

   m_lock.writeLock();

   InetAddressIndexEntry *entry;
   HASH_FIND(hh, m_root, key, sizeof(key), entry);
   if (entry != NULL)
   {
      HASH_DEL(m_root, entry);
      entry->object.~shared_ptr();
      MemFree(entry);
   }
   m_lock.unlock();
}

/**
 * Get object by IP address
 */
shared_ptr<NetObj> InetAddressIndex::get(const InetAddress& addr) const
{
   shared_ptr<NetObj> object;

   if (!addr.isValid())
      return object;

   BYTE key[18];
   addr.buildHashKey(key);

   m_lock.readLock();

   InetAddressIndexEntry *entry;
   HASH_FIND(hh, m_root, key, sizeof(key), entry);
   if (entry != NULL)
   {
      object = entry->object;
   }
   m_lock.unlock();
   return object;
}

/**
 * Find object using comparator
 */
shared_ptr<NetObj> InetAddressIndex::find(bool (*comparator)(NetObj *, void *), void *context) const
{
   shared_ptr<NetObj> object;

   m_lock.readLock();

   InetAddressIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_root, entry, tmp)
   {
      if (comparator(entry->object.get(), context))
      {
         object = entry->object;
         break;
      }
   }

   m_lock.unlock();
   return object;
}

/**
 * Get index size
 */
int InetAddressIndex::size() const
{
   m_lock.readLock();
   int s = HASH_COUNT(m_root);
   m_lock.unlock();
   return s;
}

/**
 * Get all objects
 */
unique_ptr<SharedObjectArray<NetObj>> InetAddressIndex::getObjects(bool (*filter)(NetObj *, void *), void *context) const
{
   unique_ptr<SharedObjectArray<NetObj>> objects = make_unique<SharedObjectArray<NetObj>>();

   m_lock.readLock();
   InetAddressIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_root, entry, tmp)
   {
      if ((filter == nullptr) || filter(entry->object.get(), context))
      {
         objects->add(entry->object);
      }
   }
   m_lock.unlock();
   return objects;
}

/**
 * Execute given callback for each object
 */
void InetAddressIndex::forEach(void (*callback)(const InetAddress& addr, NetObj *, void *), void *context) const
{
   m_lock.readLock();
   InetAddressIndexEntry *entry, *tmp;
   HASH_ITER(hh, m_root, entry, tmp)
   {
      callback(entry->addr, entry->object.get(), context);
   }
   m_lock.unlock();
}
