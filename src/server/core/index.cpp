/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: index.cpp
**
**/

#include "nxcore.h"


/**
 * Constructor for object index
 */
ObjectIndex::ObjectIndex()
{
	m_size = 0;
	m_allocated = 0;
	m_elements = NULL;
}

/**
 * Destructor
 */
ObjectIndex::~ObjectIndex()
{
	safe_free(m_elements);
}

/**
 * Compare index elements - qsort callback
 */
static int IndexCompare(const void *pArg1, const void *pArg2)
{
   return (((INDEX_ELEMENT *)pArg1)->key < ((INDEX_ELEMENT *)pArg2)->key) ? -1 :
            ((((INDEX_ELEMENT *)pArg1)->key > ((INDEX_ELEMENT *)pArg2)->key) ? 1 : 0);
}

/**
 * Put element. If element with given key already exist, it will be replaced.
 *
 * @param key object's key
 * @param object object
 */
void ObjectIndex::put(QWORD key, NetObj *object)
{
	int pos = findElement(key);
	if (pos != -1)
	{
		// Element already exist
		m_elements[pos].object = object;
	}
	else
	{
		if (m_size == m_allocated)
		{
			m_allocated += 256;
			m_elements = (INDEX_ELEMENT *)realloc(m_elements, sizeof(INDEX_ELEMENT) * m_allocated);
		}

		m_elements[m_size].key = key;
		m_elements[m_size].object = object;
	   qsort(m_elements, m_size, sizeof(INDEX_ELEMENT), IndexCompare);
	}
}

/**
 * Find element in index
 *
 * @param key object's key
 * @return element index or -1 if not found
 */
int ObjectIndex::findElement(QWORD key)
{
   int first, last, mid;

	if (m_size == 0)
      return -1;

   dwFirst = 0;
   dwLast = dwIndexSize - 1;

   if ((dwKey < pIndex[0].dwKey) || (dwKey > pIndex[dwLast].dwKey))
      return INVALID_INDEX;

   while(dwFirst < dwLast)
   {
      dwMid = (dwFirst + dwLast) / 2;
      if (dwKey == pIndex[dwMid].dwKey)
         return dwMid;
      if (dwKey < pIndex[dwMid].dwKey)
         dwLast = dwMid - 1;
      else
         dwFirst = dwMid + 1;
   }

   if (dwKey == pIndex[dwLast].dwKey)
      return dwLast;

   return INVALID_INDEX;
}
