/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
Zone::Zone() : super(), Pollable(this, Pollable::STATUS)
{
   m_id = 0;
   m_uin = 0;
   _tcscpy(m_name, _T("Default"));
   GenerateRandomBytes(m_proxyAuthKey, ZONE_PROXY_KEY_LENGTH);
	m_idxNodeByAddr = new InetAddressIndex;
	m_idxInterfaceByAddr = new InetAddressIndex;
	m_idxSubnetByAddr = new InetAddressIndex;
   m_lastHealthCheck = TIMESTAMP_NEVER;
   m_lockedForHealthCheck = false;
}

/**
 * Constructor for new zone object
 */
Zone::Zone(int32_t uin, const TCHAR *name) : super(), Pollable(this, Pollable::STATUS)
{
   m_id = 0;
   m_uin = uin;
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   GenerateRandomBytes(m_proxyAuthKey, ZONE_PROXY_KEY_LENGTH);
	m_idxNodeByAddr = new InetAddressIndex;
	m_idxInterfaceByAddr = new InetAddressIndex;
	m_idxSubnetByAddr = new InetAddressIndex;
   m_lastHealthCheck = TIMESTAMP_NEVER;
   m_lockedForHealthCheck = false;
   setCreationTime();
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
bool Zone::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT zone_guid FROM zones WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   bool success = false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) == 0)
      {
         if (id == BUILTIN_OID_ZONE0)
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
         success = true;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   if (success)
   {
      success = false;
      hStmt = DBPrepare(hdb, _T("SELECT proxy_node FROM zone_proxies WHERE object_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         { 
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               auto p = make_shared<ZoneProxy>(DBGetFieldULong(hResult, i, 0));
               m_proxyNodes.put(p->nodeId, p);
            }
            DBFreeResult(hResult);
            success = true;
         }
         DBFreeStatement(hStmt);
      }
   }

   // Load access list
   if (success)
      success = loadACLFromDB(hdb, preparedStatements);

   return success;
}

/**
 * Save object to database
 */
bool Zone::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("zones"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE zones SET zone_guid=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO zones (zone_guid,id) VALUES (?,?)"));
      }
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_uin);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
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
      IntegerArray<uint64_t> proxyNodes = m_proxyNodes.keys();
      if (!proxyNodes.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO zone_proxies (object_id,proxy_node) VALUES (?,?)"), proxyNodes.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for (int i = 0; i < proxyNodes.size() && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(proxyNodes.get(i)));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
   }

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
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM shared_secrets WHERE zone=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM snmp_communities WHERE zone=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM usm_credentials WHERE zone=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM well_known_ports WHERE zone=?"));
   return success;
}

/**
 * Handler for object deletion notification
 */
void Zone::onObjectDelete(const NetObj& object)
{
   uint32_t objectId = object.getId();
   lockProperties();
   IntegerArray<uint64_t> proxyKeys = m_proxyNodes.keys();
   for (int i = 0; i < proxyKeys.size(); i++)
   {
      uint32_t proxyId = static_cast<uint32_t>(proxyKeys.get(i));
      if (proxyId == objectId)
      {
         m_proxyNodes.remove(proxyId);
         setModified(MODIFY_OTHER);
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Zone::onObjectDelete(%s [%u]): zone proxy node %s [%u] deleted"), m_name, m_id, object.getName(), objectId);
         break;
      }
   }
   unlockProperties();
   super::onObjectDelete(object);
}

/**
 * Create NXCP message with object's data
 */
void Zone::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_ZONE_UIN, m_uin);
}

/**
 * Create NXCP message with object's data (stage 2)
 */
void Zone::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   msg->setFieldFromInt32Array(VID_ZONE_PROXY_LIST, getAllProxyNodes());
}

/**
 * Modify object from message
 */
uint32_t Zone::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Modify object from message (stage 2)
 */
