/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
		*ppResult = new NXSL_Value;	// Return NULL if attribute not found
	}

	return 0;
}


//
// Set node's custom attribute
// First argument is a node object, second is an attribute name, third is new value
// Returns previous value
//

static int F_SetCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	NXSL_Object *object;
	Node *node;
	const TCHAR *value;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString() || !argv[2]->isString())
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
		*ppResult = new NXSL_Value;	// Return NULL if attribute not found
	}

	node->SetCustomAttribute(argv[1]->getValueAsCString(), argv[2]->getValueAsCString());

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
	Interface *ifc = node->findInterface(argv[1]->getValueAsUInt32(), INADDR_ANY);
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
// Get interface object by index
// Parameters: node object and interface index
//

static int F_GetInterfaceObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	Interface *ifc = node->findInterface(argv[1]->getValueAsUInt32(), INADDR_ANY);
	if (ifc != NULL)
	{
		*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, ifc));
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
				DbgPrintf(4, _T("NXSL::FindNodeObject(%s [%d], '%s'): access denied for node %s [%d]"),
				          (currNode != NULL) ? currNode->Name() : _T("null"), (currNode != NULL) ? currNode->Id() : 0,
							 argv[1]->getValueAsCString(), node->Name(), node->Id());
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
// Get node object's parents
// First argument: node object
// Returns array of accessible parent objects
//

static int F_GetNodeParents(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	*ppResult = new NXSL_Value(node->getParentsForNXSL());
	return 0;
}


//
// Get node's interfaces
// First argument: node object
// Returns array of interface objects
//

static int F_GetNodeInterfaces(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	*ppResult = new NXSL_Value(node->getInterfacesForNXSL());
	return 0;
}


//
// Get event's named parameter
// First argument: event object
// Second argument: parameter's name
// Returns parameter's value or null
//

static int F_GetEventParameter(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslEventClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	Event *e = (Event *)object->getData();
	const TCHAR *value = e->getNamedParameter(argv[1]->getValueAsCString());
	*ppResult = (value != NULL) ? new NXSL_Value(value) : new NXSL_Value;
	return 0;
}


//
// Set event's named parameter
// First argument: event object
// Second argument: parameter's name
// Third argument: new value
//

static int F_SetEventParameter(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslEventClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString() || !argv[2]->isString())
		return NXSL_ERR_NOT_STRING;

	Event *e = (Event *)object->getData();
	e->setNamedParameter(argv[1]->getValueAsCString(), argv[2]->getValueAsCString());
	*ppResult = new NXSL_Value;
	return 0;
}


//
// Post event
// Syntax:
//    PostEvent(node, event, tag, ...)
// where:
//     node - node object to send event on behalf of
//     event - event code
//     tag - user tag (optional)
//     ... - optional parameters, will be passed as %1, %2, etc.
//

static int F_PostEvent(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (argc < 2)
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	// Validate first argument
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	
	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();

	// Validate secod argument - event code
	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	// User tag
	const TCHAR *userTag = NULL;
	if (argc > 2)
	{
		if (!argv[2]->isString())
			return NXSL_ERR_NOT_STRING;
		userTag = argv[2]->getValueAsCString();
	}

	// Post event
	char format[] = "ssssssssssssssssssssssssssssssss";
	const TCHAR *plist[32];
	int eargc = 0;
	for(int i = 3; (i < argc) && (eargc < 32); i++)
		plist[eargc++] = argv[i]->getValueAsCString();
	format[eargc] = 0;
	BOOL success = PostEventWithTag(argv[1]->getValueAsUInt32(), node->Id(), userTag, format,
	                                plist[0], plist[1], plist[2], plist[3],
	                                plist[4], plist[5], plist[6], plist[7],
	                                plist[8], plist[9], plist[10], plist[11],
	                                plist[12], plist[13], plist[14], plist[15],
	                                plist[16], plist[17], plist[18], plist[19],
	                                plist[20], plist[21], plist[22], plist[23],
	                                plist[24], plist[25], plist[26], plist[27],
	                                plist[28], plist[29], plist[30], plist[31]);

	*ppResult = new NXSL_Value((LONG)success);
	return 0;
}


//
// Additional server functions to use within all scripts
//

static NXSL_ExtFunction m_nxslServerFunctions[] =
{
   { _T("GetCustomAttribute"), F_GetCustomAttribute, 2 },
   { _T("GetEventParameter"), F_GetEventParameter, 2 },
   { _T("GetInterfaceName"), F_GetInterfaceName, 2 },
   { _T("GetInterfaceObject"), F_GetInterfaceObject, 2 },
   { _T("GetNodeInterfaces"), F_GetNodeInterfaces, 1 },
   { _T("GetNodeParents"), F_GetNodeParents, 1 },
	{ _T("FindNodeObject"), F_FindNodeObject, 2 },
	{ _T("PostEvent"), F_PostEvent, -1 },
   { _T("SetCustomAttribute"), F_SetCustomAttribute, 3 },
   { _T("SetEventParameter"), F_SetEventParameter, 3 },
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
