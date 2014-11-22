/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
bool StringSet::contains(const TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = (int)(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   return entry != NULL;
}

/**
 * Check if two given sets are equal
 */
bool StringSet::equals(StringSet *s)
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
int StringSet::size()
{
   return HASH_COUNT(m_data);
}

/**
 * Enumerate entries
 */
void StringSet::forEach(bool (*cb)(const TCHAR *, void *), void *userData)
{
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if (!cb(entry->str, userData))
         break;
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
void StringSet::fillMessage(NXCPMessage *msg, UINT32 baseId, UINT32 countId)
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
void StringSet::addAllFromMessage(NXCPMessage *msg, UINT32 baseId, UINT32 countId, bool clearBeforeAdd, bool toUppercase)
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
String StringSet::getAll(const TCHAR *separator)
{
   String result;
   StringSetEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      if ((separator != NULL) && (result.getSize() > 0))
         result += separator;
      result += entry->str;
   }
   return result;
}
