/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Externals
 */
extern UINT32 g_nxslNumSituationFunctions;
extern NXSL_ExtFunction g_nxslSituationFunctions[];

void RegisterDCIFunctions(NXSL_Environment *pEnv);
int F_map(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);

/**
 * Get node's custom attribute
 * First argument is a node object, and second is an attribute name
 */
static int F_GetCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	NXSL_Object *object;
	const TCHAR *value;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
	    _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netxmsObject = (NetObj *)object->getData();
	value = netxmsObject->getCustomAttribute(argv[1]->getValueAsCString());
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

/**
 * Set node's custom attribute
 * First argument is a node object, second is an attribute name, third is new value
 * Returns previous value
 */
static int F_SetCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	NXSL_Object *object;
	const TCHAR *value;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString() || !argv[2]->isString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
	    _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netxmsObject = (NetObj *)object->getData();
	value = netxmsObject->getCustomAttribute(argv[1]->getValueAsCString());
	if (value != NULL)
	{
		*ppResult = new NXSL_Value(value);
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if attribute not found
	}

	netxmsObject->setCustomAttribute(argv[1]->getValueAsCString(), argv[2]->getValueAsCString());

	return 0;
}

/**
 * Delete node's custom attribute
 * First argument is a node object, second is an attribute name
 */
static int F_DeleteCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	NXSL_Object *object;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
	    _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netxmsObject = (NetObj *)object->getData();
	netxmsObject->deleteCustomAttribute(argv[1]->getValueAsCString());
   *ppResult = new NXSL_Value;
	return 0;
}

/**
 * Get interface name by index
 * Parameters: node object and interface index
 */
static int F_GetInterfaceName(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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
		*ppResult = new NXSL_Value(ifc->getName());
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if interface not found
	}

	return 0;
}

/**
 * Get interface object by index
 * Parameters: node object and interface index
 */
static int F_GetInterfaceObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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

/**
 * Find node object
 * First argument: current node object or null
 * Second argument: node id or name
 * Returns node object or null if requested node was not found or access to it was denied
 */
static int F_FindNodeObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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
		if ((o != NULL) && (o->getObjectClass() == OBJECT_NODE))
			node = (Node *)o;
	}
	else
	{
		node = (Node *)FindObjectByName(argv[1]->getValueAsCString(), OBJECT_NODE);
	}

	if (node != NULL)
	{
		if (g_flags & AF_CHECK_TRUSTED_NODES)
		{
			if ((currNode != NULL) && (node->isTrustedNode(currNode->getId())))
			{
				*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node));
			}
			else
			{
				// No access, return null
				*ppResult = new NXSL_Value;
				DbgPrintf(4, _T("NXSL::FindNodeObject(%s [%d], '%s'): access denied for node %s [%d]"),
				          (currNode != NULL) ? currNode->getName() : _T("null"), (currNode != NULL) ? currNode->getId() : 0,
							 argv[1]->getValueAsCString(), node->getName(), node->getId());
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

/**
 * Find object
 * First argument: object id or name
 * Second argument (optional): current node object or null
 * Returns generic object or null if requested object was not found or access to it was denied
 */
static int F_FindObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	NetObj *object = NULL;

	if ((argc !=1) && (argc != 2))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	if (argc == 2 && (!argv[1]->isNull() && !argv[1]->isObject()))
		return NXSL_ERR_NOT_OBJECT;

	if (argv[0]->isInteger())
	{
		object = FindObjectById(argv[0]->getValueAsUInt32());
	}
	else
	{
		object = FindObjectByName(argv[0]->getValueAsCString(), -1);
	}

	if (object != NULL)
	{
		if (g_flags & AF_CHECK_TRUSTED_NODES)
		{
			Node *currNode = NULL;
			if ((argc == 2) && !argv[1]->isNull())
			{
				NXSL_Object *o = argv[1]->getValueAsObject();
				if (_tcscmp(o->getClass()->getName(), g_nxslNodeClass.getName()))
					return NXSL_ERR_BAD_CLASS;

				currNode = (Node *)o->getData();
			}
			if ((currNode != NULL) && (object->isTrustedNode(currNode->getId())))
			{
				*ppResult = new NXSL_Value(new NXSL_Object((object->getObjectClass() == OBJECT_NODE) ? (NXSL_Class *)&g_nxslNodeClass : (NXSL_Class *)&g_nxslNetObjClass, object));
			}
			else
			{
				// No access, return null
				*ppResult = new NXSL_Value;
				DbgPrintf(4, _T("NXSL::FindObject('%s', %s [%d]): access denied for node %s [%d]"),
				          argv[0]->getValueAsCString(),
				          (currNode != NULL) ? currNode->getName() : _T("null"), (currNode != NULL) ? currNode->getId() : 0,
							 object->getName(), object->getId());
			}
		}
		else
		{
			*ppResult = new NXSL_Value(new NXSL_Object((object->getObjectClass() == OBJECT_NODE) ? (NXSL_Class *)&g_nxslNodeClass : (NXSL_Class *)&g_nxslNetObjClass, object));
		}
	}
	else
	{
		// Object not found, return null
		*ppResult = new NXSL_Value;
	}

	return 0;
}

