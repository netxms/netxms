/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: traffic_hosts.cpp
** Observation-point host record index, host matching pass, and host-level
** data serving for the traffic observer subsystem.
**/

#include "nxcore.h"
#include <traffic-connector.h>

#define DEBUG_TAG_TRAFFIC_POLL   L"traffic.poll"

/**
 * Host record index: key "pointId:hostKey" -> record. Owns records.
 */
static StringObjectMap<ObservationPointHostRecord> s_hostRecords(Ownership::True);
static HashSet<uint32_t> s_observedNodes;
static RWLock s_hostRecordsLock;

/**
 * Build index key for a host record
 */
static inline void BuildRecordKey(uint32_t pointId, const char *hostKey, wchar_t *buffer, size_t size)
{
   nx_swprintf(buffer, size, L"%u:%hs", pointId, hostKey);
}

/**
 * Rebuild the observed-node set from the record map (write lock must be held)
 */
static void RebuildObservedNodesLocked()
{
   s_observedNodes.clear();
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
      s_observedNodes.put(p->value->nodeId);
}

/**
 * Remove all in-memory records belonging to a point (write lock must be held)
 */
static void RemovePointRecordsLocked(uint32_t pointId)
{
   StringList keysToRemove;
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
   {
      if (p->value->pointId == pointId)
         keysToRemove.add(p->key);
   }
   for (int i = 0; i < keysToRemove.size(); i++)
      s_hostRecords.remove(keysToRemove.get(i));
}

/**
 * Push object update for nodes whose observation point set changed (the node
 * message carries the list of observing points)
 */
static void NotifyNodesOnObservationChange(const HashSet<uint32_t>& nodes)
{
   for (const uint32_t *nodeId : nodes)
   {
      shared_ptr<NetObj> node = FindObjectById(*nodeId, OBJECT_NODE);
      if (node != nullptr)
         node->markAsModified(MODIFY_RUNTIME);
   }
}

/**
 * Load host records from the database at startup
 */
void LoadObservationPointHosts()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, L"SELECT point_id,host_key,ip_addr,vlan,node_id,first_seen,last_seen FROM observation_point_hosts");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      s_hostRecordsLock.writeLock();
      for (int i = 0; i < count; i++)
      {
         auto r = new ObservationPointHostRecord();
         r->pointId = DBGetFieldULong(hResult, i, 0);
         DBGetFieldUTF8(hResult, i, 1, r->hostKey, sizeof(r->hostKey));
         wchar_t ipText[64];
         DBGetField(hResult, i, 2, ipText, 64);
         r->ipAddr = InetAddress::parse(ipText);
         r->vlan = DBGetFieldLong(hResult, i, 3);
         r->nodeId = DBGetFieldULong(hResult, i, 4);
         r->firstSeen = static_cast<time_t>(DBGetFieldULong(hResult, i, 5));
         r->lastSeen = static_cast<time_t>(DBGetFieldULong(hResult, i, 6));

         wchar_t key[192];
         BuildRecordKey(r->pointId, r->hostKey, key, 192);
         s_hostRecords.set(key, r);
      }
      RebuildObservedNodesLocked();
      s_hostRecordsLock.unlock();
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 3, L"Loaded %d observation point host record(s)", count);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Find a node observing given IP across all zones, applying the strict collision
 * rule: if the IP resolves to nodes in more than one zone, no node is returned
 * and *collision is incremented.
 */
static shared_ptr<Node> FindObservedNodeAllZones(const InetAddress& ipAddr, int *collisions)
{
   unique_ptr<SharedObjectArray<NetObj>> zones = g_idxZoneByUIN.getObjects();
   shared_ptr<Node> match;
   for (int i = 0; i < zones->size(); i++)
   {
      Zone *zone = static_cast<Zone*>(zones->get(i));
      shared_ptr<Node> node = FindNodeByIP(zone->getUIN(), ipAddr);
      if (node == nullptr)
         continue;
      if ((match != nullptr) && (match->getId() != node->getId()))
      {
         (*collisions)++;
         return shared_ptr<Node>();
      }
      match = node;
   }
   return match;
}

/**
 * Persist and index the matched host set for one point. Takes the matched
 * records (non-owning array) and transfers record ownership to the index.
 */
