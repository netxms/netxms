/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
 * Object index element
 */
struct INDEX_ELEMENT
{
   UINT64 key;
   void *object;
};

/**
 * Index head
 */
struct INDEX_HEAD
{
   INDEX_ELEMENT *elements;
   size_t size;
   size_t allocated;
   VolatileCounter readers;
};

/**
 * Constructor for object index
 */
AbstractIndexBase::AbstractIndexBase(bool owner)
{
	m_primary = MemAllocStruct<INDEX_HEAD>();
   m_secondary = MemAllocStruct<INDEX_HEAD>();
	m_writerLock = MutexCreate();
	m_owner = owner;
	m_objectDestructor = free;
}

/**
 * Destructor
 */
AbstractIndexBase::~AbstractIndexBase()
{
   if (m_owner)
   {
      for(int i = 0; i < m_primary->size; i++)
         destroyObject(m_primary->elements[i].object);
   }
	MemFree(m_primary->elements);
   MemFree(m_primary);
   MemFree(m_secondary->elements);
   MemFree(m_secondary);
	MutexDestroy(m_writerLock);
}

/**
 * Swap indexes and wait for new secondary copy to became writable
 */
void AbstractIndexBase::swapAndWait()
{
   m_secondary = InterlockedExchangeObjectPointer(&m_primary, m_secondary);
   do
   {
      ThreadSleepMs(10);
   }
   while(m_secondary->readers > 0);
}

/**
 * Acquire index
 */
INDEX_HEAD *AbstractIndexBase::acquireIndex()
{
   INDEX_HEAD *h = m_primary;
   InterlockedIncrement(&m_primary->readers);
   return h;
}

/**
 * Release index
 */
