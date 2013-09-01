/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2013 Victor Kirhenshtein
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


//
// Variable destructor
//

NXSL_Variable::~NXSL_Variable()
{
   free(m_pszName);
   delete m_pValue;
}


//
// Set variable's value
//

void NXSL_Variable::setValue(NXSL_Value *pValue)
{
   delete m_pValue;
   m_pValue = pValue;
}


//
// Variable system constructors
//

NXSL_VariableSystem::NXSL_VariableSystem(bool constant)
{
   m_dwNumVariables = 0;
   m_ppVariableList = NULL;
	m_isConstant = constant;
}

NXSL_VariableSystem::NXSL_VariableSystem(NXSL_VariableSystem *pSrc)
{
   UINT32 i;

   m_dwNumVariables = pSrc->m_dwNumVariables;
   m_ppVariableList = (NXSL_Variable **)malloc(sizeof(NXSL_Variable *) * m_dwNumVariables);
   for(i = 0; i < m_dwNumVariables; i++)
      m_ppVariableList[i] = new NXSL_Variable(pSrc->m_ppVariableList[i]);
	m_isConstant = pSrc->m_isConstant;
}

/**
 * Variable system destructor
 */
NXSL_VariableSystem::~NXSL_VariableSystem()
{
   UINT32 i;

   for(i = 0; i < m_dwNumVariables; i++)
      delete m_ppVariableList[i];
   safe_free(m_ppVariableList);
}

/**
 * Merge with another variable system
 */
void NXSL_VariableSystem::merge(NXSL_VariableSystem *src)
{
	UINT32 i;

	for(i = 0; i < src->m_dwNumVariables; i++)
	{
		const TCHAR *name = src->m_ppVariableList[i]->getName();
		if (find(name) == NULL)
		{
			create(name, new NXSL_Value(src->m_ppVariableList[i]->getValue()));
		}
	}
}

/**
 * Find variable by name
 */
NXSL_Variable *NXSL_VariableSystem::find(const TCHAR *pszName)
{
   UINT32 i;

   for(i = 0; i < m_dwNumVariables; i++)
      if (!_tcscmp(pszName, m_ppVariableList[i]->getName()))
         return m_ppVariableList[i];
   return NULL;
}

/**
 * Create variable
 */
NXSL_Variable *NXSL_VariableSystem::create(const TCHAR *pszName, NXSL_Value *pValue)
{
   NXSL_Variable *pVar;

	pVar = new NXSL_Variable(pszName, (pValue != NULL) ? pValue : new NXSL_Value, m_isConstant);
   m_ppVariableList = (NXSL_Variable **)realloc(m_ppVariableList, sizeof(NXSL_Variable *) * (m_dwNumVariables + 1));
   m_ppVariableList[m_dwNumVariables] = pVar;
   m_dwNumVariables++;
   return pVar;
}
