/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: traffic.cpp
**
** AI assistant tools for the traffic observation framework (traffic observers,
** observation points, observed hosts). All data is served live from the
** connector; historical series come from regular DCIs.
**/

#include "aitools.h"
#include <traffic-connector.h>
#include <algorithm>
#include <map>
#include <vector>

#define DEFAULT_ROW_LIMIT   50
#define MAX_ROW_LIMIT       1000

/**
 * Convert data collection error into text
 */
static const char *DataCollectionErrorToText(DataCollectionError rc)
{
   switch(rc)
   {
      case DCE_SUCCESS:
         return "success";
      case DCE_COMM_ERROR:
         return "communication error with traffic analyzer backend";
      case DCE_NOT_SUPPORTED:
         return "not supported by this traffic analyzer";
      case DCE_NO_SUCH_INSTANCE:
         return "host is not known to the traffic analyzer";
      case DCE_ACCESS_DENIED:
         return "access denied by traffic analyzer backend";
      default:
         return "collection error";
   }
}

/**
 * Convert traffic observer connection state into text
 */
static const char *ObserverStateToText(int16_t state)
{
   switch(state)
   {
      case TRAFFIC_OBSERVER_STATE_CONNECTED:
         return "CONNECTED";
      case TRAFFIC_OBSERVER_STATE_UNREACHABLE:
         return "UNREACHABLE";
      case TRAFFIC_OBSERVER_STATE_AUTH_FAILURE:
         return "AUTH_FAILURE";
      default:
         return "UNKNOWN";
   }
}

/**
 * Convert observation point state into text
 */
static const char *PointStateToText(int16_t state)
{
   switch(state)
   {
      case OBSERVATION_POINT_STATE_ACTIVE:
         return "ACTIVE";
      case OBSERVATION_POINT_STATE_INACTIVE:
         return "INACTIVE";
      default:
         return "UNKNOWN";
   }
}

/**
 * Convert capability bit mask into JSON array of names
 */
static json_t *CapabilitiesToJson(uint64_t capabilities)
{
   static const struct
   {
      uint64_t bit;
      const char *name;
   } names[] =
   {
      { TRAFFIC_CAPABILITY_HOST_L7, "host-l7-breakdown" },
      { TRAFFIC_CAPABILITY_HOST_PEERS, "host-peers" },
      { TRAFFIC_CAPABILITY_POINT_L7, "point-l7-breakdown" },
      { TRAFFIC_CAPABILITY_POINT_TOP_TALKERS, "point-top-talkers" },
      { TRAFFIC_CAPABILITY_POINT_DSCP, "point-dscp-breakdown" },
      { TRAFFIC_CAPABILITY_HISTORICAL_TIMESERIES, "historical-timeseries" },
      { TRAFFIC_CAPABILITY_SYNC_HOST_ALIASES, "sync-host-aliases" },
      { TRAFFIC_CAPABILITY_HOST_SET_AUTHORITATIVE, "authoritative-host-set" },
      { 0, nullptr }
   };

   json_t *array = json_array();
   for(int i = 0; names[i].name != nullptr; i++)
   {
      if (capabilities & names[i].bit)
         json_array_append_new(array, json_string(names[i].name));
   }
   return array;
}

/**
 * Read row limit argument
 */
static int GetRowLimit(json_t *arguments)
{
   int limit = json_object_get_int32(arguments, "limit", DEFAULT_ROW_LIMIT);
   if (limit <= 0)
      limit = DEFAULT_ROW_LIMIT;
   return std::min(limit, MAX_ROW_LIMIT);
}

/**
 * Check if table column holds numeric data
 */
static inline bool IsNumericColumn(int32_t dataType)
{
   return (dataType == DCI_DT_INT) || (dataType == DCI_DT_UINT) || (dataType == DCI_DT_INT64) ||
          (dataType == DCI_DT_UINT64) || (dataType == DCI_DT_COUNTER32) || (dataType == DCI_DT_COUNTER64) ||
          (dataType == DCI_DT_FLOAT);
}

