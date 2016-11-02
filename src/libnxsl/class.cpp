/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: class.cpp
**
**/

#include "libnxsl.h"

/**
 * Class constructor
 */
NXSL_Class::NXSL_Class()
{
   setName(_T("Object"));
   m_methods = new StringObjectMap<NXSL_ExtMethod>(true);
   m_methods->setIgnoreCase(false);
}

/**
 * Class destructor
 */
NXSL_Class::~NXSL_Class()
{
   delete m_methods;
}

/**
 * Set class name. Should be called only from constructor.
 */
void NXSL_Class::setName(const TCHAR *name)
{
   nx_strncpy(m_name, name, MAX_CLASS_NAME);
   m_classHierarchy.add(name);
}

/**
 * Get attribute
 * Default implementation always returns error
 */
NXSL_Value *NXSL_Class::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   return NULL;
}

/**
 * Set attribute
 * Default implementation always returns error
 */
bool NXSL_Class::setAttr(NXSL_Object *object, const TCHAR *attr, NXSL_Value *value)
{
   return false;
}

/**
 * Call method
 * Default implementation calls methods registered with NXSL_REGISTER_METHOD macro.
 */
int NXSL_Class::callMethod(const TCHAR *name, NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   NXSL_ExtMethod *m = m_methods->get(name);
   if (m != NULL)
   {
      if ((argc == m->numArgs) || (m->numArgs == -1))
         return m->handler(object, argc, argv, result, vm);
      else
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   }
   return NXSL_ERR_NO_SUCH_METHOD;
}

/**
 * Object creation handler. Called by interpreter immediately after new object was created.
 */
void NXSL_Class::onObjectCreate(NXSL_Object *object)
{
}

/**
 * Object deletion handler. Called by interpreter when last reference to object being deleted.
 */
void NXSL_Class::onObjectDelete(NXSL_Object *object)
{
}

/**
 * Create new NXSL object of given class
 */
NXSL_Object::NXSL_Object(NXSL_Class *nxslClass, void *data)
{
   m_class = nxslClass;
	m_data = (__nxsl_class_data *)malloc(sizeof(__nxsl_class_data));
	m_data->refCount = 1;
	m_data->data = data;
	m_class->onObjectCreate(this);
}

/**
 * Create new reference to existing object
 */
NXSL_Object::NXSL_Object(NXSL_Object *object)
{
   m_class = object->m_class;
   m_data = object->m_data;
	m_data->refCount++;
}

/**
 * Object destructor
 */
NXSL_Object::~NXSL_Object()
{
	m_data->refCount--;
	if (m_data->refCount == 0)
	{
		m_class->onObjectDelete(this);
		free(m_data);
	}
}
