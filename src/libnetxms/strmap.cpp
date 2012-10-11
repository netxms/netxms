/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

/**
 * Copy constructor
 */
StringMap::StringMap(const StringMap &src) : StringMapBase(true)
{
	DWORD i;

	m_size = src.m_size;
	m_objectOwner = src.m_objectOwner;
	m_keys = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
	m_values = (void **)malloc(sizeof(void *) * m_size);
	for(i = 0; i < m_size; i++)
	{
		m_keys[i] = _tcsdup(src.m_keys[i]);
		m_values[i] = _tcsdup((TCHAR *)src.m_values[i]);
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
	DWORD i;

	clear();
	m_size = src.m_size;
	m_keys = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
	m_values = (void **)malloc(sizeof(void *) * m_size);
	for(i = 0; i < m_size; i++)
	{
		m_keys[i] = _tcsdup(src.m_keys[i]);
		m_values[i] = _tcsdup((TCHAR *)src.m_values[i]);
	}
	return *this;
}

/**
 * Set value from DWORD
 */
void StringMap::set(const TCHAR *key, DWORD value)
{
	TCHAR buffer[32];

	_sntprintf(buffer, 32, _T("%u"), (unsigned int)value);
	set(key, buffer);
}

/**
 * Get value by key as DWORD
 */
DWORD StringMap::getULong(const TCHAR *key, DWORD defaultValue)
{
	const TCHAR *value = get(key);
	if (value == NULL)
		return defaultValue;
	return _tcstoul(value, NULL, 0);
}

/**
 * Get value by key as boolean
 */
bool StringMap::getBoolean(const TCHAR *key, bool defaultValue)
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