/**
 * Convert table cell into JSON value
 */
static json_t *TableCellToJson(const Table& table, int row, int col)
{
   switch(table.getColumnDataType(col))
   {
      case DCI_DT_INT:
      case DCI_DT_INT64:
         return json_integer(table.getAsInt64(row, col));
      case DCI_DT_UINT:
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER32:
      case DCI_DT_COUNTER64:
         return json_integer(static_cast<json_int_t>(table.getAsUInt64(row, col)));
      case DCI_DT_FLOAT:
         return json_real(table.getAsDouble(row, col));
      default:
         return json_string_t(table.getAsString(row, col, L""));
   }
}

/**
 * Row annotation callback: receives row index and row JSON object, may add fields
 */
typedef std::function<void(const Table&, int, json_t*)> RowAnnotator;

/**
 * Convert table into compact JSON envelope { entries, total_available, truncated }.
 * Rows are ordered by descending sum of all numeric columns when sortByVolume is set,
 * so truncation keeps the most significant rows.
 */
static json_t *TableToCompactJson(const Table& table, int limit, bool sortByVolume, const RowAnnotator& annotator = RowAnnotator())
{
   int rowCount = table.getNumRows();
   int colCount = table.getNumColumns();

   std::vector<int> order(rowCount);
   for(int i = 0; i < rowCount; i++)
      order[i] = i;

   if (sortByVolume && (rowCount > 1))
   {
      std::vector<double> volume(rowCount, 0.0);
      for(int i = 0; i < rowCount; i++)
      {
         for(int j = 0; j < colCount; j++)
         {
            if (IsNumericColumn(table.getColumnDataType(j)))
               volume[i] += table.getAsDouble(i, j);
         }
      }
      std::stable_sort(order.begin(), order.end(), [&volume] (int a, int b) { return volume[a] > volume[b]; });
   }

   json_t *entries = json_array();
   int count = std::min(rowCount, limit);
   for(int i = 0; i < count; i++)
   {
      int row = order[i];
      json_t *entry = json_object();
      for(int j = 0; j < colCount; j++)
      {
         char name[64];
         wchar_to_utf8(table.getColumnName(j), -1, name, sizeof(name));
         json_object_set_new(entry, name, TableCellToJson(table, row, j));
      }
      if (annotator)
         annotator(table, row, entry);
      json_array_append_new(entries, entry);
   }

   json_t *envelope = json_object();
   json_object_set_new(envelope, "entries", entries);
   json_object_set_new(envelope, "total_available", json_integer(rowCount));
   json_object_set_new(envelope, "truncated", json_boolean(rowCount > count));
   return envelope;
}

/**
 * Matched node index for an observation point: IP address text -> node ID.
 * Only nodes readable by the caller are included.
 */
typedef std::map<std::string, uint32_t> MatchedHostIndex;