static void UpdatePointHostRecords(uint32_t pointId, ObjectArray<ObservationPointHostRecord> *matches)
{
   // Persist to database (full replace for this point)
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DBBegin(hdb);

   DB_STATEMENT hDelete = DBPrepare(hdb, L"DELETE FROM observation_point_hosts WHERE point_id=?");
   if (hDelete != nullptr)
   {
      DBBind(hDelete, 1, DB_SQLTYPE_INTEGER, pointId);
      DBExecute(hDelete);
      DBFreeStatement(hDelete);
   }

   if (matches->size() > 0)
   {
      DB_STATEMENT hInsert = DBPrepare(hdb,
         L"INSERT INTO observation_point_hosts (point_id,host_key,ip_addr,vlan,node_id,first_seen,last_seen) VALUES (?,?,?,?,?,?,?)", true);
      if (hInsert != nullptr)
      {
         for (int i = 0; i < matches->size(); i++)
         {
            ObservationPointHostRecord *r = matches->get(i);
            wchar_t ipText[64];
            DBBind(hInsert, 1, DB_SQLTYPE_INTEGER, r->pointId);
            DBBind(hInsert, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, r->hostKey, DB_BIND_STATIC);
            DBBind(hInsert, 3, DB_SQLTYPE_VARCHAR, r->ipAddr.toString(ipText), DB_BIND_STATIC);
            DBBind(hInsert, 4, DB_SQLTYPE_INTEGER, r->vlan);
            DBBind(hInsert, 5, DB_SQLTYPE_INTEGER, r->nodeId);
            DBBind(hInsert, 6, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(r->firstSeen));
            DBBind(hInsert, 7, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(r->lastSeen));
            DBExecute(hInsert);
         }
         DBFreeStatement(hInsert);
      }
   }

   DBCommit(hdb);
   DBConnectionPoolReleaseConnection(hdb);

   // Transfer records into the in-memory index; track nodes whose observation
   // set changes so their object messages (observation point list) get refreshed
   HashSet<uint32_t> oldNodes, newNodes;
   s_hostRecordsLock.writeLock();
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
   {
      if (p->value->pointId == pointId)
         oldNodes.put(p->value->nodeId);
   }
   RemovePointRecordsLocked(pointId);
   for (int i = 0; i < matches->size(); i++)
   {
      ObservationPointHostRecord *r = matches->get(i);
      wchar_t key[192];
      BuildRecordKey(r->pointId, r->hostKey, key, 192);
      newNodes.put(r->nodeId);
      s_hostRecords.set(key, r);   // index takes ownership
   }
   RebuildObservedNodesLocked();
   s_hostRecordsLock.unlock();

   HashSet<uint32_t> affectedNodes;
   for (const uint32_t *nodeId : oldNodes)
      if (!newNodes.contains(*nodeId))
         affectedNodes.put(*nodeId);
   for (const uint32_t *nodeId : newNodes)
      if (!oldNodes.contains(*nodeId))
         affectedNodes.put(*nodeId);
   NotifyNodesOnObservationChange(affectedNodes);
}

/**
 * Run the host matching pass for a single in-scope observation point
 */
