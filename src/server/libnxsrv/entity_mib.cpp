/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
class NXSL_ComponentHandle
{
public:
   shared_ptr<ComponentTree> tree;
   Component *component;

   NXSL_ComponentHandle(shared_ptr<ComponentTree> t, Component *c)
   {
      tree = t;
      component = c;
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
	m_timestamp = time(nullptr);
}

/**
 * Component tree destructor
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
	if (m_root != nullptr)
		m_root->fillMessage(msg, baseId);
}

/**
 * Check if two component trees are equal
 */
bool ComponentTree::equals(const ComponentTree *t) const
{
   if (m_root == nullptr)
      return t->m_root == nullptr;
   if (t->m_root == nullptr)
      return false;
   return m_root->equals(t->m_root);
}

/**
 * Get root component as NXSL object
 */
NXSL_Value *ComponentTree::getRootForNXSL(NXSL_VM *vm, shared_ptr<ComponentTree> tree)
{
   return vm->createValue(vm->createObject(&g_nxslComponentClass, new NXSL_ComponentHandle(tree, tree->m_root)));
}

/**
 * Constructor
 */
Component::Component(uint32_t index, const TCHAR *name) : m_children(0, 16, Ownership::True)
{
	m_index = index;
	m_class = COMPONENT_CLASS_UNKNOWN;
	m_ifIndex = 0;
	m_name = MemCopyString(name);
	m_description = nullptr;
	m_model = nullptr;
	m_serial = nullptr;
	m_vendor = nullptr;
	m_firmware = nullptr;
	m_parentIndex = 0;
	m_position = -1;
	m_parent = nullptr;
}

/**
 * Constructor for creating from scratch
 */
Component::Component(uint32_t index, uint32_t pclass, uint32_t parentIndex, int32_t position, uint32_t ifIndex,
         const TCHAR *name, const TCHAR *description, const TCHAR *model, const TCHAR *serial,
         const TCHAR *vendor, const TCHAR *firmware) : m_children(0, 16, Ownership::True)
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
   m_parent = nullptr;
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
}

/**
 * Update component from SNMP
 */
uint32_t Component::updateFromSnmp(SNMP_Transport *snmp)
{
	uint32_t oid[16] = { 1, 3, 6, 1, 2, 1, 47, 1, 1, 1, 1, 0, 0 };
	uint32_t rc;
	TCHAR buffer[256];

	oid[12] = m_index;

	oid[11] = 5;	// entPhysicalClass
	if ((rc = SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, &m_class, sizeof(uint32_t), 0)) != SNMP_ERR_SUCCESS)
		return rc;

	oid[11] = 4;	// entPhysicalContainedIn
	if ((rc = SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, &m_parentIndex, sizeof(uint32_t), 0)) != SNMP_ERR_SUCCESS)
		return rc;

   oid[11] = 6;   // entPhysicalParentRelPos
   if ((rc = SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, &m_position, sizeof(uint32_t), 0)) != SNMP_ERR_SUCCESS)
      return rc;

	oid[11] = 2;	// entPhysicalDescr
	if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_description = MemCopyString(buffer);
	else
      m_description = MemCopyString(_T(""));

	oid[11] = 13;	// entPhysicalModelName
	if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_model = MemCopyString(buffer);
	else
      m_model = MemCopyString(_T(""));

	oid[11] = 11;	// entPhysicalSerialNum
	if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_serial = MemCopyString(buffer);
	else
      m_serial = MemCopyString(_T(""));

	oid[11] = 12;	// entPhysicalMfgName
	if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
	   m_vendor = MemCopyString(buffer);
	else
      m_vendor = MemCopyString(_T(""));

	oid[11] = 9;	// entPhysicalFirmwareRev
	if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 13, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
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
   if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 14, buffer, sizeof(buffer), SG_STRING_RESULT) == SNMP_ERR_SUCCESS)
   {
      if (!_tcsncmp(buffer, _T(".1.3.6.1.2.1.2.2.1.1."), 21))
      {
         m_ifIndex = _tcstoul(&buffer[21], nullptr, 10);
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
		   m_children.add(e);
		   e->m_parent = this;
			e->buildTree(elements);
		}
	}
	m_children.sort(ComponentComparator);
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
	for(int i = 0; i < m_children.size(); i++)
	   m_children.get(i)->print(console, level + 1);
}

/**
 * Fill NXCP message
 */
uint32_t Component::fillMessage(NXCPMessage *msg, uint32_t baseId) const
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

	msg->setField(baseId + 19, static_cast<uint32_t>(m_children.size()));
	uint32_t fieldId = baseId + 20;
	for(int i = 0; i < m_children.size(); i++)
		fieldId = m_children.get(i)->fillMessage(msg, fieldId);

	return fieldId;
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
       (m_children.size() != c->m_children.size()) ||
       _tcscmp(CHECK_NULL_EX(m_name), CHECK_NULL_EX(c->m_name)) ||
       _tcscmp(CHECK_NULL_EX(m_description), CHECK_NULL_EX(c->m_description)) ||
       _tcscmp(CHECK_NULL_EX(m_model), CHECK_NULL_EX(c->m_model)) ||
       _tcscmp(CHECK_NULL_EX(m_serial), CHECK_NULL_EX(c->m_serial)) ||
       _tcscmp(CHECK_NULL_EX(m_vendor), CHECK_NULL_EX(c->m_vendor)) ||
       _tcscmp(CHECK_NULL_EX(m_firmware), CHECK_NULL_EX(c->m_firmware)))
      return false;

   for(int i = 0; i < m_children.size(); i++)
      if (!m_children.get(i)->equals(c->m_children.get(i)))
         return false;

   return true;
}