/**
 * Get node object's parents
 * First argument: node object
 * Returns array of accessible parent objects
 */
static int F_GetNodeParents(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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

/**
 * Get node object's templates
 * First argument: node object
 * Returns array of accessible template objects
 */
static int F_GetNodeTemplates(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	*ppResult = new NXSL_Value(node->getTemplatesForNXSL());
	return 0;
}

/**
 * Get object's parents
 * First argument: NetXMS object (NetObj, Node, or Interface)
 * Returns array of accessible parent objects
 */
static int F_GetObjectParents(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
	    _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netobj = (NetObj *)object->getData();
	*ppResult = new NXSL_Value(netobj->getParentsForNXSL());
	return 0;
}

/**
 * Get object's children
 * First argument: NetXMS object (NetObj, Node, or Interface)
 * Returns array of accessible child objects
 */
static int F_GetObjectChildren(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
	    _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netobj = (NetObj *)object->getData();
	*ppResult = new NXSL_Value(netobj->getChildrenForNXSL());
	return 0;
}

/**
 * Get node's interfaces
 * First argument: node object
 * Returns array of interface objects
 */
static int F_GetNodeInterfaces(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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

/**
 * Get event's named parameter
 * First argument: event object
 * Second argument: parameter's name
 * Returns parameter's value or null
 */
static int F_GetEventParameter(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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

/**
 * Set event's named parameter
 * First argument: event object
 * Second argument: parameter's name
 * Third argument: new value
 */
static int F_SetEventParameter(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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

/**
 * Post event
 * Syntax:
 *    PostEvent(node, event, tag, ...)
 * where:
 *     node - node object to send event on behalf of
 *     event - event code
 *     tag - user tag (optional)
 *     ... - optional parameters, will be passed as %1, %2, etc.
 */
static int F_PostEvent(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
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

	// Event code
	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	UINT32 eventCode = 0;
	if (argv[1]->isInteger())
	{
		eventCode = argv[1]->getValueAsUInt32();
	}
	else
	{
		eventCode = EventCodeFromName(argv[1]->getValueAsCString(), 0);
	}

	BOOL success;
	if (eventCode > 0)
	{
		// User tag
		const TCHAR *userTag = NULL;
		if ((argc > 2) && !argv[2]->isNull())
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
		success = PostEventWithTag(eventCode, node->getId(), userTag, format,
		                           plist[0], plist[1], plist[2], plist[3],
		                           plist[4], plist[5], plist[6], plist[7],
		                           plist[8], plist[9], plist[10], plist[11],
		                           plist[12], plist[13], plist[14], plist[15],
		                           plist[16], plist[17], plist[18], plist[19],
		                           plist[20], plist[21], plist[22], plist[23],
		                           plist[24], plist[25], plist[26], plist[27],
		                           plist[28], plist[29], plist[30], plist[31]);
	}
	else
	{
		success = FALSE;
	}

	*ppResult = new NXSL_Value((LONG)success);
	return 0;
}

/**
 * Create node object
 * Syntax:
 *    CreateNode(parent, name, primaryHostName)
 * where:
 *     parent          - parent object
 *     name            - name for new node
 *     primaryHostName - primary host name for new node
 * Return value:
 *     new node object or null on failure
 */
static int F_CreateNode(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *parent = (NetObj*)obj->getData();
	if (parent->getObjectClass() != OBJECT_CONTAINER && parent->getObjectClass() != OBJECT_SERVICEROOT)
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString() || !argv[2]->isString())
		return NXSL_ERR_NOT_STRING;

	const TCHAR *pname = argv[2]->getValueAsCString();
	if (*pname == 0)
		pname = argv[1]->getValueAsCString();
	Node *node = PollNewNode(ntohl(ResolveHostName(pname)), 0, 0, 0, 0, argv[1]->getValueAsCString(), 0, 0, NULL, 0, true, false);
	if (node != NULL)
	{
		node->setPrimaryName(pname);
		parent->AddChild(node);
		node->AddParent(parent);
		node->unhide();
		*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node));
	}
	else
	{
		*ppResult = new NXSL_Value;
	}
	return 0;
}

/**
 * Create container object
 * Syntax:
 *    CreateContainer(parent, name)
 * where:
 *     parent - parent object
 *     name   - name for new container
 * Return value:
 *     new container object
 */
static int F_CreateContainer(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *parent = (NetObj*)obj->getData();
	if (parent->getObjectClass() != OBJECT_CONTAINER && parent->getObjectClass() != OBJECT_SERVICEROOT)
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	const TCHAR *name = argv[1]->getValueAsCString();

	Container *container = new Container(name, 0);
	NetObjInsert(container, TRUE);
	parent->AddChild(container);
	container->AddParent(parent);
	container->unhide();

	*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, container));

	return 0;
}

