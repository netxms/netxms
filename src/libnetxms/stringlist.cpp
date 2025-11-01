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
** File: stringlist.cpp
**
**/

#include "libnetxms.h"
#include <nxcpapi.h>

#define INITIAL_CAPACITY   256

#define CHECK_ALLOCATION do { \
   if (m_allocated == m_count) { \
      int step = std::min(4096, m_allocated); \
      m_allocated += step; \
      TCHAR **values = m_pool.allocateArray<TCHAR*>(m_allocated); \
      memcpy(values, m_values, (m_allocated - step) * sizeof(TCHAR*)); \
      m_values = values; \
   } } while(0)

/**
 * Constructor: create empty string list
 */
StringList::StringList()
{
	m_count = 0;
	m_allocated = INITIAL_CAPACITY;
	m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
}

/**
 * Constructor: create copy of existing string list. Null-safe, passing null pointer will create empty string list.
 */
StringList::StringList(const StringList *src)
{
	m_count = 0;
   m_allocated = (src != nullptr) ? src->m_allocated : INITIAL_CAPACITY;
   m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
   addAll(src);
}

/**
 * Copy constructor
 */
StringList::StringList(const StringList &src)
{
   m_count = 0;
   m_allocated = src.m_allocated;
   m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
   addAll(&src);
}

/**
 * Move constructor
 */
StringList::StringList(StringList&& src) : m_pool(std::move(src.m_pool))
{
   m_count = src.m_count;
   m_allocated = src.m_allocated;
   m_values = src.m_values;
   src.m_count = 0;
   src.m_allocated = 0;
   src.m_values = nullptr;
}

/**
 * Constructor: create string list by splitting source string at separators
 */
StringList::StringList(const TCHAR *src, const TCHAR *separator)
{
	m_count = 0;
	m_allocated = INITIAL_CAPACITY;
   m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
   splitAndAdd(src, separator);
}

/**
 * Constructor: create string list from NXCP message
 */
StringList::StringList(const NXCPMessage& msg, uint32_t baseId, uint32_t countId)
{
   m_count = msg.getFieldAsInt32(countId);
   m_allocated = m_count;
   m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
   uint32_t fieldId = baseId;
   for(int i = 0; i < m_count; i++)
   {
      m_values[i] = msg.getFieldAsString(fieldId++, &m_pool);
      if (m_values[i] == nullptr)
         m_values[i] = m_pool.copyString(_T(""));
   }
}

/**
 * Constructor: create string list from NXCP message
 */
StringList::StringList(const NXCPMessage& msg, uint32_t fieldId)
{
   size_t size;
   const BYTE *data = msg.getBinaryFieldPtr(fieldId, &size);
   ConstByteStream in(data, size);
   m_count = in.readUInt16B();
   m_allocated = m_count;
   m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
   for(int i = 0; i < m_count; i++)
   {
      m_values[i] = in.readNXCPString(&m_pool);
      if (m_values[i] == nullptr)
         m_values[i] = m_pool.copyString(_T(""));
   }
}

/**
 * Constructor: create string list from JSON array
 */
StringList::StringList(json_t *json)
{
   if (json_is_array(json) && (json_array_size(json) > 0))
   {
      m_count = static_cast<int>(json_array_size(json));
      m_allocated = m_count;
      m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
      for(int i = 0; i < m_count; i++)
      {
         json_t *e = json_array_get(json, i);
         if (json_is_string(e))
         {
            const char *s = json_string_value(e);
            if ((s != nullptr) && (*s != 0))
            {
               size_t l = strlen(s) + 1;
               m_values[i] = m_pool.allocateString(l);
               utf8_to_tchar(s, -1, m_values[i], l);
            }
            else
            {
               m_values[i] = m_pool.copyString(_T(""));
            }
         }
         else
         {
            m_values[i] = m_pool.copyString(_T(""));
         }
      }
   }
   else
   {
      m_count = 0;
      m_allocated = INITIAL_CAPACITY;
      m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
   }
}

/**
 * Move assignment operator
 */
StringList& StringList::operator=(StringList&& src)
{
   clear();
   m_pool = std::move(src.m_pool);
   m_count = src.m_count;
   m_allocated = src.m_allocated;
   m_values = src.m_values;
   src.m_count = 0;
   src.m_allocated = 0;
   src.m_values = nullptr;
   return *this;
}

/**
 * Clear list
 */
void StringList::clear()
{
	m_pool.clear();
   m_count = 0;
   m_allocated = INITIAL_CAPACITY;
   m_values = m_pool.allocateArray<TCHAR*>(m_allocated);
}

/**
 * Add string to list
 */
void StringList::add(const TCHAR *value)
{
   CHECK_ALLOCATION;
   m_values[m_count++] = m_pool.copyString(value);
}