void RunObservationPointHostMatching(ObservationPoint *point)
{
   shared_ptr<TrafficObserver> observer = point->getOwner();
   if (observer == nullptr)
      return;

   TrafficConnectorInterface *connector = FindTrafficConnector(observer->getConnectorName());
   if ((connector == nullptr) || (connector->GetActiveHosts == nullptr))
      return;

   std::string externalId = point->getExternalId();
   json_t *credentials = observer->getCredentials();
   ObjectArray<TrafficHostEntry> *hosts = connector->GetActiveHosts(externalId.c_str(), credentials);
   json_decref(credentials);
   if (hosts == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Host matching for observation point [%u]: active host list unavailable", point->getId());
      return;
   }

   int32_t zoneUIN = point->getEffectiveZoneUIN();
   bool allZones = (zoneUIN < 0);
   uint32_t pointId = point->getId();

   ObjectArray<ObservationPointHostRecord> matches(hosts->size(), 64, Ownership::False);
   int collisions = 0;
   for (int i = 0; i < hosts->size(); i++)
   {
      TrafficHostEntry *h = hosts->get(i);
      if ((h->flags & (TRAFFIC_HOST_MULTICAST | TRAFFIC_HOST_BROADCAST)) != 0)
         continue;
      if (!h->ipAddr.isValidUnicast())
         continue;

      shared_ptr<Node> node = allZones ? FindObservedNodeAllZones(h->ipAddr, &collisions) : FindNodeByIP(zoneUIN, h->ipAddr);
      if (node == nullptr)
         continue;

      auto r = new ObservationPointHostRecord();
      r->pointId = pointId;
      strlcpy(r->hostKey, h->hostKey, sizeof(r->hostKey));
      r->ipAddr = h->ipAddr;
      r->vlan = h->vlan;
      r->nodeId = node->getId();
      r->firstSeen = (h->firstSeen != 0) ? h->firstSeen : time(nullptr);
      r->lastSeen = (h->lastSeen != 0) ? h->lastSeen : time(nullptr);
      matches.add(r);
   }
   delete hosts;

   UpdatePointHostRecords(pointId, &matches);
   nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 5, L"Host matching for observation point [%u]: %d host record(s), %d cross-zone collision(s) skipped",
      pointId, matches.size(), collisions);
}

/**
 * Drop all in-memory host records for a point (called on point deletion; the
 * database rows are removed by ObservationPoint::deleteFromDatabase)
 */
void DropObservationPointHosts(uint32_t pointId)
{
   HashSet<uint32_t> droppedNodes;
   s_hostRecordsLock.writeLock();
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
   {
      if (p->value->pointId == pointId)
         droppedNodes.put(p->value->nodeId);
   }
   RemovePointRecordsLocked(pointId);
   RebuildObservedNodesLocked();
   s_hostRecordsLock.unlock();

   NotifyNodesOnObservationChange(droppedNodes);
}

/**
 * Age out host records not seen within the retention period (housekeeper task)
 */
void AgeObservationPointHosts(DB_HANDLE hdb, time_t cycleStartTime)
{
   int32_t retentionDays = ConfigReadInt(L"TrafficObserver.HostRetentionTime", 7);
   if (retentionDays <= 0)
      return;
   time_t cutoff = cycleStartTime - static_cast<time_t>(retentionDays) * 86400;

   wchar_t query[256];
   nx_swprintf(query, 256, L"DELETE FROM observation_point_hosts WHERE last_seen<%u", static_cast<uint32_t>(cutoff));
   DBQuery(hdb, query);

   HashSet<uint32_t> agedNodes;
   s_hostRecordsLock.writeLock();
   StringList keysToRemove;
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
   {
      if (p->value->lastSeen < cutoff)
      {
         keysToRemove.add(p->key);
         agedNodes.put(p->value->nodeId);
      }
   }
   for (int i = 0; i < keysToRemove.size(); i++)
      s_hostRecords.remove(keysToRemove.get(i));
   if (keysToRemove.size() > 0)
      RebuildObservedNodesLocked();
   s_hostRecordsLock.unlock();

   if (keysToRemove.size() > 0)
   {
      NotifyNodesOnObservationChange(agedNodes);
      nxlog_debug_tag(DEBUG_TAG_TRAFFIC_POLL, 4, L"Aged out %d observation point host record(s)", keysToRemove.size());
   }
}

/**
 * Get a copy of all host records observing given node
 */
unique_ptr<ObjectArray<ObservationPointHostRecord>> GetObservationPointHostsForNode(uint32_t nodeId)
{
   auto result = make_unique<ObjectArray<ObservationPointHostRecord>>(16, 16, Ownership::True);
   s_hostRecordsLock.readLock();
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
   {
      if (p->value->nodeId == nodeId)
         result->add(new ObservationPointHostRecord(*p->value));
   }
   s_hostRecordsLock.unlock();
   return result;
}

/**
 * Get a copy of all matched host records (records with an associated node) of given observation point
 */
unique_ptr<ObjectArray<ObservationPointHostRecord>> GetObservationPointMatchedHosts(uint32_t pointId)
{
   auto result = make_unique<ObjectArray<ObservationPointHostRecord>>(16, 16, Ownership::True);
   s_hostRecordsLock.readLock();
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
   {
      if ((p->value->pointId == pointId) && (p->value->nodeId != 0))
         result->add(new ObservationPointHostRecord(*p->value));
   }
   s_hostRecordsLock.unlock();
   return result;
}