uint32_t Zone::modifyFromMessageInternalStage2(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_ZONE_PROXY_LIST))
   {
      IntegerArray<uint32_t> newProxyList;
      msg.getFieldAsInt32Array(VID_ZONE_PROXY_LIST, &newProxyList);
      for (int i = 0; i < newProxyList.size(); i++)
      {
         if (!m_proxyNodes.contains(newProxyList.get(i)))
         {
            auto p = make_shared<ZoneProxy>(newProxyList.get(i));
            m_proxyNodes.put(p->nodeId, p);
         }
      }

      IntegerArray<uint64_t> oldProxyList = m_proxyNodes.keys();
      for (int i = 0; i < oldProxyList.size(); i++)
      {
         uint32_t proxyId = static_cast<uint32_t>(oldProxyList.get(i));

         int j;
         for(j = 0; j < newProxyList.size(); j++)
         {
            if (proxyId == newProxyList.get(j))
               break;
         }
         if (j == newProxyList.size())
            m_proxyNodes.remove(proxyId);
      }
   }
   return super::modifyFromMessageInternalStage2(msg, session);
}

/**
 * Add node as zone proxy
 */
void Zone::addProxy(const Node& node)
{
   uint32_t nodeId = node.getId();
   if (!m_proxyNodes.contains(nodeId))
   {
      auto p = make_shared<ZoneProxy>(nodeId);
      m_proxyNodes.put(p->nodeId, p);
      setModified(MODIFY_OTHER);
   }
}

/**
 * Update interface index
 */
void Zone::updateInterfaceIndex(const InetAddress& oldIp, const InetAddress& newIp, const shared_ptr<Interface>& iface)
{
   m_idxInterfaceByAddr->remove(oldIp);
   m_idxInterfaceByAddr->put(newIp, iface);
}

/**
 * Update node index
 */
void Zone::updateNodeIndex(const InetAddress& oldIp, const InetAddress& newIp, const shared_ptr<Node>& node)
{
   m_idxNodeByAddr->remove(oldIp);
   m_idxNodeByAddr->put(newIp, node);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Zone::showThresholdSummary() const
{
	return true;
}

/**
 * Remove interface from index
 */
void Zone::removeFromIndex(const Interface& iface)
{
   const ObjectArray<InetAddress>& list = iface.getIpAddressList()->getList();
   for(int i = 0; i < list.size(); i++)
   {
      InetAddress *addr = list.get(i);
      if (addr->isValidUnicast())
      {
	      shared_ptr<NetObj> o = m_idxInterfaceByAddr->get(*addr);
	      if ((o != nullptr) && (o->getId() == iface.getId()))
	      {
		      m_idxInterfaceByAddr->remove(*addr);
	      }
      }
   }
}

/**
 * Callback for processing zone configuration synchronization
 */
static EnumerationCallbackResult ForceConfigurationSync(const uint32_t *nodeId, void *arg)
{
   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(*nodeId, OBJECT_NODE));
   if (node != nullptr)
      node->scheduleDataCollectionSyncWithAgent();
   return _CONTINUE;
}

/**
 * Context for zone proxy comparator
 */
struct ZoneProxyComparatorContext
{
   NetObj* object;
   HashSet<uint32_t>* syncSet;
   bool backup;
};

/**
 * Zone proxy comparator for finding proxy assigned to given node
 */
static bool ZoneProxyComparator(ZoneProxy *p, ZoneProxyComparatorContext* context)
{
   if (p->nodeId == context->object->getAssignedZoneProxyId(context->backup))
   {
      if (p->isAvailable)
      {
         return true;
      }
      else
      {
         InterlockedDecrement(&p->assignments);
         context->syncSet->put(p->nodeId);

         uint32_t otherProxy = context->object->getAssignedZoneProxyId(!context->backup);
         if (otherProxy != 0)
            context->syncSet->put(otherProxy);
      }
   }
   return false;
}

/**
 * Get proxy node for given object. Always prefers proxy that is already assigned to the object
 * and will update assigned proxy property if changed.
 */