#ifdef UNICODE

/**
 * Add multibyte string to list
 */
void StringList::addMBString(const char *value)
{
   CHECK_ALLOCATION;
   size_t l = strlen(value);
   WCHAR *s = m_pool.allocateArray<WCHAR>(l + 1);
   mb_to_wchar(value, -1, s, static_cast<int>(l + 1));
   m_values[m_count++] = s;
}

#endif

/**
 * Add signed 32-bit integer as string
 */
void StringList::add(int32_t value)
{
	TCHAR buffer[32];
	add(IntegerToString(value, buffer));
}

/**
 * Add unsigned 32-bit integer as string
 */
void StringList::add(uint32_t value)
{
	TCHAR buffer[32];
   add(IntegerToString(value, buffer));
}

/**
 * Add signed 64-bit integer as string
 */
void StringList::add(int64_t value)
{
	TCHAR buffer[32];
   add(IntegerToString(value, buffer));
}

/**
 * Add unsigned 64-bit integer as string
 */
void StringList::add(uint64_t value)
{
	TCHAR buffer[32];
   add(IntegerToString(value, buffer));
}

/**
 * Add floating point number as string
 */
void StringList::add(double value)
{
	TCHAR buffer[64];
	_sntprintf(buffer, 64, _T("%f"), value);
	add(buffer);
}

/**
 * Replace string at given position
 */
void StringList::replace(int index, const TCHAR *value)
{
	if ((index < 0) || (index >= m_count))
		return;

	m_values[index] = m_pool.copyString(value);
}

/**
 * Add or replace string at given position
 */
void StringList::addOrReplace(int index, const TCHAR *value)
{
   if (index < 0)
      return;

   if (index < m_count)
   {
      m_values[index] = m_pool.copyString(value);
   }
   else
   {
      for(int i = m_count; i < index; i++)
      {
         CHECK_ALLOCATION;
         m_values[m_count++] = nullptr;
      }
      add(value);
   }
}

/**
 * Add or replace string at given position
 */
void StringList::addOrReplacePreallocated(int index, TCHAR *value)
{
   addOrReplace(index, value);
   MemFree(value);
}

/**
 * Insert string into list
 */
void StringList::insert(int pos, const TCHAR *value)
{
   if ((pos < 0) || (pos > m_count))
      return;

   CHECK_ALLOCATION;

   if (pos < m_count)
      memmove(&m_values[pos + 1], &m_values[pos], (m_count - pos) * sizeof(TCHAR*));
   m_count++;
   m_values[pos] = m_pool.copyString(value);
}

#ifdef UNICODE

/**
 * Insert multibyte string to list
 */
void StringList::insertMBString(int pos, const char *value)
{
   if ((pos < 0) || (pos > m_count))
      return;

   CHECK_ALLOCATION;

   size_t l = strlen(value);
   WCHAR *s = m_pool.allocateArray<WCHAR>(l + 1);
   mb_to_wchar(value, -1, s, static_cast<int>(l + 1));
   if (pos < m_count)
      memmove(&m_values[pos + 1], &m_values[pos], (m_count - pos) * sizeof(TCHAR*));
   m_count++;
   m_values[pos] = s;
}

#endif

/**
 * Get index of given value. Returns zero-based index ot -1 
 * if given value not found in the list. If list contains duplicate values,
 * index of first occurence will be returned.
 */
int StringList::indexOf(const TCHAR *value) const
{
	for(int i = 0; i < m_count; i++)
		if ((m_values[i] != NULL) && !_tcscmp(m_values[i], value))
			return i;
	return -1;
}

/**
 * Get index of given value using case-insensetive compare. Returns zero-based index ot -1 
 * if given value not found in the list. If list contains duplicate values,
 * index of first occurence will be returned.
 */
int StringList::indexOfIgnoreCase(const TCHAR *value) const
{
	for(int i = 0; i < m_count; i++)
		if ((m_values[i] != NULL) && !_tcsicmp(m_values[i], value))
			return i;
	return -1;
}

/**
 * Remove element at given index
 */
void StringList::remove(int index)
{
	if ((index < 0) || (index >= m_count))
		return;

	m_count--;
	memmove(&m_values[index], &m_values[index + 1], (m_count - index) * sizeof(TCHAR *));
}

/**
 * Add all values from another list
 */
void StringList::addAll(const StringList *src)
{
   if (src != nullptr)
      for(int i = 0; i < src->m_count; i++)
         add(src->m_values[i]);
}

/**
 * Insert all values from another list
 */
void StringList::insertAll(int pos, const StringList *src)
{
   if (src != nullptr)
      for(int i = 0; i < src->m_count; i++)
         insert(pos++, src->m_values[i]);
}

/**
 * Merge with another list
 */
