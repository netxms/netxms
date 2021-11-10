/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: hash_index.cpp
**
**/

#include "nxcore.h"
#include <uthash.h>

/**
 * Index element
 */
struct HashIndexElement
{
   UT_hash_handle hh;
   shared_ptr<NetObj> object;
   BYTE key[4];   // Actual key length may differ
};

/**
 * Index head
 */
struct HashIndexHead
{
   HashIndexElement *elements;
   VolatileCounter readers;
   VolatileCounter writers;
};

/**
 * Constructor
 */
HashIndexBase::HashIndexBase(size_t keyLength) : m_writerLock(MutexType::FAST)
{
   m_primary = MemAllocStruct<HashIndexHead>();
   m_secondary = MemAllocStruct<HashIndexHead>();
   m_keyLength = keyLength;
}

/**
 * Destructor
 */
HashIndexBase::~HashIndexBase()
{
   HashIndexElement *entry, *tmp;
   HASH_ITER(hh, m_primary->elements, entry, tmp)
   {
      HASH_DEL(m_primary->elements, entry);
      entry->object.~shared_ptr();
      MemFree(entry);
   }
   MemFree(m_primary);

   HASH_ITER(hh, m_secondary->elements, entry, tmp)
   {
      HASH_DEL(m_secondary->elements, entry);
      entry->object.~shared_ptr();
      MemFree(entry);
   }
   MemFree(m_secondary);
}

/**
 * Swap indexes and wait for new secondary copy to became writable
 */
void HashIndexBase::swapAndWait()
{
   m_secondary = InterlockedExchangeObjectPointer(&m_primary, m_secondary);
   InterlockedIncrement(&m_secondary->writers);
   while(m_secondary->readers > 0)
      ThreadSleepMs(10);
}

/**
 * Acquire index
 */
HashIndexHead *HashIndexBase::acquireIndex() const
{
   HashIndexHead *h;
   while(true)
   {
      h = m_primary;
      InterlockedIncrement(&h->readers);
      if (h->writers == 0)
         break;
      InterlockedDecrement(&h->readers);
   }
   return h;
}

/**
 * Release index
 */
static inline void ReleaseIndex(HashIndexHead *h)
{
   InterlockedDecrement(&h->readers);
}

/**
 * Add element to index
 */
static inline bool SetElement(HashIndexHead *index, const void *key, size_t keyLength, const shared_ptr<NetObj>& object)
{
   bool replace;
   HashIndexElement *entry;
   HASH_FIND(hh, index->elements, key, keyLength, entry);
   if (entry != nullptr)
   {
      entry->object = object;
      replace = true;
   }
   else
   {
      entry = static_cast<HashIndexElement*>(MemAlloc(sizeof(HashIndexElement) + keyLength - 4));
      new(&entry->object) shared_ptr<NetObj>(object);
      memcpy(entry->key, key, keyLength);
      HASH_ADD_KEYPTR(hh, index->elements, entry->key, keyLength, entry);
      replace = false;
   }
   return replace;
}

/**
 * Add object to index
 */
bool HashIndexBase::put(const void *key, const shared_ptr<NetObj>& object)
{
   m_writerLock.lock();
   SetElement(m_secondary, key, m_keyLength, object);
   swapAndWait();
   bool replace = SetElement(m_secondary, key, m_keyLength, object);
   InterlockedDecrement(&m_secondary->writers);
   m_writerLock.unlock();
   return replace;
}

/**
 * Remove specific element from index
 */
static inline void RemoveElement(HashIndexHead *index, const void *key, size_t keyLength)
{
   HashIndexElement *entry;
   HASH_FIND(hh, index->elements, key, keyLength, entry);
   if (entry != nullptr)
   {
      HASH_DEL(index->elements, entry);
      entry->object.~shared_ptr();
      MemFree(entry);
   }
}

/**
 * Remove object with given key
 */
void HashIndexBase::remove(const void *key)
{
   m_writerLock.lock();
   RemoveElement(m_secondary, key, m_keyLength);
   swapAndWait();
   RemoveElement(m_secondary, key, m_keyLength);
   InterlockedDecrement(&m_secondary->writers);
   m_writerLock.unlock();
}

/**
 * Get object with given key
 */
shared_ptr<NetObj> HashIndexBase::get(const void *key) const
{
   HashIndexHead *index = acquireIndex();
   HashIndexElement *entry;
   HASH_FIND(hh, index->elements, key, m_keyLength, entry);
   shared_ptr<NetObj> object = (entry != nullptr) ? entry->object : shared_ptr<NetObj>();
   ReleaseIndex(index);
   return object;
}

/**
 * Find element within index
 */
shared_ptr<NetObj> HashIndexBase::find(bool (*comparator)(NetObj *, void *), void *context) const
{
   shared_ptr<NetObj> object;
   HashIndexHead *index = acquireIndex();
   HashIndexElement *entry, *tmp;
   HASH_ITER(hh, index->elements, entry, tmp)
   {
      if (comparator(entry->object.get(), context))
      {
         object = entry->object;
         break;
      }
   }
   ReleaseIndex(index);
   return object;
}

/**
 * Get index size
 */
int HashIndexBase::size() const
{
   HashIndexHead *index = acquireIndex();
   int s = HASH_COUNT(index->elements);
   ReleaseIndex(index);
   return s;
}

/**
 * Enumerate all elements
 */
void HashIndexBase::forEach(void (*callback)(const void *, NetObj *, void *), void *context) const
{
   HashIndexHead *index = acquireIndex();
   HashIndexElement *entry, *tmp;
   HASH_ITER(hh, index->elements, entry, tmp)
   {
      callback(entry->key, entry->object.get(), context);
   }
   ReleaseIndex(index);
}
