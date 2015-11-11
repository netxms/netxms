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
** File: strmap.cpp
**
**/

#include "libnetxms.h"
#include "strmap-internal.h"

/**
 * Copy constructor
 */
StringMap::StringMap(const StringMap &src) : StringMapBase(true)
{
	m_objectOwner = src.m_objectOwner;
   m_ignoreCase = src.m_ignoreCase;
   m_objectDestructor = src.m_objectDestructor;

   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, src.m_data, entry, tmp)
   {
      setObject(_tcsdup(m_ignoreCase ? entry->originalKey : entry->key), _tcsdup((TCHAR *)entry->value), true);
   }
}

/**
 * Destructor
 */

StringMap::~StringMap()
{
}

/**
 * Assignment
 */
StringMap& StringMap::operator =(const StringMap &src)
{
	clear();
	m_objectOwner = src.m_objectOwner;
   m_ignoreCase = src.m_ignoreCase;
   m_objectDestructor = src.m_objectDestructor;

   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, src.m_data, entry, tmp)
   {
      setObject(_tcsdup(m_ignoreCase ? entry->originalKey : entry->key), _tcsdup((TCHAR *)entry->value), true);
   }
	return *this;
}

/**
 * Add all values from another string map
 */
void StringMap::addAll(StringMap *src)
{
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, src->m_data, entry, tmp)
   {
      setObject(_tcsdup(src->m_ignoreCase ? entry->originalKey : entry->key), _tcsdup((TCHAR *)entry->value), true);
   }
}

/**
 * Set value from UINT32
 */
void StringMap::set(const TCHAR *key, UINT32 value)
{
	TCHAR buffer[32];

	_sntprintf(buffer, 32, _T("%u"), (unsigned int)value);
	set(key, buffer);
}

/**
 * Get value by key as UINT32
 */
UINT32 StringMap::getULong(const TCHAR *key, UINT32 defaultValue) const
{
	const TCHAR *value = get(key);
	if (value == NULL)
		return defaultValue;
	return _tcstoul(value, NULL, 0);
}

/**
 * Get value by key as boolean
 */
bool StringMap::getBoolean(const TCHAR *key, bool defaultValue) const
{
	const TCHAR *value = get(key);
	if (value == NULL)
		return defaultValue;
	if (!_tcsicmp(value, _T("false")))
		return false;
	if (!_tcsicmp(value, _T("true")))
		return true;
	return (_tcstoul(value, NULL, 0) != 0) ? true : false;
}

/**
 * Fill NXCP message with map data
 */
void StringMap::fillMessage(NXCPMessage *msg, UINT32 sizeFieldId, UINT32 baseFieldId) const
{
   msg->setField(sizeFieldId, (UINT32)size());
   UINT32 id = baseFieldId;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      msg->setField(id++, m_ignoreCase ? entry->originalKey : entry->key);
      msg->setField(id++, (TCHAR *)entry->value);
   }
}