static MatchedHostIndex BuildMatchedHostIndex(uint32_t pointId, uint32_t userId)
{
   MatchedHostIndex index;
   unique_ptr<ObjectArray<ObservationPointHostRecord>> records = GetObservationPointMatchedHosts(pointId);
   for(int i = 0; i < records->size(); i++)
   {
      ObservationPointHostRecord *r = records->get(i);
      shared_ptr<NetObj> node = FindObjectById(r->nodeId, OBJECT_NODE);
      if ((node == nullptr) || !node->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;
      char ipText[64];
      index[r->ipAddr.toStringA(ipText)] = r->nodeId;
   }
   return index;
}

/**
 * Annotate row with matched node information when the given column holds an IP address of a matched host
 */
static void AnnotateMatchedNode(const MatchedHostIndex& index, const Table& table, int row, const wchar_t *column, json_t *entry)
{
   int col = table.getColumnIndex(column);
   if (col == -1)
      return;

   char ipText[256];
   wchar_to_utf8(table.getAsString(row, col, L""), -1, ipText, sizeof(ipText));
   auto it = index.find(ipText);
   if (it == index.end())
      return;

   shared_ptr<NetObj> node = FindObjectById(it->second, OBJECT_NODE);
   if (node == nullptr)
      return;
   json_object_set_new(entry, "node_id", json_integer(node->getId()));
   json_object_set_new(entry, "node_name", json_string_t(node->getName()));
}

/**
 * Create observer summary
 */
static json_t *CreateObserverSummary(TrafficObserver *observer)
{
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(observer->getId()));
   json_object_set_new(json, "name", json_string_t(observer->getName()));
   json_object_set_new(json, "status", json_string_t(GetStatusAsText(observer->getStatus(), true)));
   json_object_set_new(json, "connector", json_string_t(observer->getConnectorName()));
   json_object_set_new(json, "connection_state", json_string(ObserverStateToText(observer->getConnectionState())));
   json_object_set_new(json, "backend_product", json_string(observer->getBackendProduct().c_str()));
   json_object_set_new(json, "backend_version", json_string(observer->getBackendVersion().c_str()));
   json_object_set_new(json, "backend_edition", json_string(observer->getBackendEdition().c_str()));
   json_object_set_new(json, "capabilities", CapabilitiesToJson(observer->getCapabilities()));
   json_object_set_new(json, "zone_uin", json_integer(observer->getZoneUIN()));
   json_object_set_new(json, "last_discovery_time", json_time_string(observer->getLastDiscoveryTime()));
   json_object_set_new(json, "last_discovery_success", json_boolean(observer->getLastDiscoveryStatus() == 0));

   uint32_t linkedNodeId = observer->getLinkedNodeId();
   if (linkedNodeId != 0)
   {
      json_object_set_new(json, "linked_node_id", json_integer(linkedNodeId));
      shared_ptr<NetObj> node = FindObjectById(linkedNodeId, OBJECT_NODE);
      if (node != nullptr)
         json_object_set_new(json, "linked_node_name", json_string_t(node->getName()));
   }

   unique_ptr<SharedObjectArray<NetObj>> points = observer->getChildren(OBJECT_OBSERVATIONPOINT);
   json_object_set_new(json, "observation_point_count", json_integer(points->size()));
   return json;
}

/**
 * Create observation point summary
 */
static json_t *CreatePointSummary(ObservationPoint *point)
{
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(point->getId()));
   json_object_set_new(json, "name", json_string_t(point->getName()));
   json_object_set_new(json, "status", json_string_t(GetStatusAsText(point->getStatus(), true)));
   json_object_set_new(json, "observer_id", json_integer(point->getObserverId()));
   shared_ptr<TrafficObserver> observer = point->getOwner();
   if (observer != nullptr)
      json_object_set_new(json, "observer_name", json_string_t(observer->getName()));
   json_object_set_new(json, "external_id", json_string(point->getExternalId().c_str()));
   json_object_set_new(json, "type", json_string(point->getPointType().c_str()));
   json_object_set_new(json, "state", json_string(PointStateToText(point->getState())));
   json_object_set_new(json, "provider_state", json_string(point->getProviderState().c_str()));
   json_object_set_new(json, "in_scope", json_boolean(point->isInScope()));
   json_object_set_new(json, "zone_uin", json_integer(point->getEffectiveZoneUIN()));
   json_object_set_new(json, "sampling_rate", json_integer(point->getSamplingRate()));
   json_object_set_new(json, "local_networks", json_string(point->getLocalNetworks().c_str()));
   json_object_set_new(json, "last_discovery_time", json_time_string(point->getLastDiscoveryTime()));
   json_object_set_new(json, "matched_node_count", json_integer(GetObservationPointMatchedHosts(point->getId())->size()));
   return json;
}

/**
 * Resolve observation point argument with access check. Returns nullptr and sets error text on failure.
 */
