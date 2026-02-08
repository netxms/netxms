/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

#define ADDR(index) ((void *)((char *)m_data + m_elementSize * (index)))

/**
 * Default element destructor
 */
static void DefaultObjectDestructor(void *element, Array *array)
{
   MemFree(element);
}

/**
 * Constructor
 *
 * @param initial initial array size
 * @param grow number of elements to add when array has to grow
 * @param owner if set to true, array is an owner of added objects and will destroy them on removed from array
 * @param objectDestructor custom object destructor or NULL
 */
Array::Array(int initial, int grow, Ownership owner, void (*objectDestructor)(void *, Array *))
{
   m_size = 0;
   m_grow = (grow > 0) ? grow : 16;
   m_allocated = (initial >= 0) ? initial : 16;
   m_elementSize = sizeof(void *);
   m_data = (m_allocated > 0) ? (void **)MemAlloc(m_elementSize * m_allocated) : nullptr;
   m_objectOwner = (owner == Ownership::True);
   m_objectDestructor = (objectDestructor != nullptr) ? objectDestructor : DefaultObjectDestructor;
   m_storePointers = true;
   m_context = nullptr;
}

/**
 * Constructor for struct arrays
 */
Array::Array(const void *data, int initial, int grow, size_t elementSize)
{
   m_size = (data != nullptr) ? initial : 0;
   m_grow = (grow > 0) ? grow : 16;
   m_allocated = (initial >= 0) ? initial : 16;
   m_elementSize = elementSize;
   if (m_allocated > 0)
   {
      m_data = (void **)MemAlloc(m_elementSize * m_allocated);
      if (data != nullptr)
      {
         memcpy(m_data, data, initial * m_elementSize);
      }
   }
   else
   {
      m_data = nullptr;
   }
   m_objectOwner = false;
   m_objectDestructor = DefaultObjectDestructor;
   m_storePointers = false;
   m_context = nullptr;
}

/**
 * Copy constructor
 */
Array::Array(const Array& src)
{
   m_size = src.m_size;
   m_grow = src.m_grow;
   m_allocated = src.m_allocated;
   m_elementSize = src.m_elementSize;
   m_data = (src.m_data != nullptr) ? MemCopyBlock(src.m_data, m_elementSize * m_allocated) : nullptr;
   m_objectOwner = src.m_objectOwner;
   m_objectDestructor = src.m_objectDestructor;
   m_storePointers = src.m_storePointers;
   m_context = src.m_context;
}

/**
 * Move constructor
 */
Array::Array(Array&& src)
{
   m_size = src.m_size;
   m_grow = src.m_grow;
   m_allocated = src.m_allocated;
   m_elementSize = src.m_elementSize;
   m_data = src.m_data;
   m_objectOwner = src.m_objectOwner;
   m_objectDestructor = src.m_objectDestructor;
   m_storePointers = src.m_storePointers;
   m_context = src.m_context;

   // Reset source object
   src.m_size = 0;
   src.m_allocated = 0;
   src.m_data = nullptr;
}

/**
 * Destructor
 */
Array::~Array()
{
	if (m_objectOwner)
	{
	   if (m_storePointers)
	   {
         for(int i = 0; i < m_size; i++)
            destroyObject(m_data[i]);
	   }
	   else
	   {
         for(int i = 0; i < m_size; i++)
            destroyObject(ADDR(i));
	   }
	}
	MemFree(m_data);
}

/**
 * Add element. Pointer sized elements must be passed by value, larger elements - by pointer.
 */
int Array::add(void *element)
{
	if (m_size == m_allocated)
	{
		m_allocated += m_grow;
		m_data = MemRealloc(m_data, m_elementSize * m_allocated);
	}
   if (m_storePointers)
   {
	   m_data[m_size++] = element;
   }
   else
   {
   	memcpy(ADDR(m_size), element, m_elementSize);
      m_size++;
   }
	return m_size - 1;
}

/**
 * Add all elements from source array
 */
