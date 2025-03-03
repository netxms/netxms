/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

/**
 * Standard object destructor
 */
static void ObjectDestructor(void *object, StringMapBase *map)
{
	MemFree(object);
}

/**
 * Default onstructor
 */
StringMapBase::StringMapBase(Ownership objectOwner, void (*destructor)(void *, StringMapBase *))
{
	m_data = nullptr;
	m_objectOwner = (objectOwner == Ownership::True);
   m_ignoreCase = true;
   m_context = nullptr;
	m_objectDestructor = (destructor != nullptr) ? destructor : ObjectDestructor;
}

/**
 * Move constructor
 */
StringMapBase::StringMapBase(StringMapBase&& src)
{
   m_data = src.m_data;
   m_objectOwner = src.m_objectOwner;
   m_ignoreCase = src.m_ignoreCase;
   m_context = src.m_context;
   m_objectDestructor = src.m_objectDestructor;

   src.m_data = nullptr;
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
      MemFree(entry->key);
      MemFree(entry->originalKey);
      if (m_objectOwner)
         destroyObject(entry->value);
      MemFree(entry);
   }
}

/**
 * Find entry index by key
 */
StringMapEntry *StringMapBase::find(const TCHAR *key, size_t keyLen) const
{
	if (key == nullptr)
		return nullptr;

   StringMapEntry *entry;
   if (m_ignoreCase)
   {
      TCHAR *ukey = static_cast<TCHAR*>(MemAllocLocal(keyLen + sizeof(TCHAR)));
      memcpy(ukey, key, keyLen);
      *((TCHAR *)((BYTE *)ukey + keyLen)) = 0;
      _tcsupr(ukey);
      HASH_FIND(hh, m_data, ukey, (unsigned int)keyLen, entry);
      MemFreeLocal(ukey);
   }
   else
   {
      HASH_FIND(hh, m_data, key, (unsigned int)keyLen, entry);
   }
   return entry;
}

/**
 * Set value
 */
void StringMapBase::setObject(TCHAR *key, void *value, bool keyPreAllocated)
{
   if (key == nullptr)
   {
      if (m_objectOwner)
         destroyObject(value);
      return;
   }

	StringMapEntry *entry = find(key, _tcslen(key) * sizeof(TCHAR));
	if (entry != nullptr)
	{
		if (keyPreAllocated)
      {
         if (m_ignoreCase)
         {
            MemFree(entry->originalKey);
            entry->originalKey = key;
         }
         else
         {
            MemFree(key);
         }
      }
      else if (m_ignoreCase)
      {
         MemFree(entry->originalKey);
         entry->originalKey = MemCopyString(key);
      }
		if (m_objectOwner)
         destroyObject(entry->value);
      entry->value = value;
	}
	else
	{
      entry = MemAllocStruct<StringMapEntry>();
      entry->key = keyPreAllocated ? key : MemCopyString(key);
      if (m_ignoreCase)
      {
         entry->originalKey = MemCopyString(entry->key);
         _tcsupr(entry->key);
      }
      else
      {
         entry->originalKey = nullptr;
      }
      int keyLen = (int)(_tcslen(key) * sizeof(TCHAR));
      entry->value = value;
      HASH_ADD_KEYPTR(hh, m_data, entry->key, keyLen, entry);
	}
}

/**
 * Get value by key
 */
void *StringMapBase::getObject(const TCHAR *key) const
{
   if (key == nullptr)
      return nullptr;
	StringMapEntry *entry = find(key, _tcslen(key) * sizeof(TCHAR));
   return (entry != nullptr) ? entry->value : nullptr;
}

/**
 * Get value by key
 */
void *StringMapBase::getObject(const TCHAR *key, size_t len) const
{
   if (key == nullptr)
      return nullptr;
   StringMapEntry *entry = find(key, len * sizeof(TCHAR));
   return (entry != nullptr) ? entry->value : nullptr;
}

/**
 * Delete value
 */
void StringMapBase::remove(const TCHAR *key, size_t keyLen)
{
   StringMapEntry *entry = find(key, keyLen * sizeof(TCHAR));
   if (entry != nullptr)
   {
      HASH_DEL(m_data, entry);
      MemFree(entry->key);
      MemFree(entry->originalKey);
		if (m_objectOwner)
         destroyObject(entry->value);
      MemFree(entry);
   }
}