static shared_ptr<ObservationPoint> ResolvePoint(json_t *arguments, uint32_t userId, std::string *error)
{
   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "point", OBJECT_OBSERVATIONPOINT);
   if (object == nullptr)
   {
      *error = "Observation point not found";
      return shared_ptr<ObservationPoint>();
   }
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      *error = "Access denied";
      return shared_ptr<ObservationPoint>();
   }
   return static_pointer_cast<ObservationPoint>(object);
}

/**
 * Get list of traffic observers
 */
std::string F_GetTrafficObservers(json_t *arguments, uint32_t userId)
{
   unique_ptr<SharedObjectArray<NetObj>> observers = g_idxTrafficObserverById.getObjects(
      [userId] (NetObj *object) -> bool
      {
         return !object->isDeleted() && object->checkAccessRights(userId, OBJECT_ACCESS_READ);
      });

   if (observers->size() == 0)
      return std::string("No traffic observers are configured or accessible. Traffic analysis tools are not available.");

   json_t *output = json_array();
   for(int i = 0; i < observers->size(); i++)
   {
      json_array_append_new(output, CreateObserverSummary(static_cast<TrafficObserver*>(observers->get(i))));
   }
   return JsonToString(output);
}

/**
 * Get list of observation points, optionally restricted to one observer
 */
std::string F_GetObservationPoints(json_t *arguments, uint32_t userId)
{
   uint32_t observerId = 0;
   if (json_object_get(arguments, "observer") != nullptr)
   {
      shared_ptr<NetObj> observer = FindObjectByNameOrId(arguments, "observer", OBJECT_TRAFFICOBSERVER);
      if (observer == nullptr)
         return std::string("Traffic observer not found");
      if (!observer->checkAccessRights(userId, OBJECT_ACCESS_READ))
         return std::string("Access denied");
      observerId = observer->getId();
   }

   unique_ptr<SharedObjectArray<NetObj>> points = g_idxObservationPointById.getObjects(
      [userId, observerId] (NetObj *object) -> bool
      {
         if (object->isDeleted() || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
            return false;
         return (observerId == 0) || (static_cast<ObservationPoint*>(object)->getObserverId() == observerId);
      });

   if (points->size() == 0)
      return std::string("No observation points found");

   json_t *output = json_array();
   for(int i = 0; i < points->size(); i++)
   {
      json_array_append_new(output, CreatePointSummary(static_cast<ObservationPoint*>(points->get(i))));
   }
   return JsonToString(output);
}

/**
 * Get scalar point metric as JSON value (integer when possible, string otherwise, null on error)
 */
static json_t *PointMetricToJson(ObservationPoint *point, const wchar_t *metric)
{
   wchar_t value[MAX_RESULT_LENGTH];
   if (point->getMetricFromConnector(metric, value, MAX_RESULT_LENGTH) != DCE_SUCCESS)
      return json_null();

   wchar_t *eptr;
   int64_t n = wcstoll(value, &eptr, 10);
   if ((*eptr == 0) && (value[0] != 0))
      return json_integer(n);
   double d = wcstod(value, &eptr);
   if ((*eptr == 0) && (value[0] != 0))
      return json_real(d);
   return json_string_w(value);
}

/**
 * Get current statistics of an observation point
 */
std::string F_GetObservationPointStatistics(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<ObservationPoint> point = ResolvePoint(arguments, userId, &error);
   if (point == nullptr)
      return error;

   static const struct
   {
      const wchar_t *metric;
      const char *tag;
   } metrics[] =
   {
      { L"ThroughputIn", "throughput_in_bps" },
      { L"ThroughputOut", "throughput_out_bps" },
      { L"BytesIn", "bytes_in" },
      { L"BytesOut", "bytes_out" },
      { L"PacketsIn", "packets_in" },
      { L"PacketsOut", "packets_out" },
      { L"ActiveHosts", "active_hosts" },
      { L"ActiveFlows", "active_flows" },
      { L"Drops", "dropped_packets" },
      { L"TcpRetransmits", "tcp_retransmits" },
      { nullptr, nullptr }
   };

   json_t *output = CreatePointSummary(point.get());
   json_t *stats = json_object();
   int available = 0;
   for(int i = 0; metrics[i].metric != nullptr; i++)
   {
      json_t *value = PointMetricToJson(point.get(), metrics[i].metric);
      if (!json_is_null(value))
         available++;
      json_object_set_new(stats, metrics[i].tag, value);
   }
   json_object_set_new(output, "statistics", stats);
   if (available == 0)
      json_object_set_new(output, "note", json_string("Traffic analyzer backend did not return any statistics (backend unreachable or point inactive)"));
   json_object_set_new(output, "as_of", json_time_string(time(nullptr)));
   return JsonToString(output);
}

/**
 * Get point-level table (top talkers, L7, DSCP) with common error handling
 */
static std::string GetPointTable(json_t *arguments, uint32_t userId, const wchar_t *metric, uint64_t requiredCapability, const wchar_t *hostColumn)
{
   std::string error;
   shared_ptr<ObservationPoint> point = ResolvePoint(arguments, userId, &error);
   if (point == nullptr)
      return error;

   shared_ptr<TrafficObserver> observer = point->getOwner();
   if (observer == nullptr)
      return std::string("Observation point is not attached to a traffic observer");
   if (!(observer->getCapabilities() & requiredCapability))
      return std::string("Traffic analyzer backend does not support this query");

   shared_ptr<Table> table;
   DataCollectionError rc = point->getTableFromConnector(metric, &table);
   if (rc != DCE_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Traffic table \"%s\" query for observation point [%u] %s failed (%hs)", metric, point->getId(), point->getName(), DataCollectionErrorToText(rc));
      return std::string("Cannot retrieve data: ") + DataCollectionErrorToText(rc);
   }

   json_t *output;
   if (hostColumn != nullptr)
   {
      MatchedHostIndex index = BuildMatchedHostIndex(point->getId(), userId);
      output = TableToCompactJson(*table, GetRowLimit(arguments), true,
         [&index, hostColumn] (const Table& t, int row, json_t *entry) { AnnotateMatchedNode(index, t, row, hostColumn, entry); });
   }
   else
   {
      output = TableToCompactJson(*table, GetRowLimit(arguments), true);
   }
   json_object_set_new(output, "point_id", json_integer(point->getId()));
   json_object_set_new(output, "point_name", json_string_t(point->getName()));
   json_object_set_new(output, "as_of", json_time_string(time(nullptr)));
   return JsonToString(output);
}