void Array::addAll(const Array& src)
{
   if ((src.m_elementSize != m_elementSize) || (src.m_size == 0))
      return;
   if (m_size + src.m_size > m_allocated)
   {
      m_allocated += std::max(m_grow, src.m_size - (m_allocated - m_size));
      m_data = MemRealloc(m_data, m_elementSize * m_allocated);
   }
   memcpy(ADDR(m_size), src.m_data, src.m_size * m_elementSize);
   m_size += src.m_size;
}

/**
 * Add placeholder for element. Returns pointer to element.
 */
void *Array::addPlaceholder()
{
   if (m_size == m_allocated)
   {
      m_allocated += m_grow;
      m_data = MemRealloc(m_data, m_elementSize * m_allocated);
   }

   void *element;
   if (m_storePointers)
   {
      element = &m_data[m_size++];
   }
   else
   {
      element = ADDR(m_size);
      m_size++;
   }
   return element;
}

/**
 * Set element at given index. If index is within array bounds, element at this position will be replaced,
 * otherwise array will be expanded as required. Other new positions created during expansion will
 * be filled with NULL values.
 */
void Array::set(int index, void *element)
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
			m_data = MemRealloc(m_data, m_elementSize * m_allocated);
		}
		memset(ADDR(m_size), 0, m_elementSize * (index - m_size));
		m_size = index + 1;
	}

   if (m_storePointers)
   	m_data[index] = element;
   else
   	memcpy(ADDR(index), element, m_elementSize);
}

/**
 * Replace element at given index. If index is beyond array bounds,
 * this method will do nothing.
 */
void Array::replace(int index, void *element)
{
	if ((index < 0) || (index >= m_size))
		return;

	if (m_objectOwner)
		destroyObject(m_data[index]);

   if (m_storePointers)
   	m_data[index] = element;
   else
   	memcpy(ADDR(index), element, m_elementSize);
}

/**
 * Replace element at given index with placeholder. If index is beyond array bounds,
 * this method will do nothing. Returns element address or NULL.
 */
void *Array::replaceWithPlaceholder(int index)
{
   if ((index < 0) || (index >= m_size))
      return NULL;

   if (m_objectOwner)
      destroyObject(m_data[index]);

   return m_storePointers ? &m_data[index] : ADDR(index);
}

/**
 * Insert element at given index. If index is within array bounds, elements starting at this position will be shifted to the right,
 * otherwise array will be expanded as required. Other new positions created during expansion will be filled with NULL values.
 */
void Array::insert(int index, void *element)
{
   if (index < 0)
      return;

   if (index < m_size)
   {
      if (m_size == m_allocated)
      {
         m_allocated += m_grow;
         m_data = MemRealloc(m_data, m_elementSize * m_allocated);
      }
      memmove(ADDR(index + 1), ADDR(index), m_elementSize * (m_size - index));
      m_size++;
   }
   else
   {
      // Expand array
      if (index >= m_allocated)
      {
         m_allocated += m_grow * ((index - m_allocated) / m_grow + 1);
         m_data = MemRealloc(m_data, m_elementSize * m_allocated);
      }
      memset(ADDR(m_size), 0, m_elementSize * (index - m_size));
      m_size = index + 1;
   }

   if (m_storePointers)
      m_data[index] = element;
   else
      memcpy(ADDR(index), element, m_elementSize);
}

/**
 * Remove element at given index
 */
bool Array::internalRemove(int index, bool allowDestruction)
{
	if ((index < 0) || (index >= m_size))
		return false;

	if (m_objectOwner && allowDestruction)
		destroyObject(m_data[index]);
	m_size--;
	memmove(ADDR(index), ADDR(index + 1), m_elementSize * (m_size - index));
	return true;
}

/**
 * Clear array
 */
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
		m_data = MemRealloc(m_data, m_elementSize * m_grow);
		m_allocated = m_grow;
	}
}

/**
 * Shrink array to given size
 */
void Array::shrinkTo(int size)
{
   if ((size < 0) || (size >= m_size))
      return;

   if (m_objectOwner)
   {
      for(int i = size; i < m_size; i++)
         destroyObject(m_data[i]);
   }

   m_size = size;
}

