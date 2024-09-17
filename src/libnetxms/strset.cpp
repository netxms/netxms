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
** File: strset.cpp
**
**/

#include "libnetxms.h"
#include <nxcpapi.h>
#include <uthash.h>

/**
 * Entry
 */
struct StringSetEntry
{
   UT_hash_handle hh;
   TCHAR *str;
   int count;
};

/**
 * Constructor
 */
StringSet::StringSet(bool counting)
{
   m_data = nullptr;
   m_counting = counting;
}

/**
 * Destructor
 */
StringSet::~StringSet()
{
   clear();
}

/**
 * Add string to set
 */
int StringSet::add(const TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = static_cast<int>(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   if (entry == nullptr)
   {
      entry = MemAllocStruct<StringSetEntry>();
      entry->str = MemCopyString(str);
      entry->count = 1;
      HASH_ADD_KEYPTR(hh, m_data, entry->str, keyLen, entry);
   }
   else if (m_counting)
   {
      entry->count++;
   }
   return entry->count;
}

/**
 * Add string to set - must be allocated by malloc()
 */
int StringSet::addPreallocated(TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = static_cast<int>(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   if (entry == nullptr)
   {
      entry = MemAllocStruct<StringSetEntry>();
      entry->str = str;
      entry->count = 1;
      HASH_ADD_KEYPTR(hh, m_data, entry->str, keyLen, entry);
   }
   else
   {
      MemFree(str);
      if (m_counting)
         entry->count++;
   }
   return entry->count;
}

/**
 * Remove string from set
 */
int StringSet::remove(const TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = static_cast<int>(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   if (entry == nullptr)
      return 0;
   if (--entry->count == 0)
   {
      HASH_DEL(m_data, entry);
      MemFree(entry->str);
      MemFree(entry);
      return 0;
   }
   return entry->count;
}

/**
 * Clear set
 */
void StringSet::clear()
{
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      HASH_DEL(m_data, entry);
      MemFree(entry->str);
      MemFree(entry);
   }
}

/**
 * Check if given string is in set
 */
bool StringSet::contains(const TCHAR *str) const
{
   StringSetEntry *entry;
   size_t keyLen = _tcslen(str) * sizeof(TCHAR);
   HASH_FIND(hh, m_data, str, keyLen, entry);
   return entry != nullptr;
}

/**
 * Get reference count for given string (0 if not in the set)
 */
int StringSet::count(const TCHAR *str) const
{
   StringSetEntry *entry;
   size_t keyLen = _tcslen(str) * sizeof(TCHAR);
   HASH_FIND(hh, m_data, str, keyLen, entry);
   return (entry != nullptr) ? entry->count : 0;
}

/**
 * Check if two given sets are equal
 */
bool StringSet::equals(const StringSet *s) const
{
   if (s->size() != size())
      return false;

   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (!s->contains(entry->str))
         return false;
   }
   return true;
}

/**
 * Get set size
 */
size_t StringSet::size() const
{
   return HASH_COUNT(m_data);
}

/**
 * Enumerate entries
 */
void StringSet::forEach(bool (*cb)(const TCHAR*, void*), void *context) const
{
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (!cb(entry->str, context))
         break;
   }
}

/**
 * Enumerate entries
 */
void StringSet::forEach(std::function<bool(const TCHAR*)> cb) const
{
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (!cb(entry->str))
         break;
   }
}

/**
 * Split source string and add all elements
 */
void StringSet::splitAndAdd(const TCHAR *src, const TCHAR *separator)
{
   int slen = (int)_tcslen(separator);
   if (slen == 0)
   {
      add(src);
      return;
   }

   const TCHAR *curr = src;
   while(curr != nullptr)
   {
      const TCHAR *next = _tcsstr(curr, separator);
      if (next != nullptr)
      {
         int len = (int)(next - curr);
         TCHAR *value = (TCHAR *)malloc((len + 1) * sizeof(TCHAR));
         memcpy(value, curr, len * sizeof(TCHAR));
         value[len] = 0;
         addPreallocated(value);
         next += slen;
      }
      else
      {
         add(curr);
      }
      curr = next;
   }
}

/**
 * Add all entries from source set
 */
void StringSet::addAll(const StringSet *src)
{
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, src->m_data, entry, tmp)
   {
      add(entry->str);
   }
}

/**
 * Add all entries from source list
 */
void StringSet::addAll(const StringList *src)
{
   for(int i = 0; i < src->size(); i++)
      add(src->get(i));
}

/**
 * Add all entries from TCHAR pointer arrays
 */
void StringSet::addAll(TCHAR **strings, int count)
{
   for(int i = 0; i < count; i++)
      if (strings[i] != nullptr)
         add(strings[i]);
}

/**
 * Add all entries from TCHAR pointer arrays. Takes ownership of pre-allocated strings.
 */
void StringSet::addAllPreallocated(TCHAR **strings, int count)
{
   for(int i = 0; i < count; i++)
      if (strings[i] != nullptr)
         addPreallocated(strings[i]);
}

