/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2009 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: strmap.cpp
**
**/

#include "libnetxms.h"


//
// Constructors
//

StringMap::StringMap()
{
	m_size = 0;
	m_keys = NULL;
	m_values = NULL;
}


//
// Copy constructor
//

StringMap::StringMap(const StringMap &src)
{
	DWORD i;

	m_size = src.m_size;
	m_keys = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
	m_values = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
	for(i = 0; i < m_size; i++)
	{
		m_keys[i] = _tcsdup(src.m_keys[i]);
		m_values[i] = _tcsdup(src.m_values[i]);
	}
}


//
// Destructor
//

StringMap::~StringMap()
{
	clear();
}


//
// Assignment
//

StringMap& StringMap::operator =(const StringMap &src)
{
	DWORD i;

	clear();
	m_size = src.m_size;
	m_keys = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
	m_values = (TCHAR **)malloc(sizeof(TCHAR *) * m_size);
	for(i = 0; i < m_size; i++)
	{
		m_keys[i] = _tcsdup(src.m_keys[i]);
		m_values[i] = _tcsdup(src.m_values[i]);
	}
	return *this;
}


//
// Clear map
//

void StringMap::clear()
{
	DWORD i;

	for(i = 0; i < m_size; i++)
	{
		safe_free(m_keys[i]);
		safe_free(m_values[i]);
	}
	m_size = 0;
	safe_free_and_null(m_keys);
	safe_free_and_null(m_values);
}


//
// Find value by key
//

DWORD StringMap::find(const TCHAR *key)
{
	DWORD i;

	for(i = 0; i < m_size; i++)
	{
		if (!_tcsicmp(key, m_keys[i]))
			return i;
	}
	return INVALID_INDEX;
}


//
// Set value - arguments are preallocated dynamic strings
//

void StringMap::setPreallocated(TCHAR *key, TCHAR *value)
{
	DWORD index;

	index = find(key);
	if (index != INVALID_INDEX)
	{
		free(key);	// Not needed
		safe_free(m_values[index]);
		m_values[index] = value;
	}
	else
	{
		m_keys = (TCHAR **)realloc(m_keys, (m_size + 1) * sizeof(TCHAR *));
		m_values = (TCHAR **)realloc(m_values, (m_size + 1) * sizeof(TCHAR *));
		m_keys[m_size] = key;
		m_values[m_size] = value;
		m_size++;
	}
}


//
// Set value
//

void StringMap::set(const TCHAR *key, const TCHAR *value)
{
	DWORD index;

	index = find(key);
	if (index != INVALID_INDEX)
	{
		safe_free(m_values[index]);
		m_values[index] = _tcsdup(value);
	}
	else
	{
		m_keys = (TCHAR **)realloc(m_keys, (m_size + 1) * sizeof(TCHAR *));
		m_values = (TCHAR **)realloc(m_values, (m_size + 1) * sizeof(TCHAR *));
		m_keys[m_size] = _tcsdup(key);
		m_values[m_size] = _tcsdup(value);
		m_size++;
	}
}


//
// Get value by key
//

const TCHAR *StringMap::get(const TCHAR *key)
{
	DWORD index;

	index = find(key);
	return (index != INVALID_INDEX) ? m_values[index] : NULL;
}


//
// Delete value
//

void StringMap::remove(const TCHAR *key)
{
	DWORD index;

	index = find(key);
	if (index != INVALID_INDEX)
	{
		safe_free(m_keys[index]);
		safe_free(m_values[index]);
		m_size--;
		memmove(&m_keys[index], &m_keys[index + 1], sizeof(TCHAR *) * (m_size - index));
		memmove(&m_values[index], &m_values[index + 1], sizeof(TCHAR *) * (m_size - index));
	}
}