/**
 * Get IDs of observation points that observe given node (deduplicated)
 */
void GetObservationPointIdsForNode(uint32_t nodeId, IntegerArray<uint32_t> *pointIds)
{
   s_hostRecordsLock.readLock();
   for (KeyValuePair<ObservationPointHostRecord> *p : s_hostRecords)
   {
      if ((p->value->nodeId == nodeId) && !pointIds->contains(p->value->pointId))
         pointIds->add(p->value->pointId);
   }
   s_hostRecordsLock.unlock();
}

/**
 * Check whether given node is observed by any observation point
 */
bool IsNodeObserved(uint32_t nodeId)
{
   s_hostRecordsLock.readLock();
   bool observed = s_observedNodes.contains(nodeId);
   s_hostRecordsLock.unlock();
   return observed;
}

/**
 * Build live active-host table for an observation point (CMD_QUERY_TRAFFIC_DATA).
 * Rows are annotated with the matched node where a host record exists.
 */
uint32_t GetObservationPointActiveHosts(ObservationPoint *point, shared_ptr<Table> *result)
{
   shared_ptr<TrafficObserver> observer = point->getOwner();
   if (observer == nullptr)
      return RCC_INCOMPATIBLE_OPERATION;

   TrafficConnectorInterface *connector = FindTrafficConnector(observer->getConnectorName());
   if ((connector == nullptr) || (connector->GetActiveHosts == nullptr))
      return RCC_INCOMPATIBLE_OPERATION;

   json_t *credentials = observer->getCredentials();
   ObjectArray<TrafficHostEntry> *hosts = connector->GetActiveHosts(point->getExternalId().c_str(), credentials);
   json_decref(credentials);
   if (hosts == nullptr)
      return RCC_COMM_FAILURE;

   auto table = make_shared<Table>();
   table->addColumn(L"HOST_KEY", DCI_DT_STRING, L"Host key", true);
   table->addColumn(L"NAME", DCI_DT_STRING, L"Name");
   table->addColumn(L"IP", DCI_DT_STRING, L"IP address");
   table->addColumn(L"VLAN", DCI_DT_INT, L"VLAN");
   table->addColumn(L"MAC", DCI_DT_STRING, L"MAC address");
   table->addColumn(L"FLAGS", DCI_DT_STRING, L"Flags");
   table->addColumn(L"NODE_ID", DCI_DT_UINT, L"Node ID");
   table->addColumn(L"NODE", DCI_DT_STRING, L"Node");
   table->addColumn(L"FIRST_SEEN", DCI_DT_STRING, L"First seen");
   table->addColumn(L"LAST_SEEN", DCI_DT_STRING, L"Last seen");

   uint32_t pointId = point->getId();
   for (int i = 0; i < hosts->size(); i++)
   {
      TrafficHostEntry *h = hosts->get(i);

      wchar_t flagsText[64] = L"";
      if (h->flags & TRAFFIC_HOST_LOCAL)
         wcscat(flagsText, L"local ");
      if (h->flags & TRAFFIC_HOST_MULTICAST)
         wcscat(flagsText, L"multicast ");
      if (h->flags & TRAFFIC_HOST_BROADCAST)
         wcscat(flagsText, L"broadcast ");
      if (h->flags & TRAFFIC_HOST_BLACKLISTED)
         wcscat(flagsText, L"blacklisted ");
      TrimW(flagsText);

      uint32_t nodeId = 0;
      wchar_t key[192];
      BuildRecordKey(pointId, h->hostKey, key, 192);
      s_hostRecordsLock.readLock();
      ObservationPointHostRecord *r = s_hostRecords.get(key);
      if (r != nullptr)
         nodeId = r->nodeId;
      s_hostRecordsLock.unlock();

      wchar_t nodeName[MAX_OBJECT_NAME] = L"";
      if (nodeId != 0)
      {
         shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
         if (node != nullptr)
            wcslcpy(nodeName, node->getName(), MAX_OBJECT_NAME);
      }

      wchar_t hostKeyText[128], ipText[64], macText[64], timeText[64];
      utf8_to_wchar(h->hostKey, -1, hostKeyText, 128);

      table->addRow();
      table->set(0, hostKeyText);
      table->set(1, h->name);
      table->set(2, h->ipAddr.toString(ipText));
      table->set(3, h->vlan);
      table->set(4, h->macAddr.isValid() ? h->macAddr.toString(macText) : L"");
      table->set(5, flagsText);
      table->set(6, nodeId);
      table->set(7, nodeName);
      table->set(8, (h->firstSeen != 0) ? FormatTimestamp(h->firstSeen, timeText) : L"");
      table->set(9, (h->lastSeen != 0) ? FormatTimestamp(h->lastSeen, timeText) : L"");
   }
   delete hosts;

   *result = table;
   return RCC_SUCCESS;
}