/**
 * Get top talkers of an observation point
 */
std::string F_GetObservationPointTopTalkers(json_t *arguments, uint32_t userId)
{
   return GetPointTable(arguments, userId, L"TopTalkers", TRAFFIC_CAPABILITY_POINT_TOP_TALKERS, L"HOST");
}

/**
 * Get L7 application breakdown of an observation point
 */
std::string F_GetObservationPointL7Breakdown(json_t *arguments, uint32_t userId)
{
   return GetPointTable(arguments, userId, L"L7Breakdown", TRAFFIC_CAPABILITY_POINT_L7, nullptr);
}

/**
 * Get DSCP breakdown of an observation point
 */
std::string F_GetObservationPointDSCPBreakdown(json_t *arguments, uint32_t userId)
{
   return GetPointTable(arguments, userId, L"DSCPBreakdown", TRAFFIC_CAPABILITY_POINT_DSCP, nullptr);
}

/**
 * Get active hosts seen by an observation point
 */
std::string F_GetObservationPointActiveHosts(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<ObservationPoint> point = ResolvePoint(arguments, userId, &error);
   if (point == nullptr)
      return error;

   // Optional filter: IP address, CIDR, or substring of host name / node name
   InetAddress ipFilter;
   wchar_t textFilter[256] = L"";
   const char *filter = json_object_get_string_utf8(arguments, "filter", nullptr);
   if ((filter != nullptr) && (*filter != 0))
   {
      const char *slash = strchr(filter, '/');
      if (slash != nullptr)
      {
         char addr[64];
         strlcpy(addr, filter, std::min(sizeof(addr), static_cast<size_t>(slash - filter + 1)));
         ipFilter = InetAddress::parse(addr);
         if (ipFilter.isValid())
            ipFilter.setMaskBits(strtol(slash + 1, nullptr, 10));
      }
      else
      {
         ipFilter = InetAddress::parse(filter);
      }
      if (!ipFilter.isValid())
         utf8_to_wchar(filter, -1, textFilter, 256);
   }
   bool unmatchedOnly = json_object_get_boolean(arguments, "unmatched_only", false);

   shared_ptr<Table> hosts;
   uint32_t rcc = GetObservationPointActiveHosts(point.get(), userId, &hosts);
   if (rcc == RCC_INCOMPATIBLE_OPERATION)
      return std::string("Traffic analyzer backend does not support active host listing");
   if (rcc != RCC_SUCCESS)
      return std::string("Cannot retrieve active host list from traffic analyzer backend");

   int ipColumn = hosts->getColumnIndex(L"IP");
   int nameColumn = hosts->getColumnIndex(L"NAME");
   int nodeIdColumn = hosts->getColumnIndex(L"NODE_ID");
   int nodeNameColumn = hosts->getColumnIndex(L"NODE");

   int totalActiveHosts = hosts->getNumRows();
   for(int i = totalActiveHosts - 1; i >= 0; i--)
   {
      bool keep = true;
      if (unmatchedOnly && (hosts->getAsUInt64(i, nodeIdColumn) != 0))
      {
         keep = false;
      }
      else if (ipFilter.isValid())
      {
         InetAddress ip = InetAddress::parse(hosts->getAsString(i, ipColumn, L""));
         keep = ip.isValid() && ipFilter.contains(ip);
      }
      else if (textFilter[0] != 0)
      {
         keep = (wcsistr(hosts->getAsString(i, nameColumn, L""), textFilter) != nullptr) ||
                (wcsistr(hosts->getAsString(i, nodeNameColumn, L""), textFilter) != nullptr) ||
                (wcsistr(hosts->getAsString(i, ipColumn, L""), textFilter) != nullptr);
      }
      if (!keep)
         hosts->deleteRow(i);
   }

   json_t *output = TableToCompactJson(*hosts, GetRowLimit(arguments), false);
   json_object_set_new(output, "point_id", json_integer(point->getId()));
   json_object_set_new(output, "point_name", json_string_t(point->getName()));
   json_object_set_new(output, "total_active_hosts", json_integer(totalActiveHosts));
   json_object_set_new(output, "as_of", json_time_string(time(nullptr)));
   return JsonToString(output);
}