uint32_t Zone::getProxyNodeId(NetObj *object, bool backup)
{
   shared_ptr<ZoneProxy> proxy;
   HashSet<uint32_t> syncSet;

   if ((object != nullptr) && (object->getAssignedZoneProxyId(backup) != 0))
   {
      ZoneProxyComparatorContext context;
      context.object = object;
      context.syncSet = &syncSet;
      context.backup = backup;
      proxy = m_proxyNodes.find(ZoneProxyComparator, &context);
   }

   if (proxy == nullptr)
   {
      unique_ptr<SharedObjectArray<ZoneProxy>> proxyNodes = m_proxyNodes.getAll();
      if (proxyNodes->size() > (backup ? 1 : 0))
      {
         if (!backup)
            proxy = proxyNodes->getShared(0);
         for(int i = 0; i < proxyNodes->size(); i++)
         {
            shared_ptr<ZoneProxy> p = proxyNodes->getShared(i);
            if (!p->isAvailable)
               continue;

            if ((object != nullptr) && (p->nodeId == object->getAssignedZoneProxyId(!backup)))
               continue;

            if ((proxy == nullptr) || (p->compareLoad(proxy.get()) < 0) || !proxy->isAvailable)
               proxy = p;
         }
      }
      if (object != nullptr)
      {
         if (proxy != nullptr)
         {
            object->setAssignedZoneProxyId(proxy->nodeId, backup);
            InterlockedIncrement(&proxy->assignments);
            syncSet.put(proxy->nodeId);

            uint32_t otherProxy = object->getAssignedZoneProxyId(!backup);
            if (otherProxy != 0)
               syncSet.put(otherProxy);
         }
         else
         {
            object->setAssignedZoneProxyId(0, backup);
         }
      }
   }

   uint32_t id = (proxy != nullptr) ? proxy->nodeId : 0;
   nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 8, _T("Zone::getProxyNodeId: selected %s proxy [%u] for object %s [%u] in zone %s [uin=%u]"),
            backup ? _T("backup") : _T("primary"),
            id, (object != nullptr) ? object->getName() : _T("(null)"), (object != nullptr) ? object->getId() : 0,
            m_name, m_uin);

   syncSet.forEach(ForceConfigurationSync, nullptr);
   return id;
}

/**
 * Get available proxy for given object (will use backup proxy if primary is not available)
 */
uint32_t Zone::getAvailableProxyNodeId(NetObj *object)
{
   if (object == nullptr)
      return 0;

   uint32_t assignedProxyId = object->getAssignedZoneProxyId(false);
   if (assignedProxyId == 0)
      return getProxyNodeId(object, false);  // No proxy assigned yet

   if (isProxyNodeAvailable(assignedProxyId))
      return assignedProxyId;

   uint32_t backupProxyId = getProxyNodeId(object, true);
   // If backup not available, still return primary proxy ID for better event correlation
   return (backupProxyId != 0) ? backupProxyId : assignedProxyId;
}

/**
 * Get number of assignments for given proxy node
 */
uint32_t Zone::getProxyNodeAssignments(uint32_t nodeId) const
{
   shared_ptr<ZoneProxy> p = m_proxyNodes.get(nodeId);
   return (p != nullptr) ? p->assignments : 0;
}

/**
 * Check if given proxy node is available.
 */
bool Zone::isProxyNodeAvailable(uint32_t nodeId) const
{
   shared_ptr<ZoneProxy> p = m_proxyNodes.get(nodeId);
   return (p != nullptr) ? p->isAvailable : false;
}

/**
 * Get all proxy nodes. Returned array must be destroyed by the caller.
 */
IntegerArray<uint32_t> Zone::getAllProxyNodes() const
{
   IntegerArray<uint64_t> keys = m_proxyNodes.keys();
   IntegerArray<uint32_t> proxyNodes(keys.size());
   for(int i = 0; i < keys.size(); i++)
      proxyNodes.add(static_cast<uint32_t>(keys.get(i)));
   return proxyNodes;
}

/**
 * Fill configuration message for agent
 */
