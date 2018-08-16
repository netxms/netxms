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
 * Constructors
 */
HashSetBase::HashSetBase(unsigned int keylen)
{
	m_data = NULL;
   m_keylen = keylen;
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
	if (key == NULL)
		return false;

   HashSetEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   return entry != NULL;
}

/**
 * Put element
 */
void HashSetBase::_put(const void *key)
{
   if ((key == NULL) || _contains(key))
      return;

   HashSetEntry *entry = MemAllocStruct<HashSetEntry>();
   if (m_keylen <= 16)
      memcpy(entry->key.d, key, m_keylen);
   else
      entry->key.p = nx_memdup(key, m_keylen);
   HASH_ADD_KEYPTR(hh, m_data, GET_KEY(entry), m_keylen, entry);
}

/**
 * Remove element
 */
void HashSetBase::_remove(const void *key)
{
   HashSetEntry *entry;
   HASH_FIND(hh, m_data, key, m_keylen, entry);
   if (entry != NULL)
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
      if (cb(GET_KEY(entry), userData) == _STOP)
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
