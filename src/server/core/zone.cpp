/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Raden Solutions
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

#define DEBUG_TAG_ZONE_PROXY  _T("zone.proxy")

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
   m_proxyNodes = new ObjectArray<ZoneProxy>(0, 16, true);
   GenerateRandomBytes(m_proxyAuthKey, ZONE_PROXY_KEY_LENGTH);
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
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   m_proxyNodes = new ObjectArray<ZoneProxy>(0, 16, true);
   GenerateRandomBytes(m_proxyAuthKey, ZONE_PROXY_KEY_LENGTH);
	m_idxNodeByAddr = new InetAddressIndex;
	m_idxInterfaceByAddr = new InetAddressIndex;
	m_idxSubnetByAddr = new InetAddressIndex;
}

/**
 * Zone class destructor
 */
Zone::~Zone()
{
   delete m_proxyNodes;
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

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT zone_guid,snmp_ports FROM zones WHERE id=?"));
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
            nxlog_debug(4, _T("Cannot load zone object %ld - missing record in \"zones\" table"), (long)m_id);
         }
      }
      else
      {
         m_uin = DBGetFieldULong(hResult, 0, 0);
         TCHAR buffer[MAX_DB_STRING];
         DBGetField(hResult, 0, 1, buffer, MAX_DB_STRING);
         if (buffer[0] != 0)
            m_snmpPorts.splitAndAdd(buffer, _T(","));
         success = true;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   if (success)
   {
      success = false;
      hStmt = DBPrepare(hdb, _T("SELECT proxy_node FROM zone_proxies WHERE object_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwId);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         { 
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
               m_proxyNodes->add(new ZoneProxy(DBGetFieldULong(hResult, i, 0)));
            DBFreeResult(hResult);
            success = true;
         }
         DBFreeStatement(hStmt);
      }
   }

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
         hStmt = DBPrepare(hdb, _T("UPDATE zones SET zone_guid=?,snmp_ports=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO zones (zone_guid,snmp_ports,id) VALUES (?,?,?)"));
      }
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_uin);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_snmpPorts.join(_T(",")), DB_BIND_DYNAMIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM zone_proxies WHERE object_id=?"));

   if (success)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO zone_proxies (object_id,proxy_node) VALUES (?,?)"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);         
         for (int i = 0; i < m_proxyNodes->size() && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_proxyNodes->get(i)->nodeId);
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
      success = saveACLToDB(hdb);

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
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM zone_proxies WHERE object_id=?"));
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Zone::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
   msg->setField(VID_ZONE_UIN, m_uin);
   m_snmpPorts.fillMessage(msg, VID_ZONE_SNMP_PORT_LIST_BASE, VID_ZONE_SNMP_PORT_COUNT);

#if HAVE_ALLOCA
   UINT32 *idList = static_cast<UINT32*>(alloca(m_proxyNodes->size() * sizeof(UINT32)));
#else
   UINT32 *idList = MemAllocArrayNoInit<UINT32>(m_proxyNodes->size());
#endif
   for (int i = 0; i < m_proxyNodes->size(); i++)
      idList[i] = m_proxyNodes->get(i)->nodeId;
   msg->setFieldFromInt32Array(VID_ZONE_PROXY_LIST, m_proxyNodes->size(), idList);
#if !HAVE_ALLOCA
   MemFree(idList);
#endif
}

/**
 * Modify object from message
 */
