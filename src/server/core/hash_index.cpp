/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
   NetObj *object;
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
HashIndexBase::HashIndexBase(size_t keyLength)
{
   m_primary = MemAllocStruct<HashIndexHead>();
   m_secondary = MemAllocStruct<HashIndexHead>();
   m_writerLock = MutexCreate();
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
      MemFree(entry);
   }
   MemFree(m_primary);

   HASH_ITER(hh, m_secondary->elements, entry, tmp)
   {
      MemFree(entry);
   }
   MemFree(m_secondary);

   MutexDestroy(m_writerLock);
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
HashIndexHead *HashIndexBase::acquireIndex()
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
static inline bool SetElement(HashIndexHead *index, const void *key, size_t keyLength, NetObj *object)
{
   bool replace;
   HashIndexElement *entry;
   HASH_FIND(hh, index->elements, key, keyLength, entry);
   if (entry != NULL)
   {
      entry->object = object;
      replace = true;
   }
   else
   {
      entry = static_cast<HashIndexElement*>(MemAlloc(sizeof(HashIndexElement) + keyLength - 4));
      memcpy(entry->key, key, keyLength);
      entry->object = object;
      HASH_ADD_KEYPTR(hh, index->elements, entry->key, keyLength, entry);
      replace = false;
   }
   return replace;
}

/**
 * Add object to index
 */
bool HashIndexBase::put(const void *key, NetObj *object)
{
   MutexLock(m_writerLock);
   SetElement(m_secondary, key, m_keyLength, object);
   swapAndWait();
   bool replace = SetElement(m_secondary, key, m_keyLength, object);
   InterlockedDecrement(&m_secondary->writers);
   MutexUnlock(m_writerLock);
   return replace;
}

/**
 * Remove specific element from index
 */
static inline void RemoveElement(HashIndexHead *index, const void *key, size_t keyLength)
{
   HashIndexElement *entry;
   HASH_FIND(hh, index->elements, key, keyLength, entry);
   if (entry != NULL)
   {
      HASH_DEL(index->elements, entry);
      MemFree(entry);
   }
}

/**
 * Remove object with given key
 */
void HashIndexBase::remove(const void *key)
{
   MutexLock(m_writerLock);
   RemoveElement(m_secondary, key, m_keyLength);
   swapAndWait();
   RemoveElement(m_secondary, key, m_keyLength);
   InterlockedDecrement(&m_secondary->writers);
   MutexUnlock(m_writerLock);
}

/**
 * Get object with given key
 */
NetObj *HashIndexBase::get(const void *key)
{
   HashIndexHead *index = acquireIndex();
   HashIndexElement *entry;
   HASH_FIND(hh, index->elements, key, m_keyLength, entry);
   NetObj *object = (entry != NULL) ? entry->object : NULL;
   ReleaseIndex(index);
   return object;
}

/**
 * Find element within index
 */
NetObj *HashIndexBase::find(bool (*comparator)(NetObj *, void *), void *context)
{
   NetObj *object = NULL;
   HashIndexHead *index = acquireIndex();
   HashIndexElement *entry, *tmp;
   HASH_ITER(hh, index->elements, entry, tmp)
   {
      if (comparator(entry->object, context))
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
int HashIndexBase::size()
{
   HashIndexHead *index = acquireIndex();
   int s = HASH_COUNT(index->elements);
   ReleaseIndex(index);
   return s;
}

/**
 * Enumerate all elements
 */
void HashIndexBase::forEach(void (*callback)(const void *, NetObj *, void *), void *context)
{
   HashIndexHead *index = acquireIndex();
   HashIndexElement *entry, *tmp;
   HASH_ITER(hh, index->elements, entry, tmp)
   {
      callback(entry->key, entry->object, context);
   }
   ReleaseIndex(index);
}
