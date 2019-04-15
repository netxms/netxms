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

/**
 * Node component
 */
class LIBNXSRV_EXPORTABLE Component
{
protected:
   UINT32 m_index;
   UINT32 m_class;
   UINT32 m_ifIndex;
   TCHAR *m_name;
   TCHAR *m_description;
   TCHAR *m_model;
   TCHAR *m_serial;
   TCHAR *m_vendor;
   TCHAR *m_firmware;
   UINT32 m_parentIndex;
   ObjectArray<Component> m_children;

public:
   Component(UINT32 index, const TCHAR *name);
   virtual ~Component();

   UINT32 updateFromSnmp(SNMP_Transport *snmp);
   void buildTree(ObjectArray<Component> *elements);

   UINT32 getIndex() const { return m_index; }
   UINT32 getParentIndex() const { return m_parentIndex; }
   NXSL_Array *getChildrenForNXSL(NXSL_VM *vm) const;

   UINT32 getClass() const { return m_class; }
   const TCHAR *getDescription() const { return m_description; }
   const TCHAR *getFirmware() const { return m_firmware; }
   const TCHAR *getModel() const { return m_model; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getSerial() const { return m_serial; }
   const TCHAR *getVendor() const { return m_vendor; }

   UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId) const;

   void print(ServerConsole *console, int level) const;
};

/**
 * Node component tree
 */
class LIBNXSRV_EXPORTABLE ComponentTree : public RefCountObject
{
private:
   Component *m_root;

public:
   ComponentTree(Component *root);
   virtual ~ComponentTree();

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;
   void print(ServerConsole *console) const { if (m_root != NULL) m_root->print(console, 0); }

   bool isEmpty() const { return m_root == NULL; }
   const Component *getRoot() const { return m_root; }
};

/**
 * Build component tree form entity MIB
 */
ComponentTree LIBNXSRV_EXPORTABLE *BuildComponentTree(SNMP_Transport *snmp, const TCHAR *debugInfo);

/**
 * NXSL "Component" class
 */
class NXSL_ComponentClass : public NXSL_Class
{
public:
   NXSL_ComponentClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const char *attr);
};

/**
 * NXSL class "Component" instance
 */
extern NXSL_ComponentClass LIBNXSRV_EXPORTABLE g_nxslComponentClass;

#endif