/**
 * Get index of given element
 */
int Array::indexOf(void *element) const
{
   if (m_storePointers)
   {
	   for(int i = 0; i < m_size; i++)
      {
         if (m_data[i] == element)
            return i;
      }
   }
   else
   {
	   for(int i = 0; i < m_size; i++)
      {
         if (!memcmp(ADDR(i), element, m_elementSize))
            return i;
      }
   }
   return -1;
}

/**
 * Sort array elements
 */
void Array::sort(int (*cb)(const void *, const void *))
{
   qsort(m_data, m_size, m_elementSize, cb);
}

/**
 * Sort array elements with context
 */
void Array::sort(int (*cb)(void *, const void *, const void *), void *context)
{
   QSort(m_data, m_size, m_elementSize, cb, context);
}

/**
 * Find element (assuming array is already sorted)
 */
void *Array::bsearch(const void *key, int (*cb)(const void *, const void *)) const
{
   return ::bsearch(key, m_data, m_size, m_elementSize, cb);
}

/**
 * Find element by enumeration
 */
void *Array::find(std::function<bool (const void*)> comparator) const
{
   for(int i = 0; i < m_size; i++)
   {
      if (comparator(m_storePointers ? m_data[i] : ADDR(i)))
         return m_storePointers ? m_data[i] : ADDR(i);
   }
   return nullptr;
}

/**
 * Swap two arrays
 */
void Array::swap(Array& other)
{
   std::swap(m_size, other.m_size);
   std::swap(m_allocated, other.m_allocated);
   std::swap(m_grow, other.m_grow);
   std::swap(m_elementSize, other.m_elementSize);
   std::swap(m_data, other.m_data);
   std::swap(m_objectOwner, other.m_objectOwner);
   std::swap(m_context, other.m_context);
   std::swap(m_storePointers, other.m_storePointers);
   std::swap(m_objectDestructor, other.m_objectDestructor);
}

/**
 * Swap two elements in array
 */
void Array::swap(int index1, int index2)
{
   if ((index1 < 0) || (index1 >= m_size) || (index2 < 0) || (index2 >= m_size) || (index1 == index2))
      return;

   if (m_storePointers)
   {
      void *temp = m_data[index1];
      m_data[index1] = m_data[index2];
      m_data[index2] = temp;
   }
   else
   {
      void *temp = MemAlloc(m_elementSize);
      memcpy(temp, ADDR(index1), m_elementSize);
      memcpy(ADDR(index1), ADDR(index2), m_elementSize);
      memcpy(ADDR(index2), temp, m_elementSize);
      MemFree(temp);
   }
}

/**
 * ********************************************************
 * 
 * Array iterator
 * 
 * ********************************************************
 */

/**
 * Array iterator
 */
ArrayIterator::ArrayIterator(Array *array, int pos)
{
   m_array = array;
   m_pos = pos;
}

/**
 * Next element availability indicator
 */
bool ArrayIterator::hasNext()
{
   return (m_pos + 1) < m_array->size();
}

/**
 * Get next element and advance iterator
 */
void *ArrayIterator::next()
{
   return m_array->get(++m_pos);
}

/**
 * Remove current element
 */
void ArrayIterator::remove()
{
   if ((m_pos >= m_array->size()) || (m_pos < 0))
      return;

   m_array->remove(m_pos);
   m_pos--;
}

/**
 * Remove current element without calling it's destructor
 */
void ArrayIterator::unlink()
{
   if ((m_pos >= m_array->size()) || (m_pos < 0))
      return;

   m_array->unlink(m_pos);
   m_pos--;
}

/**
 * Check iterators equality
 */
bool ArrayIterator::equals(AbstractIterator *other)
{
   return (other != nullptr) ? (m_pos == static_cast<ArrayIterator*>(other)->m_pos) : false;
}

/**
 * Get current value in C++ semantics (next value in Java semantics)
 */
void *ArrayIterator::value()
{
   return m_array->get(m_pos + 1);
}
