/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: components.cpp
**
**/

#include "libnxsrv.h"
#include <entity_mib.h>

#define DEBUG_TAG _T("snmp.entity")

/**
 * Component handle for NXSL
 */
struct NXSL_ComponentHandle
{
   ComponentTree *tree;
   Component *component;

   NXSL_ComponentHandle(ComponentTree *t, Component *c)
   {
      tree = t;
      component = c;
      t->incRefCount();
   }

   ~NXSL_ComponentHandle()
   {
      tree->decRefCount();
   }

   NXSL_Array *getChildrenForNXSL(NXSL_VM *vm)
   {
      return component->getChildrenForNXSL(vm, this);
   }
};

/**
 * Component tree constructor
 */
ComponentTree::ComponentTree(Component *root)
{
	m_root = root;
}

/**
 * Componnet tree destructor
 */
ComponentTree::~ComponentTree()
{
	delete m_root;
}

/**
 * Fill NXCP message with tree data
 */
void ComponentTree::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
	if (m_root != NULL)
		m_root->fillMessage(msg, baseId);
}

/**
 * Check if two component trees are equal
 */
bool ComponentTree::equals(const ComponentTree *t) const
{
   if (m_root == NULL)
      return t->m_root == NULL;
   if (t->m_root == NULL)
      return false;
   return m_root->equals(t->m_root);
}

/**
 * Get root component as NXSL object
 */
NXSL_Value *ComponentTree::getRootForNXSL(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslComponentClass, new NXSL_ComponentHandle(this, m_root)));
}

/**
 * Constructor
 */
Component::Component(UINT32 index, const TCHAR *name)
{
	m_index = index;
	m_class = 2; // unknown
	m_ifIndex = 0;
	m_name = MemCopyString(name);
	m_description = NULL;
	m_model = NULL;
	m_serial = NULL;
	m_vendor = NULL;
	m_firmware = NULL;
	m_parentIndex = 0;
	m_position = -1;
   m_children = new ObjectArray<Component>(0, 16, true);
}

/**
 * Constructor for creating from scratch
 */
Component::Component(UINT32 index, UINT32 pclass, UINT32 parentIndex, UINT32 position, UINT32 ifIndex,
         const TCHAR *name, const TCHAR *description, const TCHAR *model, const TCHAR *serial,
         const TCHAR *vendor, const TCHAR *firmware)
{
   m_index = index;
   m_class = pclass;
   m_ifIndex = ifIndex;
   m_name = MemCopyString(name);
   m_description = MemCopyString(description);
   m_model = MemCopyString(model);
   m_serial = MemCopyString(serial);
   m_vendor = MemCopyString(vendor);
   m_firmware = MemCopyString(firmware);
   m_parentIndex = parentIndex;
   m_position = position;
   m_children = new ObjectArray<Component>(0, 16, true);
}

/**
 * Destructor
 */
Component::~Component()
{
	MemFree(m_name);
	MemFree(m_description);
	MemFree(m_model);
	MemFree(m_serial);
	MemFree(m_vendor);
	MemFree(m_firmware);
   delete m_children;
}

/**
 * Update component from SNMP
 */