void Zone::fillAgentConfigurationMessage(NXCPMessage *msg) const
{
   lockProperties();
   msg->setField(VID_ZONE_UIN, m_uin);
   msg->setField(VID_SHARED_SECRET, m_proxyAuthKey, ZONE_PROXY_KEY_LENGTH);
   unlockProperties();

   unique_ptr<SharedObjectArray<ZoneProxy>> proxyNodes = m_proxyNodes.getAll();
   uint32_t fieldId = VID_ZONE_PROXY_BASE, count = 0;
   for(int i = 0; i < proxyNodes->size(); i++)
   {
      ZoneProxy *p = proxyNodes->get(i);
      shared_ptr<NetObj> node = FindObjectById(p->nodeId, OBJECT_NODE);
      if (node != nullptr)
      {
         msg->setField(fieldId++, p->nodeId);
         msg->setField(fieldId++, static_cast<Node&>(*node).getIpAddress());
         fieldId += 8;
         count++;
      }
   }
   msg->setField(VID_ZONE_PROXY_COUNT, count);
}

/**
 * Acquire connection to any available proxy node
 */
shared_ptr<AgentConnectionEx> Zone::acquireConnectionToProxy(bool validate)
{
   uint32_t nodeId = getProxyNodeId(nullptr);
   if (nodeId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 7, _T("Zone::acquireConnectionToProxy: no active proxy in zone %s [uin=%u]"), m_name, m_uin);
      return shared_ptr<AgentConnectionEx>();
   }

   shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
   return (node != nullptr) ? static_cast<Node&>(*node).acquireProxyConnection(ZONE_PROXY, validate) : shared_ptr<AgentConnectionEx>();
}

/**
 * Update proxy status. Passive mode should be used when actual communication with the proxy
 * should be avoided (for example during server startup).
 */
void Zone::updateProxyStatus(const shared_ptr<Node>& node, bool activeMode)
{
   shared_ptr<ZoneProxy> p = m_proxyNodes.get(node->getId());
   if (p != nullptr)
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
         // Update should be executed on separate thread to avoid server deadlock
         ThreadPoolExecute(g_mainThreadPool, self(), &Zone::updateProxyLoadData, node);
      }
   }
}

/**
 * Update proxy load information
 */
void Zone::updateProxyLoadData(shared_ptr<Node> node)
{
   double cpuLoad = node->getMetricFromAgentAsDouble(_T("System.CPU.LoadAvg15"), -1);
   double dataCollectorLoad = node->getMetricFromAgentAsDouble(_T("Agent.ThreadPool.LoadAverage15(DATACOLL)"), -1);
   int64_t dataSenderQueueSize = node->getMetricFromAgentAsInt32(_T("Agent.DataCollectorQueueSize"), -1);  // FIXME: rename to Agent.DataSenderQueueSize
   if ((cpuLoad >= 0) || (dataCollectorLoad >= 0) || (dataSenderQueueSize >= 0))
   {
      shared_ptr<ZoneProxy> p = m_proxyNodes.get(node->getId());
      if (p != nullptr)
      {
         if (cpuLoad >= 0)
            p->cpuLoad = cpuLoad;
         if (dataCollectorLoad >= 0)
            p->dataCollectorLoad = dataCollectorLoad;
         if (dataSenderQueueSize >= 0)
         {
            UpdateExpMovingAverage(p->rawDataSenderLoad, EMA_EXP_15, dataSenderQueueSize);
            double load = GetExpMovingAverageValue(p->rawDataSenderLoad);
            p->dataSenderLoadTrend = load - p->dataSenderLoad;
            p->dataSenderLoad = load;
         }
      }
   }
}

/**
 * Filter nodes by assigned proxy ID
 */
static bool ProxyFilter(NetObj *object, uint32_t *proxyId)
{
   return object->getAssignedZoneProxyId(false) == *proxyId;
}

/**
 * Compare nodes by estimated proxy load
 */
static int CompareNodesByProxyLoad(const NetObj& n1, const NetObj& n2)
{
   double d = static_cast<const Node&>(n1).getProxyLoadFactor() - static_cast<const Node&>(n2).getProxyLoadFactor();
   return (d < 0) ? -1 : (d > 0) ? 1 : 0;
}

/**
 * Migrate proxy load
 */
