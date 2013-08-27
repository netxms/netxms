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
bool StringSet::exist(const TCHAR *str)
{
   StringSetEntry *entry;
   int keyLen = (int)(_tcslen(str) * sizeof(TCHAR));
   HASH_FIND(hh, m_data, str, keyLen, entry);
   return entry != NULL;
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
      cb(entry->str, userData);
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