/**
 * Verify that a (point, host) record maps to given node
 */
static bool VerifyHostRecord(uint32_t pointId, const char *hostKey, uint32_t nodeId)
{
   wchar_t key[192];
   BuildRecordKey(pointId, hostKey, key, 192);
   s_hostRecordsLock.readLock();
   ObservationPointHostRecord *r = s_hostRecords.get(key);
   bool match = (r != nullptr) && (r->nodeId == nodeId);
   s_hostRecordsLock.unlock();
   return match;
}

/**
 * Parse a host-level DCI name "Host.<Metric>(<pointId>:<hostKey>)" into its
 * instance components and the connector metric key. Returns false on malformed
 * input. On success pointId/hostKey point into the supplied instance buffer.
 */
static bool ParseHostMetricName(const wchar_t *name, char *instance, size_t instanceSize,
   uint32_t *pointId, char **hostKey, wchar_t *metric, size_t metricSize)
{
   if (!AgentGetParameterArgA(name, 1, instance, instanceSize) || (instance[0] == 0))
      return false;

   char *colon = strchr(instance, ':');
   if (colon == nullptr)
      return false;
   *colon = 0;
   *pointId = strtoul(instance, nullptr, 10);
   *hostKey = colon + 1;
   if ((*pointId == 0) || (**hostKey == 0))
      return false;

   const wchar_t *paren = wcschr(name, L'(');
   size_t baseLen = (paren != nullptr) ? static_cast<size_t>(paren - name) : wcslen(name);
   wchar_t base[MAX_PARAM_NAME];
   if (baseLen >= MAX_PARAM_NAME)
      baseLen = MAX_PARAM_NAME - 1;
   memcpy(base, name, baseLen * sizeof(wchar_t));
   base[baseLen] = 0;
   wcslcpy(metric, !wcsncmp(base, L"Host.", 5) ? base + 5 : base, metricSize);
   return true;
}

/**
 * Serve host-level metric for a node DCI (origin DS_TRAFFIC_OBSERVER)
 */
DataCollectionError GetObservationPointHostMetric(Node *node, const wchar_t *name, wchar_t *buffer, size_t size)
{
   char instance[256], *hostKey;
   uint32_t pointId;
   wchar_t metric[MAX_PARAM_NAME];
   if (!ParseHostMetricName(name, instance, sizeof(instance), &pointId, &hostKey, metric, MAX_PARAM_NAME))
      return DCE_NOT_SUPPORTED;

   if (!VerifyHostRecord(pointId, hostKey, node->getId()))
      return DCE_NOT_SUPPORTED;   // node not (or no longer) observed by referenced point

   shared_ptr<NetObj> pobj = FindObjectById(pointId, OBJECT_OBSERVATIONPOINT);
   if (pobj == nullptr)
      return DCE_NOT_SUPPORTED;
   ObservationPoint *point = static_cast<ObservationPoint*>(pobj.get());
   shared_ptr<TrafficObserver> observer = point->getOwner();
   if (observer == nullptr)
      return DCE_NOT_SUPPORTED;
   TrafficConnectorInterface *connector = FindTrafficConnector(observer->getConnectorName());
   if ((connector == nullptr) || (connector->GetHostMetric == nullptr))
      return DCE_NOT_SUPPORTED;

   json_t *credentials = observer->getCredentials();
   DataCollectionError rc = connector->GetHostMetric(point->getExternalId().c_str(), hostKey, metric, buffer, size, credentials);
   json_decref(credentials);
   return rc;
}

