/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
   uint64_t key;
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
   uint64_t maxKey;
   VolatileCounter readers;
   VolatileCounter writers;
};

/**
 * Default object destructor
 */
static void DefaultObjectDestructor(void *object, AbstractIndexBase *index)
{
   MemFree(object);
}

/**
 * Constructor for object index
 */
AbstractIndexBase::AbstractIndexBase(Ownership owner) : m_writerLock(MutexType::FAST)
{
	m_primary = MemAllocStruct<INDEX_HEAD>();
   m_secondary = MemAllocStruct<INDEX_HEAD>();
	m_owner = static_cast<bool>(owner);
	m_startupMode = false;
	m_dirty = false;
	m_objectDestructor = DefaultObjectDestructor;
}

/**
 * Destructor
 */
AbstractIndexBase::~AbstractIndexBase()
{
   if (m_owner)
   {
      for(size_t i = 0; i < m_primary->size; i++)
         destroyObject(m_primary->elements[i].object);
   }
	MemFree(m_primary->elements);
   MemFree(m_primary);
   MemFree(m_secondary->elements);
   MemFree(m_secondary);
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
 * Set/clear startup mode
 */
void AbstractIndexBase::setStartupMode(bool startupMode)
{
   if (m_startupMode == startupMode)
      return;

   m_startupMode = startupMode;
   if (!startupMode)
   {
      if (m_primary->size > 0)
      {
         qsort(m_primary->elements, m_primary->size, sizeof(INDEX_ELEMENT), IndexCompare);
         m_primary->maxKey = m_primary->elements[m_primary->size - 1].key;
      }
      else
      {
         m_primary->maxKey = 0;
      }

      m_secondary->maxKey = m_primary->maxKey;
      m_secondary->allocated = m_primary->allocated;
      m_secondary->size = m_primary->size;
      MemFree(m_secondary->elements);
      if (m_secondary->allocated > 0)
      {
         m_secondary->elements = MemAllocArray<INDEX_ELEMENT>(m_secondary->allocated);
         memcpy(m_secondary->elements, m_primary->elements, m_secondary->size * sizeof(INDEX_ELEMENT));
      }
      else
      {
         m_secondary->elements = nullptr;
      }
   }
   m_dirty = false;
}

/**
 * Swap indexes and wait for new secondary copy to became writable
 */
void AbstractIndexBase::swapAndWait()
{
   m_secondary = InterlockedExchangeObjectPointer(&m_primary, m_secondary);
   InterlockedIncrement(&m_secondary->writers);
   while(m_secondary->readers > 0)
      ThreadSleepMs(10);
}

/**
 * Acquire index
 */
INDEX_HEAD *AbstractIndexBase::acquireIndex() const
{
   INDEX_HEAD *h;
   while(true)
   {
      h = m_primary;
      InterlockedIncrement(&h->readers);
      if (h->writers == 0)
         break;
      InterlockedDecrement(&h->readers);
   }
   return h;
}

/**
 * Release index
 */
static inline void ReleaseIndex(INDEX_HEAD *h)
{
   InterlockedDecrement(&h->readers);
}

/**
 * Put element. If element with given key already exist, it will be replaced.
 *
 * @param key object's key
 * @param object object
 * @return true if existing object was replaced
 */
bool AbstractIndexBase::put(uint64_t key, void *object)
{
   if (m_startupMode)
   {
      if (m_primary->size == m_primary->allocated)
      {
         m_primary->allocated += 1024;
         m_primary->elements = MemReallocArray<INDEX_ELEMENT>(m_primary->elements, m_primary->allocated);
      }

      m_primary->elements[m_primary->size].key = key;
      m_primary->elements[m_primary->size].object = object;
      m_primary->size++;
      m_dirty = true;
      return false;
   }

   bool replace = false;
	void *oldObject = nullptr;

   m_writerLock.lock();

	ssize_t pos = findElement(m_secondary, key);
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
		   m_secondary->allocated += 1024;
		   m_secondary->elements = MemReallocArray<INDEX_ELEMENT>(m_secondary->elements, m_secondary->allocated);
		}

		m_secondary->elements[m_secondary->size].key = key;
		m_secondary->elements[m_secondary->size].object = object;
		m_secondary->size++;
		if (key < m_secondary->maxKey)
		{
		   qsort(m_secondary->elements, m_secondary->size, sizeof(INDEX_ELEMENT), IndexCompare);
		}
		else
		{
		   m_secondary->maxKey = key;
		}
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
      if (key < m_secondary->maxKey)
      {
         memcpy(m_secondary->elements, m_primary->elements, m_secondary->size * sizeof(INDEX_ELEMENT));
      }
      else
      {
         m_secondary->maxKey = key;
         m_secondary->elements[m_secondary->size - 1].key = key;
         m_secondary->elements[m_secondary->size - 1].object = object;
      }
	}

   InterlockedDecrement(&m_secondary->writers);

   m_writerLock.unlock();
	return replace;
}