/**
 * Resolve node argument with access check
 */
static shared_ptr<Node> ResolveNode(json_t *arguments, uint32_t userId, std::string *error)
{
   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object", OBJECT_NODE);
   if (object == nullptr)
   {
      *error = "Node not found";
      return shared_ptr<Node>();
   }
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      *error = "Access denied";
      return shared_ptr<Node>();
   }
   return static_pointer_cast<Node>(object);
}

/**
 * Get host records of a node, optionally restricted to one observation point given in "point" argument.
 * Returns empty array and sets error text if the point argument cannot be resolved.
 */
static unique_ptr<ObjectArray<ObservationPointHostRecord>> GetNodeHostRecords(Node *node, json_t *arguments, uint32_t userId, std::string *error)
{
   unique_ptr<ObjectArray<ObservationPointHostRecord>> records = GetObservationPointHostsForNode(node->getId());
   if (json_object_get(arguments, "point") != nullptr)
   {
      shared_ptr<ObservationPoint> point = ResolvePoint(arguments, userId, error);
      if (point == nullptr)
      {
         records->clear();
         return records;
      }
      for(int i = records->size() - 1; i >= 0; i--)
      {
         if (records->get(i)->pointId != point->getId())
            records->remove(i);
      }
   }
   return records;
}

/**
 * Create host record summary (point, host key, IP, VLAN, timestamps)
 */