void Zone::migrateProxyLoad(ZoneProxy *source, ZoneProxy *target)
{
   unique_ptr<SharedObjectArray<NetObj>> nodes = g_idxNodeById.getObjects(ProxyFilter, &source->nodeId);
   nodes->sort(CompareNodesByProxyLoad);

   double loadFactor = 0;
   for(int i = 0; i < nodes->size(); i++)
      loadFactor += static_cast<Node*>(nodes->get(i))->getProxyLoadFactor();
   double targetLoadFactor = loadFactor * 0.75;

   nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): %d nodes on proxy [%u], load factor %f, target %f"),
            m_name, m_uin, nodes->size(), source->nodeId, loadFactor, targetLoadFactor);

   for(int i = 0; (i < nodes->size()) && (loadFactor > targetLoadFactor); i++)
   {
      Node *n = static_cast<Node*>(nodes->get(i));
      nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): moving node %s [%u] from [%u] to [%u]"),
               m_name, m_uin, n->getName(), n->getId(), source->nodeId, target->nodeId);
      InterlockedDecrement(&source->assignments);
      InterlockedIncrement(&target->assignments);
      n->setAssignedZoneProxyId(target->nodeId, false);
      if (n->getAssignedZoneProxyId(true) == target->nodeId)
      {
         nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): backup proxy for node node %s [%u] should be changed"),
                  m_name, m_uin, n->getName(), n->getId());
         n->setAssignedZoneProxyId(0, true);
         InterlockedDecrement(&target->assignments);
         shared_ptr<NetObj> node = nodes->getShared(i);
         ThreadPoolExecute(g_mainThreadPool,
            [node] () -> void
            {
               // The following call will force update of backup proxy for node
               static_cast<Node&>(*node).getEffectiveSnmpProxy(ProxySelection::BACKUP);
            });
      }
      loadFactor -= n->getProxyLoadFactor();
   }

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(target->nodeId, OBJECT_NODE));
   if (node != nullptr)
      node->scheduleDataCollectionSyncWithAgent();

   node = static_pointer_cast<Node>(FindObjectById(source->nodeId, OBJECT_NODE));
   if (node != nullptr)
      node->scheduleDataCollectionSyncWithAgent();
}

/**
 * Lock zone for zone health poll
 */
bool Zone::lockForStatusPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated)
   {
      if ((m_status != STATUS_UNMANAGED) &&
          !m_lockedForHealthCheck &&
          (static_cast<uint32_t>(time(nullptr) - m_lastHealthCheck) >= getCustomAttributeAsUInt32(_T("SysConfig:Objects.StatusPollingInterval"), g_statusPollingInterval)))
      {
         m_lockedForHealthCheck = true;
         success = true;
      }
   }
   unlockProperties();
   return success;
}

/**
 * Do zone health check
 */
