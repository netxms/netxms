/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
};

/**
 * Constructor
 */
StringSet::StringSet()
{
   m_data = NULL;
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
void StringSet::add(const TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = (int)(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   if (entry == NULL)
   {
      entry = (StringSetEntry *)malloc(sizeof(StringSetEntry));
      entry->str = _tcsdup(str);
      HASH_ADD_KEYPTR(hh, m_data, entry->str, keyLen, entry);
   }
}

/**
 * Add string to set - must be allocated by malloc()
 */
void StringSet::addPreallocated(TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = (int)(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   if (entry == NULL)
   {
      entry = (StringSetEntry *)malloc(sizeof(StringSetEntry));
      entry->str = str;
      HASH_ADD_KEYPTR(hh, m_data, entry->str, keyLen, entry);
   }
   else
   {
      free(str);
   }
}

/**
 * Remove string from set
 */
void StringSet::remove(const TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = (int)(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   if (entry != NULL)
   {
      HASH_DEL(m_data, entry);
      free(entry->str);
      free(entry);
   }
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
      free(entry->str);
      free(entry);
   }
}

/**
 * Check if given string is in set
 */
bool StringSet::contains(const TCHAR *str) const
{
   StringSetEntry *entry;
   int keyLen = (int)(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   return entry != NULL;
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
int StringSet::size() const
{
   return HASH_COUNT(m_data);
}

/**
 * Enumerate entries
 */
void StringSet::forEach(bool (*cb)(const TCHAR *, void *), void *userData) const
{
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (!cb(entry->str, userData))
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
   while(curr != NULL)
   {
      const TCHAR *next = _tcsstr(curr, separator);
      if (next != NULL)
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
void StringSet::addAll(StringSet *src)
{
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, src->m_data, entry, tmp)
   {
      add(entry->str);
   }
}

/**
 * Add all entries from TCHAR pointer arrays
 */
void StringSet::addAll(TCHAR **strings, int count)
{
   for(int i = 0; i < count; i++)
      if (strings[i] != NULL)
         add(strings[i]);
}

/**
 * Add all entries from TCHAR pointer arrays. Takes ownership of pre-allocated strings.
 */
void StringSet::addAllPreallocated(TCHAR **strings, int count)
{
   for(int i = 0; i < count; i++)
      if (strings[i] != NULL)
         addPreallocated(strings[i]);
}

/**
 * Fill NXCP message with string set data
 *
 * @param msg NXCP message
 * @param baseId base ID for data fields
 * @param countId ID of field where number of strings should be placed
 */
void StringSet::fillMessage(NXCPMessage *msg, UINT32 baseId, UINT32 countId) const
{
   UINT32 varId = baseId;
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      msg->setField(varId++, entry->str);
   }
   msg->setField(countId, varId - baseId);
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
void StringSet::addAllFromMessage(const NXCPMessage *msg, UINT32 baseId, UINT32 countId, bool clearBeforeAdd, bool toUppercase)
{
   if (clearBeforeAdd)
      clear();

   int count = (int)msg->getFieldAsUInt32(countId);
   UINT32 varId = baseId;
   for(int i = 0; i < count; i++)
   {
      TCHAR *str = msg->getFieldAsString(varId++);
      if (str != NULL)
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
   String result;
   result.setAllocationStep(4096);
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if ((separator != NULL) && (result.length() > 0))
         result += separator;
      result += entry->str;
   }
   return result;
}

/**
 * Hash map iterator
 */
StringSetIterator::StringSetIterator(StringSet *stringSet)
{
   m_stringSet = stringSet;
   m_curr = NULL;
   m_next = NULL;
}

/**
 * Next element availability indicator
 */
bool StringSetIterator::hasNext()
{
   if (m_stringSet->m_data == NULL)
      return false;

   return (m_curr != NULL) ? (m_next != NULL) : true;
}

/**
 * Get next element
 */
void *StringSetIterator::next()
{
   if (m_stringSet->m_data == NULL)
      return NULL;

   if (m_curr == NULL)  // iteration not started
   {
      HASH_ITER_START(hh, m_stringSet->m_data, m_curr, m_next);
   }
   else
   {
      if (m_next == NULL)
         return NULL;

      HASH_ITER_NEXT(hh, m_curr, m_next);
   }
   return m_curr->str;
}

/**
 * Remove current element
 */
void StringSetIterator::remove()
{
   if (m_curr == NULL)
      return;

   HASH_DEL(m_stringSet->m_data, m_curr);
   free(m_curr->str);
   free(m_curr);
}