/**
 * Remove object from index
 *
 * @param key object's key
 */
void AbstractIndexBase::remove(uint64_t key)
{
   if (m_startupMode)
   {
      if (m_dirty)
      {
         qsort(m_primary->elements, m_primary->size, sizeof(INDEX_ELEMENT), IndexCompare);
         m_primary->maxKey = (m_primary->size > 0) ? m_primary->elements[m_primary->size - 1].key : 0;
         m_dirty = false;
      }
      ssize_t pos = findElement(m_primary, key);
      if (pos != -1)
      {
         if (m_owner)
            destroyObject(m_primary->elements[pos].object);
         m_primary->size--;
         memmove(&m_primary->elements[pos], &m_primary->elements[pos + 1], sizeof(INDEX_ELEMENT) * (m_primary->size - pos));
      }
      return;
   }

   m_writerLock.lock();

	ssize_t pos = findElement(m_secondary, key);
	if (pos != -1)
	{
      m_secondary->size--;
      memmove(&m_secondary->elements[pos], &m_secondary->elements[pos + 1], sizeof(INDEX_ELEMENT) * (m_secondary->size - pos));
      if (m_secondary->maxKey == key)
         m_secondary->maxKey = (m_secondary->size > 0) ? m_secondary->elements[m_secondary->size - 1].key : 0;

      swapAndWait();

      if (m_owner)
         destroyObject(m_secondary->elements[pos].object);
      m_secondary->size--;
      memmove(&m_secondary->elements[pos], &m_secondary->elements[pos + 1], sizeof(INDEX_ELEMENT) * (m_secondary->size - pos));
      if (m_secondary->maxKey == key)
         m_secondary->maxKey = (m_secondary->size > 0) ? m_secondary->elements[m_secondary->size - 1].key : 0;

      InterlockedDecrement(&m_secondary->writers);
   }

   m_writerLock.unlock();
}

/**
 * Clear index
 */
void AbstractIndexBase::clear()
{
   m_writerLock.lock();

   m_secondary->size = 0;
   m_secondary->allocated = 0;
   m_secondary->maxKey = 0;
   MemFreeAndNull(m_secondary->elements);

   swapAndWait();

   if (m_owner)
   {
      for(size_t i = 0; i < m_secondary->size; i++)
         destroyObject(m_secondary->elements[i].object);
   }

   m_secondary->size = 0;
   m_secondary->allocated = 0;
   m_secondary->maxKey = 0;
   MemFreeAndNull(m_secondary->elements);

   InterlockedDecrement(&m_secondary->writers);

   m_writerLock.unlock();
}

/**
 * Find element in index
 *
 * @param key object's key
 * @return element index or -1 if not found
 */
ssize_t AbstractIndexBase::findElement(INDEX_HEAD *index, uint64_t key)
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
void *AbstractIndexBase::get(uint64_t key) const
{
   if (m_startupMode && m_dirty)
   {
      qsort(m_primary->elements, m_primary->size, sizeof(INDEX_ELEMENT), IndexCompare);
      m_primary->maxKey = (m_primary->size > 0) ? m_primary->elements[m_primary->size - 1].key : 0;
      const_cast<AbstractIndexBase*>(this)->m_dirty = false;   // This is internal marker, changing it does not break const contract
   }
   INDEX_HEAD *index = acquireIndex();
	ssize_t pos = findElement(index, key);
	void *object = (pos == -1) ? nullptr : index->elements[pos].object;
   ReleaseIndex(index);
	return object;
}

/**
 * Get all keys in index
 */
IntegerArray<uint64_t> AbstractIndexBase::keys() const
{
   INDEX_HEAD *index = acquireIndex();
   IntegerArray<uint64_t> result(index->size);
   for(size_t i = 0; i < index->size; i++)
      result.add(index->elements[i].key);
   ReleaseIndex(index);
   return result;
}

/**
 * Get index size
 */
size_t AbstractIndexBase::size() const
{
   INDEX_HEAD *index = acquireIndex();
	size_t s = index->size;
   ReleaseIndex(index);
	return s;
}

/**
 * Find object by comparing it with given data using external comparator
 *
 * @param comparator comparing function (must return true for object to be found)
 * @param data user data passed to comparator
 */
void *AbstractIndexBase::find(bool (*comparator)(void *, void *), void *data) const
{
	void *result = nullptr;

   INDEX_HEAD *index = acquireIndex();
	for(size_t i = 0; i < index->size; i++)
		if (comparator(index->elements[i].object, data))
		{
			result = index->elements[i].object;
			break;
		}
   ReleaseIndex(index);

	return result;
}

/**
 * Find object by comparing it with given data using external comparator
 *
 * @param comparator comparing function (must return true for object to be found)
 */
