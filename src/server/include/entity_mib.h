/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: entity_mib.h
**
**/

#ifndef _entity_mib_h_
#define _entity_mib_h_

#include <nms_common.h>
#include <nxsrvapi.h>
#include <server_console.h>
#include <nxsl.h>

class NXSL_ComponentHandle;

/**
 * Component classes
 */
enum ComponentClass
{
   COMPONENT_CLASS_OTHER = 1,
   COMPONENT_CLASS_UNKNOWN = 2,
   COMPONENT_CLASS_CHASSIS = 3,
   COMPONENT_CLASS_BACKPLANE = 4,
   COMPONENT_CLASS_CONTAINER = 5,
   COMPONENT_CLASS_POWER_SUPPLY = 6,
   COMPONENT_CLASS_FAN = 7,
   COMPONENT_CLASS_SENSOR = 8,
   COMPONENT_CLASS_MODULE = 9,
   COMPONENT_CLASS_PORT = 10,
   COMPONENT_CLASS_STACK = 11,
   COMPONENT_CLASS_CPU = 12
};

#ifdef _WIN32
class Component;
template class LIBNXSRV_EXPORTABLE ObjectArray<Component>;
#endif

/**
 * Node component
 */
class LIBNXSRV_EXPORTABLE Component
{
   friend class NXSL_ComponentHandle;

protected:
   uint32_t m_index;         // Unique index of this component
   uint32_t m_class;         // Component class
   uint32_t m_ifIndex;       // Interface index if component represents network interface
   TCHAR *m_name;
   TCHAR *m_description;
   TCHAR *m_model;
   TCHAR *m_serial;
   TCHAR *m_vendor;
   TCHAR *m_firmware;
   uint32_t m_parentIndex;   // Index of parent component or 0 for root
   int32_t m_position;       // Component relative position within parent
   Component *m_parent;
   ObjectArray<Component> m_children;

   NXSL_Array *getChildrenForNXSL(NXSL_VM *vm, NXSL_ComponentHandle *handle) const;

public:
   Component(uint32_t index, const TCHAR *name);
   Component(uint32_t index, uint32_t pclass, uint32_t parentIndex, int32_t position, uint32_t ifIndex,
         const TCHAR *name, const TCHAR *description, const TCHAR *model, const TCHAR *serial,
         const TCHAR *vendor, const TCHAR *firmware);
   ~Component();

   uint32_t updateFromSnmp(SNMP_Transport *snmp);
   void buildTree(ObjectArray<Component> *elements);

   uint32_t getIndex() const { return m_index; }
   uint32_t getParentIndex() const { return m_parentIndex; }
   const Component *getParent() const { return m_parent; }
   int32_t getPosition() const { return m_position; }
   const ObjectArray<Component>& getChildren() const { return m_children; }

   template<typename C> void forEach(void (*callback)(const Component*, C*), C *context) const
   {
      for(int i = 0; i < m_children.size(); i++)
         callback(m_children.get(i), context);
   }

   uint32_t getClass() const { return m_class; }
   const TCHAR *getDescription() const { return m_description; }
   uint32_t getIfIndex() const { return m_ifIndex; }
   const TCHAR *getFirmware() const { return m_firmware; }
   const TCHAR *getModel() const { return m_model; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getSerial() const { return m_serial; }
   const TCHAR *getVendor() const { return m_vendor; }

   bool equals(const Component *c) const;

   uint32_t fillMessage(NXCPMessage *msg, uint32_t baseId) const;

   void print(ServerConsole *console, int level) const;
};

/**
 * Node component tree
 */
class LIBNXSRV_EXPORTABLE ComponentTree
{
private:
   Component *m_root;
   time_t m_timestamp;

public:
   ComponentTree(Component *root);
   ~ComponentTree();

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;
   void print(ServerConsole *console) const { if (m_root != nullptr) m_root->print(console, 0); }

   bool isEmpty() const { return m_root == NULL; }
   const Component *getRoot() const { return m_root; }
   bool equals(const ComponentTree *t) const;
   time_t getTimestamp() const { return m_timestamp; }

   static NXSL_Value *getRootForNXSL(NXSL_VM *vm, shared_ptr<ComponentTree> tree);
};

/**
 * Build component tree form entity MIB
 */
shared_ptr<ComponentTree> LIBNXSRV_EXPORTABLE BuildComponentTree(SNMP_Transport *snmp, const TCHAR *debugInfo);

/**
 * NXSL "Component" class
 */
class LIBNXSRV_EXPORTABLE NXSL_ComponentClass : public NXSL_Class
{
public:
   NXSL_ComponentClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual void onObjectDelete(NXSL_Object *object) override;
};

/**
 * NXSL class "Component" instance
 */
extern NXSL_ComponentClass LIBNXSRV_EXPORTABLE g_nxslComponentClass;

#endif
