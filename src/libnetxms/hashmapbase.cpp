/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2018 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: hashmapbase.cpp
**
**/

#include "libnetxms.h"
#include <uthash.h>

/**
 * Entry
 */
struct HashMapEntry
{
   UT_hash_handle hh;
   union
   {
      BYTE d[16];
      void *p;
   } key;
   void *value;
};

/**
 * Delete key
 */
#define DELETE_KEY(m, e) do { if ((m)->m_keylen > 16) MemFree((e)->key.p); } while(0)

/**
 * Get pointer to key
 */
#define GET_KEY(e) ((m_keylen <= 16) ? (e)->key.d : (e)->key.p)

/**
 * Standard object destructor
 */
static void ObjectDestructor(void *object, HashMapBase *map)
{
	MemFree(object);
}

/**
 * Constructor
 */
HashMapBase::HashMapBase(Ownership objectOwner, unsigned int keylen, void (*destructor)(void *, HashMapBase *))
{
   m_data = nullptr;
   m_objectOwner = (objectOwner == Ownership::True);
   m_keylen = keylen;
   m_objectDestructor = (destructor != nullptr) ? destructor : ObjectDestructor;
   m_context = nullptr;
}

/**
 * Destructor
 */
HashMapBase::~HashMapBase()
{
	clear();
}

/**
 * Clear map
 */
void HashMapBase::clear()
{
   HashMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      HASH_DEL(m_data, entry);
      DELETE_KEY(this, entry);
      if (m_objectOwner)
         destroyObject(entry->value);
      MemFree(entry);
   }
}

/**
 * Find entry index by key
 */
HashMapEntry *HashMapBase::find(const void *key) const
{
	if (key == nullptr)
		return nullptr;

   HashMapEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   return entry;
}

/**
 * Set value
 */
void HashMapBase::_set(const void *key, void *value)
{
   if (key == nullptr)
      return;

	HashMapEntry *entry = find(key);
	if (entry != nullptr)
	{
		if (m_objectOwner)
         destroyObject(entry->value);
      entry->value = value;
	}
	else
	{
      entry = MemAllocStruct<HashMapEntry>();
      if (m_keylen <= 16)
         memcpy(entry->key.d, key, m_keylen);
      else
         entry->key.p = MemCopyBlock(key, m_keylen);
      entry->value = value;
      HASH_ADD_KEYPTR(hh, m_data, GET_KEY(entry), m_keylen, entry);
	}
}

/**
 * Get value by key
 */
void *HashMapBase::_get(const void *key) const
{
   HashMapEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   return (entry != nullptr) ? entry->value : nullptr;
}

/**
 * Delete value
 */
void HashMapBase::_remove(const void *key, bool destroyValue)
{
   HashMapEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   if (entry != nullptr)
   {
      HASH_DEL(m_data, entry);
      DELETE_KEY(this, entry);
		if (m_objectOwner && destroyValue)
         destroyObject(entry->value);
      MemFree(entry);
   }
}

/**
 * Enumerate entries
 * Returns true if whole map was enumerated and false if enumeration was aborted by callback.
 */
EnumerationCallbackResult HashMapBase::forEach(EnumerationCallbackResult (*cb)(const void *, void *, void *), void *context) const
{
   EnumerationCallbackResult result = _CONTINUE;
   HashMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(GET_KEY(entry), entry->value, context) == _STOP)
      {
         result = _STOP;
         break;
      }
   }
   return result;
}

/**
 * Enumerate entries
 * Returns true if whole map was enumerated and false if enumeration was aborted by callback.
 */
EnumerationCallbackResult HashMapBase::forEach(std::function<EnumerationCallbackResult (const void*, void*)> cb) const
{
   EnumerationCallbackResult result = _CONTINUE;
   HashMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(GET_KEY(entry), entry->value) == _STOP)
      {
         result = _STOP;
         break;
      }
   }
   return result;
}

/**
 * Find entry
 */
const void *HashMapBase::findElement(bool (*comparator)(const void *, const void *, void *), void *context) const
{
   const void *result = nullptr;
   HashMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (comparator(GET_KEY(entry), entry->value, context))
      {
         result = entry->value;
         break;
      }
   }
   return result;
}

/**
 * Find entry
 */
const void *HashMapBase::findElement(std::function<bool (const void*, const void*)> comparator) const
{
   const void *result = nullptr;
   HashMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (comparator(GET_KEY(entry), entry->value))
      {
         result = entry->value;
         break;
      }
   }
   return result;
}

/**
 * Get size
 */
int HashMapBase::size() const
{
   return HASH_COUNT(m_data);
}

/**
 * ********************************************************
 * 
 * Hash map iterator
 * 
 * ********************************************************
 */

/**
 * Hash map iterator
 */
HashMapIterator::HashMapIterator(HashMapBase *hashMap)
{
   m_hashMap = hashMap;
   m_curr = nullptr;
   m_next = nullptr;
}

/**
 * Hash map iterator copy constructor
 */
HashMapIterator::HashMapIterator(const HashMapIterator& other)
{
   m_hashMap = other.m_hashMap;
   m_curr = other.m_curr;
   m_next = other.m_next;
}

/**
 * Next element availability indicator
 */
bool HashMapIterator::hasNext()
{
   if (m_hashMap->m_data == nullptr)
      return false;

   return (m_curr != nullptr) ? (m_next != nullptr) : true;
}

/**
 * Get next element and advance iterator
 */
void *HashMapIterator::next()
{
   if (m_hashMap->m_data == nullptr)
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
   {
      m_curr = m_hashMap->m_data;
   }
   else
   {
      if (m_next == nullptr)
         return nullptr;
      m_curr = m_next;
   }
   m_next = static_cast<HashMapEntry*>(m_curr->hh.next);
   return m_curr->value;
}

/**
 * Remove current element
 */
void HashMapIterator::remove()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_hashMap->m_data, m_curr);
   DELETE_KEY(m_hashMap, m_curr);
   if (m_hashMap->m_objectOwner)
      m_hashMap->destroyObject(m_curr->value);
   MemFree(m_curr);
}

/**
 * Remove current element without destroying it
 */
void HashMapIterator::unlink()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_hashMap->m_data, m_curr);
   DELETE_KEY(m_hashMap, m_curr);
   MemFree(m_curr);
}

/**
 * Check iterators equality
 */
bool HashMapIterator::equals(AbstractIterator *other)
{
   if (other == nullptr)
      return false;

   auto otherIterator = static_cast<HashMapIterator*>(other);
   void *data = key();
   void *otherData = otherIterator->key();

   if (data == nullptr && otherData == nullptr)
      return true;

   if (data == nullptr || otherData == nullptr)
      return false;

   if (m_hashMap->m_keylen != otherIterator->m_hashMap->m_keylen)
      return false;

   return memcmp(data, otherData, m_hashMap->m_keylen) == 0;
}

/**
 * Get current value in C++ semantics (next value in Java semantics)
 */
void *HashMapIterator::value()
{
   if (m_hashMap->m_data == nullptr) //no data
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
      return m_hashMap->m_data->value;

   if (m_next == nullptr)
      return nullptr;

   return m_next->value;
}

/**
 * Get current key in C++ semantics (next key in Java semantics)
 */
void *HashMapIterator::key()
{
   if (m_hashMap == nullptr || m_hashMap->m_data == nullptr) //no data
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
      return m_hashMap->m_keylen <= 16 ? m_hashMap->m_data->key.d : m_hashMap->m_data->key.p;

   if (m_next == nullptr)
      return nullptr;

   return m_hashMap->m_keylen <= 16 ? m_next->key.d : m_next->key.p;
}