void StringList::merge(const StringList& src, bool matchCase)
{
   for(int i = 0; i < src.m_count; i++)
   {
      if ((matchCase ? indexOf(src.m_values[i]) : indexOfIgnoreCase(src.m_values[i])) == -1)
         add(src.m_values[i]);
   }
}

/**
 * Join all elements into single string. Resulted string is dynamically allocated
 * and must be destroyed by caller with free() call. For empty list empty string will be returned.
 *
 * @param separator separator string
 * @return string with all list elements concatenated
 */
TCHAR *StringList::join(const TCHAR *separator) const
{
   if (m_count == 0)
      return MemCopyString(_T(""));

   int i;
   size_t len = 0;
   for(i = 0; i < m_count; i++)
      len += _tcslen(m_values[i]);
   TCHAR *result = MemAllocString(len + _tcslen(separator) * (m_count - 1) + 1);
   _tcscpy(result, m_values[0]);
   for(i = 1; i < m_count; i++)
   {
      _tcscat(result, separator);
      _tcscat(result, CHECK_NULL_EX(m_values[i]));
   }
   return result;
}

/**
 * Split source string into pieces using given separator and add all
 * resulting substrings to the list.
 */
void StringList::splitAndAdd(const TCHAR *src, const TCHAR *separator)
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
         TCHAR *value = m_pool.allocateArray<TCHAR>(len + 1);
         memcpy(value, curr, len * sizeof(TCHAR));
         value[len] = 0;
         CHECK_ALLOCATION;
         m_values[m_count++] = value;
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
 * Comparator for qsort - ascending, case sensitive
 */
static int sortcb_asc_case(const void *s1, const void *s2)
{
   return _tcscmp(*static_cast<TCHAR const * const *>(s1), *static_cast<TCHAR const * const *>(s2));
}

/**
 * Comparator for qsort - ascending, case insensitive
 */
static int sortcb_asc_nocase(const void *s1, const void *s2)
{
   return _tcsicmp(*static_cast<TCHAR const * const *>(s1), *static_cast<TCHAR const * const *>(s2));
}

/**
 * Comparator for qsort - descending, case sensitive
 */
static int sortcb_desc_case(const void *s1, const void *s2)
{
   return -_tcscmp(*static_cast<TCHAR const * const *>(s1), *static_cast<TCHAR const * const *>(s2));
}

/**
 * Comparator for qsort - descending, case insensitive
 */
static int sortcb_desc_nocase(const void *s1, const void *s2)
{
   return -_tcsicmp(*static_cast<TCHAR const * const *>(s1), *static_cast<TCHAR const * const *>(s2));
}

/**
 * Sort string list
 */
void StringList::sort(bool ascending, bool caseSensitive)
{
   qsort(m_values, m_count, sizeof(TCHAR *),
         ascending ?
            (caseSensitive ? sortcb_asc_case : sortcb_asc_nocase) :
            (caseSensitive ? sortcb_desc_case : sortcb_desc_nocase)
      );
}

/**
 * Fill NXCP message with list data
 */
void StringList::fillMessage(NXCPMessage *msg, uint32_t baseId, uint32_t countId) const
{
   msg->setField(countId, static_cast<uint32_t>(m_count));
   uint32_t fieldId = baseId;
   for(int i = 0; i < m_count; i++)
   {
      msg->setField(fieldId++, CHECK_NULL_EX(m_values[i]));
   }
}

/**
 * Load data from NXCP message
 */
void StringList::addAllFromMessage(const NXCPMessage& msg, uint32_t baseId, uint32_t countId)
{
   int count = msg.getFieldAsInt32(countId);
   uint32_t id = baseId;
   for(int i = 0; i < count; i++)
   {
      TCHAR *value = msg.getFieldAsString(id++, &m_pool);
      if (value != nullptr)
      {
         CHECK_ALLOCATION;
         m_values[m_count++] = value;
      }
      else
      {
         add(_T(""));
      }
   }
}

/**
 * Constructor: create string list from NXCP message
 */
void StringList::addAllFromMessage(const NXCPMessage& msg, uint32_t fieldId)
{
   size_t size;
   const BYTE *data = msg.getBinaryFieldPtr(fieldId, &size);
   ConstByteStream in(data, size);
   uint16_t count = in.readUInt16B();
   for(int i = 0; i < count; i++)
   {
      TCHAR *item = in.readNXCPString(&m_pool);
      if (item != nullptr)
      {
         CHECK_ALLOCATION;
         m_values[m_count++] = item;
      }
   }
}

/**
 * Serialize to JSON
 */
json_t *StringList::toJson() const
{
   json_t *root = json_array();
   for(int i = 0; i < m_count; i++)
   {
      json_array_append_new(root, json_string_t(CHECK_NULL_EX(m_values[i])));
   }
   return root;
}
