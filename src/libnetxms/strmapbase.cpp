/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: strmapbase.cpp
**
**/

#include "libnetxms.h"
#include "strmap-internal.h"

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

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
StringMapBase::StringMapBase(bool objectOwner)
{
	m_size = 0;
	m_data = NULL;
	m_objectOwner = objectOwner;
   m_ignoreCase = true;
	m_objectDestructor = ObjectDestructor;
}

/**
 * Destructor
 */
StringMapBase::~StringMapBase()
{
	clear();
}

/**
 * Clear map
 */
void StringMapBase::clear()
{
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      HASH_DEL(m_data, entry);
      free(entry->key);
      destroyObject(entry->value);
      free(entry);
   }
	m_size = 0;
}

/**
 * Find entry index by key
 */
StringMapEntry *StringMapBase::find(const TCHAR *key)
{
	if (key == NULL)
		return NULL;

   StringMapEntry *entry;
   int keyLen = (int)(_tcslen(key) * sizeof(TCHAR));
   if (m_ignoreCase)
   {
      TCHAR *ukey = (TCHAR *)alloca(keyLen + sizeof(TCHAR));
      memcpy(ukey, key, keyLen + sizeof(TCHAR));
      _tcsupr(ukey);
      HASH_FIND(hh, m_data, ukey, keyLen, entry);
   }
   else
   {
      HASH_FIND(hh, m_data, key, keyLen, entry);
   }
   return entry;
}

/**
 * Set value
 */
void StringMapBase::setObject(TCHAR *key, void *value, bool keyPreAllocated)
{
   if (key == NULL)
      return;

	StringMapEntry *entry = find(key);
	if (entry != NULL)
	{
		if (keyPreAllocated)
			free(key);
		if (m_objectOwner)
         destroyObject(entry->value);
      entry->value = value;
	}
	else
	{
      entry = (StringMapEntry *)malloc(sizeof(StringMapEntry));
      entry->key = keyPreAllocated ? key : _tcsdup(key);
      if (m_ignoreCase)
         _tcsupr(entry->key);
      int keyLen = (int)(_tcslen(key) * sizeof(TCHAR));
      entry->value = value;
      HASH_ADD_KEYPTR(hh, m_data, entry->key, keyLen, entry);
		m_size++;
	}
}

/**
 * Get value by key
 */
void *StringMapBase::getObject(const TCHAR *key)
{
	StringMapEntry *entry = find(key);
   return (entry != NULL) ? entry->value : NULL;
}

/**
 * Delete value
 */
void StringMapBase::remove(const TCHAR *key)
{
   StringMapEntry *entry = find(key);
   if (entry != NULL)
   {
      free(entry->key);
		if (m_objectOwner)
         destroyObject(entry->value);
      free(entry);
      m_size--;
   }
}

/**
 * Enumerate entries
 * Returns true if whole map was enumerated and false if enumeration was aborted by callback.
 */
bool StringMapBase::forEach(bool (*cb)(const TCHAR *, const void *, void *), void *userData)
{
   bool result = true;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (!cb(entry->key, entry->value, userData))
      {
         result = false;
         break;
      }
   }
   return result;
}

/**
 * Find entry
 */
const void *StringMapBase::findElement(bool (*comparator)(const TCHAR *, const void *, void *), void *userData)
{
   const void *result = NULL;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (comparator(entry->key, entry->value, userData))
      {
         result = entry->value;
         break;
      }
   }
   return result;
}