inline void ReleaseIndex(INDEX_HEAD *h)
{
   InterlockedDecrement(&h->readers);
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
bool AbstractIndexBase::put(UINT64 key, void *object)
{
	bool replace = false;
	void *oldObject = NULL;

	MutexLock(m_writerLock);

	int pos = findElement(m_secondary, key);
	if (pos != -1)
	{
		// Element already exist
      oldObject = m_secondary->elements[pos].object;
      m_secondary->elements[pos].object = object;
		replace = true;
	}
	else
	{
		if (m_secondary->size == m_secondary->allocated)
		{
		   m_secondary->allocated += 256;
		   m_secondary->elements = MemReallocArray<INDEX_ELEMENT>(m_secondary->elements, m_secondary->allocated);
		}

		m_secondary->elements[m_secondary->size].key = key;
		m_secondary->elements[m_secondary->size].object = object;
		m_secondary->size++;
	   qsort(m_secondary->elements, m_secondary->size, sizeof(INDEX_ELEMENT), IndexCompare);
	}

	swapAndWait();

	if (replace)
	{
      m_secondary->elements[pos].object = object;
	   if (m_owner)
	      destroyObject(oldObject);
	}
	else
	{
	   if (m_primary->allocated > m_secondary->allocated)
	   {
	      m_secondary->allocated = m_primary->allocated;
         m_secondary->elements = MemReallocArray<INDEX_ELEMENT>(m_secondary->elements, m_secondary->allocated);
	   }
	   m_secondary->size = m_primary->size;
	   memcpy(m_secondary->elements, m_primary->elements, m_secondary->size * sizeof(INDEX_ELEMENT));
	}

	MutexUnlock(m_writerLock);
	return replace;
}

/**
 * Remove object from index
 *
 * @param key object's key
 */
void AbstractIndexBase::remove(UINT64 key)
{
   MutexLock(m_writerLock);

	int pos = findElement(m_secondary, key);
	if (pos != -1)
	{
      m_secondary->size--;
      memmove(&m_secondary->elements[pos], &m_secondary->elements[pos + 1], sizeof(INDEX_ELEMENT) * (m_secondary->size - pos));
	}

	swapAndWait();

   if (m_owner)
      destroyObject(m_secondary->elements[pos].object);
   m_secondary->size--;
   memmove(&m_secondary->elements[pos], &m_secondary->elements[pos + 1], sizeof(INDEX_ELEMENT) * (m_secondary->size - pos));

   MutexUnlock(m_writerLock);
}

/**
 * Clear index
 */
void AbstractIndexBase::clear()
{
   MutexLock(m_writerLock);

   m_secondary->size = 0;
   m_secondary->allocated = 0;
   MemFreeAndNull(m_secondary->elements);

   swapAndWait();

   if (m_owner)
   {
      for(int i = 0; i < m_secondary->size; i++)
         destroyObject(m_secondary->elements[i].object);
   }

   m_secondary->size = 0;
   m_secondary->allocated = 0;
   MemFreeAndNull(m_secondary->elements);

   MutexUnlock(m_writerLock);
}

/**
 * Find element in index
 *
 * @param key object's key
 * @return element index or -1 if not found
 */
int AbstractIndexBase::findElement(INDEX_HEAD *index, UINT64 key)
{
   size_t first, last, mid;

	if (index->size == 0)
      return -1;

   first = 0;
   last = index->size - 1;

   if ((key < index->elements[0].key) || (key > index->elements[last].key))
      return -1;

   while(first < last)
   {
      mid = (first + last) / 2;
      if (key == index->elements[mid].key)
         return mid;
      if (key < index->elements[mid].key)
         last = mid - 1;
      else
         first = mid + 1;
   }

   if (key == index->elements[last].key)
      return last;

   return -1;
}

/**
 * Get object by key
 *
 * @param key key
 * @return object with given key or NULL
 */
void *AbstractIndexBase::get(UINT64 key)
{
   INDEX_HEAD *index = acquireIndex();
	int pos = findElement(index, key);
	void *object = (pos == -1) ? NULL : index->elements[pos].object;
   ReleaseIndex(index);
	return object;
}

/**
 * Get index size
 */
int AbstractIndexBase::size()
{
   INDEX_HEAD *index = acquireIndex();
	int s = index->size;
   ReleaseIndex(index);
	return s;
}

/**
 * Find object by comparing it with given data using external comparator
 *
 * @param comparator comparing function (must return true for object to be found)
 * @param data user data passed to comparator
 */
void *AbstractIndexBase::find(bool (*comparator)(void *, void *), void *data)
{
	void *result = NULL;

   INDEX_HEAD *index = acquireIndex();
	for(int i = 0; i < index->size; i++)
		if (comparator(index->elements[i].object, data))
		{
			result = index->elements[i].object;
			break;
		}
   ReleaseIndex(index);

	return result;
}

/**
 * Find objects by comparing it with given data using external comparator
 *
 * @param comparator comparing function (must return true for object to be found)
 * @param data user data passed to comparator
 */
void AbstractIndexBase::findObjects(Array *resultSet, bool (*comparator)(void *, void *), void *data)
{
   INDEX_HEAD *index = acquireIndex();
   for(int i = 0; i < index->size; i++)
   {
      if (comparator(index->elements[i].object, data))
         resultSet->add(index->elements[i].object);
   }
   ReleaseIndex(index);
}

/**
 * Execute callback for each object. Callback should return true to continue enumeration.
 *
 * @param callback
 * @param data user data passed to callback
 */
void AbstractIndexBase::forEach(void (*callback)(void *, void *), void *data)
{
   INDEX_HEAD *index = acquireIndex();
	for(int i = 0; i < index->size; i++)
		callback(index->elements[i].object, data);
   ReleaseIndex(index);
}

/**
 * Get all objects in index. Result array created dynamically and
 * must be destroyed by the caller. Changes in result array will
 * not affect content of the index.
 *
 * @param updateRefCount if set to true, reference count for each object will be increased
 */
ObjectArray<NetObj> *ObjectIndex::getObjects(bool updateRefCount, bool (*filter)(NetObj *, void *), void *userData)
{
   INDEX_HEAD *index = acquireIndex();
   ObjectArray<NetObj> *result = new ObjectArray<NetObj>(index->size);
   for(int i = 0; i < index->size; i++)
   {
      if ((filter == NULL) || filter(static_cast<NetObj*>(index->elements[i].object), userData))
      {
         if (updateRefCount)
            static_cast<NetObj*>(index->elements[i].object)->incRefCount();
         result->add(static_cast<NetObj*>(index->elements[i].object));
      }
   }
   ReleaseIndex(index);
   return result;
}
