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
** File: stringlist.cpp
**
**/

#include "libnetxms.h"

#define ALLOCATION_STEP		16

/**
 * Constructor: create empty string list
 */
StringList::StringList()
{
	m_count = 0;
	m_allocated = ALLOCATION_STEP;
	m_values = (TCHAR **)malloc(sizeof(TCHAR *) * m_allocated);
	memset(m_values, 0, sizeof(TCHAR *) * m_allocated);
}

/**
 * Constructor: create copy of existing string list
 */
StringList::StringList(StringList *src)
{
	m_count = 0;
   m_allocated = src->m_allocated;
	m_values = (TCHAR **)malloc(sizeof(TCHAR *) * m_allocated);
	memset(m_values, 0, sizeof(TCHAR *) * m_allocated);
   addAll(src);
}

/**
 * Constructor: create string list by splitting source string at separators
 */
StringList::StringList(const TCHAR *src, const TCHAR *separator)
{
	m_count = 0;
	m_allocated = ALLOCATION_STEP;
	m_values = (TCHAR **)malloc(sizeof(TCHAR *) * m_allocated);
	memset(m_values, 0, sizeof(TCHAR *) * m_allocated);
   splitAndAdd(src, separator);
}

/**
 * Destructor
 */
StringList::~StringList()
{
	for(int i = 0; i < m_count; i++)
		safe_free(m_values[i]);
	safe_free(m_values);
}

/**
 * Clear list
 */
void StringList::clear()
{
	for(int i = 0; i < m_count; i++)
		safe_free(m_values[i]);
	m_count = 0;
	memset(m_values, 0, sizeof(TCHAR *) * m_allocated);
}

/**
 * Add preallocated string to list
 */
void StringList::addPreallocated(TCHAR *value)
{
	if (m_allocated == m_count)
	{
		m_allocated += ALLOCATION_STEP;
		m_values = (TCHAR **)realloc(m_values, sizeof(TCHAR *) * m_allocated);
	}
	m_values[m_count++] = (value != NULL) ? value : _tcsdup(_T(""));
}

/**
 * Add string to list
 */
void StringList::add(const TCHAR *value)
{
	addPreallocated(_tcsdup(value));
}

/**
 * Add signed 32-bit integer as string
 */
void StringList::add(INT32 value)
{
	TCHAR buffer[32];

	_sntprintf(buffer, 32, _T("%d"), (int)value);
	add(buffer);
}

/**
 * Add unsigned 32-bit integer as string
 */
void StringList::add(UINT32 value)
{
	TCHAR buffer[32];

	_sntprintf(buffer, 32, _T("%u"), (unsigned int)value);
	add(buffer);
}

/**
 * Add signed 64-bit integer as string
 */
void StringList::add(INT64 value)
{
	TCHAR buffer[32];

	_sntprintf(buffer, 32, INT64_FMT, value);
	add(buffer);
}

/**
 * Add unsigned 64-bit integer as string
 */
void StringList::add(UINT64 value)
{
	TCHAR buffer[32];

	_sntprintf(buffer, 32, UINT64_FMT, value);
	add(buffer);
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

	safe_free(m_values[index]);
	m_values[index] = _tcsdup(value);
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
	   safe_free(m_values[index]);
      m_values[index] = _tcsdup(value);
   }
   else
   {
      for(int i = m_count; i < index; i++)
         add(_T(""));
      add(value);
   }
}

/**
 * Add or replace string at given position
 */
void StringList::addOrReplacePreallocated(int index, TCHAR *value)
{
   if (index < 0)
      return;

   if (index < m_count)
   {
	   safe_free(m_values[index]);
      m_values[index] = value;
   }
   else
   {
      for(int i = m_count; i < index; i++)
         add(_T(""));
      addPreallocated(value);
   }
}

/**
 * Get index of given value. Returns zero-based index ot -1 
 * if given value not found in the list. If list contains duplicate values,
 * index of first occurence will be returned.
 */
int StringList::indexOf(const TCHAR *value)
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
int StringList::indexOfIgnoreCase(const TCHAR *value)
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

	safe_free(m_values[index]);
	m_count--;
	memmove(&m_values[index], &m_values[index + 1], (m_count - index) * sizeof(TCHAR *));
}

/**
 * Add all values from another list
 */
void StringList::addAll(const StringList *src)
{
   for(int i = 0; i < src->m_count; i++)
      add(src->m_values[i]);
}

/**
 * Merge with another list
 */
void StringList::merge(const StringList *src, bool matchCase)
{
   for(int i = 0; i < src->m_count; i++)
   {
      if ((matchCase ? indexOf(src->m_values[i]) : indexOfIgnoreCase(src->m_values[i])) == -1)
         add(src->m_values[i]);
   }
}

/**
 * Join all elements into single string. Resulted string is dynamically allocated
 * and must be destroyed by caller with free() call. For empty list empty string will be returned.
 *
 * @param separator separator string
 * @return string with all list elements concatenated
 */
TCHAR *StringList::join(const TCHAR *separator)
{
   if (m_count == 0)
      return _tcsdup(_T(""));

   int i, len = 0;
   for(i = 0; i < m_count; i++)
      len += (int)_tcslen(m_values[i]);
   TCHAR *result = (TCHAR *)malloc((len + _tcslen(separator) * (m_count - 1) + 1) * sizeof(TCHAR));
   _tcscpy(result, m_values[0]);
   for(i = 1; i < m_count; i++)
   {
      _tcscat(result, separator);
      _tcscat(result, m_values[i]);
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