/**
 * Delete value
 */
void *StringMapBase::unlink(const TCHAR *key)
{
   StringMapEntry *entry = find(key, _tcslen(key) * sizeof(TCHAR));
   void *value;
   if (entry != nullptr)
   {
      HASH_DEL(m_data, entry);
      MemFree(entry->key);
      MemFree(entry->originalKey);
      value = entry->value;
      MemFree(entry);
   }
   else
   {
      value = nullptr;
   }
   return value;
}

/**
 * Enumerate entries
 * Returns _CONTINUE if whole map was enumerated and _STOP if enumeration was aborted by callback.
 */
EnumerationCallbackResult StringMapBase::forEach(EnumerationCallbackResult (*cb)(const TCHAR*, void*, void*), void *context)
{
   EnumerationCallbackResult result = _CONTINUE;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(m_ignoreCase ? entry->originalKey : entry->key, entry->value, context) == _STOP)
      {
         result = _STOP;
         break;
      }
   }
   return result;
}

/**
 * Enumerate entries
 * Returns _CONTINUE if whole map was enumerated and _STOP if enumeration was aborted by callback.
 */
EnumerationCallbackResult StringMapBase::forEach(EnumerationCallbackResult (*cb)(const TCHAR*, const void*, void*), void *context) const
{
   EnumerationCallbackResult result = _CONTINUE;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(m_ignoreCase ? entry->originalKey : entry->key, entry->value, context) == _STOP)
      {
         result = _STOP;
         break;
      }
   }
   return result;
}

/**
 * Enumerate entries
 * Returns _CONTINUE if whole map was enumerated and _STOP if enumeration was aborted by callback.
 */
EnumerationCallbackResult StringMapBase::forEach(std::function<EnumerationCallbackResult (const TCHAR*, void*)> cb)
{
   EnumerationCallbackResult result = _CONTINUE;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(m_ignoreCase ? entry->originalKey : entry->key, entry->value) == _STOP)
      {
         result = _STOP;
         break;
      }
   }
   return result;
}

/**
 * Enumerate entries
 * Returns _CONTINUE if whole map was enumerated and _STOP if enumeration was aborted by callback.
 */
EnumerationCallbackResult StringMapBase::forEach(std::function<EnumerationCallbackResult (const TCHAR*, const void*)> cb) const
{
   EnumerationCallbackResult result = _CONTINUE;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (cb(m_ignoreCase ? entry->originalKey : entry->key, entry->value) == _STOP)
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
const void *StringMapBase::findElement(bool (*comparator)(const TCHAR*, const void*, void*), void *context) const
{
   const void *result = nullptr;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (comparator(m_ignoreCase ? entry->originalKey : entry->key, entry->value, context))
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
const void *StringMapBase::findElement(std::function<bool (const TCHAR*, const void*)> comparator) const
{
   const void *result = nullptr;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (comparator(m_ignoreCase ? entry->originalKey : entry->key, entry->value))
      {
         result = entry->value;
         break;
      }
   }
   return result;
}

/**
 * Filter elements (delete those for which filter callback returns false)
 */
void StringMapBase::filterElements(bool (*filter)(const TCHAR *, const void *, void *), void *userData)
{
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (!filter(m_ignoreCase ? entry->originalKey : entry->key, entry->value, userData))
      {
         HASH_DEL(m_data, entry);
         MemFree(entry->key);
         MemFree(entry->originalKey);
         if (m_objectOwner)
            destroyObject(entry->value);
         MemFree(entry);
      }
   }
}

/**
 * Filter elements (return those for which filter callback return true)
 */
StructArray<KeyValuePair<void>> *StringMapBase::toArray(bool (*filter)(const TCHAR *, const void *, void *), void *userData) const
{
   StructArray<KeyValuePair<void>> *a = new StructArray<KeyValuePair<void>>();
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if ((filter == nullptr) || filter(m_ignoreCase ? entry->originalKey : entry->key, entry->value, userData))
      {
         KeyValuePair<void> p;
         p.key = m_ignoreCase ? entry->originalKey : entry->key;
         p.value = entry->value;
         a->add(&p);
      }
   }
   return a;
}