static json_t *CreateHostRecordSummary(const ObservationPointHostRecord *r)
{
   json_t *json = json_object();
   json_object_set_new(json, "point_id", json_integer(r->pointId));
   shared_ptr<NetObj> pobj = FindObjectById(r->pointId, OBJECT_OBSERVATIONPOINT);
   if (pobj != nullptr)
   {
      ObservationPoint *point = static_cast<ObservationPoint*>(pobj.get());
      json_object_set_new(json, "point_name", json_string_t(point->getName()));
      json_object_set_new(json, "point_state", json_string(PointStateToText(point->getState())));
      json_object_set_new(json, "sampling_rate", json_integer(point->getSamplingRate()));
      shared_ptr<TrafficObserver> observer = point->getOwner();
      if (observer != nullptr)
      {
         json_object_set_new(json, "observer_id", json_integer(observer->getId()));
         json_object_set_new(json, "observer_name", json_string_t(observer->getName()));
      }
   }
   json_object_set_new(json, "host_key", json_string(r->hostKey));
   json_object_set_new(json, "ip_address", r->ipAddr.toJson());
   json_object_set_new(json, "vlan", json_integer(r->vlan));
   json_object_set_new(json, "first_seen", json_time_string(r->firstSeen));
   json_object_set_new(json, "last_seen", json_time_string(r->lastSeen));

   char instance[192];
   snprintf(instance, sizeof(instance), "%u:%s", r->pointId, r->hostKey);
   json_object_set_new(json, "dci_instance", json_string(instance));
   return json;
}

/**
 * Get traffic context for a node: which observation points see it, and current host-level counters
 */
std::string F_GetNodeTrafficContext(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Node> node = ResolveNode(arguments, userId, &error);
   if (node == nullptr)
      return error;

   json_t *output = json_object();
   json_object_set_new(output, "node_id", json_integer(node->getId()));
   json_object_set_new(output, "node_name", json_string_t(node->getName()));

   unique_ptr<ObjectArray<ObservationPointHostRecord>> records = GetObservationPointHostsForNode(node->getId());
   json_object_set_new(output, "observed", json_boolean(records->size() > 0));
   if (records->size() == 0)
   {
      json_object_set_new(output, "note", json_string("Node is not seen by any observation point. Traffic data is not available for this node; use get-observation-points to check observer scope and local networks."));
      return JsonToString(output);
   }

   static const struct
   {
      const wchar_t *metric;
      const char *tag;
   } metrics[] =
   {
      { L"Host.BytesIn", "bytes_in" },
      { L"Host.BytesOut", "bytes_out" },
      { L"Host.PacketsIn", "packets_in" },
      { L"Host.PacketsOut", "packets_out" },
      { L"Host.ActiveFlows", "active_flows" },
      { L"Host.TcpRetransmits", "tcp_retransmits" },
      { L"Host.Alerts", "backend_alerts" },
      { nullptr, nullptr }
   };

   json_t *points = json_array();
   for(int i = 0; i < records->size(); i++)
   {
      ObservationPointHostRecord *r = records->get(i);
      json_t *entry = CreateHostRecordSummary(r);

      json_t *counters = json_object();
      DataCollectionError lastError = DCE_SUCCESS;
      for(int j = 0; metrics[j].metric != nullptr; j++)
      {
         wchar_t name[256], value[MAX_RESULT_LENGTH];
         nx_swprintf(name, 256, L"%s(%u:%hs)", metrics[j].metric, r->pointId, r->hostKey);
         DataCollectionError rc = GetObservationPointHostMetric(node.get(), name, value, MAX_RESULT_LENGTH);
         if (rc == DCE_SUCCESS)
         {
            json_object_set_new(counters, metrics[j].tag, json_integer(wcstoll(value, nullptr, 10)));
         }
         else
         {
            lastError = rc;
            json_object_set_new(counters, metrics[j].tag, json_null());
         }
      }
      json_object_set_new(entry, "current_counters", counters);
      if (lastError != DCE_SUCCESS)
         json_object_set_new(entry, "counters_error", json_string(DataCollectionErrorToText(lastError)));
      json_array_append_new(points, entry);
   }
   json_object_set_new(output, "observation_points", points);
   json_object_set_new(output, "as_of", json_time_string(time(nullptr)));
   return JsonToString(output);
}

