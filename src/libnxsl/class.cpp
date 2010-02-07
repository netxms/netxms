/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005-2010 Victor Kirhenshtein
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
** File: class.cpp
**
**/

#include "libnxsl.h"


//
// Class constructor
//

NXSL_Class::NXSL_Class()
{
   strcpy(m_szName, "generic");
}


//
// Class destructor
//

NXSL_Class::~NXSL_Class()
{
}


//
// Get attribute
// Default implementation - always returns error
//

NXSL_Value *NXSL_Class::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   return NULL;
}


//
// Set attribute
// Default implementation - always returns error
//

BOOL NXSL_Class::setAttr(NXSL_Object *pObject, const TCHAR *pszAttr, NXSL_Value *pValue)
{
   return FALSE;
}


//
// Object deletion handler
//

void NXSL_Class::onObjectDelete(NXSL_Object *object)
{
}


//
// Object constructors
//

NXSL_Object::NXSL_Object(NXSL_Class *pClass, void *pData)
{
   m_pClass = pClass;
   m_pData = pData;
}

NXSL_Object::NXSL_Object(NXSL_Object *pObject)
{
   m_pClass = pObject->m_pClass;
   m_pData = pObject->m_pData;
}


//
// Object destructor
//

NXSL_Object::~NXSL_Object()
{
}