void *AbstractIndexBase::find(std::function<bool (void*)> comparator) const
{
   void *result = nullptr;

   INDEX_HEAD *index = acquireIndex();
   for(size_t i = 0; i < index->size; i++)
      if (comparator(index->elements[i].object))
      {
         result = index->elements[i].object;
         break;
      }
   ReleaseIndex(index);

   return result;
}

/**
 * Find all matching elements by comparing it with given data using external comparator
 *
 * @param comparator comparing function (must return true for object to be found)
 * @param data user data passed to comparator
 */
void AbstractIndexBase::findAll(Array *resultSet, bool (*comparator)(void *, void *), void *data) const
{
   INDEX_HEAD *index = acquireIndex();
   for(size_t i = 0; i < index->size; i++)
   {
      if (comparator(index->elements[i].object, data))
         resultSet->add(index->elements[i].object);
   }
   ReleaseIndex(index);
}

/**
 * Find all matching elements by comparing it with given data using external comparator
 *
 * @param comparator comparing function (must return true for object to be found)
 */
void AbstractIndexBase::findAll(Array *resultSet, std::function<bool (void*)> comparator) const
{
   INDEX_HEAD *index = acquireIndex();
   for(size_t i = 0; i < index->size; i++)
   {
      if (comparator(index->elements[i].object))
         resultSet->add(index->elements[i].object);
   }
   ReleaseIndex(index);
}

/**
 * Execute callback for each object.
 *
 * @param callback
 * @param data user data passed to callback
 */
void AbstractIndexBase::forEach(EnumerationCallbackResult (*callback)(void*, void*), void *data) const
{
   INDEX_HEAD *index = acquireIndex();
	for(size_t i = 0; i < index->size; i++)
		if (callback(index->elements[i].object, data) != _CONTINUE)
		   break;
   ReleaseIndex(index);
}

/**
 * Execute callback for each object.
 *
 * @param callback
 */
void AbstractIndexBase::forEach(std::function<EnumerationCallbackResult (void*)> callback) const
{
   INDEX_HEAD *index = acquireIndex();
   for(size_t i = 0; i < index->size; i++)
      if (callback(index->elements[i].object) != _CONTINUE)
         break;
   ReleaseIndex(index);
}

/**
 * Get all objects in index. Result array created dynamically and
 * must be destroyed by the caller. Changes in result array will
 * not affect content of the index.
 */
unique_ptr<SharedObjectArray<NetObj>> ObjectIndex::getObjects(bool (*filter)(NetObj*, void*), void *context)
{
   INDEX_HEAD *index = acquireIndex();
   auto result = make_unique<SharedObjectArray<NetObj>>(index->size);
   for(size_t i = 0; i < index->size; i++)
   {
      if ((filter == nullptr) || filter(static_cast<shared_ptr<NetObj>*>(index->elements[i].object)->get(), context))
      {
         result->add(*static_cast<shared_ptr<NetObj>*>(index->elements[i].object));
      }
   }
   ReleaseIndex(index);
   return result;
}

/**
 * Get all objects in index. Result array created dynamically and
 * must be destroyed by the caller. Changes in result array will
 * not affect content of the index.
 */
unique_ptr<SharedObjectArray<NetObj>> ObjectIndex::getObjects(std::function<bool (NetObj*)> filter)
{
   INDEX_HEAD *index = acquireIndex();
   auto result = make_unique<SharedObjectArray<NetObj>>(index->size);
   for(size_t i = 0; i < index->size; i++)
   {
      if (filter(static_cast<shared_ptr<NetObj>*>(index->elements[i].object)->get()))
      {
         result->add(*static_cast<shared_ptr<NetObj>*>(index->elements[i].object));
      }
   }
   ReleaseIndex(index);
   return result;
}

/**
 * Get all objects in index. Objects will be added to array supplied by caller.
 */
void ObjectIndex::getObjects(SharedObjectArray<NetObj> *destination, bool (*filter)(NetObj *, void *), void *context)
{
   INDEX_HEAD *index = acquireIndex();
   for(size_t i = 0; i < index->size; i++)
   {
      if ((filter == nullptr) || filter(static_cast<shared_ptr<NetObj>*>(index->elements[i].object)->get(), context))
      {
         destination->add(*static_cast<shared_ptr<NetObj>*>(index->elements[i].object));
      }
   }
   ReleaseIndex(index);
}

/**
 * Get all objects in index. Objects will be added to array supplied by caller.
 */
void ObjectIndex::getObjects(SharedObjectArray<NetObj> *destination, std::function<bool (NetObj*)> filter)
{
   INDEX_HEAD *index = acquireIndex();
   for(size_t i = 0; i < index->size; i++)
   {
      if (filter(static_cast<shared_ptr<NetObj>*>(index->elements[i].object)->get()))
      {
         destination->add(*static_cast<shared_ptr<NetObj>*>(index->elements[i].object));
      }
   }
   ReleaseIndex(index);
}
