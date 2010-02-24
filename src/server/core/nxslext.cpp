/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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

extern DWORD g_nxslNumSituationFunctions;
extern NXSL_ExtFunction g_nxslSituationFunctions[];

void RegisterDCIFunctions(NXSL_Environment *pEnv);


//
// Get node's custom attribute
// First argument is a node object, and second is an attribute name
//

static int F_GetCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	NXSL_Object *object;
	Node *node;
	const TCHAR *value;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	node = (Node *)object->getData();
	value = node->GetCustomAttribute(argv[1]->getValueAsCString());
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
// Get interface name by index
// Parameters: node object and interface index
//

static int F_GetInterfaceName(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	Interface *ifc = node->FindInterface(argv[1]->getValueAsUInt32(), INADDR_ANY);
	if (ifc != NULL)
	{
		*ppResult = new NXSL_Value(ifc->Name());
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if interface not found
	}

	return 0;
}


//
// Find node object
// First argument: current node object or null
// Second argument: node id or name
// Returns node object or null if requested node was not found or access to it was denied
//

static int F_FindNodeObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	Node *currNode = NULL, *node = NULL;

	if (!argv[0]->isNull())
	{
		if (!argv[0]->isObject())
			return NXSL_ERR_NOT_OBJECT;

		NXSL_Object *object = argv[0]->getValueAsObject();
		if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
			return NXSL_ERR_BAD_CLASS;

		currNode = (Node *)object->getData();
	}

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	if (argv[1]->isInteger())
	{
		NetObj *o = FindObjectById(argv[1]->getValueAsUInt32());
		if ((o != NULL) && (o->Type() == OBJECT_NODE))
			node = (Node *)o;
	}
	else
	{
		node = (Node *)FindObjectByName(argv[1]->getValueAsCString(), OBJECT_NODE);
	}

	if (node != NULL)
	{
		if (g_dwFlags & AF_CHECK_TRUSTED_NODES)
		{
			if ((currNode != NULL) && (node->IsTrustedNode(currNode->Id())))
			{
				*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node));
			}
			else
			{
				// No access, return null
				*ppResult = new NXSL_Value;
			}
		}
		else
		{
			*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node));
		}
	}
	else
	{
		// Node not found, return null
		*ppResult = new NXSL_Value;
	}

	return 0;
}


//
// Additional server functions to use within all scripts
//

static NXSL_ExtFunction m_nxslServerFunctions[] =
{
   { "GetCustomAttribute", F_GetCustomAttribute, 2 },
   { "GetInterfaceName", F_GetInterfaceName, 2 },
	{ "FindNodeObject", F_FindNodeObject, 2 }
};


//
// Constructor for server default script environment
//

NXSL_ServerEnv::NXSL_ServerEnv()
               : NXSL_Environment()
{
	setLibrary(g_pScriptLibrary);
	registerFunctionSet(sizeof(m_nxslServerFunctions) / sizeof(NXSL_ExtFunction), m_nxslServerFunctions);
	RegisterDCIFunctions(this);
	registerFunctionSet(g_nxslNumSituationFunctions, g_nxslSituationFunctions);
}


//
// Script trace output
//

void NXSL_ServerEnv::trace(int level, const TCHAR *text)
{
	if (level == 0)
	{
		nxlog_write(MSG_OTHER, EVENTLOG_INFORMATION_TYPE, "s", text);
	}
	else
	{
		DbgPrintf(level, _T("%s"), text);
	}
}