/**
 * Delete object
 * Syntax:
 *    DeleteObject(object)
 * where:
 *     object - object to remove, must be of class NetObj, Interface, or Node
 * Return value:
 *     null
 */
static int F_DeleteObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslNetObjClass.getName()) &&
       _tcscmp(obj->getClass()->getName(), g_nxslInterfaceClass.getName()) &&
       _tcscmp(obj->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netobj = (NetObj*)obj->getData();
	netobj->deleteObject();

	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Bind object to container
 * Syntax:
 *    BindObject(parent, child)
 * where:
 *     parent - container object
 *     child  - either node or container or subnet to be bound to parent
 * Return value:
 *     null
 */
static int F_BindObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject() || !argv[1]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netobj = (NetObj*)obj->getData();
	if (netobj->getObjectClass() != OBJECT_CONTAINER)
		return NXSL_ERR_BAD_CLASS;

	NXSL_Object *obj2 = argv[1]->getValueAsObject();
	if (_tcscmp(obj2->getClass()->getName(), g_nxslNetObjClass.getName()) &&
		_tcscmp(obj2->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *child = (NetObj*)obj2->getData();
	if (child->getObjectClass() != OBJECT_CONTAINER && child->getObjectClass() != OBJECT_SUBNET && child->getObjectClass() != OBJECT_NODE)
		return NXSL_ERR_BAD_CLASS;

	if (child->isChild(netobj->getId())) // prevent loops
		return NXSL_ERR_INVALID_OBJECT_OPERATION;

	netobj->AddChild(child);
	child->AddParent(netobj);
	netobj->calculateCompoundStatus();

	*ppResult = new NXSL_Value;

	return 0;
}

/**
 * Remove (unbind) object from container
 * Syntax:
 *    UnbindObject(parent, child)
 * where:
 *     parent - container object
 *     child  - either node or container or subnet to be removed from container
 * Return value:
 *     null
 */
static int F_UnbindObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject() || !argv[1]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netobj = (NetObj*)obj->getData();
	if (netobj->getObjectClass() != OBJECT_CONTAINER)
		return NXSL_ERR_BAD_CLASS;

	NXSL_Object *obj2 = argv[1]->getValueAsObject();
	if (_tcscmp(obj2->getClass()->getName(), g_nxslNetObjClass.getName()) &&
		_tcscmp(obj2->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *child = (NetObj*)obj2->getData();
	if (child->getObjectClass() != OBJECT_CONTAINER && child->getObjectClass() != OBJECT_SUBNET && child->getObjectClass() != OBJECT_NODE)
		return NXSL_ERR_BAD_CLASS;

	netobj->DeleteChild(child);
	child->DeleteParent(netobj);

	*ppResult = new NXSL_Value;

	return 0;
}

/**
 * Rename object
 * Syntax:
 *    RenameObject(object, name)
 * where:
 *     object - NetXMS object (Node, Interface, or NetObj)
 *     name   - new name for object
 * Return value:
 *     null
 */
static int F_RenameObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	((NetObj *)object->getData())->setName(argv[1]->getValueAsCString());

	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Manage object (set to managed state)
 * Syntax:
 *    ManageObject(object)
 * where:
 *     object - NetXMS object (Node, Interface, or NetObj)
 * Return value:
 *     null
 */
static int F_ManageObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	((NetObj *)object->getData())->setMgmtStatus(TRUE);

	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Unmanage object (set to unmanaged state)
 * Syntax:
 *    UnmanageObject(object)
 * where:
 *     object - NetXMS object (Node, Interface, or NetObj)
 * Return value:
 *     null
 */
static int F_UnmanageObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNetObjClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) &&
		 _tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	((NetObj *)object->getData())->setMgmtStatus(FALSE);

	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Set interface expected state
 * Syntax:
 *    SetInterfaceExpectedState(interface, state)
 * where:
 *     interface - Interface object
 *     state     - state ID or name. Possible values: 0 "UP", 1 "DOWN", 2 "IGNORE"
 * Return value:
 *     null
 */
static int F_SetInterfaceExpectedState(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslInterfaceClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	int state;
	if (argv[1]->isInteger())
	{
		state = argv[1]->getValueAsInt32();
	}
	else if (argv[1]->isString())
	{
		static const TCHAR *stateNames[] = { _T("UP"), _T("DOWN"), _T("IGNORE"), NULL };
		const TCHAR *name = argv[1]->getValueAsCString();
		for(state = 0; stateNames[state] != NULL; state++)
			if (!_tcsicmp(stateNames[state], name))
				break;
	}
	else
	{
		return NXSL_ERR_NOT_STRING;
	}

	if ((state >= 0) && (state <= 2))
		((Interface *)object->getData())->setExpectedState(state);

	*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Create new SNMP transport object
 * Syntax:
 *    CreateSNMPTransport(node)
 * where:
 *     node - node to create SNMP transport for
 * Return value:
 *     new SNMP_Transport object
 */
static int F_CreateSNMPTransport(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node*)obj->getData();
	if (node != NULL)
	{
		SNMP_Transport *t = node->createSnmpTransport();
      *ppResult = (t != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslSnmpTransportClass, t)) : new NXSL_Value;
	}
	else
	{
		*ppResult = new NXSL_Value;
	}

	return 0;
}

/**
 * Do SNMP GET for the given object id
 * Syntax:
 *    SNMPGet(transport, oid)
 * where:
 *     transport - NXSL transport object
 *		 oid - SNMP object id
 * Return value:
 *     new SNMP_VarBind object
 */
static int F_SNMPGet(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	UINT32 len;
	static UINT32 requestId = 1;
	UINT32 varName[MAX_OID_LEN], result = SNMP_ERR_SUCCESS;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslSnmpTransportClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	SNMP_Transport *trans = (SNMP_Transport*)obj->getData();

   // Create PDU and send request
   size_t nameLen = SNMPParseOID(argv[1]->getValueAsString(&len), varName, MAX_OID_LEN);
   if (nameLen == 0)
		return NXSL_ERR_BAD_CONDITION;

   SNMP_PDU *pdu = new SNMP_PDU(SNMP_GET_REQUEST, requestId++, trans->getSnmpVersion());
	pdu->bindVariable(new SNMP_Variable(varName, nameLen));

	SNMP_PDU *rspPDU;
   result = trans->doRequest(pdu, &rspPDU, g_snmpTimeout, 3 /* num retries */);
   if (result == SNMP_ERR_SUCCESS)
   {
      if ((rspPDU->getNumVariables() > 0) && (rspPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         SNMP_Variable *pVar = rspPDU->getVariable(0);
		   *ppResult = new NXSL_Value(new NXSL_Object(&g_nxslSnmpVarBindClass, pVar));
         rspPDU->unlinkVariables();
	   }
      else
      {
   		*ppResult = new NXSL_Value;
      }
      delete rspPDU;
   }
	else
	{
		*ppResult = new NXSL_Value;
	}
   delete pdu;
	return 0;
}

/**
 * Do SNMP GET for the given object id
 * Syntax:
 *    SNMPGetValue(transport, oid)
 * where:
 *     transport - NXSL transport object
 *		 oid - SNMP object id
 * Return value:
 *     value for the given oid
 */
static int F_SNMPGetValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	TCHAR buffer[4096];
	UINT32 len;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslSnmpTransportClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	SNMP_Transport *trans = (SNMP_Transport*)obj->getData();

	if (SnmpGetEx(trans, argv[1]->getValueAsString(&len), NULL, 0, buffer, sizeof(buffer), SG_STRING_RESULT, NULL) == SNMP_ERR_SUCCESS)
	{
		*ppResult = new NXSL_Value(buffer);
	}
	else
	{
		*ppResult = new NXSL_Value;
	}

	return 0;
}

/**
 * Do SNMP SET for the given object id
 * Syntax:
 *    SNMPSet(transport, oid, value, [data_type])
 * where:
 *     transport - NXSL transport object
 *		 oid - SNMP object id
 *		 value - value to set
 *		 data_type (optional)
 * Return value:
 *     value for the given oid
 */
static int F_SNMPSet(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	SNMP_PDU *request = NULL, *response;
	UINT32 len;
	LONG ret = FALSE;

	if (argc < 3 || argc > 4)
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	if (!argv[1]->isString() || !argv[2]->isString() || (argc == 4 && !argv[3]->isString()))
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslSnmpTransportClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	SNMP_Transport *trans = (SNMP_Transport*)obj->getData();

	// Create request
	request = new SNMP_PDU(SNMP_SET_REQUEST, getpid(), trans->getSnmpVersion());

	if (SNMPIsCorrectOID(argv[1]->getValueAsString(&len)))
	{
		SNMP_Variable *var = new SNMP_Variable(argv[1]->getValueAsString(&len));
		if (argc == 3)
		{
			var->setValueFromString(ASN_OCTET_STRING, argv[2]->getValueAsString(&len));
		}
		else
		{
			UINT32 dataType = SNMPResolveDataType(argv[3]->getValueAsString(&len));
			if (dataType == ASN_NULL)
			{
				DbgPrintf(6, _T("SNMPSet: failed to resolve data type '%s', assume string"),
					argv[3]->getValueAsString(&len));
				dataType = ASN_OCTET_STRING;
			}
			var->setValueFromString(dataType, argv[2]->getValueAsString(&len));
		}
		request->bindVariable(var);
	}
	else
	{
		DbgPrintf(6, _T("SNMPSet: Invalid OID: %s"), argv[1]->getValueAsString(&len));
		goto finish;
	}

	// Send request and process response
	UINT32 snmpResult;
	if ((snmpResult = trans->doRequest(request, &response, g_snmpTimeout, 3)) == SNMP_ERR_SUCCESS)
	{
		if (response->getErrorCode() != 0)
		{
			DbgPrintf(6, _T("SNMPSet: operation failed (error code %d)"), response->getErrorCode());
			goto finish;
		}
		else
		{
			DbgPrintf(6, _T("SNMPSet: success"));
			ret = TRUE;
		}
		delete response;
	}
	else
	{
		DbgPrintf(6, _T("SNMPSet: %s"), SNMPGetErrorText(snmpResult));
	}

finish:
   delete request;
	*ppResult = new NXSL_Value(ret);
	return 0;
}

/**
 * Do SNMP walk starting from the given oid
 * Syntax:
 *    SNMPWalk(transport, oid)
 * where:
 *     transport - NXSL transport object
 *		 oid - SNMP object id
 * Return value:
 *     an array of VarBind objects
 */
static int F_SNMPWalk(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	static UINT32 requestId = 1;
	UINT32 rootName[MAX_OID_LEN], name[MAX_OID_LEN], result;
   size_t rootNameLen, nameLen;
	SNMP_PDU *rqPDU, *rspPDU;
	BOOL isRunning = TRUE;
	int i = 0;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (_tcscmp(obj->getClass()->getName(), g_nxslSnmpTransportClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	SNMP_Transport *trans = (SNMP_Transport*)obj->getData();

   // Get root
   rootNameLen = SNMPParseOID(argv[1]->getValueAsCString(), rootName, MAX_OID_LEN);
   if (rootNameLen == 0)
      return SNMP_ERR_BAD_OID;

	memcpy(name, rootName, rootNameLen * sizeof(UINT32));
   nameLen = rootNameLen;

	NXSL_Array *varList = new NXSL_Array;

   // Walk the MIB
   while(isRunning)
   {
      rqPDU = new SNMP_PDU(SNMP_GET_NEXT_REQUEST, requestId++, trans->getSnmpVersion());
      rqPDU->bindVariable(new SNMP_Variable(name, nameLen));
      result = trans->doRequest(rqPDU, &rspPDU, g_snmpTimeout, 3);

      // Analyze response
      if (result == SNMP_ERR_SUCCESS)
      {
         if ((rspPDU->getNumVariables() > 0) &&
             (rspPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *var = rspPDU->getVariable(0);
            if ((var->getType() != ASN_NO_SUCH_OBJECT) &&
                (var->getType() != ASN_NO_SUCH_INSTANCE))
            {
               // Do we have to stop walking?
               if ((var->getName()->getLength() < rootNameLen) ||
                   (memcmp(rootName, var->getName()->getValue(), rootNameLen * sizeof(UINT32))) ||
                   ((var->getName()->getLength() == nameLen) &&
                    (!memcmp(var->getName()->getValue(), name, var->getName()->getLength() * sizeof(UINT32)))))
               {
                  isRunning = FALSE;
                  delete rspPDU;
                  delete rqPDU;
                  break;
               }
               memcpy(name, var->getName()->getValue(), var->getName()->getLength() * sizeof(UINT32));
               nameLen = var->getName()->getLength();
					varList->set(i++, new NXSL_Value(new NXSL_Object(&g_nxslSnmpVarBindClass, var)));
					rspPDU->unlinkVariables();
            }
            else
            {
               result = SNMP_ERR_NO_OBJECT;
               isRunning = FALSE;
            }
         }
         else
         {
            if (rspPDU->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
               result = SNMP_ERR_NO_OBJECT;
            else
               result = SNMP_ERR_AGENT;
            isRunning = FALSE;
         }
         delete rspPDU;
      }
      else
      {
         isRunning = FALSE;
      }
      delete rqPDU;
   }

	if (result != SNMP_ERR_SUCCESS)
	{
		DbgPrintf(9, _T("SNMPWalk returned %d"), result);
		*ppResult = new NXSL_Value;
      delete varList;
	}
	else
	{
		*ppResult = new NXSL_Value(varList);
	}
	return 0;
}

/**
 * Read parameter's value from agent
 * Syntax:
 *    AgentReadParameter(object, name)
 * where:
 *     object - NetXMS node object
 *     name   - name of the parameter
 * Return value:
 *     paramater's value on success and null on failure
 */
static int F_AgentReadParameter(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	TCHAR buffer[MAX_RESULT_LENGTH];
	UINT32 rcc = ((Node *)object->getData())->getItemFromAgent(argv[1]->getValueAsCString(), MAX_RESULT_LENGTH, buffer);
	if (rcc == DCE_SUCCESS)
		*ppResult = new NXSL_Value(buffer);
	else
		*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Read table value from agent
 * Syntax:
 *    AgentReadTable(object, name)
 * where:
 *     object - NetXMS node object
 *     name   - name of the table
 * Return value:
 *     table value on success and null on failure
 */
static int F_AgentReadTable(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Table *table;
   UINT32 rcc = ((Node *)object->getData())->getTableFromAgent(argv[1]->getValueAsCString(), &table);
	if (rcc == DCE_SUCCESS)
      *ppResult = new NXSL_Value(new NXSL_Object(&g_nxslTableClass, table));
	else
		*ppResult = new NXSL_Value;
	return 0;
}

/**
 * Get server's configuration variable
 * First argument is a variable name
 * Optional second argumet is default value
 * Returns variable's value if found, default value if not found, and null if not found and no default value given
 */
static int F_GetConfigurationVariable(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if ((argc == 0) || (argc > 2))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	TCHAR buffer[MAX_DB_STRING];
	if (ConfigReadStr(argv[0]->getValueAsCString(), buffer, MAX_DB_STRING, _T("")))
	{
		*ppResult = new NXSL_Value(buffer);
	}
	else
	{
		*ppResult = (argc == 2) ? new NXSL_Value(argv[1]) : new NXSL_Value;
	}

	return 0;
}

/**
 * Additional server functions to use within all scripts
 */
static NXSL_ExtFunction m_nxslServerFunctions[] =
{
	{ _T("map"), F_map, -1 },
	{ _T("AgentReadParameter"), F_AgentReadParameter, 2 },
	{ _T("AgentReadTable"), F_AgentReadTable, 2 },
	{ _T("CreateSNMPTransport"), F_CreateSNMPTransport, 1 },
	{ _T("DeleteCustomAttribute"), F_DeleteCustomAttribute, 2 },
   { _T("GetConfigurationVariable"), F_GetConfigurationVariable, -1 },
   { _T("GetCustomAttribute"), F_GetCustomAttribute, 2 },
   { _T("GetEventParameter"), F_GetEventParameter, 2 },
   { _T("GetInterfaceName"), F_GetInterfaceName, 2 },
   { _T("GetInterfaceObject"), F_GetInterfaceObject, 2 },
   { _T("GetNodeInterfaces"), F_GetNodeInterfaces, 1 },
   { _T("GetNodeParents"), F_GetNodeParents, 1 },
   { _T("GetNodeTemplates"), F_GetNodeTemplates, 1 },
   { _T("GetObjectChildren"), F_GetObjectChildren, 1 },
   { _T("GetObjectParents"), F_GetObjectParents, 1 },
	{ _T("FindNodeObject"), F_FindNodeObject, 2 },
	{ _T("FindObject"), F_FindObject, -1 },
   { _T("ManageObject"), F_ManageObject, 1 },
	{ _T("PostEvent"), F_PostEvent, -1 },
	{ _T("RenameObject"), F_RenameObject, 2 },
   { _T("SetCustomAttribute"), F_SetCustomAttribute, 3 },
   { _T("SetEventParameter"), F_SetEventParameter, 3 },
	{ _T("SetInterfaceExpectedState"), F_SetInterfaceExpectedState, 2 },
	{ _T("SNMPGet"), F_SNMPGet, 2 },
	{ _T("SNMPGetValue"), F_SNMPGetValue, 2 },
	{ _T("SNMPSet"), F_SNMPSet, -1 /* 3 or 4 */ },
	{ _T("SNMPWalk"), F_SNMPWalk, 2 },
   { _T("UnmanageObject"), F_UnmanageObject, 1 }
};

/**
 * Additional server functions to manage objects (disabled by default)
 */
static NXSL_ExtFunction m_nxslServerFunctionsForContainers[] =
{
	{ _T("BindObject"), F_BindObject, 2 },
	{ _T("CreateContainer"), F_CreateContainer, 2 },
	{ _T("CreateNode"), F_CreateNode, 3 },
	{ _T("DeleteObject"), F_DeleteObject, 1 },
	{ _T("UnbindObject"), F_UnbindObject, 2 }
};

/*** NXSL_ServerEnv class implementation ***/

/**
 * Constructor for server default script environment
 */
NXSL_ServerEnv::NXSL_ServerEnv() : NXSL_Environment()
{
	m_console = NULL;
	setLibrary(g_pScriptLibrary);
	registerFunctionSet(sizeof(m_nxslServerFunctions) / sizeof(NXSL_ExtFunction), m_nxslServerFunctions);
	RegisterDCIFunctions(this);
	registerFunctionSet(g_nxslNumSituationFunctions, g_nxslSituationFunctions);
	if (g_flags & AF_ENABLE_NXSL_CONTAINER_FUNCS)
		registerFunctionSet(sizeof(m_nxslServerFunctionsForContainers) / sizeof(NXSL_ExtFunction), m_nxslServerFunctionsForContainers);
   // Pass event to modules
   CALL_ALL_MODULES(pfNXSLServerEnvConfig, (this));
}

/**
 * Script trace output
 */
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

/**
 * Print script output to console
 */
void NXSL_ServerEnv::print(NXSL_Value *value)
{
	if (m_console != NULL)
	{
		const TCHAR *text = value->getValueAsCString();
		ConsolePrintf(m_console, _T("%s"), CHECK_NULL(text));
	}
}

/**
 * Constructor for environment intended for passing script's output to client
 */
NXSL_ClientSessionEnv::NXSL_ClientSessionEnv(ClientSession *session, NXCPMessage *response) : NXSL_ServerEnv()
{
   m_session = session;
   m_response = response;
}

/**
 * Send script output to user
 */
void NXSL_ClientSessionEnv::print(NXSL_Value *value)
{
	if (m_session != NULL && m_response != NULL)
	{
		const TCHAR *text = value->getValueAsCString();
		m_response->setField(VID_MESSAGE, text);
		m_session->sendMessage(m_response);
	}
}

/**
 * Trace output
 */
void NXSL_ClientSessionEnv::trace(int level, const TCHAR *text)
{
	if (m_session != NULL && m_response != NULL)
	{
      size_t len = _tcslen(text);
      TCHAR *t = (TCHAR *)malloc((len + 2) * sizeof(TCHAR));
      memcpy(t, text, len * sizeof(TCHAR));
      t[len] = _T('\n');
      t[len + 1] = 0;
		m_response->setField(VID_MESSAGE, t);
		m_session->sendMessage(m_response);
      free(t);
	}
   NXSL_ServerEnv::trace(level, text);
}

/**
 * Call hook script
 */
NXSL_VM *FindHookScript(const TCHAR *hookName)
{
	TCHAR scriptName[MAX_PATH] = _T("Hook::");
	nx_strncpy(&scriptName[6], hookName, MAX_PATH - 6);
	NXSL_VM *vm = g_pScriptLibrary->createVM(scriptName, new NXSL_ServerEnv);
	if (vm == NULL)
		DbgPrintf(7, _T("FindHookScript: hook script \"%s\" not found"), scriptName);
   return vm;
}