void Zone::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): started"), m_name, m_uin);

   unique_ptr<SharedObjectArray<ZoneProxy>> proxyNodes = m_proxyNodes.getAll();

   // Calculate average load
   int count = 0;
   double dataSenderLoad = 0, dataCollectorLoad = 0, cpuLoad = 0;
   uint32_t assignments = 0;
   for(int i = 0; i < proxyNodes->size(); i++)
   {
      ZoneProxy *p = proxyNodes->get(i);
      if (!p->isAvailable)
         continue;

      dataSenderLoad += p->dataSenderLoad;
      dataCollectorLoad += p->dataCollectorLoad;
      cpuLoad += p->cpuLoad;
      assignments += p->assignments;
      count++;
   }

   // Check if load balancing is needed
   if (count > 1)
   {
      dataSenderLoad /= count;
      dataCollectorLoad /= count;
      cpuLoad /= count;
      assignments /= count;
      nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): %d active proxies, average values: %f/%f/%f"),
               m_name, m_uin, count, dataSenderLoad, dataCollectorLoad, cpuLoad);

      time_t now = time(nullptr);
      ObjectArray<ZoneProxy> sources(count);  // potential sources for removing load
      ObjectArray<ZoneProxy> targets(count);  // potential sources for adding load
      for(int i = 0; i < proxyNodes->size(); i++)
      {
         ZoneProxy *p = proxyNodes->get(i);
         if (!p->isAvailable)
            continue;

         if ((p->dataSenderLoad <= dataSenderLoad) &&
             (p->dataCollectorLoad <= dataCollectorLoad) &&
             (p->cpuLoad <= cpuLoad) &&
             (static_cast<uint32_t>(p->assignments) <= assignments) &&
             (now - p->loadBalanceTimestamp >= 420))  // was not re-balanced within last 7 minutes
         {
            targets.add(p);
         }
         else if (((p->dataSenderLoad > dataSenderLoad) ||
                   (p->dataSenderLoadTrend > 0) ||
                   (p->dataCollectorLoad > dataCollectorLoad) ||
                   ((p->cpuLoad > cpuLoad) && (p->cpuLoad > 1)) ||
                   (static_cast<uint32_t>(p->assignments) > assignments * 2)) &&
                  (now - p->loadBalanceTimestamp >= 420))  // was not re-balanced within last 7 minutes
         {
            sources.add(p);
         }
      }

      if (!sources.isEmpty() && !targets.isEmpty())
      {
         int j = 0;
         for(int i = 0; i < sources.size(); i++)
         {
            ZoneProxy *s = sources.get(i);
            ZoneProxy *t = targets.get(j);
            j++;
            if (j == targets.size())
               j = 0;

            nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): moving load: [%u] -> [%u]"), m_name, m_uin, s->nodeId, t->nodeId);
            migrateProxyLoad(s, t);
            s->loadBalanceTimestamp = now;
            t->loadBalanceTimestamp = now;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): proxy load balancing is not %s"),
                  m_name, m_uin, sources.isEmpty() ? _T("needed") : _T("possible"));
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): proxy load balancing is not needed"), m_name, m_uin);
   }

   lockProperties();
   m_lastHealthCheck = time(nullptr);
   m_lockedForHealthCheck = false;
   unlockProperties();

   nxlog_debug_tag(DEBUG_TAG_ZONE_PROXY, 6, _T("ZoneHealthCheck(%s [%u]): completed"), m_name, m_uin);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Zone::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslZoneClass, new shared_ptr<Zone>(self())));
}

/**
 * Dump interface index to console
 */
void Zone::dumpInterfaceIndex(ServerConsole *console) const
{
   DumpIndex(console, m_idxInterfaceByAddr);
}

/**
 * Dump node index to console
 */
void Zone::dumpNodeIndex(ServerConsole *console) const
{
   DumpIndex(console, m_idxNodeByAddr);
}

/**
 * Dump subnet index to console
 */
void Zone::dumpSubnetIndex(ServerConsole *console) const
{
   DumpIndex(console, m_idxSubnetByAddr);
}

/**
 * Print proxy node info to server console
 */
static EnumerationCallbackResult PrintProxyNodeInfo(ZoneProxy *p, ServerConsole *console)
{
   console->printf(_T("      [\x1b[33;1m%7u\x1b[0m] assignments=%u available=\x1b[%s\x1b[0m senderLoad=%f(%f) dcLoad=%f cpuLoad=%f\n"),
            p->nodeId, p->assignments, p->isAvailable ? _T("32;1myes") : _T("31;1mno"),
            p->dataSenderLoad, p->dataSenderLoadTrend, p->dataCollectorLoad, p->cpuLoad);
   return _CONTINUE;
}

/**
 * Dump internal state to console
 */
void Zone::dumpState(ServerConsole *console) const
{
   if (m_proxyNodes.size() > 0)
   {
      console->print(_T("   Proxies:\n"));
      m_proxyNodes.forEach(PrintProxyNodeInfo, console);
   }
   else
   {
      console->print(_T("   No proxy nodes defined\n"));
   }
}

/**
 * Serialize object to JSON
 */
json_t *Zone::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "uin", json_integer(m_uin));
   unlockProperties();

   json_object_set_new(root, "proxyNodeId", json_object_array(*m_proxyNodes.getAll()));

   return root;
}