UINT32 Zone::modifyFromMessageInternal(NXCPMessage *request)
{
   if(request->isFieldExist(VID_ZONE_PROXY_LIST))
   {
      IntegerArray<UINT32> newProxyList;
      request->getFieldAsInt32Array(VID_ZONE_PROXY_LIST, &newProxyList);
      for (int i = 0; i < newProxyList.size(); i++)
      {
         int j;
         for(j = 0; j < m_proxyNodes->size(); j++)
         {
            if (m_proxyNodes->get(j)->nodeId == newProxyList.get(i))
               break;
         }
         if (j == m_proxyNodes->size())
            m_proxyNodes->add(new ZoneProxy(newProxyList.get(i)));
      }

      Iterator<ZoneProxy> *it = m_proxyNodes->iterator();
      while(it->hasNext())
      {
         ZoneProxy *proxy = it->next();

         int j;
         for(j = 0; j < newProxyList.size(); j++)
         {
            if (proxy->nodeId == newProxyList.get(j))
               break;
         }
         if (j == newProxyList.size())
            it->remove();
      }
      delete it;
   }

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
 * Callback for processing zone configuration synchronization
 */
static EnumerationCallbackResult ForceConfigurationSync(const UINT32 *nodeId, void *arg)
{
   Node *node = static_cast<Node*>(FindObjectById(*nodeId, OBJECT_NODE));
   if (node != NULL)
      node->forceSyncDataCollectionConfig();
   return _CONTINUE;
}

/**
 * Get proxy node for given object. Always prefers proxy that is already assigned to the object
 * and will update assigned proxy property if changed.
 */
UINT32 Zone::getProxyNodeId(NetObj *object, bool backup)
{
   ZoneProxy *proxy = NULL;
   HashSet<UINT32> syncSet;

   lockProperties();

   if ((object != NULL) && (object->getAssignedZoneProxyId(backup) != 0))
   {
      for(int i = 0; i < m_proxyNodes->size(); i++)
      {
         ZoneProxy *p = m_proxyNodes->get(i);
         if (p->nodeId == object->getAssignedZoneProxyId(backup))
         {
            if (p->isAvailable)
            {
               proxy = p;
            }
            else
            {
               p->assignments--;
               syncSet.put(p->nodeId);

               UINT32 otherProxy = object->getAssignedZoneProxyId(!backup);
               if (otherProxy != 0)
                  syncSet.put(otherProxy);
            }
            break;
         }
      }
   }

   if (proxy == NULL)
   {
      if (m_proxyNodes->size() > (backup ? 1 : 0))
      {
         if (!backup)
            proxy = m_proxyNodes->get(0);
         for(int i = 0; i < m_proxyNodes->size(); i++)
         {
            ZoneProxy *p = m_proxyNodes->get(i);
            if (!p->isAvailable)
               continue;

            if ((object != NULL) && (p->nodeId == object->getAssignedZoneProxyId(!backup)))
               continue;

            if ((proxy == NULL) || (p->loadAverage < proxy->loadAverage) || !proxy->isAvailable)
               proxy = p;
         }
      }
      if (object != NULL)
      {
         if (proxy != NULL)
         {
            object->setAssignedZoneProxyId(proxy->nodeId, backup);
            proxy->assignments++;
            syncSet.put(proxy->nodeId);

            UINT32 otherProxy = object->getAssignedZoneProxyId(!backup);
            if (otherProxy != 0)
               syncSet.put(otherProxy);
         }
         else
         {
            object->setAssignedZoneProxyId(0, backup);
         }
      }
   }

   UINT32 id = (proxy != NULL) ? proxy->nodeId : 0;
   nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 8, _T("Zone::getProxyNodeId: selected %s proxy [%u] for object %s [%u] in zone %s [uin=%u]"),
            backup ? _T("backup") : _T("primary"),
            id, (object != NULL) ? object->getName() : _T("(null)"), (object != NULL) ? object->getId() : 0,
            m_name, m_uin);

   unlockProperties();

   syncSet.forEach(ForceConfigurationSync, NULL);
   return id;
}

/**
 * Check if given node is a proxy for this zone
 */
bool Zone::isProxyNode(UINT32 nodeId) const
{
   bool result = false;
   lockProperties();
   for(int i = 0; i < m_proxyNodes->size(); i++)
      if (m_proxyNodes->get(i)->nodeId == nodeId)
      {
         result = true;
         break;
      }
   unlockProperties();
   return result;
}

/**
 * Get all proxy nodes. Returned array must be destroyed by the caller.
 */
IntegerArray<UINT32> *Zone::getAllProxyNodes() const
{
   lockProperties();
   IntegerArray<UINT32> *nodes = new IntegerArray<UINT32>(m_proxyNodes->size());
   for(int i = 0; i < m_proxyNodes->size(); i++)
      nodes->add(m_proxyNodes->get(i)->nodeId);
   unlockProperties();
   return nodes;
}

/**
 * Fill configuration message for agent
 */