/**
 * Serve host-level table for a node DCI (origin DS_TRAFFIC_OBSERVER)
 */
DataCollectionError GetObservationPointHostTable(Node *node, const wchar_t *name, shared_ptr<Table> *table)
{
   char instance[256], *hostKey;
   uint32_t pointId;
   wchar_t metric[MAX_PARAM_NAME];
   if (!ParseHostMetricName(name, instance, sizeof(instance), &pointId, &hostKey, metric, MAX_PARAM_NAME))
      return DCE_NOT_SUPPORTED;

   if (!VerifyHostRecord(pointId, hostKey, node->getId()))
      return DCE_NOT_SUPPORTED;

   shared_ptr<NetObj> pobj = FindObjectById(pointId, OBJECT_OBSERVATIONPOINT);
   if (pobj == nullptr)
      return DCE_NOT_SUPPORTED;
   ObservationPoint *point = static_cast<ObservationPoint*>(pobj.get());
   shared_ptr<TrafficObserver> observer = point->getOwner();
   if (observer == nullptr)
      return DCE_NOT_SUPPORTED;
   TrafficConnectorInterface *connector = FindTrafficConnector(observer->getConnectorName());
   if ((connector == nullptr) || (connector->GetHostTable == nullptr))
      return DCE_NOT_SUPPORTED;

   json_t *credentials = observer->getCredentials();
   DataCollectionError rc = connector->GetHostTable(point->getExternalId().c_str(), hostKey, metric, table, credentials);
   json_decref(credentials);
   return rc;
}

/**
 * Build the Traffic.ObservationPoints internal table for a node (one row per
 * host record referencing the node). Empty (not an error) on unobserved nodes.
 */
DataCollectionError GetNodeObservationPointsTable(Node *node, shared_ptr<Table> *result)
{
   auto table = make_shared<Table>();
   table->addColumn(L"INSTANCE", DCI_DT_STRING, L"Instance", true);
   table->addColumn(L"OBSERVER", DCI_DT_STRING, L"Observer");
   table->addColumn(L"POINT", DCI_DT_STRING, L"Observation point");
   table->addColumn(L"POINT_ID", DCI_DT_UINT, L"Point object ID");
   table->addColumn(L"HOST_KEY", DCI_DT_STRING, L"Host key");
   table->addColumn(L"IP", DCI_DT_STRING, L"IP address");
   table->addColumn(L"VLAN", DCI_DT_INT, L"VLAN");
   table->addColumn(L"SAMPLING_RATE", DCI_DT_UINT, L"Sampling rate");

   unique_ptr<ObjectArray<ObservationPointHostRecord>> records = GetObservationPointHostsForNode(node->getId());
   for (int i = 0; i < records->size(); i++)
   {
      ObservationPointHostRecord *r = records->get(i);

      wchar_t pointName[MAX_OBJECT_NAME] = L"", observerName[MAX_OBJECT_NAME] = L"";
      uint32_t samplingRate = 0;
      shared_ptr<NetObj> pobj = FindObjectById(r->pointId, OBJECT_OBSERVATIONPOINT);
      if (pobj != nullptr)
      {
         wcslcpy(pointName, pobj->getName(), MAX_OBJECT_NAME);
         samplingRate = static_cast<ObservationPoint*>(pobj.get())->getSamplingRate();
         shared_ptr<TrafficObserver> observer = static_cast<ObservationPoint*>(pobj.get())->getOwner();
         if (observer != nullptr)
            wcslcpy(observerName, observer->getName(), MAX_OBJECT_NAME);
      }

      wchar_t instance[224], hostKeyText[128], ipText[64];
      nx_swprintf(instance, 224, L"%u:%hs", r->pointId, r->hostKey);
      utf8_to_wchar(r->hostKey, -1, hostKeyText, 128);
      r->ipAddr.toString(ipText);

      table->addRow();
      table->set(0, instance);
      table->set(1, observerName);
      table->set(2, pointName);
      table->set(3, r->pointId);
      table->set(4, hostKeyText);
      table->set(5, ipText);
      table->set(6, r->vlan);
      table->set(7, samplingRate);
   }

   *result = table;
   return DCE_SUCCESS;
}
