/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
// Constructor
//

StringMap::StringMap()
{
	m_dwSize = 0;
	m_ppszKeys = NULL;
	m_ppszValues = NULL;
}


//
// Destructor
//

StringMap::~StringMap()
{
	Clear();
}


//
// Clear map
//

void StringMap::Clear(void)
{
	DWORD i;

	for(i = 0; i < m_dwSize; i++)
	{
		safe_free(m_ppszKeys[i]);
		safe_free(m_ppszValues[i]);
	}
	m_dwSize = 0;
	safe_free_and_null(m_ppszKeys);
	safe_free_and_null(m_ppszValues);
}


//
// Find value by key
//

DWORD StringMap::Find(const TCHAR *pszKey)
{
	DWORD i;

	for(i = 0; i < m_dwSize; i++)
	{
		if (!_tcsicmp(pszKey, m_ppszKeys[i]))
			return i;
	}
	return INVALID_INDEX;
}


//
// Set value
//

void StringMap::Set(const TCHAR *pszKey, const TCHAR *pszValue)
{
	DWORD dwIndex;

	dwIndex = Find(pszKey);
	if (dwIndex != INVALID_INDEX)
	{
		safe_free(m_ppszValues[dwIndex]);
		m_ppszValues[dwIndex] = _tcsdup(pszValue);
	}
	else
	{
		m_ppszKeys = (TCHAR **)realloc(m_ppszKeys, (m_dwSize + 1) * sizeof(TCHAR *));
		m_ppszValues = (TCHAR **)realloc(m_ppszValues, (m_dwSize + 1) * sizeof(TCHAR *));
		m_ppszKeys[m_dwSize] = _tcsdup(pszKey);
		m_ppszValues[m_dwSize] = _tcsdup(pszValue);
		m_dwSize++;
	}
}


//
// Get value by key
//

TCHAR *StringMap::Get(const TCHAR *pszKey)
{
	DWORD dwIndex;

	dwIndex = Find(pszKey);
	return (dwIndex != INVALID_INDEX) ? m_ppszValues[dwIndex] : NULL;
}


//
// Delete value
//

void StringMap::Delete(const TCHAR *pszKey)
{
	DWORD dwIndex;

	dwIndex = Find(pszKey);
	if (dwIndex != INVALID_INDEX)
	{
		safe_free(m_ppszKeys[dwIndex]);
		safe_free(m_ppszValues[dwIndex]);
		m_dwSize--;
		memmove(&m_ppszKeys[dwIndex], &m_ppszKeys[dwIndex + 1], sizeof(TCHAR *) * (m_dwSize - dwIndex));
		memmove(&m_ppszValues[dwIndex], &m_ppszValues[dwIndex + 1], sizeof(TCHAR *) * (m_dwSize - dwIndex));
	}
}

