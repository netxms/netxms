/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: variable.cpp
**
**/

#include "libnxsl.h"


//
// Variable constructors
//

NXSL_Variable::NXSL_Variable(const TCHAR *pszName)
{
   m_pszName = _tcsdup(pszName);
   m_pValue = new NXSL_Value;    // Create NULL value
	m_isConstant = false;
}

NXSL_Variable::NXSL_Variable(const TCHAR *pszName, NXSL_Value *pValue, bool constant)
{
   m_pszName = _tcsdup(pszName);
   m_pValue = pValue;
	m_isConstant = constant;
}

NXSL_Variable::NXSL_Variable(NXSL_Variable *pSrc)
{
   m_pszName = _tcsdup(pSrc->m_pszName);
   m_pValue = new NXSL_Value(pSrc->m_pValue);
	m_isConstant = pSrc->m_isConstant;
}

/**
 * Variable destructor
 */
NXSL_Variable::~NXSL_Variable()
{
   free(m_pszName);
   delete m_pValue;
}

/**
 * Set variable's value
 */
void NXSL_Variable::setValue(NXSL_Value *pValue)
{
   delete m_pValue;
   m_pValue = pValue;
}

/**
 * Create new variable system
 */
NXSL_VariableSystem::NXSL_VariableSystem(bool constant)
{
   m_variables = new ObjectArray<NXSL_Variable>(16, 16, true);
	m_isConstant = constant;
}

/**
 * Clone existing variable system
 */
NXSL_VariableSystem::NXSL_VariableSystem(NXSL_VariableSystem *src)
{
   m_variables = new ObjectArray<NXSL_Variable>(src->m_variables->size(), 16, true);
   for(int i = 0; i < src->m_variables->size(); i++)
      m_variables->add(new NXSL_Variable(src->m_variables->get(i)));
	m_isConstant = src->m_isConstant;
}

/**
 * Variable system destructor
 */
NXSL_VariableSystem::~NXSL_VariableSystem()
{
   delete m_variables;
}

/**
 * Merge with another variable system
 */
void NXSL_VariableSystem::merge(NXSL_VariableSystem *src)
{
	for(int i = 0; i < src->m_variables->size(); i++)
	{
      NXSL_Variable *v = src->m_variables->get(i);
      if (find(v->getName()) == NULL)
		{
         create(v->getName(), new NXSL_Value(v->getValue()));
		}
	}
}

/**
 * Callback for adding variables
 */
static EnumerationCallbackResult AddVariableCallback(const TCHAR *key, const void *value, void *data)
{
   if (((NXSL_VariableSystem *)data)->find(key) == NULL)
      ((NXSL_VariableSystem *)data)->create(key, new NXSL_Value((NXSL_Value *)value));
   return _CONTINUE;
}

/**
 * Add all values from given map
 */
void NXSL_VariableSystem::addAll(StringObjectMap<NXSL_Value> *src)
{
   src->forEach(AddVariableCallback, this);
}

/**
 * Find variable by name
 */
NXSL_Variable *NXSL_VariableSystem::find(const TCHAR *pszName)
{
   for(int i = 0; i < m_variables->size(); i++)
   {
      NXSL_Variable *v = m_variables->get(i);
      if (!_tcscmp(pszName, v->getName()))
         return v;
   }
   return NULL;
}

/**
 * Create variable
 */
NXSL_Variable *NXSL_VariableSystem::create(const TCHAR *pszName, NXSL_Value *pValue)
{
   NXSL_Variable *v = new NXSL_Variable(pszName, (pValue != NULL) ? pValue : new NXSL_Value, m_isConstant);
	m_variables->add(v);
   return v;
}
