/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
void RegisterDCIFunctions(NXSL_Environment *pEnv);
int F_map(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_mapList(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_FindAlarmById(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_FindAlarmByKey(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_FindAlarmByKeyRegex(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_GetSyslogRuleCheckCount(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetSyslogRuleMatchCount(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

/**
 * Get node's custom attribute
 * First argument is a node object, and second is an attribute name
 */
static int F_GetCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netxmsObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
	NXSL_Value *value = netxmsObject->getCustomAttributeForNXSL(vm, argv[1]->getValueAsCString());
	*ppResult = (value != nullptr) ? value : vm->createValue(); // Return nullptr if attribute not found
	return 0;
}

/**
 * Set node's custom attribute
 * First argument is a node object, second is an attribute name, third is new value
 * Returns previous value
 */
static int F_SetCustomAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString() || !argv[2]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netxmsObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   NXSL_Value *value = netxmsObject->getCustomAttributeForNXSL(vm, argv[1]->getValueAsCString());
   *ppResult = (value != nullptr) ? value : vm->createValue(); // Return nullptr if attribute not found
	netxmsObject->setCustomAttribute(argv[1]->getValueAsCString(), argv[2]->getValueAsCString(), StateChange::IGNORE);
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
	if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netxmsObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
	netxmsObject->deleteCustomAttribute(argv[1]->getValueAsCString());
   *ppResult = vm->createValue();
	return 0;
}

/**
 * Get interface name by index
 * Parameters: node object and interface index
 */
static int F_GetInterfaceName(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	shared_ptr<Interface> iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByIndex(argv[1]->getValueAsUInt32());
	*result = (iface != nullptr) ? vm->createValue(iface->getName()) : vm->createValue();
	return 0;
}

/**
 * Get interface object by index
 * Parameters: node object and interface index
 */
static int F_GetInterfaceObject(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	shared_ptr<Interface> iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByIndex(argv[1]->getValueAsUInt32());
	*result = (iface != nullptr) ? iface->createNXSLObject(vm) : vm->createValue();
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
	shared_ptr<Node> currNode, node;

	if (!argv[0]->isNull())
	{
		if (!argv[0]->isObject())
			return NXSL_ERR_NOT_OBJECT;

		NXSL_Object *object = argv[0]->getValueAsObject();
		if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
			return NXSL_ERR_BAD_CLASS;

		currNode = *static_cast<shared_ptr<Node>*>(object->getData());
	}

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	if (argv[1]->isInteger())
	{
		node = static_pointer_cast<Node>(FindObjectById(argv[1]->getValueAsUInt32(), OBJECT_NODE));
	}
	else
	{
		node = static_pointer_cast<Node>(FindObjectByName(argv[1]->getValueAsCString(), OBJECT_NODE));
	}

	if (node != nullptr)
	{
		if (g_flags & AF_CHECK_TRUSTED_NODES)
		{
			if ((currNode != nullptr) && (node->isTrustedNode(currNode->getId())))
			{
				*ppResult = node->createNXSLObject(vm);
			}
			else
			{
				// No access, return null
				*ppResult = vm->createValue();
				DbgPrintf(4, _T("NXSL::FindNodeObject(%s [%d], '%s'): access denied for node %s [%d]"),
				          (currNode != nullptr) ? currNode->getName() : _T("null"), (currNode != nullptr) ? currNode->getId() : 0,
							 argv[1]->getValueAsCString(), node->getName(), node->getId());
			}
		}
		else
		{
			*ppResult = node->createNXSLObject(vm);
		}
	}
	else
	{
		// Node not found, return null
		*ppResult = vm->createValue();
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
	if ((argc !=1) && (argc != 2))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isString())
		return NXSL_ERR_NOT_STRING;

	if (argc == 2 && (!argv[1]->isNull() && !argv[1]->isObject()))
		return NXSL_ERR_NOT_OBJECT;

   shared_ptr<NetObj> object;
	if (argv[0]->isInteger())
	{
		object = FindObjectById(argv[0]->getValueAsUInt32());
	}
	else
	{
		object = FindObjectByName(argv[0]->getValueAsCString(), -1);
	}

	if (object != nullptr)
	{
		if (g_flags & AF_CHECK_TRUSTED_NODES)
		{
			Node *currNode = nullptr;
			if ((argc == 2) && !argv[1]->isNull())
			{
				NXSL_Object *o = argv[1]->getValueAsObject();
				if (!o->getClass()->instanceOf(g_nxslNodeClass.getName()))
					return NXSL_ERR_BAD_CLASS;

				currNode = static_cast<shared_ptr<Node>*>(o->getData())->get();
			}
			if ((currNode != nullptr) && (object->isTrustedNode(currNode->getId())))
			{
				*ppResult = object->createNXSLObject(vm);
			}
			else
			{
				// No access, return null
				*ppResult = vm->createValue();
				DbgPrintf(4, _T("NXSL::FindObject('%s', %s [%d]): access denied for node %s [%d]"),
				          argv[0]->getValueAsCString(),
				          (currNode != nullptr) ? currNode->getName() : _T("null"), (currNode != nullptr) ? currNode->getId() : 0,
							 object->getName(), object->getId());
			}
		}
		else
		{
			*ppResult = object->createNXSLObject(vm);;
		}
	}
	else
	{
		// Object not found, return null
		*ppResult = vm->createValue();
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
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
	*ppResult = vm->createValue(node->getParentsForNXSL(vm));
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
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
	*ppResult = vm->createValue(node->getTemplatesForNXSL(vm));
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
	if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	NetObj *netobj = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
	*ppResult = vm->createValue(netobj->getParentsForNXSL(vm));
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
   if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   NetObj *netobj = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
	*ppResult = vm->createValue(netobj->getChildrenForNXSL(vm));
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
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
	*ppResult = vm->createValue(node->getInterfacesForNXSL(vm));
	return 0;
}

/**
 * Get all nodes
 * Returns array of accessible node objects
 * (empty array if trusted nodes check is on and current node not provided)
 */
static int F_GetAllNodes(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   Node *node = nullptr;
   if (argc > 0)
   {
      if (!argv[0]->isObject())
         return NXSL_ERR_NOT_OBJECT;

      NXSL_Object *object = argv[0]->getValueAsObject();
      if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
         return NXSL_ERR_BAD_CLASS;

      node = static_cast<shared_ptr<Node>*>(object->getData())->get();
   }

   NXSL_Array *a = new NXSL_Array(vm);
   if (!(g_flags & AF_CHECK_TRUSTED_NODES) || (node != nullptr))
   {
      SharedObjectArray<NetObj> *nodes = g_idxNodeById.getObjects();
      int index = 0;
      for(int i = 0; i < nodes->size(); i++)
      {
         Node *n = (Node *)nodes->get(i);
         if ((node == nullptr) || n->isTrustedNode(node->getId()))
         {
            a->set(index++, n->createNXSLObject(vm));
         }
      }
      delete nodes;
   }
   *ppResult = vm->createValue(a);
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
	if (!object->getClass()->instanceOf(g_nxslEventClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	Event *e = (Event *)object->getData();
	const TCHAR *value = e->getNamedParameter(argv[1]->getValueAsCString());
	*ppResult = (value != nullptr) ? vm->createValue(value) : vm->createValue();
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
	if (!object->getClass()->instanceOf(g_nxslEventClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString() || !argv[2]->isString())
		return NXSL_ERR_NOT_STRING;

	Event *e = (Event *)object->getData();
	e->setNamedParameter(argv[1]->getValueAsCString(), argv[2]->getValueAsCString());
	*ppResult = vm->createValue();
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
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();

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
		const TCHAR *userTag = nullptr;
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
		success = PostEventWithTag(eventCode, EventOrigin::NXSL, 0, node->getId(), userTag, format,
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

	*ppResult = vm->createValue((LONG)success);
	return 0;
}

/**
 * Load event from database by ID
 */
static int F_LoadEvent(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Event *e = FindEventInLoggerQueue(argv[0]->getValueAsUInt64());
   if (e == nullptr)
      e = LoadEventFromDatabase(argv[0]->getValueAsUInt64());
   *result = (e != nullptr) ? vm->createValue(new NXSL_Object(vm, &g_nxslEventClass, e, false)) : vm->createValue();
   return 0;
}

/**
 * Create node object
 * Syntax:
 *    CreateNode(parent, name, primaryHostName, zoneUIN)
 * where:
 *     parent          - parent object
 *     name            - name for new node
 *     primaryHostName - primary host name for new node (optional)
 *     zoneUIN         - zone UIN (optional)
 * Return value:
 *     new node object or null on failure
 */
static int F_CreateNode(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if ((argc < 2) || (argc > 4))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (!obj->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	shared_ptr<NetObj> parent = *static_cast<shared_ptr<NetObj>*>(obj->getData());
	if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString() || ((argc > 2) && !argv[2]->isString()))
		return NXSL_ERR_NOT_STRING;

	if ((argc > 3) && !argv[3]->isInteger())
	   return NXSL_ERR_NOT_INTEGER;

	const TCHAR *pname;
	if (argc > 2)
	{
	   pname = argv[2]->getValueAsCString();
	   if (*pname == 0)
	      pname = argv[1]->getValueAsCString();
	}
	else
	{
      pname = argv[1]->getValueAsCString();
	}
	NewNodeData newNodeData(InetAddress::resolveHostName(pname));
	_tcslcpy(newNodeData.name, argv[1]->getValueAsCString(), MAX_OBJECT_NAME);
	newNodeData.zoneUIN = (argc > 3) ? argv[3]->getValueAsUInt32() : 0;
	newNodeData.doConfPoll = true;

	ObjectTransactionStart();
   shared_ptr<Node> node = PollNewNode(&newNodeData);
	if (node != nullptr)
	{
		node->setPrimaryHostName(pname);
		parent->addChild(node);
		node->addParent(parent);
		node->unhide();
		*ppResult = node->createNXSLObject(vm);
	}
	else
	{
		*ppResult = vm->createValue();
	}
	ObjectTransactionEnd();
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
	if (!obj->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	shared_ptr<NetObj> parent = *static_cast<shared_ptr<NetObj>*>(obj->getData());
	if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
		return NXSL_ERR_BAD_CLASS;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	const TCHAR *name = argv[1]->getValueAsCString();

	shared_ptr<Container> container = MakeSharedNObject<Container>(name, 0);
	NetObjInsert(container, true, false);
	parent->addChild(container);
	container->addParent(parent);
	container->unhide();

	*ppResult = container->createNXSLObject(vm);
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
   if (!obj->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   static_cast<shared_ptr<NetObj>*>(obj->getData())->get()->deleteObject();

	*ppResult = vm->createValue();
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

   NXSL_Object *nxslParent = argv[0]->getValueAsObject();
   if (!nxslParent->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<NetObj> parent = *static_cast<shared_ptr<NetObj>*>(nxslParent->getData());
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   NXSL_Object *nxslChild = argv[1]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<NetObj> child = *static_cast<shared_ptr<NetObj>*>(nxslChild->getData());
   if (!IsValidParentClass(child->getObjectClass(), parent->getObjectClass()))
      return NXSL_ERR_BAD_CLASS;

   if (child->isChild(parent->getId())) // prevent loops
      return NXSL_ERR_INVALID_OBJECT_OPERATION;

   parent->addChild(child);
   child->addParent(parent);
   parent->calculateCompoundStatus();

   *ppResult = vm->createValue();
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

   NXSL_Object *nxslParent = argv[0]->getValueAsObject();
   if (!nxslParent->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *parent = static_cast<shared_ptr<NetObj>*>(nxslParent->getData())->get();
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   NXSL_Object *nxslChild = argv[1]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *child = static_cast<shared_ptr<NetObj>*>(nxslChild->getData())->get();
   parent->deleteChild(*child);
   child->deleteParent(*parent);

   *ppResult = vm->createValue();
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
   if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setName(argv[1]->getValueAsCString());

   *ppResult = vm->createValue();
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
   if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMgmtStatus(true);

	*ppResult = vm->createValue();
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
   if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMgmtStatus(false);

	*ppResult = vm->createValue();
	return 0;
}

/**
 * EnterMaintenance object (set to unmanaged state)
 * Syntax:
 *    EnterMaintenance(object, comments)
 * where:
 *     object - NetXMS object (Node, Interface, or NetObj)
 * Return value:
 *     null
 */
static int F_EnterMaintenance(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

   if ((argc > 1) && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->enterMaintenanceMode(0, (argc > 1) ? argv[1]->getValueAsCString() : nullptr);

	*ppResult = vm->createValue();
	return 0;
}

/**
 * LeaveMaintenance object (set to unmanaged state)
 * Syntax:
 *    LeaveMaintenance(object)
 * where:
 *     object - NetXMS object (Node, Interface, or NetObj)
 * Return value:
 *     null
 */
static int F_LeaveMaintenance(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->leaveMaintenanceMode(0);

	*ppResult = vm->createValue();
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
	if (!object->getClass()->instanceOf(g_nxslInterfaceClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	int state;
	if (argv[1]->isInteger())
	{
		state = argv[1]->getValueAsInt32();
	}
	else if (argv[1]->isString())
	{
		static const TCHAR *stateNames[] = { _T("UP"), _T("DOWN"), _T("IGNORE"), nullptr };
		const TCHAR *name = argv[1]->getValueAsCString();
		for(state = 0; stateNames[state] != nullptr; state++)
			if (!_tcsicmp(stateNames[state], name))
				break;
	}
	else
	{
		return NXSL_ERR_NOT_STRING;
	}

	if ((state >= 0) && (state <= 2))
	   static_cast<shared_ptr<Interface>*>(object->getData())->get()->setExpectedState(state);

	*ppResult = vm->createValue();
	return 0;
}

/**
 * Create new SNMP transport object
 * Syntax:
 *    CreateSNMPTransport(node, [port], [context])
 * where:
 *     node    - node to create SNMP transport for
 *     port    - SNMP port (optional)
 *     context - SNMP context (optional)
 * Return value:
 *     new SNMP_Transport object
 */
static int F_CreateSNMPTransport(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if ((argc > 1) && !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc > 2) && !argv[2]->isString())
      return NXSL_ERR_NOT_STRING;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (!obj->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = static_cast<shared_ptr<Node>*>(obj->getData())->get();
	if (node != nullptr)
	{
	   UINT16 port = (argc > 1) ? (UINT16)argv[1]->getValueAsInt32() : 0;
	   const TCHAR *context = (argc > 2) ? argv[2]->getValueAsCString() : nullptr;
		SNMP_Transport *t = node->createSnmpTransport(port, SNMP_VERSION_DEFAULT, context);
      *ppResult = (t != nullptr) ? vm->createValue(new NXSL_Object(vm, &g_nxslSnmpTransportClass, t)) : vm->createValue();
	}
	else
	{
		*ppResult = vm->createValue();
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
	UINT32 varName[MAX_OID_LEN], result = SNMP_ERR_SUCCESS;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *obj = argv[0]->getValueAsObject();
	if (!obj->getClass()->instanceOf(g_nxslSnmpTransportClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	SNMP_Transport *trans = (SNMP_Transport*)obj->getData();

   // Create PDU and send request
   size_t nameLen = SNMPParseOID(argv[1]->getValueAsString(&len), varName, MAX_OID_LEN);
   if (nameLen == 0)
		return NXSL_ERR_BAD_CONDITION;

   SNMP_PDU *pdu = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), trans->getSnmpVersion());
	pdu->bindVariable(new SNMP_Variable(varName, nameLen));

	SNMP_PDU *rspPDU;
   result = trans->doRequest(pdu, &rspPDU, SnmpGetDefaultTimeout(), 3);
   if (result == SNMP_ERR_SUCCESS)
   {
      if ((rspPDU->getNumVariables() > 0) && (rspPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         SNMP_Variable *pVar = rspPDU->getVariable(0);
		   *ppResult = vm->createValue(new NXSL_Object(vm, &g_nxslSnmpVarBindClass, pVar));
         rspPDU->unlinkVariables();
	   }
      else
      {
   		*ppResult = vm->createValue();
      }
      delete rspPDU;
   }
	else
	{
		*ppResult = vm->createValue();
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
	if (!obj->getClass()->instanceOf(g_nxslSnmpTransportClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	SNMP_Transport *trans = (SNMP_Transport*)obj->getData();

	if (SnmpGetEx(trans, argv[1]->getValueAsString(&len), nullptr, 0, buffer, sizeof(buffer), SG_STRING_RESULT, nullptr) == SNMP_ERR_SUCCESS)
	{
		*ppResult = vm->createValue(buffer);
	}
	else
	{
		*ppResult = vm->createValue();
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
static int F_SNMPSet(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc < 3 || argc > 4)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;
   if (!argv[1]->isString() || !argv[2]->isString() || ((argc == 4) && !argv[3]->isString()))
      return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslSnmpTransportClass.getName()))
      return NXSL_ERR_BAD_CLASS;
   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   SNMP_PDU *request = new SNMP_PDU(SNMP_SET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   SNMP_PDU *response = nullptr;
   bool success = false;

   if (SNMPIsCorrectOID(argv[1]->getValueAsCString()))
   {
      SNMP_Variable *var = new SNMP_Variable(argv[1]->getValueAsCString());
      if (argc == 3)
      {
         var->setValueFromString(ASN_OCTET_STRING, argv[2]->getValueAsCString());
      }
      else
      {
         UINT32 dataType = SNMPResolveDataType(argv[3]->getValueAsCString());
         if (dataType == ASN_NULL)
         {
            nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPSet: failed to resolve data type '%s', assume string"),
               argv[3]->getValueAsCString());
            dataType = ASN_OCTET_STRING;
         }
         var->setValueFromString(dataType, argv[2]->getValueAsCString());
      }
      request->bindVariable(var);
   }
   else
   {
      nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPSet: Invalid OID: %s"), argv[1]->getValueAsCString());
      goto finish;
   }

   // Send request and process response
   UINT32 snmpResult;
   if ((snmpResult = transport->doRequest(request, &response, SnmpGetDefaultTimeout(), 3)) == SNMP_ERR_SUCCESS)
   {
      if (response->getErrorCode() != 0)
      {
         nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPSet: operation failed (error code %d)"), response->getErrorCode());
         goto finish;
      }
      else
      {
         nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPSet: success"));
         success = true;
      }
      delete response;
   }
   else
   {
      nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPSet: %s"), SNMPGetErrorText(snmpResult));
   }

finish:
   delete request;
   *result = vm->createValue(success);
   return 0;
}

/**
 * SNMP walk callback
 */
static UINT32 WalkCallback(SNMP_Variable *var, SNMP_Transport *transport, void *userArg)
{
   NXSL_VM *vm = static_cast<NXSL_VM*>(static_cast<NXSL_Array*>(userArg)->vm());
   static_cast<NXSL_Array*>(userArg)->append(vm->createValue(new NXSL_Object(vm, &g_nxslSnmpVarBindClass, new SNMP_Variable(var))));
   return SNMP_ERR_SUCCESS;
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
static int F_SNMPWalk(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;
	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslSnmpTransportClass.getName()))
      return NXSL_ERR_BAD_CLASS;
   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   NXSL_Array *varbinds = new NXSL_Array(vm);
   if (SnmpWalk(transport, argv[1]->getValueAsCString(), WalkCallback, varbinds) == SNMP_ERR_SUCCESS)
   {
      *result = vm->createValue(varbinds);
   }
   else
   {
      *result = vm->createValue();
      delete varbinds;
   }
   return 0;
}

/**
 * Execute agent action
 * Syntax:
 *    AgentExecuteAction(object, name, ...)
 * where:
 *     object - NetXMS node object
 *     name   - name of the parameter
 * Return value:
 *     true on success
 */
static int F_AgentExecuteAction(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if (argc < 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   for(int i = 1; i < argc; i++)
      if (!argv[i]->isString())
         return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
   shared_ptr<AgentConnectionEx> conn = node->createAgentConnection();
   if (conn != nullptr)
   {
      StringList list;
      for(int i = 2; (i < argc) && (i < 128); i++)
         list.add(argv[i]->getValueAsCString());
      UINT32 rcc = conn->execAction(argv[1]->getValueAsCString(), list, false, nullptr, nullptr);
      *ppResult = vm->createValue((rcc == ERR_SUCCESS) ? 1 : 0);
      nxlog_debug(5, _T("NXSL: F_AgentExecuteAction: action %s on node %s [%d]: RCC=%d"), argv[1]->getValueAsCString(), node->getName(), node->getId(), rcc);
   }
   else
   {
      *ppResult = vm->createValue(0);
   }
   return 0;
}

/**
 * Agent action output handler
 */
static void ActionOutputHandler(ActionCallbackEvent event, const TCHAR *text, void *userData)
{
   if (event == ACE_DATA)
      static_cast<StringBuffer*>(userData)->append(text);
}

/**
 * Execute agent action
 * Syntax:
 *    AgentExecuteActionWithOutput(object, name, ...)
 * where:
 *     object - NetXMS node object
 *     name   - name of the parameter
 * Return value:
 *     action output on success and null on failure
 */
static int F_AgentExecuteActionWithOutput(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if (argc < 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   for(int i = 1; i < argc; i++)
      if (!argv[i]->isString())
         return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
   shared_ptr<AgentConnectionEx> conn = node->createAgentConnection();
   if (conn != nullptr)
   {
      StringList list;
      for(int i = 2; (i < argc) && (i < 128); i++)
         list.add(argv[i]->getValueAsCString());
      StringBuffer output;
      UINT32 rcc = conn->execAction(argv[1]->getValueAsCString(), list, true, ActionOutputHandler, &output);
      *ppResult = (rcc == ERR_SUCCESS) ? vm->createValue(output) : vm->createValue();
      nxlog_debug(5, _T("NXSL: F_AgentExecuteActionWithOutput: action %s on node %s [%d]: RCC=%d"), argv[1]->getValueAsCString(), node->getName(), node->getId(), rcc);
   }
   else
   {
      *ppResult = vm->createValue();
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
static int F_AgentReadParameter(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	TCHAR buffer[MAX_RESULT_LENGTH];
	UINT32 rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getMetricFromAgent(argv[1]->getValueAsCString(), MAX_RESULT_LENGTH, buffer);
	*result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
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
static int F_AgentReadTable(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Table *table;
   UINT32 rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getTableFromAgent(argv[1]->getValueAsCString(), &table);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Object(vm, &g_nxslTableClass, table)) : vm->createValue();
	return 0;
}

/**
 * Read list from agent
 * Syntax:
 *    AgentReadList(object, name)
 * where:
 *     object - NetXMS node object
 *     name   - name of the list
 * Return value:
 *     list values (as an array) on success and null on failure
 */
static int F_AgentReadList(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	StringList *list;
   UINT32 rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getListFromAgent(argv[1]->getValueAsCString(), &list);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, list)) : vm->createValue();
   delete list;
	return 0;
}

/**
 * Read parameter's value from network device driver
 * Syntax:
 *    DriverReadParameter(object, name)
 * where:
 *     object - NetXMS node object
 *     name   - name of the parameter
 * Return value:
 *     paramater's value on success and null on failure
 */
static int F_DriverReadParameter(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   if (!argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslNodeClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   TCHAR buffer[MAX_RESULT_LENGTH];
   UINT32 rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getMetricFromDeviceDriver(argv[1]->getValueAsCString(), buffer, MAX_RESULT_LENGTH);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
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

	TCHAR buffer[MAX_CONFIG_VALUE];
	if (ConfigReadStr(argv[0]->getValueAsCString(), buffer, MAX_CONFIG_VALUE, _T("")))
	{
		*ppResult = vm->createValue(buffer);
	}
	else
	{
		*ppResult = (argc == 2) ? vm->createValue(argv[1]) : vm->createValue();
	}

	return 0;
}

/**
 * Get country alpha code from numeric code
 */
static int F_CountryAlphaCode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *code = CountryAlphaCode(argv[0]->getValueAsCString());
   *result = (code != nullptr) ? vm->createValue(code) : vm->createValue();
   return 0;
}

/**
 * Get country name from code
 */
static int F_CountryName(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *name = CountryName(argv[0]->getValueAsCString());
   *result = (name != nullptr) ? vm->createValue(name) : vm->createValue();
   return 0;
}

/**
 * Get currency alpha code from numeric code
 */
static int F_CurrencyAlphaCode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *code = CurrencyAlphaCode(argv[0]->getValueAsCString());
   *result = (code != nullptr) ? vm->createValue(code) : vm->createValue();
   return 0;
}

/**
 * Get currency exponent
 */
static int F_CurrencyExponent(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   *result = vm->createValue(CurrencyExponent(argv[0]->getValueAsCString()));
   return 0;
}

/**
 * Get country name from code
 */
static int F_CurrencyName(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *name = CurrencyName(argv[0]->getValueAsCString());
   *result = (name != nullptr) ? vm->createValue(name) : vm->createValue();
   return 0;
}

/**
 * Get event name from code
 */
static int F_EventNameFromCode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   TCHAR eventName[MAX_EVENT_NAME];
   if (EventNameFromCode(argv[0]->getValueAsUInt32(), eventName))
      *result = vm->createValue(eventName);
   else
      *result = vm->createValue();
   return 0;
}

/**
 * Get event code from name
 */
static int F_EventCodeFromName(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   *result = vm->createValue(EventCodeFromName(argv[0]->getValueAsCString(), 0));
   return 0;
}

/**
 * Count scheduled tasks by key
 */
static int F_CountScheduledTasksByKey(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   *result = vm->createValue(CountScheduledTasksByKey(argv[0]->getValueAsCString()));
   return 0;
}

/**
 * Create user agent message
 * Syntax:
 *    CreateUserAgentMessage(object, message, startTime, endTime)
 * where:
 *     object    - NetXMS object
 *     message   - message to be sent
 *     startTime - start time of message delivery
 *     endTime   - end time of message delivery
 * Return value:
 *     message id
 */
static int F_CreateUserAgentNotification(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   if (!argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   if (!argv[2]->isNumeric() || !argv[3]->isNumeric())
      return NXSL_ERR_NOT_INTEGER;

   if (!argv[4]->isBoolean())
      return NXSL_ERR_NOT_BOOLEAN;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   UINT32 len = MAX_USER_AGENT_MESSAGE_SIZE;
   const TCHAR *message = argv[1]->getValueAsString(&len);
   IntegerArray<UINT32> *idList = new IntegerArray<UINT32>(16,16);
   idList->add(static_cast<shared_ptr<NetObj>*>(object->getData())->get()->getId());
   time_t startTime = (time_t)argv[2]->getValueAsUInt64();
   time_t endTime = (time_t)argv[3]->getValueAsUInt64();

   UserAgentNotificationItem *uan = CreateNewUserAgentNotification(message, idList, startTime, endTime, argv[3]->getValueAsBoolean(), 0);

   *result = vm->createValue(uan->getId());
   uan->decRefCount();
   return 0;
}

/**
 * Expand macros in string
 * Syntax:
 *    ExpandString(string, [object], [event])
 * Returnded value:
 *    expanded string
 */
static int F_ExpandString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<NetObj> object;
   if (argc > 1)
   {
      if (!argv[1]->isObject(_T("NetObj")))
         return NXSL_ERR_NOT_OBJECT;
      object = *static_cast<shared_ptr<NetObj>*>(argv[1]->getValueAsObject()->getData());
   }
   else
   {
      object = FindObjectById(g_dwMgmtNode);
      if (object == nullptr)
         object = g_entireNetwork;
   }

   Event *event = nullptr;
   if (argc > 2)
   {
      if (!argv[2]->isObject(_T("Event")))
         return NXSL_ERR_NOT_OBJECT;
   }

   *result = vm->createValue(object->expandText(argv[0]->getValueAsCString(), nullptr, event, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr));
   return 0;
}

/**
 * Sends e-mail to specified recipients
 * Syntax:
 *    SendMail(recipients, subject, text, ishtml)
 * Returnded value:
 *    none
 */
static int F_SendMail(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isString() || !argv[1]->isString() || !argv[2]->isString())
		return NXSL_ERR_NOT_STRING;

	if (!argv[3]->isBoolean())
		return  NXSL_ERR_NOT_BOOLEAN;

	StringBuffer rcpts(argv[0]->getValueAsCString());
	rcpts.trim();
	const TCHAR *subj = argv[1]->getValueAsCString();
	const TCHAR *text = argv[2]->getValueAsCString();
	bool ishtml = argv[3]->getValueAsBoolean();

	if (!rcpts.isEmpty())
	{
		nxlog_debug_tag(_T("SendMail.nxsl"), 3, _T("Sending mail to %s: \"%s\""), (const TCHAR *)rcpts, text);
		TCHAR *curr = rcpts.getBuffer(), *next;
		do
		{
			next = _tcschr(curr, _T(';'));
			if (next != nullptr)
				*next = 0;
			StrStrip(curr);
			PostMail(curr, subj, text, ishtml);
			curr = next + 1;
		} while(next != nullptr);
	}
	else
	{
		nxlog_debug_tag(_T("SendMail.nxsl"), 3, _T("Empty recipients list - mail will not be sent"));
	}
	return 0;
}

/**
 * Additional server functions to use within all scripts
 */
static NXSL_ExtFunction m_nxslServerFunctions[] =
{
	{ "map", F_map, -1 },
   { "mapList", F_mapList, -1 },
   { "AgentExecuteAction", F_AgentExecuteAction, -1 },
   { "AgentExecuteActionWithOutput", F_AgentExecuteActionWithOutput, -1 },
	{ "AgentReadList", F_AgentReadList, 2 },
	{ "AgentReadParameter", F_AgentReadParameter, 2 },
	{ "AgentReadTable", F_AgentReadTable, 2 },
	{ "CreateSNMPTransport", F_CreateSNMPTransport, -1 },
   { "CountryAlphaCode", F_CountryAlphaCode, 1 },
   { "CountryName", F_CountryName, 1 },
   { "CountScheduledTasksByKey", F_CountScheduledTasksByKey, 1 },
   { "CreateUserAgentNotification", F_CreateUserAgentNotification, 4},
   { "CurrencyAlphaCode", F_CurrencyAlphaCode, 1 },
   { "CurrencyExponent", F_CurrencyExponent, 1 },
   { "CurrencyName", F_CurrencyName, 1 },
	{ "DeleteCustomAttribute", F_DeleteCustomAttribute, 2 },
   { "DriverReadParameter", F_DriverReadParameter, 2 },
   { "EnterMaintenance", F_EnterMaintenance, -1 },
   { "EventNameFromCode", F_EventNameFromCode, 1 },
   { "EventCodeFromName", F_EventCodeFromName, 1 },
   { "ExpandString", F_ExpandString, -1 },
   { "GetAllNodes", F_GetAllNodes, -1 },
   { "GetConfigurationVariable", F_GetConfigurationVariable, -1 },
   { "GetCustomAttribute", F_GetCustomAttribute, 2 },
   { "GetEventParameter", F_GetEventParameter, 2 },
   { "GetInterfaceName", F_GetInterfaceName, 2 },
   { "GetInterfaceObject", F_GetInterfaceObject, 2 },
   { "GetNodeInterfaces", F_GetNodeInterfaces, 1 },
   { "GetNodeParents", F_GetNodeParents, 1 },
   { "GetNodeTemplates", F_GetNodeTemplates, 1 },
   { "GetObjectChildren", F_GetObjectChildren, 1 },
   { "GetObjectParents", F_GetObjectParents, 1 },
   { "GetSyslogRuleCheckCount", F_GetSyslogRuleCheckCount, -1 },
   { "GetSyslogRuleMatchCount", F_GetSyslogRuleMatchCount, -1 },
	{ "FindAlarmById", F_FindAlarmById, 1 },
	{ "FindAlarmByKey", F_FindAlarmByKey, 1 },
   { "FindAlarmByKeyRegex", F_FindAlarmByKeyRegex, 1 },
	{ "FindNodeObject", F_FindNodeObject, 2 },
	{ "FindObject", F_FindObject, -1 },
   { "LeaveMaintenance", F_LeaveMaintenance, 1 },
   { "LoadEvent", F_LoadEvent, 1 },
   { "ManageObject", F_ManageObject, 1 },
	{ "PostEvent", F_PostEvent, -1 },
	{ "RenameObject", F_RenameObject, 2 },
	{ "SendMail", F_SendMail, 4 },
   { "SetCustomAttribute", F_SetCustomAttribute, 3 },
   { "SetEventParameter", F_SetEventParameter, 3 },
	{ "SetInterfaceExpectedState", F_SetInterfaceExpectedState, 2 },
	{ "SNMPGet", F_SNMPGet, 2 },
	{ "SNMPGetValue", F_SNMPGetValue, 2 },
	{ "SNMPSet", F_SNMPSet, -1 /* 3 or 4 */ },
	{ "SNMPWalk", F_SNMPWalk, 2 },
   { "UnmanageObject", F_UnmanageObject, 1 }
};

/**
 * Additional server functions to manage objects (disabled by default)
 */
static NXSL_ExtFunction m_nxslServerFunctionsForContainers[] =
{
	{ "BindObject", F_BindObject, 2 },
	{ "CreateContainer", F_CreateContainer, 2 },
	{ "CreateNode", F_CreateNode, -1 },
	{ "DeleteObject", F_DeleteObject, 1 },
	{ "UnbindObject", F_UnbindObject, 2 }
};

/*** NXSL_ServerEnv class implementation ***/

/**
 * Constructor for server default script environment
 */
NXSL_ServerEnv::NXSL_ServerEnv() : NXSL_Environment()
{
	m_console = nullptr;
	setLibrary(GetServerScriptLibrary());
	registerFunctionSet(sizeof(m_nxslServerFunctions) / sizeof(NXSL_ExtFunction), m_nxslServerFunctions);
	RegisterDCIFunctions(this);
	if (g_flags & AF_ENABLE_NXSL_CONTAINER_FUNCTIONS)
		registerFunctionSet(sizeof(m_nxslServerFunctionsForContainers) / sizeof(NXSL_ExtFunction), m_nxslServerFunctionsForContainers);
   if (g_flags & AF_ENABLE_NXSL_FILE_IO_FUNCTIONS)
      registerIOFunctions();
   CALL_ALL_MODULES(pfNXSLServerEnvConfig, (this));
}

/**
 * Script trace output
 */
void NXSL_ServerEnv::trace(int level, const TCHAR *text)
{
	if (level == 0)
	{
		nxlog_write(NXLOG_INFO, _T("%s"), text);
	}
	else
	{
		nxlog_debug(level, _T("%s"), text);
	}
}

/**
 * Print script output to console
 */
void NXSL_ServerEnv::print(NXSL_Value *value)
{
	if (m_console != nullptr)
	{
		const TCHAR *text = value->getValueAsCString();
		ConsolePrintf(m_console, _T("%s"), CHECK_NULL(text));
	}
}

/**
 * Additional VM configuration
 */
void NXSL_ServerEnv::configureVM(NXSL_VM *vm)
{
   NXSL_Environment::configureVM(vm);

   vm->setStorage(&g_nxslPstorage);

   // Severity codes
   vm->addConstant("Severity::NORMAL", vm->createValue(SEVERITY_NORMAL));
   vm->addConstant("Severity::WARNING", vm->createValue(SEVERITY_WARNING));
   vm->addConstant("Severity::MINOR", vm->createValue(SEVERITY_MINOR));
   vm->addConstant("Severity::MAJOR", vm->createValue(SEVERITY_MAJOR));
   vm->addConstant("Severity::CRITICAL", vm->createValue(SEVERITY_CRITICAL));

   // Object status codes
   vm->addConstant("Status::NORMAL", vm->createValue(STATUS_NORMAL));
   vm->addConstant("Status::WARNING", vm->createValue(STATUS_WARNING));
   vm->addConstant("Status::MINOR", vm->createValue(STATUS_MINOR));
   vm->addConstant("Status::MAJOR", vm->createValue(STATUS_MAJOR));
   vm->addConstant("Status::CRITICAL", vm->createValue(STATUS_CRITICAL));
   vm->addConstant("Status::UNKNOWN", vm->createValue(STATUS_UNKNOWN));
   vm->addConstant("Status::UNMANAGED", vm->createValue(STATUS_UNMANAGED));
   vm->addConstant("Status::DISABLED", vm->createValue(STATUS_DISABLED));
   vm->addConstant("Status::TESTING", vm->createValue(STATUS_TESTING));

   // DCI data types
   vm->addConstant("DCI::INT32", vm->createValue(DCI_DT_INT));
   vm->addConstant("DCI::UINT32", vm->createValue(DCI_DT_UINT));
   vm->addConstant("DCI::COUNTER32", vm->createValue(DCI_DT_COUNTER32));
   vm->addConstant("DCI::INT64", vm->createValue(DCI_DT_INT64));
   vm->addConstant("DCI::UINT64", vm->createValue(DCI_DT_UINT64));
   vm->addConstant("DCI::COUNTER64", vm->createValue(DCI_DT_COUNTER64));
   vm->addConstant("DCI::FLOAT", vm->createValue(DCI_DT_FLOAT));
   vm->addConstant("DCI::STRING", vm->createValue(DCI_DT_STRING));
   vm->addConstant("DCI::NULL", vm->createValue(DCI_DT_NULL));

   // DCI states
   vm->addConstant("DCI::ACTIVE", vm->createValue(ITEM_STATUS_ACTIVE));
   vm->addConstant("DCI::DISABLED", vm->createValue(ITEM_STATUS_DISABLED));
   vm->addConstant("DCI::UNSUPPORTED", vm->createValue(ITEM_STATUS_NOT_SUPPORTED));

   // DCI data source (origin)
   vm->addConstant("DataSource::AGENT", vm->createValue(DS_NATIVE_AGENT));
   vm->addConstant("DataSource::DEVICE_DRIVER", vm->createValue(DS_DEVICE_DRIVER));
   vm->addConstant("DataSource::INTERNAL", vm->createValue(DS_INTERNAL));
   vm->addConstant("DataSource::MQTT", vm->createValue(DS_MQTT));
   vm->addConstant("DataSource::PUSH", vm->createValue(DS_PUSH_AGENT));
   vm->addConstant("DataSource::SCRIPT", vm->createValue(DS_SCRIPT));
   vm->addConstant("DataSource::SMCLP", vm->createValue(DS_SMCLP));
   vm->addConstant("DataSource::SNMP", vm->createValue(DS_SNMP_AGENT));
   vm->addConstant("DataSource::SSH", vm->createValue(DS_SSH));
   vm->addConstant("DataSource::WEB_SERVICE", vm->createValue(DS_WEB_SERVICE));
   vm->addConstant("DataSource::WINPERF", vm->createValue(DS_WINPERF));

   // Node state flags
   vm->addConstant("NodeState::Unreachable", vm->createValue(DCSF_UNREACHABLE));
   vm->addConstant("NodeState::NetworkPathProblem", vm->createValue(DCSF_NETWORK_PATH_PROBLEM));
   vm->addConstant("NodeState::AgentUnreachable", vm->createValue(NSF_AGENT_UNREACHABLE));
   vm->addConstant("NodeState::EtherNetIPUnreachable", vm->createValue(NSF_ETHERNET_IP_UNREACHABLE));
   vm->addConstant("NodeState::SNMPUnreachable", vm->createValue(NSF_SNMP_UNREACHABLE));
   vm->addConstant("NodeState::CacheModeNotSupported", vm->createValue(NSF_CACHE_MODE_NOT_SUPPORTED));

   // Cluster state flags
   vm->addConstant("ClusterState::Unreachable", vm->createValue(DCSF_UNREACHABLE));
   vm->addConstant("ClusterState::NetworkPathProblem", vm->createValue(DCSF_NETWORK_PATH_PROBLEM));
   vm->addConstant("ClusterState::Down", vm->createValue(CLSF_DOWN));

   // Sensor state flags
   vm->addConstant("SensorState::Unreachable", vm->createValue(DCSF_UNREACHABLE));
   vm->addConstant("SensorState::NetworkPathProblem", vm->createValue(DCSF_NETWORK_PATH_PROBLEM));
   vm->addConstant("SensorState::Provisioned", vm->createValue(SSF_PROVISIONED));
   vm->addConstant("SensorState::Registered", vm->createValue(SSF_REGISTERED));
   vm->addConstant("SensorState::Active", vm->createValue(SSF_ACTIVE));
   vm->addConstant("SensorState::PendingConfigUpdate", vm->createValue(SSF_CONF_UPDATE_PENDING));

   CALL_ALL_MODULES(pfNXSLServerVMConfig, (vm));
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
	if (m_session != nullptr && m_response != nullptr)
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
	if ((m_session != nullptr) && (m_response != nullptr))
	{
      size_t len = _tcslen(text);
      TCHAR *t = MemAllocString(len + 2);
      memcpy(t, text, len * sizeof(TCHAR));
      t[len] = _T('\n');
      t[len + 1] = 0;
		m_response->setField(VID_MESSAGE, t);
		m_session->sendMessage(m_response);
      MemFree(t);
	}
   NXSL_ServerEnv::trace(level, text);
}

/**
 * Call hook script
 */
NXSL_VM *FindHookScript(const TCHAR *hookName, shared_ptr<NetObj> object)
{
	TCHAR scriptName[MAX_PATH] = _T("Hook::");
	nx_strncpy(&scriptName[6], hookName, MAX_PATH - 6);
	NXSL_VM *vm = CreateServerScriptVM(scriptName, object);
	if (vm == nullptr)
		DbgPrintf(7, _T("FindHookScript: hook script \"%s\" not found"), scriptName);
   return vm;
}