/*
 * Get child components as NXSL Array
 */
NXSL_Array *Component::getChildrenForNXSL(NXSL_VM *vm, NXSL_ComponentHandle *handle) const
{
   NXSL_Array *components = new NXSL_Array(vm);
   for(int i = 0; i < m_children.size(); i++)
   {
      components->set(i, vm->createValue(vm->createObject(&g_nxslComponentClass, new NXSL_ComponentHandle(handle->tree, m_children.get(i)))));
   }
   return components;
}

/**
 * Physical entity tree walk callback
 */
static uint32_t EntityWalker(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
	TCHAR buffer[256];
	Component *element = new Component(var->getName().getElement(12), var->getValueAsString(buffer, 256));
	uint32_t rc = element->updateFromSnmp(transport);
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
shared_ptr<ComponentTree> LIBNXSRV_EXPORTABLE BuildComponentTree(SNMP_Transport *snmp, const TCHAR *debugInfo)
{
	nxlog_debug_tag(DEBUG_TAG, 5, _T("Building component tree for %s"), debugInfo);
	ObjectArray<Component> elements(16, 16);
	shared_ptr<ComponentTree> tree;
	if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.47.1.1.1.1.7"), EntityWalker, &elements) == SNMP_ERR_SUCCESS)
	{
	   nxlog_debug_tag(DEBUG_TAG, 6, _T("BuildComponentTree(%s): %d elements found"), debugInfo, elements.size());

		Component *root = nullptr;
		for(int i = 0; i < elements.size(); i++)
			if (elements.get(i)->getParentIndex() == 0)
			{
				root = elements.get(i);
				break;
			}

		if (root != nullptr)
		{
			root->buildTree(&elements);
			tree = make_shared<ComponentTree>(root);
		   nxlog_debug_tag(DEBUG_TAG, 5, _T("BuildComponentTree(%s): component tree created successfully"), debugInfo);

		   // Check for components without parent
		   for(int i = 0; i < elements.size(); i++)
		   {
		      Component *c = elements.get(i);
		      if ((c->getParent() == nullptr) && (c != root))
		      {
		         nxlog_debug_tag(DEBUG_TAG, 6, _T("BuildComponentTree(%s): found element without parent but it is not root (index:%u, parentIndex:%u, name:%s)"),
		                  debugInfo, c->getIndex(), c->getParentIndex(), c->getName());
		         delete c;
		      }
		   }
		}
		else
		{
		   nxlog_debug_tag(DEBUG_TAG, 6, _T("BuildComponentTree(%s): root element not found"), debugInfo);
			elements.setOwner(Ownership::True);	// cause element destruction on exit
		}
	}
	else
	{
	   nxlog_debug_tag(DEBUG_TAG, 6, _T("BuildComponentTree(%s): SNMP WALK failed"), debugInfo);
		elements.setOwner(Ownership::True);	// cause element destruction on exit
	}
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
NXSL_Value *NXSL_ComponentClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   const uint32_t classCount = 13;
   static const TCHAR *className[classCount] = { _T(""), _T("other"), _T("unknown"), _T("chassis"), _T("backplane"),
                                                 _T("container"), _T("power supply"), _T("fan"), _T("sensor"),
                                                 _T("module"), _T("port"), _T("stack"), _T("cpu") };

   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   NXSL_ComponentHandle *handle = static_cast<NXSL_ComponentHandle*>(object->getData());
   Component *component = (handle != nullptr) ? handle->component : nullptr;  // Can be null if called by scanAttributes()
   if (NXSL_COMPARE_ATTRIBUTE_NAME("class"))
   {
      if (component->getClass() >= classCount)
         value = vm->createValue(className[2]); // Unknown class
      else
         value = vm->createValue(className[component->getClass()]);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("children"))
   {
      value = vm->createValue(handle->getChildrenForNXSL(vm));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(component->getDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ifIndex"))
   {
      value = vm->createValue(component->getIfIndex());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("firmware"))
   {
      value = vm->createValue(component->getFirmware());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("model"))
   {
      value = vm->createValue(component->getModel());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(component->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serial"))
   {
      value = vm->createValue(component->getSerial());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vendor"))
   {
      value = vm->createValue(component->getVendor());
   }
   return value;
}

/**
 * NXSL class "Component" instance
 */
NXSL_ComponentClass LIBNXSRV_EXPORTABLE g_nxslComponentClass;
