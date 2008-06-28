/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: nxslext.cpp
**
**/

#include "nxcore.h"


//
// Externals
//

void RegisterDCIFunctions(NXSL_Environment *pEnv);


//
// Get node's custom attribute
// First argument is a node object, and second is an attribute name
//

static int F_GetCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	NXSL_Object *object;
	Node *node;
	const TCHAR *value;

	if (!argv[0]->IsObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->IsString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->GetValueAsObject();
	if (_tcscmp(object->Class()->Name(), "NetXMS_Node"))
		return NXSL_ERR_BAD_CLASS;

	node = (Node *)object->Data();
	value = node->GetCustomAttribute(argv[1]->GetValueAsCString());
	if (value != NULL)
	{
		*ppResult = new NXSL_Value(value);
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
	}

	return 0;
}


//
// Additional server functions to use within all scripts
//

static NXSL_ExtFunction m_nxslServerFunctions[] =
{
   { "GetCustomAttribute", F_GetCustomAttribute, 2 }
};


//
// Constructor for server default script environment
//

NXSL_ServerEnv::NXSL_ServerEnv()
               : NXSL_Environment()
{
	SetLibrary(g_pScriptLibrary);
	RegisterFunctionSet(sizeof(m_nxslServerFunctions) / sizeof(NXSL_ExtFunction), m_nxslServerFunctions);
	RegisterDCIFunctions(this);
}
