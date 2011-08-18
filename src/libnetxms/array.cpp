/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: array.cpp
**
**/

#include "libnetxms.h"


//
// Constructor
//

Array::Array(int initial, int grow, bool owner)
{
	m_size = 0;
	m_grow = (grow > 0) ? grow : 16;
	m_allocated = (initial >= 0) ? initial : 16;
	m_data = (m_allocated > 0) ? (void **)malloc(sizeof(void *) * m_allocated) : NULL;
	m_objectOwner = owner;
}


//
// Destructor
//

Array::~Array()
{
	if (m_objectOwner)
	{
		for(int i = 0; i < m_size; i++)
			destroyObject(m_data[i]);
	}
	safe_free(m_data);
}


//
// Add object
//

int Array::add(void *object)
{
	if (m_size == m_allocated)
	{
		m_allocated += m_grow;
		m_data = (void **)realloc(m_data, sizeof(void *) * m_allocated);
	}
	m_data[m_size++] = object;
	return m_size - 1;
}


//
// Set object at given index. If index is within array bounds, object at this position will be replaced,
// otherwise array will be expanded as required. Other new positions created during expansion will
// be filled with NULL values.
//

void Array::set(int index, void *object)
{
	if (index < 0)
		return;

	if (index < m_size)
	{
		if (m_objectOwner)
			destroyObject(m_data[index]);
	}
	else
	{
		// Expand array
		if (index >= m_allocated)
		{
			m_allocated += m_grow * ((index - m_allocated) / m_grow + 1);
			m_data = (void **)realloc(m_data, sizeof(void *) * m_allocated);
		}
		memset(&m_data[m_size], 0, sizeof(void *) * (index - m_size));
		m_size = index + 1;
	}

	m_data[index] = object;
}


//
// Replace object at given index. If index is beyond array bounds,
// this method will do nothing.
//

void Array::replace(int index, void *object)
{
	if ((index < 0) || (index >= m_size))
		return;

	if (m_objectOwner)
		destroyObject(m_data[index]);

	m_data[index] = object;
}


//
// Remove element at given index
//

void Array::remove(int index)
{
	if ((index < 0) || (index >= m_size))
		return;

	if (m_objectOwner)
		destroyObject(m_data[index]);
	m_size--;
	memmove(&m_data[index], &m_data[index + 1], sizeof(void *) * (m_size - index));
}


//
// Default object destructor
//

void Array::destroyObject(void *object)
{
	safe_free(object);
}


//
// Clear array
//

void Array::clear()
{
	if (m_objectOwner)
	{
		for(int i = 0; i < m_size; i++)
			destroyObject(m_data[i]);
	}

	m_size = 0;
	if (m_allocated > m_grow)
	{
		m_data = (void **)realloc(m_data, sizeof(void *) * m_grow);
		m_allocated = m_grow;
	}
}