/**
 * Get host-level table (peers or L7) for a node from every observation point that sees it
 */
static std::string GetNodeHostTable(json_t *arguments, uint32_t userId, const wchar_t *metric, uint64_t requiredCapability, bool annotatePeers)
{
   std::string error;
   shared_ptr<Node> node = ResolveNode(arguments, userId, &error);
   if (node == nullptr)
      return error;

   unique_ptr<ObjectArray<ObservationPointHostRecord>> records = GetNodeHostRecords(node.get(), arguments, userId, &error);
   if (!error.empty())
      return error;
   if (records->size() == 0)
      return std::string("Node is not seen by any observation point (or not by the requested one). Traffic data is not available for this node.");

   int limit = GetRowLimit(arguments);

   json_t *output = json_object();
   json_object_set_new(output, "node_id", json_integer(node->getId()));
   json_object_set_new(output, "node_name", json_string_t(node->getName()));

   json_t *points = json_array();
   for(int i = 0; i < records->size(); i++)
   {
      ObservationPointHostRecord *r = records->get(i);
      json_t *entry = CreateHostRecordSummary(r);

      shared_ptr<NetObj> pobj = FindObjectById(r->pointId, OBJECT_OBSERVATIONPOINT);
      shared_ptr<TrafficObserver> observer = (pobj != nullptr) ? static_cast<ObservationPoint*>(pobj.get())->getOwner() : shared_ptr<TrafficObserver>();
      if ((observer == nullptr) || !(observer->getCapabilities() & requiredCapability))
      {
         json_object_set_new(entry, "error", json_string("Traffic analyzer backend does not support this query"));
         json_array_append_new(points, entry);
         continue;
      }

      wchar_t name[256];
      nx_swprintf(name, 256, L"%s(%u:%hs)", metric, r->pointId, r->hostKey);
      shared_ptr<Table> table;
      DataCollectionError rc = GetObservationPointHostTable(node.get(), name, &table);
      if (rc != DCE_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Traffic table \"%s\" query for node [%u] %s failed (%hs)", name, node->getId(), node->getName(), DataCollectionErrorToText(rc));
         json_object_set_new(entry, "error", json_string(DataCollectionErrorToText(rc)));
         json_array_append_new(points, entry);
         continue;
      }

      json_t *data;
      if (annotatePeers)
      {
         MatchedHostIndex index = BuildMatchedHostIndex(r->pointId, userId);
         data = TableToCompactJson(*table, limit, true,
            [&index] (const Table& t, int row, json_t *e) { AnnotateMatchedNode(index, t, row, L"PEER", e); });
      }
      else
      {
         data = TableToCompactJson(*table, limit, true);
      }
      json_object_update_missing(entry, data);
      json_decref(data);
      json_array_append_new(points, entry);
   }
   json_object_set_new(output, "observation_points", points);
   json_object_set_new(output, "as_of", json_time_string(time(nullptr)));
   return JsonToString(output);
}

/**
 * Get communication peers of a node
 */
std::string F_GetNodeTrafficPeers(json_t *arguments, uint32_t userId)
{
   return GetNodeHostTable(arguments, userId, L"Host.Peers", TRAFFIC_CAPABILITY_HOST_PEERS, true);
}

/**
 * Get L7 application breakdown of a node's traffic
 */
std::string F_GetNodeTrafficL7Breakdown(json_t *arguments, uint32_t userId)
{
   return GetNodeHostTable(arguments, userId, L"Host.L7Breakdown", TRAFFIC_CAPABILITY_HOST_L7, false);
}
