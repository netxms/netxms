/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
NXSL_Array::NXSL_Array(NXSL_ValueManager *vm) : NXSL_HandleCountObject(vm)
{
   m_size = 0;
   m_allocated = 0;
   m_data = nullptr;
}

/**
 *  Create copy of given array
 */
NXSL_Array::NXSL_Array(const NXSL_Array& src) : NXSL_HandleCountObject(src.m_vm)
{
	m_size = m_allocated = src.m_size;
	if (m_size > 0)
	{
		m_data = MemAllocArrayNoInit<NXSL_ArrayElement>(m_size);
		for(int i = 0; i < m_size; i++)
		{
			m_data[i].index = src.m_data[i].index;
			m_data[i].value = m_vm->createValue(src.m_data[i].value);
		}
	}
	else
	{
		m_data = nullptr;
	}
}

/**
 * Create array populated with values from string list
 */
NXSL_Array::NXSL_Array(NXSL_ValueManager *vm, const StringList& values) : NXSL_HandleCountObject(vm)
{
	m_size = m_allocated = values.size();
	if (m_size > 0)
	{
      m_data = MemAllocArrayNoInit<NXSL_ArrayElement>(m_size);
		for(int i = 0; i < m_size; i++)
		{
			m_data[i].index = i;
			m_data[i].value = m_vm->createValue(values.get(i));
		}
	}
	else
	{
		m_data = nullptr;
	}
}

/**
 * Create array populated with values from string set
 */
NXSL_Array::NXSL_Array(NXSL_ValueManager *vm, const StringSet& values) : NXSL_HandleCountObject(vm)
{
   m_size = m_allocated = values.size();
   if (m_size > 0)
   {
      m_data = MemAllocArrayNoInit<NXSL_ArrayElement>(m_size);
      int index = 0;
      auto it = values.begin();
      while(it.hasNext())
      {
         m_data[index].index = index;
         m_data[index].value = m_vm->createValue(it.next());
         index++;
      }
   }
   else
   {
      m_data = nullptr;
   }
}

/**
 * Destructor
 */
NXSL_Array::~NXSL_Array()
{
   for(int i = 0; i < m_size; i++)
      m_vm->destroyValue(m_data[i].value);
   MemFree(m_data);
}

/**
 * Compare two indexes
 */
static int CompareElements(const void *p1, const void *p2)
{
   return COMPARE_NUMBERS(static_cast<const NXSL_ArrayElement*>(p1)->index, static_cast<const NXSL_ArrayElement*>(p2)->index);
}

/**
 * Get element by index
 */
NXSL_Value *NXSL_Array::get(int index) const
{
   NXSL_ArrayElement key;
   key.index = index;
   NXSL_ArrayElement *element = static_cast<NXSL_ArrayElement*>(bsearch(&key, m_data, m_size, sizeof(NXSL_ArrayElement), CompareElements));
   return (element != nullptr) ? element->value : nullptr;
}

/**
 * Get element by internal position (used by iterator)
 */
NXSL_Value *NXSL_Array::getByPosition(int position) const
{
   if ((position < 0) || (position >= m_size))
      return nullptr;
   return m_data[position].value;
}

/**
 * Get all elements as string list
 */
StringList NXSL_Array::toStringList() const
{
   StringList list;
   for(int i = 0; i < m_size; i++)
      list.add(m_data[i].value->getValueAsCString());
   return list;
}

/**
 * Get all elements as string list
 */
void NXSL_Array::toStringList(StringList *list) const
{
   for(int i = 0; i < m_size; i++)
      list->add(m_data[i].value->getValueAsCString());
}

/**
 * Get all elements as string set
 */
StringSet *NXSL_Array::toStringSet() const
{
   auto set = new StringSet();
   for(int i = 0; i < m_size; i++)
      set->add(m_data[i].value->getValueAsCString());
   return set;
}

/**
 * Get all elements as string set
 */
void NXSL_Array::toStringSet(StringSet *set) const
{
   for(int i = 0; i < m_size; i++)
      set->add(m_data[i].value->getValueAsCString());
}

/**
 * Set element
 */
void NXSL_Array::set(int index, NXSL_Value *value)
{
	NXSL_ArrayElement *element, key;
	key.index = index;
	element = (m_size > 0) ? (NXSL_ArrayElement *)bsearch(&key, m_data, m_size, sizeof(NXSL_ArrayElement), CompareElements) : nullptr;
	if (element != nullptr)
	{
		m_vm->destroyValue(element->value);
		element->value = value;
	}
	else
	{
		if (m_size == m_allocated)
		{
			m_allocated += 64;
			m_data = MemReallocArray(m_data, m_allocated);
		}
		m_data[m_size].index = index;
		m_data[m_size].value = value;
		m_size++;
		if ((m_size > 1) && (m_data[m_size - 2].index > index))
		{
		   // do not sort if adding at the end
		   qsort(m_data, m_size, sizeof(NXSL_ArrayElement), CompareElements);
		}
	}
	value->onVariableSet();
}

/**
 * Insert value into array
 */
void NXSL_Array::insert(int index, NXSL_Value *value)
{
   int i = m_size - 1;
   while((i >= 0) && (m_data[i].index >= index))
   {
      m_data[i].index++;
      i--;
   }

   if (m_size == m_allocated)
   {
      m_allocated += 32;
      m_data = MemReallocArray(m_data, m_allocated);
   }

   i++;
   m_size++;
   memmove(&m_data[i + 1], &m_data[i], sizeof(NXSL_ArrayElement) * (m_size - i));
   m_data[i].index = index;
   m_data[i].value = value;
}

