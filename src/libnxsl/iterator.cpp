/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Helper function - create new iterator on stack
// On success, returns 0, otherwise error code
//

int NXSL_Iterator::createIterator(NXSL_Stack *stack)
{
	if (stack->getSize() < 2)
		return NXSL_ERR_DATA_STACK_UNDERFLOW;

	int rc = 0;

	NXSL_Value *value = (NXSL_Value *)stack->pop();
	if (value->isArray())
	{
		NXSL_Array *array = value->getValueAsArray();
		array->incRefCount();

		delete value;
		value = (NXSL_Value *)stack->pop();
		if (value->isString())
		{
			NXSL_Iterator *it = new NXSL_Iterator(value->getValueAsCString(), array);
			stack->push(new NXSL_Value(it));
		}
		else
		{
			array->decRefCount();
			if (array->isUnused())
				delete array;
			rc = NXSL_ERR_NOT_STRING;
		}
	}
	else
	{
		rc = NXSL_ERR_NOT_ARRAY;
	}
	delete value;

	return rc;
}


//
// Constructor
//

NXSL_Iterator::NXSL_Iterator(const TCHAR *variable, NXSL_Array *array)
{
	m_variable = _tcsdup(variable);
	m_array = array;
	m_refCount = 0;
	m_position = -1;
}


//
// Destructor
//

NXSL_Iterator::~NXSL_Iterator()
{
	m_array->decRefCount();
	if (m_array->isUnused())
		delete m_array;
	safe_free(m_variable);
}


//
// Get next element
//

NXSL_Value *NXSL_Iterator::next()
{
	m_position++;
	return m_array->getByPosition(m_position);
}
