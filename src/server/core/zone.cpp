/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: zone.cpp
**
**/

#include "nxcore.h"

/**
 * Dump index to console
 */
void DumpIndex(CONSOLE_CTX pCtx, InetAddressIndex *index);

/**
 * Zone class default constructor
 */
Zone::Zone() : super()
{
   m_id = 0;
   m_uin = 0;
   _tcscpy(m_name, _T("Default"));
   m_proxyNodeId = 0;
	m_idxNodeByAddr = new InetAddressIndex;
	m_idxInterfaceByAddr = new InetAddressIndex;
	m_idxSubnetByAddr = new InetAddressIndex;
}

/**
 * Constructor for new zone object
 */
Zone::Zone(UINT32 uin, const TCHAR *name) : super()
{
   m_id = 0;
   m_uin = uin;
   nx_strncpy(m_name, name, MAX_OBJECT_NAME);
   m_proxyNodeId = 0;
	m_idxNodeByAddr = new InetAddressIndex;
	m_idxInterfaceByAddr = new InetAddressIndex;
	m_idxSubnetByAddr = new InetAddressIndex;
}

/**
 * Zone class destructor
 */
Zone::~Zone()
{
	delete m_idxNodeByAddr;
	delete m_idxInterfaceByAddr;
	delete m_idxSubnetByAddr;
}

/**
 * Create object from database data
 */
bool Zone::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT zone_guid,proxy_node,snmp_ports FROM zones WHERE id=?"));
   if (hStmt == NULL)
      return false;

   bool success = false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) == 0)
      {
         if (dwId == BUILTIN_OID_ZONE0)
         {
            m_uin = 0;
            success = true;
         }
         else
         {
            DbgPrintf(4, _T("Cannot load zone object %ld - missing record in \"zones\" table"), (long)m_id);
         }
      }
      else
      {
         m_uin = DBGetFieldULong(hResult, 0, 0);
         m_proxyNodeId = DBGetFieldULong(hResult, 0, 1);
         TCHAR buffer[MAX_DB_STRING];
         DBGetField(hResult, 0, 2, buffer, MAX_DB_STRING);
         if (buffer[0] != 0)
            m_snmpPorts.splitAndAdd(buffer, _T(","));
         success = true;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   // Load access list
   if (success)
      success = loadACLFromDB(hdb);

   return success;
}

/**
 * Save object to database
 */
bool Zone::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("zones"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE zones SET zone_guid=?,proxy_node=?,snmp_ports=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO zones (zone_guid,proxy_node,snmp_ports,id) VALUES (?,?,?,?)"));
      }
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_uin);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_proxyNodeId);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_snmpPorts.join(_T(",")), DB_BIND_DYNAMIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
      success = saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_modified = 0;
   unlockProperties();
   return success;
}

/**
 * Delete zone object from database
 */
bool Zone::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM zones WHERE id=?"));
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Zone::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
   msg->setField(VID_ZONE_UIN, m_uin);
   msg->setField(VID_ZONE_PROXY, m_proxyNodeId);
   m_snmpPorts.fillMessage(msg, VID_ZONE_SNMP_PORT_LIST_BASE, VID_ZONE_SNMP_PORT_COUNT);
}

/**
 * Modify object from message
 */
UINT32 Zone::modifyFromMessageInternal(NXCPMessage *request)
{
	if (request->isFieldExist(VID_ZONE_PROXY))
		m_proxyNodeId = request->getFieldAsUInt32(VID_ZONE_PROXY);

	if (request->isFieldExist(VID_ZONE_SNMP_PORT_LIST_BASE) && request->isFieldExist(VID_ZONE_SNMP_PORT_COUNT))
	{
	   m_snmpPorts.clear();
      int count = request->getFieldAsUInt32(VID_ZONE_SNMP_PORT_COUNT);
      UINT32 fieldId = VID_ZONE_SNMP_PORT_LIST_BASE;
	   for(int i = 0; i < count; i++)
	   {
	      m_snmpPorts.addPreallocated(request->getFieldAsString(fieldId++));
	   }
	}

   return super::modifyFromMessageInternal(request);
}

/**
 * Update interface index
 */
void Zone::updateInterfaceIndex(const InetAddress& oldIp, const InetAddress& newIp, Interface *iface)
{
	m_idxInterfaceByAddr->remove(oldIp);
	m_idxInterfaceByAddr->put(newIp, iface);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Zone::showThresholdSummary()
{
	return true;
}

/**
 * Remove interface from index
 */
void Zone::removeFromIndex(Interface *iface)
{
   const ObjectArray<InetAddress> *list = iface->getIpAddressList()->getList();
   for(int i = 0; i < list->size(); i++)
   {
      InetAddress *addr = list->get(i);
      if (addr->isValidUnicast())
      {
	      NetObj *o = m_idxInterfaceByAddr->get(*addr);
	      if ((o != NULL) && (o->getId() == iface->getId()))
	      {
		      m_idxInterfaceByAddr->remove(*addr);
	      }
      }
   }
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Zone::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslZoneClass, this));
}

/**
 * Dump interface index to console
 */
void Zone::dumpInterfaceIndex(CONSOLE_CTX console)
{
   DumpIndex(console, m_idxInterfaceByAddr);
}

/**
 * Dump node index to console
 */
void Zone::dumpNodeIndex(CONSOLE_CTX console)
{
   DumpIndex(console, m_idxNodeByAddr);
}

/**
 * Dump subnet index to console
 */
void Zone::dumpSubnetIndex(CONSOLE_CTX console)
{
   DumpIndex(console, m_idxSubnetByAddr);
}

/**
 * Serialize object to JSON
 */
json_t *Zone::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "uin", json_integer(m_uin));
   json_object_set_new(root, "proxyNodeId", json_integer(m_proxyNodeId));
   return root;
}
