/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
#define DELETE_KEY(m, e) do { if ((m)->m_keylen > 16) free((e)->key.p); } while(0)

/**
 * Get pointer to key
 */
#define GET_KEY(e) ((m_keylen <= 16) ? (e)->key.d : (e)->key.p)

/**
 * Standard object destructor
 */
static void ObjectDestructor(void *object)
{
	free(object);
}

/**
 * Constructors
 */
HashMapBase::HashMapBase(bool objectOwner, unsigned int keylen)
{
	m_data = NULL;
	m_objectOwner = objectOwner;
   m_keylen = keylen;
	m_objectDestructor = ObjectDestructor;
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
      free(entry);
   }
}

/**
 * Find entry index by key
 */
HashMapEntry *HashMapBase::find(const void *key) const
{
	if (key == NULL)
		return NULL;

   HashMapEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   return entry;
}

/**
 * Set value
 */
void HashMapBase::_set(const void *key, void *value)
{
   if (key == NULL)
      return;

	HashMapEntry *entry = find(key);
	if (entry != NULL)
	{
		if (m_objectOwner)
         destroyObject(entry->value);
      entry->value = value;
	}
	else
	{
      entry = (HashMapEntry *)malloc(sizeof(HashMapEntry));
      if (m_keylen <= 16)
         memcpy(entry->key.d, key, m_keylen);
      else
         entry->key.p = nx_memdup(key, m_keylen);
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
   return (entry != NULL) ? entry->value : NULL;
}

/**
 * Delete value
 */
void HashMapBase::_remove(const void *key)
{
   HashMapEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   if (entry != NULL)
   {
      HASH_DEL(m_data, entry);
      DELETE_KEY(this, entry);
		if (m_objectOwner)
         destroyObject(entry->value);
      free(entry);
   }
}

/**
 * Enumerate entries
 * Returns true if whole map was enumerated and false if enumeration was aborted by callback.
 */
EnumerationCallbackResult HashMapBase::forEach(EnumerationCallbackResult (*cb)(const void *, const void *, void *), void *userData) const
{
   EnumerationCallbackResult result = _CONTINUE;
   HashMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(GET_KEY(entry), entry->value, userData) == _STOP)
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
const void *HashMapBase::findElement(bool (*comparator)(const void *, const void *, void *), void *userData) const
{
   const void *result = NULL;
   HashMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (comparator(GET_KEY(entry), entry->value, userData))
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
 * Hash map iterator
 */
HashMapIterator::HashMapIterator(HashMapBase *hashMap)
{
   m_hashMap = hashMap;
   m_curr = NULL;
   m_next = NULL;
}

/**
 * Next element availability indicator
 */
bool HashMapIterator::hasNext()
{
   if (m_hashMap->m_data == NULL)
      return false;

   return (m_curr != NULL) ? (m_next != NULL) : true;
}

/**
 * Get next element
 */
void *HashMapIterator::next()
{
   if (m_hashMap->m_data == NULL)
      return NULL;

   if (m_curr == NULL)  // iteration not started
   {
      HASH_ITER_START(hh, m_hashMap->m_data, m_curr, m_next);
   }
   else
   {
      if (m_next == NULL)
         return NULL;

      HASH_ITER_NEXT(hh, m_curr, m_next);
   }
   return m_curr->value;
}

/**
 * Remove current element
 */
void HashMapIterator::remove()
{
   if (m_curr == NULL)
      return;

   HASH_DEL(m_hashMap->m_data, m_curr);
   DELETE_KEY(m_hashMap, m_curr);
   if (m_hashMap->m_objectOwner)
      m_hashMap->destroyObject(m_curr->value);
   free(m_curr);
}
