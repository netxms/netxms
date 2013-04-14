/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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

#include "libnxsl.h"

/**
 * Create empty array
 */
NXSL_Array::NXSL_Array()
{
	m_refCount = 0;
	m_size = 0;
	m_allocated = 0;
	m_data = NULL;
}

/**
 *  Create copy of given array
 */
NXSL_Array::NXSL_Array(NXSL_Array *src)
{
	int i;

	m_refCount = 0;
	m_size = src->m_size;
	m_allocated = src->m_size;
	if (m_size > 0)
	{
		m_data = (NXSL_ArrayElement *)malloc(sizeof(NXSL_ArrayElement) * m_size);
		for(i = 0; i < m_size; i++)
		{
			m_data[i].index = src->m_data[i].index;
			m_data[i].value = new NXSL_Value(src->m_data[i].value);
		}
	}
	else
	{
		m_data = NULL;
	}
}

/**
 * Destructor
 */
NXSL_Array::~NXSL_Array()
{
	int i;

	for(i = 0; i < m_size; i++)
		delete m_data[i].value;
	safe_free(m_data);
}

/**
 * Compare two indexes
 */
static int CompareElements(const void *p1, const void *p2)
{
	return COMPARE_NUMBERS(((NXSL_ArrayElement *)p1)->index, ((NXSL_ArrayElement *)p2)->index);
}

/**
 * Get element by index
 */
NXSL_Value *NXSL_Array::get(int index)
{
	NXSL_ArrayElement *element, key;

	key.index = index;
	element = (NXSL_ArrayElement *)bsearch(&key, m_data, m_size, sizeof(NXSL_ArrayElement), CompareElements);
	return (element != NULL) ? element->value : NULL;
}

/**
 * Get element by internal position (used by iterator)
 */
NXSL_Value *NXSL_Array::getByPosition(int position)
{
	if ((position < 0) || (position >= m_size))
		return NULL;
	return m_data[position].value;
}

/**
 * Set element
 */
void NXSL_Array::set(int index, NXSL_Value *value)
{
	NXSL_ArrayElement *element, key;

	key.index = index;
	element = (NXSL_ArrayElement *)bsearch(&key, m_data, m_size, sizeof(NXSL_ArrayElement), CompareElements);
	if (element != NULL)
	{
		delete element->value;
		element->value = value;
	}
	else
	{
		if (m_size == m_allocated)
		{
			m_allocated += 32;
			m_data = (NXSL_ArrayElement *)realloc(m_data, sizeof(NXSL_ArrayElement) * m_allocated);
		}
		m_data[m_size].index = index;
		m_data[m_size].value = value;
		m_size++;
		qsort(m_data, m_size, sizeof(NXSL_ArrayElement), CompareElements);
	}
}