/**
 * Remove value from array
 */
void NXSL_Array::remove(int index)
{
   if (m_size == 0)
      return;

   int i = m_size - 1;
   while((i >= 0) && (m_data[i].index > index))
   {
      m_data[i].index--;
      i--;
   }
   if ((i >= 0) && (m_data[i].index == index))
   {
      m_vm->destroyValue(m_data[i].value);
      m_size--;
      memmove(&m_data[i], &m_data[i + 1], sizeof(NXSL_ArrayElement) * (m_size - i));
   }
}

/**
 * Call method on array
 */
int NXSL_Array::callMethod(const NXSL_Identifier& name, int argc, NXSL_Value **argv, NXSL_Value **result)
{
   if (!strcmp(name.value, "append") || !strcmp(name.value, "push"))
   {
      if (argc != 1)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;
      append(m_vm->createValue(argv[0]));
      *result = m_vm->createValue(getMaxIndex());
   }
   else if (!strcmp(name.value, "appendAll"))
   {
      if (argc != 1)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;

      if (!argv[0]->isArray())
         return NXSL_ERR_NOT_ARRAY;

      NXSL_Array *a = argv[0]->getValueAsArray();
      int size = a->size();
      for(int i = 0; i < size; i++)
         append(m_vm->createValue(a->getByPosition(i)));
      *result = m_vm->createValue(getMaxIndex());
   }
   else if (!strcmp(name.value, "indexOf"))
   {
      if (argc != 1)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;
      NXSL_ArrayElement *e = find(argv[0]);
      *result = (e != nullptr) ? m_vm->createValue(e->index) : m_vm->createValue();
   }
   else if (!strcmp(name.value, "insert"))
   {
      if (argc != 2)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;

      if (!argv[0]->isInteger())
         return NXSL_ERR_NOT_INTEGER;

      insert(argv[0]->getValueAsInt32(), m_vm->createValue(argv[1]));
      *result = m_vm->createValue();
   }
   else if (!strcmp(name.value, "insertAll"))
   {
      if (argc != 2)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;

      if (!argv[0]->isInteger())
         return NXSL_ERR_NOT_INTEGER;

      if (!argv[1]->isArray())
         return NXSL_ERR_NOT_ARRAY;

      int index = argv[0]->getValueAsInt32();
      NXSL_Array *a = argv[1]->getValueAsArray();
      int size = a->size();
      for(int i = 0; i < size; i++)
         insert(index++, m_vm->createValue(a->getByPosition(i)));
      *result = m_vm->createValue(getMaxIndex());
   }
   else if (!strcmp(name.value, "join"))
   {
      if (argc != 1)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;

      if (!argv[0]->isString())
         return NXSL_ERR_NOT_STRING;

      StringBuffer s;
      toString(&s, argv[0]->getValueAsCString(), false);
      *result = m_vm->createValue(s);
   }
   else if (!strcmp(name.value, "pop"))
   {
      if (argc != 0)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;

      *result = (m_size > 0) ? m_data[--m_size].value : m_vm->createValue();
   }
   else if (!strcmp(name.value, "remove"))
   {
      if (argc != 1)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;

      if (!argv[0]->isInteger())
         return NXSL_ERR_NOT_INTEGER;

      remove(argv[0]->getValueAsInt32());
      *result = m_vm->createValue();
   }
   else
   {
      return NXSL_ERR_NO_SUCH_METHOD;
   }
   return 0;
}

/**
 * Check if given value is in array
 */
NXSL_ArrayElement *NXSL_Array::find(NXSL_Value *value) const
{
   for(int i = 0; i < m_size; i++)
   {
      NXSL_Value *curr = m_data[i].value;
      if ((curr->getDataType() == value->getDataType()) && curr->isNumeric())
      {
         if (curr->EQ(value))
            return &m_data[i];
      }
      else if (value->isInteger() && curr->isInteger())
      {
         if (value->getValueAsInt64() == curr->getValueAsInt64())
            return &m_data[i];
      }
      else if (value->isNumeric() && curr->isNumeric())
      {
         if (value->getValueAsReal() == curr->getValueAsReal())
            return &m_data[i];
      }
      else if (value->isString() && curr->isString())
      {
         uint32_t l1, l2;
         const TCHAR *s1 = value->getValueAsString(&l1);
         const TCHAR *s2 = curr->getValueAsString(&l2);
         if ((l1 == l2) && !memcmp(s1, s2, l1 * sizeof(TCHAR)))
            return &m_data[i];
      }
      else if (value->isNull() && curr->isNull())
      {
         return &m_data[i];
      }
   }
   return nullptr;
}

/**
 * Convert array to string (recursively for array and hash map values)
 */
void NXSL_Array::toString(StringBuffer *stringBuffer, const TCHAR *separator, bool withBrackets) const
{
   if (withBrackets)
      stringBuffer->append(_T("["));
   for(int i = 0; i < m_size; i++)
   {
      if (!stringBuffer->isEmpty() && (!withBrackets || (i > 0)))
         stringBuffer->append(separator);
      NXSL_Value *e = m_data[i].value;
      if (e->isArray())
      {
         e->getValueAsArray()->toString(stringBuffer, separator, withBrackets);
      }
      else if (e->isHashMap())
      {
         e->getValueAsHashMap()->toString(stringBuffer, separator, withBrackets);
      }
      else
      {
         stringBuffer->append(e->getValueAsCString());
      }
   }
   if (withBrackets)
      stringBuffer->append(_T("]"));
}
