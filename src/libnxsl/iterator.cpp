/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: iterator.cpp
**
**/

#include "libnxsl.h"

/**
 * Helper function - create new iterator on stack
 * On success, returns 0, otherwise error code
 */
int NXSL_Iterator::createIterator(NXSL_VM *vm, NXSL_Stack *stack)
{
	if (stack->getPosition() < 2)
		return NXSL_ERR_DATA_STACK_UNDERFLOW;

	int rc = 0;

	NXSL_Value *value = (NXSL_Value *)stack->pop();
	if (value->isArray() || value->isNull() || ((value->getDataType() == NXSL_DT_BOOLEAN) && value->isFalse()))
	{
		NXSL_Array *array = value->isArray() ? value->getValueAsArray() : new NXSL_Array(vm);
		array->incHandleCount();

		vm->destroyValue(value);
		value = (NXSL_Value *)stack->pop();
		if (value->isString())
		{
			NXSL_Iterator *it = new NXSL_Iterator(vm, value->getValueAsMBString(), array);
			stack->push(vm->createValue(it));
		}
		else
		{
			array->decHandleCount();
			if (array->isUnused())
				delete array;
			rc = NXSL_ERR_NOT_STRING;
		}
	}
	else if (value->isObject() && value->getValueAsObject()->isIterable())
	{
	   NXSL_Value *object = value;
      value = (NXSL_Value *)stack->pop();
      if (value->isString())
      {
         NXSL_Iterator *it = new NXSL_Iterator(vm, value->getValueAsMBString(), object);
         stack->push(vm->createValue(it));
      }
      else
      {
         vm->destroyValue(object);
         rc = NXSL_ERR_NOT_STRING;
      }
	}
	else
	{
		rc = NXSL_ERR_NOT_ITERABLE;
	}
	vm->destroyValue(value);

	return rc;
}

/**
 * Constructor for array iterator
 */
NXSL_Iterator::NXSL_Iterator(NXSL_VM *vm, const NXSL_Identifier& variable, NXSL_Array *array) : NXSL_RuntimeObject(vm), m_variable(variable)
{
	m_array = array;
	m_refCount = 0;
	m_position = -1;
	m_object = nullptr;
	m_currValue = nullptr;
}

/**
 * Constructor for object iterator
 */
NXSL_Iterator::NXSL_Iterator(NXSL_VM *vm, const NXSL_Identifier& variable, NXSL_Value *object) : NXSL_RuntimeObject(vm), m_variable(variable)
{
   m_array = nullptr;
   m_refCount = 0;
   m_position = -1;
   m_object = object;
   m_currValue = nullptr;
}

/**
 * Destructor
 */
NXSL_Iterator::~NXSL_Iterator()
{
   if (m_array != nullptr)
   {
      m_array->decHandleCount();
      if (m_array->isUnused())
         delete m_array;
   }
   m_vm->destroyValue(m_object);
   m_vm->destroyValue(m_currValue);
}

/**
 * Get next element
 */
NXSL_Value *NXSL_Iterator::next()
{
   if (m_array != nullptr)
   {
      m_position++;
      return m_array->getByPosition(m_position);
   }
   NXSL_Value *nextValue = m_object->getValueAsObject()->getNext(m_currValue);
   m_vm->destroyValue(m_currValue);
   m_currValue = nextValue;
   return m_currValue;
}