UINT32 Component::updateFromSnmp(SNMP_Transport *snmp)
{
	UINT32 oid[16] = { 1, 3, 6, 1, 2, 1, 47, 1, 1, 1, 1, 0, 0 };
	UINT32 rc;
	TCHAR buffer[256];

	oid[12] = m_index;

	oid[11] = 5;	// entPhysicalClass
	if ((rc = SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, &m_class, sizeof(UINT32), 0)) != SNMP_ERR_SUCCESS)
		return rc;

	oid[11] = 4;	// entPhysicalContainedIn
	if ((rc = SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, &m_parentIndex, sizeof(UINT32), 0)) != SNMP_ERR_SUCCESS)
		return rc;

   oid[11] = 6;   // entPhysicalParentRelPos
   if ((rc = SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, &m_position, sizeof(INT32), 0)) != SNMP_ERR_SUCCESS)
      return rc;

	oid[11] = 2;	// entPhysicalDescr
	if (SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_description = MemCopyString(buffer);
	else
      m_description = MemCopyString(_T(""));

	oid[11] = 13;	// entPhysicalModelName
	if (SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_model = MemCopyString(buffer);
	else
      m_model = MemCopyString(_T(""));

	oid[11] = 11;	// entPhysicalSerialNum
	if (SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_serial = MemCopyString(buffer);
	else
      m_serial = MemCopyString(_T(""));

	oid[11] = 12;	// entPhysicalMfgName
	if (SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_vendor = MemCopyString(buffer);
	else
      m_vendor = MemCopyString(_T(""));

	oid[11] = 9;	// entPhysicalFirmwareRev
	if (SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_firmware = MemCopyString(buffer);
	else
	   m_firmware = MemCopyString(_T(""));

	// Read entAliasMappingIdentifier
	oid[8] = 3;
	oid[9] = 2;
	oid[10] = 1;
   oid[11] = 2;
	oid[12] = m_index;
	oid[13] = 0;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, NULL, oid, 14, buffer, sizeof(buffer), SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
   {
      if (!_tcsncmp(buffer, _T(".1.3.6.1.2.1.2.2.1.1."), 21))
      {
         m_ifIndex = _tcstoul(&buffer[21], NULL, 10);
      }
   }

	return SNMP_ERR_SUCCESS;
}

/**
 * Compare components by position within parent
 */
static int ComponentComparator(const Component **c1, const Component **c2)
{
   return (*c1)->getPosition() - (*c2)->getPosition();
}

/**
 * Build element tree
 */
void Component::buildTree(ObjectArray<Component> *elements)
{
	for(int i = 0; i < elements->size(); i++)
	{
		Component *e = elements->get(i);
		if (e->m_parentIndex == m_index)
		{
		   m_children->add(e);
			e->buildTree(elements);
		}
	}
	m_children->sort(ComponentComparator);
}

/**
 * Print element tree to given console
 */
void Component::print(ServerConsole *console, int level) const
{
   TCHAR ifInfo[128] = _T("");
   if (m_ifIndex != 0)
   {
      _sntprintf(ifInfo, 128, _T(" \x1b[33;1m[ifIndex=%u]\x1b[0m"), m_ifIndex);
   }
   console->printf(_T("%*s\x1b[1m%d\x1b[0m \x1b[32;1m%-32s\x1b[0m %s%s\n"),
            level * 4, _T(""), (int)m_index, m_name, m_description, ifInfo);
	for(int i = 0; i < m_children->size(); i++)
	   m_children->get(i)->print(console, level + 1);
}

/**
 * Fill NXCP message
 */
UINT32 Component::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
	msg->setField(baseId, m_index);
	msg->setField(baseId + 1, m_parentIndex);
   msg->setField(baseId + 2, m_position);
	msg->setField(baseId + 3, m_class);
	msg->setField(baseId + 4, m_ifIndex);
	msg->setField(baseId + 5, m_name);
	msg->setField(baseId + 6, m_description);
	msg->setField(baseId + 7, m_model);
	msg->setField(baseId + 8, m_serial);
	msg->setField(baseId + 9, m_vendor);
	msg->setField(baseId + 10, m_firmware);

	msg->setField(baseId + 19, (UINT32)m_children->size());
	UINT32 varId = baseId + 20;
	for(int i = 0; i < m_children->size(); i++)
		varId = m_children->get(i)->fillMessage(msg, varId);

	return varId;
}

/**
 * Check if two components are equal
 */
bool Component::equals(const Component *c) const
{
   if ((m_index != c->m_index) ||
       (m_position != c->m_position) ||
       (m_class != c->m_class) ||
       (m_ifIndex != c->m_ifIndex) ||
       (m_parentIndex != c->m_parentIndex) ||
       (m_children->size() != c->m_children->size()) ||
       _tcscmp(CHECK_NULL_EX(m_name), CHECK_NULL_EX(c->m_name)) ||
       _tcscmp(CHECK_NULL_EX(m_description), CHECK_NULL_EX(c->m_description)) ||
       _tcscmp(CHECK_NULL_EX(m_model), CHECK_NULL_EX(c->m_model)) ||
       _tcscmp(CHECK_NULL_EX(m_serial), CHECK_NULL_EX(c->m_serial)) ||
       _tcscmp(CHECK_NULL_EX(m_vendor), CHECK_NULL_EX(c->m_vendor)) ||
       _tcscmp(CHECK_NULL_EX(m_firmware), CHECK_NULL_EX(c->m_firmware)))
      return false;

   for(int i = 0; i < m_children->size(); i++)
      if (!m_children->get(i)->equals(c->m_children->get(i)))
         return false;

   return true;
}

/*
 * Get child components as NXSL Array
 */
NXSL_Array *Component::getChildrenForNXSL(NXSL_VM *vm, NXSL_ComponentHandle *handle) const
{
   NXSL_Array *components = new NXSL_Array(vm);
   for(int i = 0; i < m_children->size(); i++)
   {
      components->set(i, vm->createValue(new NXSL_Object(vm, &g_nxslComponentClass, new NXSL_ComponentHandle(handle->tree, m_children->get(i)))));
   }
   return components;
}

/**
 * Physical entity tree walk callback
 */
static UINT32 EntityWalker(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	TCHAR buffer[256];
	Component *element = new Component(var->getName().getElement(12), var->getValueAsString(buffer, 256));
	UINT32 rc = element->updateFromSnmp(transport);
	if (rc != SNMP_ERR_SUCCESS)
	{
		delete element;
		return rc;
	}	
	static_cast<ObjectArray<Component>*>(arg)->add(element);
	return SNMP_ERR_SUCCESS;
}

/**
 * Build components tree for given node
 */
ComponentTree LIBNXSRV_EXPORTABLE *BuildComponentTree(SNMP_Transport *snmp, const TCHAR *debugInfo)
{
	nxlog_debug_tag(DEBUG_TAG, 5, _T("Building component tree for %s"), debugInfo);
	ObjectArray<Component> elements(16, 16);
	ComponentTree *tree = NULL;
	if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.47.1.1.1.1.7"), EntityWalker, &elements) == SNMP_ERR_SUCCESS)
	{
	   nxlog_debug_tag(DEBUG_TAG, 6, _T("BuildComponentTree(%s): %d elements found"), debugInfo, elements.size());

		Component *root = NULL;
		for(int i = 0; i < elements.size(); i++)
			if (elements.get(i)->getParentIndex() == 0)
			{
				root = elements.get(i);
				break;
			}

		if (root != NULL)
		{
			root->buildTree(&elements);
			tree = new ComponentTree(root);
		}
		else
		{
		   nxlog_debug_tag(DEBUG_TAG, 6, _T("BuildComponentTree(%s): root element not found"), debugInfo);
			elements.setOwner(true);	// cause element destruction on exit
		}
	}
	else
	{
	   nxlog_debug_tag(DEBUG_TAG, 6, _T("BuildComponentTree(%s): SNMP WALK failed"), debugInfo);
		elements.setOwner(true);	// cause element destruction on exit
	}
	nxlog_debug_tag(DEBUG_TAG, 5, _T("BuildComponentTree(%s [%d]): %p"), debugInfo, tree);
	return tree;
}

/**
 * NXSL class ComponentClass: constructor
 */
NXSL_ComponentClass::NXSL_ComponentClass() : NXSL_Class()
{
   setName(_T("Component"));
}

/**
 * NXSL object destructor
 */
void NXSL_ComponentClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<NXSL_ComponentHandle*>(object->getData());
}

/**
 * NXSL class ComponentClass: get attribute
 */
NXSL_Value *NXSL_ComponentClass::getAttr(NXSL_Object *object, const char *attr)
{
   const UINT32 classCount = 12;
   static const TCHAR *className[classCount] = { _T(""), _T("other"), _T("unknown"), _T("chassis"), _T("backplane"),
                                                 _T("container"), _T("power supply"), _T("fan"), _T("sensor"),
                                                 _T("module"), _T("port"), _T("stack") };

   NXSL_VM *vm = object->vm();
   NXSL_Value *value = NULL;
   NXSL_ComponentHandle *handle = static_cast<NXSL_ComponentHandle*>(object->getData());
   Component *component = handle->component;
   if (!strcmp(attr, "class"))
   {
      if (component->getClass() >= classCount)
         value = vm->createValue(className[2]); // Unknown class
      else
         value = vm->createValue(className[component->getClass()]);
   }
   else if (!strcmp(attr, "children"))
   {
      value = vm->createValue(handle->getChildrenForNXSL(vm));
   }
   else if (!strcmp(attr, "description"))
   {
      value = vm->createValue(component->getDescription());
   }
   else if (!strcmp(attr, "ifIndex"))
   {
      value = vm->createValue(component->getIfIndex());
   }
   else if (!strcmp(attr, "firmware"))
   {
      value = vm->createValue(component->getFirmware());
   }
   else if (!strcmp(attr, "model"))
   {
      value = vm->createValue(component->getModel());
   }
   else if (!strcmp(attr, "name"))
   {
      value = vm->createValue(component->getName());
   }
   else if (!strcmp(attr, "serial"))
   {
      value = vm->createValue(component->getSerial());
   }
   else if (!strcmp(attr, "vendor"))
   {
      value = vm->createValue(component->getVendor());
   }
   return value;
}

/**
 * NXSL class "Component" instance
 */
NXSL_ComponentClass LIBNXSRV_EXPORTABLE g_nxslComponentClass;
