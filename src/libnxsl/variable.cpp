/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
#include <uthash.h>

/**
 * Create variable with NULL value
 */
NXSL_Variable::NXSL_Variable(const NXSL_Identifier& name) : m_name(name)
{
   m_name = name;
   m_value = new NXSL_Value;    // Create NULL value
   m_value->onVariableSet();
	m_constant = false;
}

/**
 * Create variable with given value
 */
NXSL_Variable::NXSL_Variable(const NXSL_Identifier& name, NXSL_Value *value, bool constant) : m_name(name)
{
   m_value = value;
   m_value->onVariableSet();
	m_constant = constant;
}

/**
 * Variable destructor
 */
NXSL_Variable::~NXSL_Variable()
{
   delete m_value;
}

/**
 * Set variable's value
 */
void NXSL_Variable::setValue(NXSL_Value *pValue)
{
   delete m_value;
   m_value = pValue;
   m_value->onVariableSet();
}

/**
 * Variable pointer structure
 */
struct NXSL_VariablePtr
{
   UT_hash_handle hh;
   NXSL_Variable v;
};

/**
 * Create new variable system
 */
NXSL_VariableSystem::NXSL_VariableSystem(bool constant)
{
   m_variables = NULL;
	m_isConstant = constant;
	m_restorePointCount = 0;
}

/**
 * Clone existing variable system
 */
NXSL_VariableSystem::NXSL_VariableSystem(NXSL_VariableSystem *src)
{
   m_variables = NULL;
   m_isConstant = src->m_isConstant;
   m_restorePointCount = 0;

   NXSL_VariablePtr *var, *tmp;
   HASH_ITER(hh, src->m_variables, var, tmp)
   {
      create(var->v.getName(), new NXSL_Value(var->v.getValue()));
   }
}

/**
 * Variable system destructor
 */
NXSL_VariableSystem::~NXSL_VariableSystem()
{
   clear();
   for(int i = 0; i < m_restorePointCount; i++)
      delete m_restorePoints[i].identifier;
}

/**
 * Clear variable system
 */
void NXSL_VariableSystem::clear()
{
   NXSL_VariablePtr *var, *tmp;
   HASH_ITER(hh, m_variables, var, tmp)
   {
      HASH_DEL(m_variables, var);
      var->v.~NXSL_Variable();
      free(var);
   }
}

/**
 * Merge with another variable system
 */
void NXSL_VariableSystem::merge(NXSL_VariableSystem *src)
{
   NXSL_VariablePtr *var, *tmp;
   HASH_ITER(hh, src->m_variables, var, tmp)
   {
      if (find(var->v.getName()) == NULL)
      {
         create(var->v.getName(), new NXSL_Value(var->v.getValue()));
      }
   }
}

/**
 * Callback for adding variables
 */
static EnumerationCallbackResult AddVariableCallback(const void *key, const void *value, void *data)
{
   if (((NXSL_VariableSystem *)data)->find(*static_cast<const NXSL_Identifier*>(key)) == NULL)
      ((NXSL_VariableSystem *)data)->create(*static_cast<const NXSL_Identifier*>(key), new NXSL_Value(static_cast<const NXSL_Value*>(value)));
   return _CONTINUE;
}

/**
 * Add all values from given map
 */
void NXSL_VariableSystem::addAll(HashMap<NXSL_Identifier, NXSL_Value> *src)
{
   src->forEach(AddVariableCallback, this);
}

/**
 * Find variable by name
 */
NXSL_Variable *NXSL_VariableSystem::find(const NXSL_Identifier& name)
{
   NXSL_VariablePtr *var;
   HASH_FIND(hh, m_variables, name.value, name.length, var);
   return (var != NULL) ? &var->v : NULL;
}

/**
 * Create variable
 */
NXSL_Variable *NXSL_VariableSystem::create(const NXSL_Identifier& name, NXSL_Value *value)
{
   NXSL_VariablePtr *var = (NXSL_VariablePtr *)calloc(1, sizeof(NXSL_VariablePtr));
   NXSL_Variable *v = new (&var->v) NXSL_Variable(name, (value != NULL) ? value : new NXSL_Value(), m_isConstant);
   HASH_ADD_KEYPTR(hh, m_variables, v->m_name.value, v->m_name.length, var);
   return v;
}

/**
 * Create restore point for variable reference
 */
bool NXSL_VariableSystem::createVariableReferenceRestorePoint(UINT32 addr, NXSL_Identifier *identifier)
{
   if (m_restorePointCount >= MAX_VREF_RESTORE_POINTS)
      return false;

   m_restorePoints[m_restorePointCount].addr = addr;
   m_restorePoints[m_restorePointCount].identifier = identifier;
   m_restorePointCount++;
   return true;
}

/**
 * Restore saved variable references
 */
void NXSL_VariableSystem::restoreVariableReferences(ObjectArray<NXSL_Instruction> *instructions)
{
   for(int i = 0; i < m_restorePointCount; i++)
      instructions->get(m_restorePoints[i].addr)->restoreVariableReference(m_restorePoints[i].identifier);
   m_restorePointCount = 0;
}
