/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2024 Raden Solutions
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
** File: hashsetbase.cpp
**
**/

#include "libnetxms.h"
#include <uthash.h>

/**
 * Entry
 */
struct HashSetEntry
{
   UT_hash_handle hh;
   union
   {
      BYTE d[16];
      void *p;
   } key;
   uint32_t count;
};

/**
 * Delete key
 */
#define DELETE_KEY(m, e) do { if ((m)->m_keylen > 16) MemFree((e)->key.p); } while(0)

/**
 * Get pointer to key
 */
#define GET_KEY(m, e) (((m)->m_keylen <= 16) ? (e)->key.d : (e)->key.p)

/**
 * Constructors
 */
HashSetBase::HashSetBase(unsigned int keylen, bool useCounter)
{
	m_data = nullptr;
   m_keylen = keylen;
   m_useCounter = useCounter;
}

/**
 * Copy constructors
 */
HashSetBase::HashSetBase(const HashSetBase &src)
{
   m_data = nullptr;
   m_keylen = src.m_keylen;
   m_useCounter = src.m_useCounter;
   copyData(src);
}

/**
 * Move constructor
 */
HashSetBase::HashSetBase(HashSetBase&& src)
{
   m_keylen = src.m_keylen;
   m_useCounter = src.m_useCounter;
   m_data = src.m_data;
   src.m_data = nullptr;
}

/**
 * Assignment
 */
HashSetBase& HashSetBase::operator =(const HashSetBase &src)
{
   assert(this != &src);
   clear();
   m_keylen = src.m_keylen;
   m_useCounter = src.m_useCounter;
   copyData(src);
   return *this;
}

/**
 * Move assignment operator
 */
HashSetBase& HashSetBase::operator =(HashSetBase&& src)
{
   assert(this != &src);
   clear();
   m_keylen = src.m_keylen;
   m_useCounter = src.m_useCounter;
   m_data = src.m_data;
   src.m_data = nullptr;
   return *this;
}

/**
 * Copy data from other hash set object (used by copy constructor and assignment operator)
 */
void HashSetBase::copyData(const HashSetBase& src)
{
   HashSetEntry *entry, *tmp;
   HASH_ITER(hh, src.m_data, entry, tmp)
   {
      HashSetEntry *newEntry = MemAllocStruct<HashSetEntry>();
      if (m_keylen <= 16)
         memcpy(newEntry->key.d, entry->key.d, m_keylen);
      else
         newEntry->key.p = MemCopyBlock(entry->key.p, m_keylen);
      newEntry->count = entry->count;
      HASH_ADD_KEYPTR(hh, m_data, GET_KEY(this, newEntry), m_keylen, newEntry);
   }
}

/**
 * Destructor
 */
HashSetBase::~HashSetBase()
{
	clear();
}

/**
 * Clear map
 */
void HashSetBase::clear()
{
   HashSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      HASH_DEL(m_data, entry);
      DELETE_KEY(this, entry);
      MemFree(entry);
   }
}

/**
 * Check if given entry presented in set
 */
bool HashSetBase::_contains(const void *key) const
{
	if (key == nullptr)
		return false;

   HashSetEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   return entry != nullptr;
}

/**
 * Put element
 */
void HashSetBase::_put(const void *key)
{
   if ((key == nullptr) || (_contains(key) && ! m_useCounter))
      return;

   HashSetEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   if (entry != nullptr)
   {
      if (m_useCounter)
         entry->count++;
   }
   else
   {
      entry = MemAllocStruct<HashSetEntry>();
      entry->count = 1;
      if (m_keylen <= 16)
         memcpy(entry->key.d, key, m_keylen);
      else
         entry->key.p = MemCopyBlock(key, m_keylen);
      HASH_ADD_KEYPTR(hh, m_data, GET_KEY(this, entry), m_keylen, entry);
   }
}

/**
 * Remove element
 */
void HashSetBase::_remove(const void *key)
{
   HashSetEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   if ((entry != nullptr) && (--entry->count == 0))
   {
      HASH_DEL(m_data, entry);
      DELETE_KEY(this, entry);
      MemFree(entry);
   }
}

/**
 * Enumerate entries
 * Returns true if whole map was enumerated and false if enumeration was aborted by callback.
 */
EnumerationCallbackResult HashSetBase::forEach(EnumerationCallbackResult (*cb)(const void *, void *), void *userData) const
{
   EnumerationCallbackResult result = _CONTINUE;
   HashSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(GET_KEY(this, entry), userData) == _STOP)
      {
         result = _STOP;
         break;
      }
   }
   return result;
}

/**
 * Get size
 */
int HashSetBase::size() const
{
   return HASH_COUNT(m_data);
}

/**
 * ********************************************************
 * 
 * Hash set iterator
 * 
 * ********************************************************
 */

/**
 * Hash set iterator
 */
HashSetIterator::HashSetIterator(const HashSetBase *hashSet)
{
   m_hashSet = const_cast<HashSetBase*>(hashSet);
   m_curr = nullptr;
   m_next = nullptr;
}

/**
 * Next element availability indicator
 */
bool HashSetIterator::hasNext()
{
   if (m_hashSet->m_data == nullptr)
      return false;

   return (m_curr != nullptr) ? (m_next != nullptr) : true;
}

/**
 * Get next element and advance iterator
 */
void *HashSetIterator::next()
{
   if (m_hashSet->m_data == nullptr)
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
   {
      m_curr = m_hashSet->m_data;
   }
   else
   {
      if (m_next == nullptr)
         return nullptr;
      m_curr = m_next;
   }
   m_next = static_cast<HashSetEntry*>(m_curr->hh.next);
   return GET_KEY(m_hashSet, m_curr);
}

/**
 * Remove current element
 */
void HashSetIterator::remove()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_hashSet->m_data, m_curr);
   DELETE_KEY(m_hashSet, m_curr);
   MemFree(m_curr);
}

/**
 * Remove current element without destroying it
 */
void HashSetIterator::unlink()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_hashSet->m_data, m_curr);
   MemFree(m_curr);
}

/**
 * Check iterators equality
 */
bool HashSetIterator::equals(AbstractIterator* other)
{
   if (other == nullptr)
      return false;

   auto otherIterator = static_cast<HashSetIterator*>(other);
   void *data = value();
   void *otherData = otherIterator->value();

   if (data == nullptr && otherData == nullptr)
      return true;

   if (data == nullptr || otherData == nullptr)
      return false;

   if (m_hashSet->m_keylen != otherIterator->m_hashSet->m_keylen)
      return false;

   return memcmp(data, otherData, m_hashSet->m_keylen) == 0;
}

/**
 * Get current value in C++ semantics (next value in Java semantics)
 */
void *HashSetIterator::value()
{
   if (m_hashSet == nullptr || m_hashSet->m_data == nullptr) // no data
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
      return GET_KEY(m_hashSet, m_hashSet->m_data);

   if (m_next == nullptr)
      return nullptr;

   return GET_KEY(m_hashSet, m_next);
}
