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
      safe_free(entry->originalKey);
      destroyObject(entry->value);
      free(entry);
   }
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
      {
         if (m_ignoreCase)
         {
            free(entry->originalKey);
            entry->originalKey = key;
         }
         else
         {
			   free(key);
         }
      }
      else if (m_ignoreCase)
      {
         free(entry->originalKey);
         entry->originalKey = _tcsdup(key);
      }
		if (m_objectOwner)
         destroyObject(entry->value);
      entry->value = value;
	}
	else
	{
      entry = (StringMapEntry *)malloc(sizeof(StringMapEntry));
      entry->key = keyPreAllocated ? key : _tcsdup(key);
      if (m_ignoreCase)
      {
         entry->originalKey = _tcsdup(entry->key);
         _tcsupr(entry->key);
      }
      else
      {
         entry->originalKey = NULL;
      }
      int keyLen = (int)(_tcslen(key) * sizeof(TCHAR));
      entry->value = value;
      HASH_ADD_KEYPTR(hh, m_data, entry->key, keyLen, entry);
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
      HASH_DEL(m_data, entry);
      free(entry->key);
      safe_free(entry->originalKey);
		if (m_objectOwner)
         destroyObject(entry->value);
      free(entry);
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
      if (!cb(m_ignoreCase ? entry->originalKey : entry->key, entry->value, userData))
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
      if (comparator(m_ignoreCase ? entry->originalKey : entry->key, entry->value, userData))
      {
         result = entry->value;
         break;
      }
   }
   return result;
}

/**
 * Convert to key/value array
 */
StructArray<KeyValuePair> *StringMapBase::toArray()
{
   StructArray<KeyValuePair> *a = new StructArray<KeyValuePair>(size());
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      KeyValuePair p;
      p.key = m_ignoreCase ? entry->originalKey : entry->key;
      p.value = entry->value;
      a->add(&p);
   }
   return a;
}

/**
 * Get size
 */
int StringMapBase::size()
{
   return HASH_COUNT(m_data);
}

/**
 * Change case sensitivity mode
 */
void StringMapBase::setIgnoreCase(bool ignore)
{ 
   if (m_ignoreCase == ignore)
      return;  // No change required

   m_ignoreCase = ignore;
   if (m_data == NULL)
      return;  // Empty set

   StringMapEntry *data = NULL;
   StringMapEntry *entry, *tmp;
   if (m_ignoreCase)
   {
      // switching to case ignore mode
      HASH_ITER(hh, m_data, entry, tmp)
      {
         HASH_DEL(m_data, entry);
         entry->originalKey = _tcsdup(entry->key);
         _tcsupr(entry->key);
         int keyLen = (int)(_tcslen(entry->key) * sizeof(TCHAR));
         HASH_ADD_KEYPTR(hh, data, entry->key, keyLen, entry);
      }
   }
   else
   {
      // switching to case sensitive mode
      HASH_ITER(hh, m_data, entry, tmp)
      {
         HASH_DEL(m_data, entry);
         free(entry->key);
         entry->key = entry->originalKey;
         entry->originalKey = NULL;
         int keyLen = (int)(_tcslen(entry->key) * sizeof(TCHAR));
         HASH_ADD_KEYPTR(hh, data, entry->key, keyLen, entry);
      }
   }
   m_data = data;
}