/**
 * Get list of all keys
 */
StringList StringMapBase::keys() const
{
   StringList list;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      list.add(m_ignoreCase ? entry->originalKey : entry->key);
   }
   return list;
}

/**
 * Get list of all values
 */
void StringMapBase::fillValues(Array *values) const
{
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      values->add(entry->value);
   }
}

/**
 * Get size
 */
int StringMapBase::size() const
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
   if (m_data == nullptr)
      return;  // Empty set

   StringMapEntry *data = nullptr;
   StringMapEntry *entry, *tmp;
   if (m_ignoreCase)
   {
      // switching to case ignore mode
      HASH_ITER(hh, m_data, entry, tmp)
      {
         HASH_DEL(m_data, entry);
         entry->originalKey = MemCopyString(entry->key);
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
         MemFree(entry->key);
         entry->key = entry->originalKey;
         entry->originalKey = nullptr;
         int keyLen = (int)(_tcslen(entry->key) * sizeof(TCHAR));
         HASH_ADD_KEYPTR(hh, data, entry->key, keyLen, entry);
      }
   }
   m_data = data;
}

/**
 * ********************************************************
 * 
 * String map iterator
 * 
 * ********************************************************
 */

/**
 * String map iterator constructor
 */
StringMapIterator::StringMapIterator(StringMapBase * map)
{
   m_map = map;
   m_curr = nullptr;
   m_next = nullptr;
}

/**
 * Next element availability indicator
 */
bool StringMapIterator::hasNext()
{
   if (m_map->m_data == nullptr)
      return false;

   return (m_curr != nullptr) ? (m_next != nullptr) : true;
}

/**
 * Get next element
 */
void *StringMapIterator::next()
{
   if (m_map->m_data == nullptr)
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
   {
      m_curr = m_map->m_data;
   }
   else
   {
      if (m_next == nullptr)
         return nullptr;
      m_curr = m_next;
   }
   m_next = static_cast<StringMapEntry*>(m_curr->hh.next);
   m_element.key = (m_map->m_ignoreCase ? m_curr->originalKey : m_curr->key);
   m_element.value = m_curr->value;
   return &m_element;
}

/**
 * Remove current element
 */
void StringMapIterator::remove()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_map->m_data, m_curr);
   MemFree(m_curr->key);
   MemFree(m_curr->originalKey);
   if (m_map->m_objectOwner)
      m_map->destroyObject(m_curr->value);
   MemFree(m_curr);
}

/**
 * Remove current element without destroying it
 */
void StringMapIterator::unlink()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_map->m_data, m_curr);
   MemFree(m_curr->key);
   MemFree(m_curr->originalKey);
   MemFree(m_curr);
}

/**
 * Check iterators equality
 */
bool StringMapIterator::equals(AbstractIterator* other)
{
   if (other == nullptr)
      return false;

   auto otherIterator = static_cast<StringMapIterator*>(other);
   const TCHAR *data = key();
   const TCHAR *otherData = otherIterator->key();

   if (data == nullptr && otherData == nullptr)
      return true;

   if (data == nullptr || otherData == nullptr)
      return false;

   return _tcscmp(data, otherData) == 0;
}

/**
 * Get current value
 */
void *StringMapIterator::value()
{
   m_element.key = nullptr;
   m_element.value = nullptr;
   if (m_map == nullptr || m_map->m_data == nullptr) //no data
      return &m_element;

   if (m_curr == nullptr)  // iteration not started
   {
      m_element.key = m_map->m_data->originalKey != nullptr ? m_map->m_data->originalKey : m_map->m_data->key;
      m_element.value = m_map->m_data->value;
   }
   else
   {
      if (m_next != nullptr)
      {
         m_element.key = m_next->originalKey != nullptr ? m_next->originalKey : m_next->key;
         m_element.value = m_next->value;
      }
   }
   return &m_element;
}

/**
 * Get current key
 */
const TCHAR *StringMapIterator::key()
{
   if (m_map == nullptr || m_map->m_data == nullptr) //no data
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
      return m_map->m_data->originalKey != nullptr ? m_map->m_data->originalKey : m_map->m_data->key;

   if (m_next == nullptr)
      return nullptr;

   return m_next->originalKey != nullptr ? m_next->originalKey : m_next->key;
}