void Zone::fillAgentConfigurationMessage(NXCPMessage *msg) const
{
   lockProperties();
   msg->setField(VID_ZONE_UIN, m_uin);
   msg->setField(VID_SHARED_SECRET, m_proxyAuthKey, ZONE_PROXY_KEY_LENGTH);

   UINT32 fieldId = VID_ZONE_PROXY_BASE, count = 0;
   for(int i = 0; i < m_proxyNodes->size(); i++)
   {
      ZoneProxy *p = m_proxyNodes->get(i);
      Node *node = static_cast<Node*>(FindObjectById(p->nodeId, OBJECT_NODE));
      if (node != NULL)
      {
         msg->setField(fieldId++, p->nodeId);
         msg->setField(fieldId++, node->getIpAddress());
         fieldId += 8;
         count++;
      }
   }
   msg->setField(VID_ZONE_PROXY_COUNT, count);

   unlockProperties();
}

/**
 * Acquire connection to any available proxy node
 */
AgentConnectionEx *Zone::acquireConnectionToProxy(bool validate)
{
   UINT32 nodeId = getProxyNodeId(NULL);
   if (nodeId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 7, _T("Zone::acquireConnectionToProxy: no active proxy in zone %s [uin=%u]"), m_name, m_uin);
      return NULL;
   }

   Node *node = static_cast<Node*>(FindObjectById(nodeId, OBJECT_NODE));
   return (node != NULL) ? node->acquireProxyConnection(ZONE_PROXY, validate) : NULL;
}

/**
 * Update proxy status. Passive mode should be used when actual communication with the proxy
 * should be avoided (for example during server startup).
 */
void Zone::updateProxyStatus(Node *node, bool activeMode)
{
   lockProperties();
   for(int i = 0; i < m_proxyNodes->size(); i++)
   {
      ZoneProxy *p = m_proxyNodes->get(i);
      if (p->nodeId == node->getId())
      {
         bool isAvailable = node->isNativeAgent() &&
                  ((node->getState() & NSF_AGENT_UNREACHABLE) == 0) &&
                  ((node->getState() & DCSF_UNREACHABLE) == 0) &&
                  (node->getStatus() != STATUS_UNMANAGED) &&
                  ((node->getFlags() & NF_DISABLE_NXCP) == 0);
         if (isAvailable != p->isAvailable)
         {
            nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 4, _T("Zone %s [uin=%u] proxy %s [%u] availability changed to %s"),
                     m_name, m_uin, node->getName(), node->getId(), isAvailable ? _T("YES") : _T("NO"));
            p->isAvailable = isAvailable;
         }
         if (isAvailable && activeMode)
         {
            p->loadAverage = node->getMetricFromAgentAsDouble(_T("System.CPU.LoadAvg15"), p->loadAverage);
         }
         break;
      }
   }
   unlockProperties();
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
void Zone::dumpInterfaceIndex(ServerConsole *console)
{
   DumpIndex(console, m_idxInterfaceByAddr);
}

/**
 * Dump node index to console
 */
void Zone::dumpNodeIndex(ServerConsole *console)
{
   DumpIndex(console, m_idxNodeByAddr);
}

/**
 * Dump subnet index to console
 */
void Zone::dumpSubnetIndex(ServerConsole *console)
{
   DumpIndex(console, m_idxSubnetByAddr);
}

/**
 * Dump internal state to console
 */
void Zone::dumpState(ServerConsole *console)
{
   lockProperties();
   if (!m_proxyNodes->isEmpty())
   {
      console->print(_T("   Proxies:\n"));
      for(int i = 0; i < m_proxyNodes->size(); i++)
      {
         ZoneProxy *p = m_proxyNodes->get(i);
         console->printf(_T("      [\x1b[33;1m%7u\x1b[0m] assignments=%u available=\x1b[%s\x1b[0m load=%f\n"), p->nodeId, p->assignments,
                  p->isAvailable ? _T("32;1myes") : _T("31;1mno"), p->loadAverage);
      }
   }
   else
   {
      console->print(_T("   No proxy nodes defined\n"));
   }
   unlockProperties();
}

/**
 * Serialize object to JSON
 */
json_t *Zone::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "uin", json_integer(m_uin));
   json_object_set_new(root, "proxyNodeId", json_object_array(m_proxyNodes));
   return root;
}