/**
 * Fill NXCP message with string set data
 *
 * @param msg NXCP message
 * @param baseId base ID for data fields
 * @param countId ID of field where number of strings should be placed
 */
void StringSet::fillMessage(NXCPMessage *msg, uint32_t baseId, uint32_t countId) const
{
   uint32_t fieldId = baseId;
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      msg->setField(fieldId++, entry->str);
   }
   msg->setField(countId, fieldId - baseId);
}

/**
 * Add all strings from NXCP message
 *
 * @param msg NXCP message
 * @param baseId base ID for data fields
 * @param countId ID of field with number of strings
 * @param clearBeforeAdd if set to true, existing content will be deleted
 * @param toUppercase if set to true, all strings will be converted to upper case before adding
 */
void StringSet::addAllFromMessage(const NXCPMessage& msg, uint32_t baseId, uint32_t countId, bool clearBeforeAdd, bool toUppercase)
{
   if (clearBeforeAdd)
      clear();

   int count = msg.getFieldAsInt32(countId);
   uint32_t fieldId = baseId;
   for(int i = 0; i < count; i++)
   {
      TCHAR *str = msg.getFieldAsString(fieldId++);
      if (str != nullptr)
      {
         if (toUppercase)
            _tcsupr(str);
         addPreallocated(str);
      }
   }
}

/**
 * Add all strings from NXCP message
 *
 * @param msg NXCP message
 * @param baseId base ID for data fields
 * @param countId ID of field with number of strings
 * @param clearBeforeAdd if set to true, existing content will be deleted
 * @param toUppercase if set to true, all strings will be converted to upper case before adding
 */
void StringSet::addAllFromMessage(const NXCPMessage &msg, uint32_t fieldId, bool clearBeforeAdd, bool toUppercase)
{
   if (clearBeforeAdd)
      clear();

   size_t size;
   const BYTE *data = msg.getBinaryFieldPtr(fieldId, &size);
   ConstByteStream in(data, size);
   int count = in.readUInt16B();
   for(int i = 0; i < count; i++)
   {
      TCHAR *str = in.readNXCPString(nullptr);
      if (str != nullptr)
      {
         if (toUppercase)
            _tcsupr(str);
         addPreallocated(str);
      }
   }
}

/**
 * Get all entries as one string, optionally separated by given separator
 *
 * @parm separator optional separator, may be NULL
 */
String StringSet::join(const TCHAR *separator)
{
   StringBuffer result;
   result.setAllocationStep(4096);
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if ((separator != nullptr) && (result.length() > 0))
         result.append(separator);
      result.append(entry->str);
   }
   return result;
}

/**
 * ********************************************************
 * 
 * String set iterator
 * 
 * ********************************************************
 */

/**
 * String set iterator
 */
StringSetIterator::StringSetIterator(const StringSet *stringSet)
{
   m_stringSet = const_cast<StringSet*>(stringSet);
   m_curr = nullptr;
   m_next = nullptr;
}

/**
 * Hash map iterator copy constructor
 */
StringSetIterator::StringSetIterator(const StringSetIterator& other)
{
   m_stringSet = other.m_stringSet;
   m_curr = other.m_curr;
   m_next = other.m_next;
}

/**
 * Next element availability indicator
 */
bool StringSetIterator::hasNext()
{
   if (m_stringSet->m_data == nullptr)
      return false;

   return (m_curr != nullptr) ? (m_next != nullptr) : true;
}

/**
 * Get next element
 */
void *StringSetIterator::next()
{
   if (m_stringSet->m_data == nullptr)
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
   {
      m_curr = m_stringSet->m_data;
   }
   else
   {
      if (m_next == nullptr)
         return nullptr;
      m_curr = m_next;
   }
   m_next = static_cast<StringSetEntry*>(m_curr->hh.next);
   return m_curr->str;
}

/**
 * Remove current element
 */
void StringSetIterator::remove()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_stringSet->m_data, m_curr);
   MemFree(m_curr->str);
   MemFree(m_curr);
}

/**
 * Remove current element without destroying it
 */
void StringSetIterator::unlink()
{
   if (m_curr == nullptr)
      return;

   HASH_DEL(m_stringSet->m_data, m_curr);
   MemFree(m_curr);
}

/**
 * Check iterators equality
 */
bool StringSetIterator::equals(AbstractIterator *other)
{
   if (other == nullptr)
      return false;

   auto otherIterator = static_cast<StringSetIterator*>(other);
   const TCHAR *data = static_cast<const TCHAR*>(value());
   const TCHAR *otherData = static_cast<const TCHAR*>(otherIterator->value());

   if (data == nullptr && otherData == nullptr)
      return true;

   if (data == nullptr || otherData == nullptr)
      return false;

   return _tcscmp(data, otherData) == 0;
}

/**
 * Get current value
 */
void *StringSetIterator::value()
{
   if (m_stringSet == nullptr || m_stringSet->m_data == nullptr) //no data
      return nullptr;

   if (m_curr == nullptr)  // iteration not started
      return m_stringSet->m_data->str;

   if (m_next == nullptr)
      return nullptr;

   return m_next->str;
}
