/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: storage.cpp
**
**/

#include "libnxsl.h"

/**
 * Constructor
 */
NXSL_Storage::NXSL_Storage()
{
}

/**
 * Destructor
 */
NXSL_Storage::~NXSL_Storage()
{
}

/**
 * Local storage constructor
 */
NXSL_LocalStorage::NXSL_LocalStorage() : NXSL_Storage()
{
   m_values = new StringObjectMap<NXSL_Value>(true);
}

/**
 * Local storage destructor
 */
NXSL_LocalStorage::~NXSL_LocalStorage()
{
   delete m_values;
}

/**
 * Write to storage. Storage becomes owner of provided value.
 * Passing NULL value will effectively remove value from storage.
 */
void NXSL_LocalStorage::write(const TCHAR *name, NXSL_Value *value)
{
   if ((value == NULL) || value->isNull())
      m_values->remove(name);
   else
      m_values->set(name, value);
}

/**
 * Read from storage. Returns new value owned by caller. Returns NXSL NULL if there are no value with given name.
 */
NXSL_Value *NXSL_LocalStorage::read(const TCHAR *name)
{
   NXSL_Value *v = m_values->get(name);
   return (v != NULL) ? new NXSL_Value(v) : new NXSL_Value();
}
