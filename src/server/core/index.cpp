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
	m_lock = RWLockCreate();
}

/**
 * Destructor
 */
ObjectIndex::~ObjectIndex()
{
	safe_free(m_elements);
	RWLockDestroy(m_lock);
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
 * @return true if existing object was replaced
 */
bool ObjectIndex::put(QWORD key, NetObj *object)
{
	bool replace = false;

	RWLockWriteLock(m_lock, INFINITE);

	int pos = findElement(key);
	if (pos != -1)
	{
		// Element already exist
		m_elements[pos].object = object;
		replace = true;
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
		m_size++;
	   qsort(m_elements, m_size, sizeof(INDEX_ELEMENT), IndexCompare);
	}

	RWLockUnlock(m_lock);
	return replace;
}

/**
 * Remove object from index
 *
 * @param key object's key
 */
void ObjectIndex::remove(QWORD key)
{
	RWLockWriteLock(m_lock, INFINITE);

	int pos = findElement(key);
	if (pos != -1)
	{
		m_size--;
		memmove(&m_elements[pos], &m_elements[pos + 1], sizeof(INDEX_ELEMENT) * (m_size - pos));
	}

	RWLockUnlock(m_lock);
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

   first = 0;
   last = m_size - 1;

   if ((key < m_elements[0].key) || (key > m_elements[last].key))
      return -1;

   while(first < last)
   {
      mid = (first + last) / 2;
      if (key == m_elements[mid].key)
         return mid;
      if (key < m_elements[mid].key)
         last = mid - 1;
      else
         first = mid + 1;
   }

   if (key == m_elements[last].key)
      return last;

   return -1;
}

/**
 * Get object by key
 *
 * @param key key
 * @return object with given key or NULL
 */
NetObj *ObjectIndex::get(QWORD key)
{
	RWLockReadLock(m_lock, INFINITE);
	int pos = findElement(key);
	NetObj *object = (pos == -1) ? NULL : m_elements[pos].object;
	RWLockUnlock(m_lock);
	return object;
}

/**
 * Get index size
 */
int ObjectIndex::getSize()
{
	RWLockReadLock(m_lock, INFINITE);
	int size = m_size;
	RWLockUnlock(m_lock);
	return size;
}

/**
 * Get all objects in index. Result array created dynamically and
 * must be destroyed by the caller. Changes in result array will
 * not affect content of the index.
 *
 * @param updateRefCount if set to true, reference count for each object will be increased
 */
ObjectArray<NetObj> *ObjectIndex::getObjects(bool updateRefCount)
{
	RWLockReadLock(m_lock, INFINITE);
	ObjectArray<NetObj> *result = new ObjectArray<NetObj>(m_size);
	for(int i = 0; i < m_size; i++)
   {
      if (updateRefCount)
         m_elements[i].object->incRefCount();
		result->add(m_elements[i].object);
   }
	RWLockUnlock(m_lock);
	return result;
}

/**
 * Find object by compating it with given data using external comparator
 *
 * @param comparator comparing finction (must return true for object to be found)
 * @param data user data passed to comparator
 */
NetObj *ObjectIndex::find(bool (*comparator)(NetObj *, void *), void *data)
{
	NetObj *result = NULL;

	RWLockReadLock(m_lock, INFINITE);
	for(int i = 0; i < m_size; i++)
		if (comparator(m_elements[i].object, data))
		{
			result = m_elements[i].object;
			break;
		}
	RWLockUnlock(m_lock);

	return result;
}

/**
 * Execute callback for each object. Callback should return true to continue enumeration.
 *
 * @param callback
 * @param data user data passed to callback
 */
void ObjectIndex::forEach(void (*callback)(NetObj *, void *), void *data)
{
	RWLockReadLock(m_lock, INFINITE);
	for(int i = 0; i < m_size; i++)
		callback(m_elements[i].object, data);
	RWLockUnlock(m_lock);
}
