/*
** NetXMS ntopng traffic connector module
** Copyright (C) 2026 Victor Kirhenshtein
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
** File: metrics.cpp
** Metric, table, and host-level data serving with a two-tier host cache.
**/

#include "ntopng.h"
#include <functional>

static json_t *GetCachedInterfaceData(json_t *credentials, const char *ifid);
static Mutex s_cacheLock(MutexType::FAST);

/**
 * Locate a field addressed by a "/"-separated spec, tolerating ntopng's
 * inconsistent shapes: a field may be a flat simple key, a nested object
 * traversed by "/", or a flat key that literally contains dots (host/data.lua
 * uses "bytes.rcvd" while host/active.lua uses nested {"bytes":{"rcvd":...}}).
 */
static json_t *LookupField(json_t *root, const char *spec)
{
   json_t *v = json_object_get(root, spec);   // flat key (incl. flat-dotted)
   if (v != nullptr)
      return v;
   v = json_object_get_by_path_a(root, spec);   // nested, traversed by '/'
   if (v != nullptr)
      return v;
   if (strchr(spec, '/') != nullptr)
   {
      char flat[256];
      strlcpy(flat, spec, sizeof(flat));
      for (char *p = flat; *p != 0; p++)
         if (*p == '/')
            *p = '.';
      v = json_object_get(root, flat);
   }
   return v;
}

static json_t *FindField(json_t *root, const char *spec)
{
   if (root == nullptr)
      return nullptr;
   json_t *v = LookupField(root, spec);
   if (v != nullptr)
      return v;

   // ntopng is inconsistent about "rcvd" vs "recvd" between the active-host list
   // and the per-host detail document - retry once with the alternate spelling.
   const char *alt = (strstr(spec, "recvd") != nullptr) ? "recvd" : ((strstr(spec, "rcvd") != nullptr) ? "rcvd" : nullptr);
   if (alt != nullptr)
   {
      const char *pos = strstr(spec, alt);
      const char *other = !strcmp(alt, "rcvd") ? "recvd" : "rcvd";
      char retry[256];
      snprintf(retry, sizeof(retry), "%.*s%s%s", static_cast<int>(pos - spec), spec, other, pos + strlen(alt));
      v = LookupField(root, retry);
   }
   return v;
}

/**
 * Read numeric value addressed by a "/"-separated spec and format it into a
 * DCI result buffer. Returns DCE_SUCCESS if the field was found and is numeric,
 * DCE_COLLECTION_ERROR if the field is absent or non-numeric.
 */
static DataCollectionError FormatNumericField(json_t *root, const char *path, wchar_t *value, size_t size)
{
   json_t *field = FindField(root, path);
   if (field == nullptr)
      return DCE_COLLECTION_ERROR;
   if (json_is_integer(field))
      nx_swprintf(value, size, L"%lld", static_cast<long long>(json_integer_value(field)));
   else if (json_is_real(field))
      nx_swprintf(value, size, L"%f", json_real_value(field));
   else if (json_is_boolean(field))
      wcslcpy(value, json_is_true(field) ? L"1" : L"0", size);
   else if (json_is_string(field))
      utf8_to_wchar(json_string_value(field), -1, value, size);
   else
      return DCE_COLLECTION_ERROR;
   return DCE_SUCCESS;
}

/**
 * Base URL for cache keys (without trailing slashes)
 */
static void GetCacheKeyPrefix(json_t *credentials, char *buffer, size_t size)
{
   const char *baseUrl = json_object_get_string_utf8(credentials, "url", "");
   size_t len = strlen(baseUrl);
   while ((len > 0) && (baseUrl[len - 1] == '/'))
      len--;
   snprintf(buffer, size, "%.*s", static_cast<int>(len), baseUrl);
}

/**
 * Cache TTL in seconds (from credentials, default 30)
 */
static inline uint32_t GetCacheTtl(json_t *credentials)
{
   return json_object_get_uint32(credentials, "cacheTtl", 30);
}

/* ------------------------------------------------------------------------- *
 *  Observer level                                                           *
 * ------------------------------------------------------------------------- */

/**
 * Observer level metric collection. ntopng has no single instance-wide stats
 * endpoint, so version comes from about.lua and uptime from any interface (it is
 * a process-wide counter).
 */
DataCollectionError NtopngGetObserverMetric(const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials)
{
   if (!wcscmp(metric, L"Version"))
   {
      json_t *response = NtopngApiGet(credentials, "/lua/rest/v2/get/ntopng/about.lua");
      if (response == nullptr)
         return DCE_COMM_ERROR;
      DataCollectionError rc = FormatNumericField(json_object_get(response, "rsp"), "version", value, size);
      json_decref(response);
      return rc;
   }
   if (!wcscmp(metric, L"Uptime"))
   {
      // Uptime is process-wide; read it from the first available interface
      json_t *response = NtopngApiGet(credentials, "/lua/rest/v2/get/ntopng/interfaces.lua");
      if (response == nullptr)
         return DCE_COMM_ERROR;
      json_t *interfaces = json_object_get(response, "rsp");
      int32_t ifid = -1;
      if (json_is_array(interfaces) && (json_array_size(interfaces) > 0))
         ifid = json_object_get_int32(json_array_get(interfaces, 0), "ifid", -1);
      json_decref(response);
      if (ifid < 0)
         return DCE_COLLECTION_ERROR;

      char path[128];
      snprintf(path, sizeof(path), "/lua/rest/v2/get/interface/data.lua?ifid=%d", ifid);
      response = NtopngApiGet(credentials, path);
      if (response == nullptr)
         return DCE_COMM_ERROR;
      DataCollectionError rc = FormatNumericField(json_object_get(response, "rsp"), "uptime_sec", value, size);
      json_decref(response);
      return rc;
   }
   return DCE_NOT_SUPPORTED;
}

/* ------------------------------------------------------------------------- *
 *  Point level                                                              *
 * ------------------------------------------------------------------------- */

/**
 * Mapping of point-level metric names to fields in interface/data.lua.
 * Direction is relative to the monitored network: download -> In, upload -> Out.
 */
static const struct
{
   const wchar_t *name;
   const char *path;
} s_pointMetrics[] =
{
   { L"ThroughputIn", "throughput/download/bps" },
   { L"ThroughputOut", "throughput/upload/bps" },
   { L"PacketsIn", "packets_download" },
   { L"PacketsOut", "packets_upload" },
   { L"BytesIn", "bytes_download" },
   { L"BytesOut", "bytes_upload" },
   { L"ActiveHosts", "num_hosts" },
   { L"ActiveFlows", "num_flows" },
   { L"Drops", "drops" },
   { L"TcpRetransmits", "tcpPacketStats/retransmissions" },
   { L"Uptime", "uptime_sec" },
   { nullptr, nullptr }
};

/**
 * Point level metric collection via cached interface/data.lua document (one
 * fetch per TTL serves all point-level metrics, including the live views'
 * multi-metric stat headers)
 */
DataCollectionError NtopngGetPointMetric(const char *pointId, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials)
{
   const char *path = nullptr;
   for (int i = 0; s_pointMetrics[i].name != nullptr; i++)
   {
      if (!wcscmp(metric, s_pointMetrics[i].name))
      {
         path = s_pointMetrics[i].path;
         break;
      }
   }
   if (path == nullptr)
      return DCE_NOT_SUPPORTED;

   LockGuard lockGuard(s_cacheLock);
   json_t *data = GetCachedInterfaceData(credentials, pointId);
   if (data == nullptr)
      return DCE_COMM_ERROR;
   return FormatNumericField(data, path, value, size);
}

/**
 * Point level table collection
 */
DataCollectionError NtopngGetPointTable(const char *pointId, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials)
{
   char url[160];
   auto table = make_shared<Table>();

   if (!wcscmp(metric, L"L7Breakdown"))
   {
      snprintf(url, sizeof(url), "/lua/rest/v2/get/interface/l7/data.lua?ifid=%s", pointId);
      json_t *response = NtopngApiGet(credentials, url);
      if (response == nullptr)
         return DCE_COMM_ERROR;

      table->addColumn(L"APPLICATION", DCI_DT_STRING, L"Application", true);
      table->addColumn(L"BYTES_SENT", DCI_DT_UINT64, L"Bytes sent");
      table->addColumn(L"BYTES_RCVD", DCI_DT_UINT64, L"Bytes received");
      table->addColumn(L"PACKETS", DCI_DT_UINT64, L"Packets");
      table->addColumn(L"FLOWS", DCI_DT_UINT64, L"Flows");
      table->addColumn(L"BREED", DCI_DT_STRING, L"Breed");

      json_t *data = json_object_get(response, "rsp");
      if (json_is_array(data))
      {
         size_t i;
         json_t *entry;
         json_array_foreach(data, i, entry)
         {
            table->addRow();
            table->set(0, json_object_get_string_utf8(entry, "application", json_object_get_string_utf8(entry, "label", "")));
            table->set(1, json_object_get_uint64(entry, "bytes.sent", json_object_get_uint64(entry, "bytes_sent", 0)));
            table->set(2, json_object_get_uint64(entry, "bytes.rcvd", json_object_get_uint64(entry, "bytes_rcvd", 0)));
            table->set(3, json_object_get_uint64(entry, "packets", 0));
            table->set(4, json_object_get_uint64(entry, "num_flows", json_object_get_uint64(entry, "flows", 0)));
            table->set(5, json_object_get_string_utf8(entry, "breed", ""));
         }
      }
      json_decref(response);
   }
   else if (!wcscmp(metric, L"TopTalkers"))
   {
      snprintf(url, sizeof(url), "/lua/rest/v2/get/interface/top/hosts.lua?ifid=%s", pointId);
      json_t *response = NtopngApiGet(credentials, url);
      if (response == nullptr)
         return DCE_COMM_ERROR;

      table->addColumn(L"HOST", DCI_DT_STRING, L"Host", true);
      table->addColumn(L"BYTES", DCI_DT_UINT64, L"Bytes");

      json_t *data = json_object_get(response, "rsp");
      if (json_is_array(data))
      {
         size_t i;
         json_t *entry;
         json_array_foreach(data, i, entry)
         {
            table->addRow();
            table->set(0, json_object_get_string_utf8(entry, "label", json_object_get_string_utf8(entry, "name", "")));
            table->set(1, json_object_get_uint64(entry, "value", json_object_get_uint64(entry, "bytes", 0)));
         }
      }
      json_decref(response);
   }
   else if (!wcscmp(metric, L"DSCPBreakdown"))
   {
      snprintf(url, sizeof(url), "/lua/rest/v2/get/interface/dscp/stats.lua?ifid=%s", pointId);
      json_t *response = NtopngApiGet(credentials, url);
      if (response == nullptr)
         return DCE_COMM_ERROR;

      table->addColumn(L"CLASS", DCI_DT_STRING, L"DSCP class", true);
      table->addColumn(L"BYTES", DCI_DT_UINT64, L"Bytes");

      json_t *data = json_object_get(response, "rsp");
      if (json_is_array(data))
      {
         size_t i;
         json_t *entry;
         json_array_foreach(data, i, entry)
         {
            table->addRow();
            table->set(0, json_object_get_string_utf8(entry, "label", json_object_get_string_utf8(entry, "name", "")));
            table->set(1, json_object_get_uint64(entry, "value", json_object_get_uint64(entry, "bytes", 0)));
         }
      }
      json_decref(response);
   }
   else
   {
      return DCE_NOT_SUPPORTED;
   }

   *value = table;
   return DCE_SUCCESS;
}

/* ------------------------------------------------------------------------- *
 *  Response cache (interface data, active host list, per-host detail)        *
 * ------------------------------------------------------------------------- */

/**
 * Cached JSON document with refresh-in-progress marker
 */
struct JsonCacheEntry
{
   json_t *data;
   time_t timestamp;
   volatile bool processing;

   JsonCacheEntry()
   {
      data = nullptr;
      timestamp = 0;
      processing = false;
   }

   ~JsonCacheEntry()
   {
      json_decref(data);
   }
};

static StringObjectMap<JsonCacheEntry> s_interfaceDataCache(Ownership::True);
static StringObjectMap<JsonCacheEntry> s_pointHostCache(Ownership::True);
static StringObjectMap<JsonCacheEntry> s_hostDetailCache(Ownership::True);

/**
 * Fetch the full active host list for an interface, paging until exhausted.
 * Returns a new JSON object mapping hostKey ("ip@vlan") to the host row, or
 * nullptr on hard failure. Called without s_cacheLock held.
 */
static json_t *FetchActiveHosts(json_t *credentials, const char *ifid)
{
   int pageSize = static_cast<int>(json_object_get_uint32(credentials, "pageSize", 1000));
   if (pageSize < 1)
      pageSize = 1000;

   // Hard ceiling on accumulated hosts - a misbehaving backend streaming full pages
   // forever must not exhaust server memory
   size_t maxHosts = json_object_get_uint32(credentials, "maxHosts", 100000);

   json_t *result = json_object();
   for (int page = 1; page <= 100000; page++)
   {
      if (json_object_size(result) >= maxHosts)
      {
         nxlog_debug_tag(DEBUG_TAG, 3, L"FetchActiveHosts(%hs): host limit reached (%u), list truncated", ifid, static_cast<uint32_t>(maxHosts));
         break;
      }
      char path[256];
      snprintf(path, sizeof(path), "/lua/rest/v2/get/host/active.lua?ifid=%s&perPage=%d&currentPage=%d&sortColumn=ip&sortOrder=asc",
         ifid, pageSize, page);
      json_t *response = NtopngApiGet(credentials, path);
      if (response == nullptr)
      {
         if (page == 1)
         {
            json_decref(result);
            return nullptr;
         }
         break;   // partial result usable; next poll repairs omissions
      }

      json_t *rsp = json_object_get(response, "rsp");
      json_t *data = json_is_array(rsp) ? rsp : json_object_get(rsp, "data");
      if (!json_is_array(data))
      {
         json_decref(response);
         break;
      }

      size_t count = json_array_size(data);
      size_t i;
      json_t *row;
      json_array_foreach(data, i, row)
      {
         const char *ip = json_object_get_string_utf8(row, "ip", nullptr);
         if (ip == nullptr)
            continue;
         char hostKey[128];
         snprintf(hostKey, sizeof(hostKey), "%s@%d", ip, json_object_get_int32(row, "vlan", 0));
         json_object_set(result, hostKey, row);   // increments reference
      }
      json_decref(response);

      if (static_cast<int>(count) < pageSize)
         break;
   }
   return result;
}

/**
 * Get a document from a cache map, refreshing via the supplied fetch function if
 * stale. One caller refreshes while others wait for the result. Assumes
 * s_cacheLock is held; may temporarily release it during I/O. Returns borrowed
 * reference valid while the lock is held.
 */
static json_t *GetCachedDocument(StringObjectMap<JsonCacheEntry> *cache, json_t *credentials, const char *key, std::function<json_t*()> fetch)
{
   wchar_t wkey[640];
   utf8_to_wchar(key, -1, wkey, 640);

   JsonCacheEntry *ce = cache->get(wkey);
   if ((ce != nullptr) && (ce->data != nullptr) && (ce->timestamp >= time(nullptr) - static_cast<time_t>(GetCacheTtl(credentials))))
      return ce->data;

   if (ce == nullptr)
   {
      ce = new JsonCacheEntry();
      cache->set(wkey, ce);
   }

   if (!ce->processing)
   {
      ce->processing = true;
      s_cacheLock.unlock();
      json_t *data = fetch();
      s_cacheLock.lock();

      if (data != nullptr)
      {
         json_decref(ce->data);
         ce->data = data;
         ce->timestamp = time(nullptr);
      }
      ce->processing = false;
   }
   else
   {
      do
      {
         s_cacheLock.unlock();
         ThreadSleepMs(200);
         s_cacheLock.lock();
      } while (ce->processing);
   }
   return ce->data;
}

/**
 * Get interface/data.lua document ("rsp" subtree) for a point from cache
 */
static json_t *GetCachedInterfaceData(json_t *credentials, const char *ifid)
{
   char key[512], prefix[384];
   GetCacheKeyPrefix(credentials, prefix, sizeof(prefix));
   snprintf(key, sizeof(key), "%s|%s", prefix, ifid);
   return GetCachedDocument(&s_interfaceDataCache, credentials, key,
      [credentials, ifid]() -> json_t*
      {
         char path[128];
         snprintf(path, sizeof(path), "/lua/rest/v2/get/interface/data.lua?ifid=%s", ifid);
         json_t *response = NtopngApiGet(credentials, path);
         json_t *data = (response != nullptr) ? json_incref(json_object_get(response, "rsp")) : nullptr;
         json_decref(response);
         return data;
      });
}

/**
 * Get active host list for a point from cache
 */
static json_t *GetCachedActiveHosts(json_t *credentials, const char *ifid)
{
   char key[512], prefix[384];
   GetCacheKeyPrefix(credentials, prefix, sizeof(prefix));
   snprintf(key, sizeof(key), "%s|%s", prefix, ifid);
   return GetCachedDocument(&s_pointHostCache, credentials, key,
      [credentials, ifid]() { return FetchActiveHosts(credentials, ifid); });
}

/**
 * Get per-host detail document from cache
 */
static json_t *GetCachedHostDetail(json_t *credentials, const char *ifid, const char *hostKey)
{
   char key[640], prefix[384];
   GetCacheKeyPrefix(credentials, prefix, sizeof(prefix));
   snprintf(key, sizeof(key), "%s|%s|%s", prefix, ifid, hostKey);
   return GetCachedDocument(&s_hostDetailCache, credentials, key,
      [credentials, ifid, hostKey]() -> json_t*
      {
         char path[256];
         snprintf(path, sizeof(path), "/lua/rest/v2/get/host/data.lua?ifid=%s&host=%s", ifid, hostKey);
         json_t *response = NtopngApiGet(credentials, path);
         json_t *data = (response != nullptr) ? json_incref(json_object_get(response, "rsp")) : nullptr;
         json_decref(response);
         return data;
      });
}

/**
 * Active host list retrieval (for matching pass and live views).
 * Built from the tier-1 cache; caller owns the returned array.
 */
ObjectArray<TrafficHostEntry> *NtopngGetActiveHosts(const char *pointId, json_t *credentials)
{
   LockGuard lockGuard(s_cacheLock);
   json_t *hosts = GetCachedActiveHosts(credentials, pointId);
   if (hosts == nullptr)
      return nullptr;

   auto result = new ObjectArray<TrafficHostEntry>(json_object_size(hosts), 64, Ownership::True);
   const char *hostKey;
   json_t *row;
   void *it = json_object_iter(hosts);
   while (it != nullptr)
   {
      hostKey = json_object_iter_key(it);
      row = json_object_iter_value(it);
      it = json_object_iter_next(hosts, it);

      auto entry = new TrafficHostEntry();
      strlcpy(entry->hostKey, hostKey, sizeof(entry->hostKey));
      entry->ipAddr = InetAddress::parse(json_object_get_string_utf8(row, "ip", ""));
      entry->vlan = json_object_get_int32(row, "vlan", 0);
      entry->macAddr = MacAddress::parse(json_object_get_string_utf8(row, "mac", ""));
      utf8_to_wchar(json_object_get_string_utf8(row, "name", ""), -1, entry->name, 192);
      entry->firstSeen = json_object_get_time(row, "first_seen");
      entry->lastSeen = json_object_get_time(row, "last_seen");
      if (json_object_get_boolean(row, "is_multicast", false))
         entry->flags |= TRAFFIC_HOST_MULTICAST;
      if (json_object_get_boolean(row, "is_broadcast", false))
         entry->flags |= TRAFFIC_HOST_BROADCAST;
      if (json_object_get_boolean(row, "is_blacklisted", false))
         entry->flags |= TRAFFIC_HOST_BLACKLISTED;
      if (json_object_get_boolean(row, "is_localhost", false))
         entry->flags |= TRAFFIC_HOST_LOCAL;
      result->add(entry);
   }
   return result;
}

/**
 * Host level metric collection. Cheap byte/flow/presence metrics are served from
 * the tier-1 active-host cache; packet, TCP and alert counters require the tier-2
 * per-host detail document.
 */
DataCollectionError NtopngGetHostMetric(const char *pointId, const char *hostKey, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials)
{
   LockGuard lockGuard(s_cacheLock);

   // Presence and byte/flow counters come from the active-host list
   if (!wcscmp(metric, L"Present") || !wcscmp(metric, L"BytesIn") || !wcscmp(metric, L"BytesOut") || !wcscmp(metric, L"ActiveFlows"))
   {
      json_t *hosts = GetCachedActiveHosts(credentials, pointId);
      if (hosts == nullptr)
         return DCE_COMM_ERROR;
      json_t *row = json_object_get(hosts, hostKey);

      if (!wcscmp(metric, L"Present"))
      {
         wcslcpy(value, (row != nullptr) ? L"1" : L"0", size);
         return DCE_SUCCESS;
      }
      if (row == nullptr)
         return DCE_NO_SUCH_INSTANCE;
      if (!wcscmp(metric, L"BytesIn"))
         return FormatNumericField(row, "bytes/rcvd", value, size);   // active list: nested {bytes:{rcvd}}
      if (!wcscmp(metric, L"BytesOut"))
         return FormatNumericField(row, "bytes/sent", value, size);
      return FormatNumericField(row, "num_flows/total", value, size);   // ActiveFlows: nested {num_flows:{total}}
   }

   // Everything else needs the detail document (host/data.lua uses flat dotted keys)
   json_t *detail = GetCachedHostDetail(credentials, pointId, hostKey);
   if (detail == nullptr)
      return DCE_NO_SUCH_INSTANCE;

   if (!wcscmp(metric, L"PacketsIn"))
      return FormatNumericField(detail, "packets.rcvd", value, size);
   if (!wcscmp(metric, L"PacketsOut"))
      return FormatNumericField(detail, "packets.sent", value, size);
   if (!wcscmp(metric, L"Alerts"))
   {
      json_t *field = FindField(detail, "num_alerts");
      if (field == nullptr)
         field = FindField(detail, "total_alerts");
      if (!json_is_integer(field) && !json_is_real(field))
         return DCE_COLLECTION_ERROR;
      nx_swprintf(value, size, L"%lld", static_cast<long long>(json_number_value(field)));
      return DCE_SUCCESS;
   }
   if (!wcscmp(metric, L"TcpRetransmits"))
   {
      // "tcpPacketStats.sent"/"tcpPacketStats.rcvd" are flat keys holding an object
      json_t *sent = json_object_get(detail, "tcpPacketStats.sent");
      json_t *rcvd = json_object_get(detail, "tcpPacketStats.rcvd");
      if ((sent == nullptr) && (rcvd == nullptr))
         return DCE_COLLECTION_ERROR;
      nx_swprintf(value, size, L"%lld",
         static_cast<long long>((sent != nullptr) ? json_object_get_int64(sent, "retransmissions", 0) : 0) +
         static_cast<long long>((rcvd != nullptr) ? json_object_get_int64(rcvd, "retransmissions", 0) : 0));
      return DCE_SUCCESS;
   }
   return DCE_NOT_SUPPORTED;
}

/**
 * Host level table collection
 */
DataCollectionError NtopngGetHostTable(const char *pointId, const char *hostKey, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials)
{
   if (!wcscmp(metric, L"L7Breakdown"))
   {
      LockGuard lockGuard(s_cacheLock);
      json_t *detail = GetCachedHostDetail(credentials, pointId, hostKey);
      if (detail == nullptr)
         return DCE_NO_SUCH_INSTANCE;

      json_t *ndpi = json_object_get(detail, "ndpi");
      if (!json_is_object(ndpi))
         return DCE_COLLECTION_ERROR;

      auto table = make_shared<Table>();
      table->addColumn(L"APPLICATION", DCI_DT_STRING, L"Application", true);
      table->addColumn(L"BYTES_SENT", DCI_DT_UINT64, L"Bytes sent");
      table->addColumn(L"BYTES_RCVD", DCI_DT_UINT64, L"Bytes received");
      table->addColumn(L"PACKETS", DCI_DT_UINT64, L"Packets");
      table->addColumn(L"FLOWS", DCI_DT_UINT64, L"Flows");
      table->addColumn(L"BREED", DCI_DT_STRING, L"Breed");

      const char *app;
      json_t *stats;
      void *it = json_object_iter(ndpi);
      while (it != nullptr)
      {
         app = json_object_iter_key(it);
         stats = json_object_iter_value(it);
         it = json_object_iter_next(ndpi, it);
         if (!json_is_object(stats))
            continue;
         table->addRow();
         table->set(0, app);
         table->set(1, json_object_get_uint64(stats, "bytes.sent", 0));
         table->set(2, json_object_get_uint64(stats, "bytes.rcvd", 0));
         table->set(3, json_object_get_uint64(stats, "packets.sent", 0) + json_object_get_uint64(stats, "packets.rcvd", 0));
         table->set(4, json_object_get_uint64(stats, "num_flows", 0));
         table->set(5, json_object_get_string_utf8(stats, "breed", ""));
      }
      *value = table;
      return DCE_SUCCESS;
   }

   if (!wcscmp(metric, L"Peers"))
   {
      auto table = make_shared<Table>();
      table->addColumn(L"PEER", DCI_DT_STRING, L"Peer", true);
      table->addColumn(L"VLAN", DCI_DT_INT, L"VLAN", true);
      table->addColumn(L"ROLE", DCI_DT_STRING, L"Role", true);
      table->addColumn(L"BYTES_SENT", DCI_DT_UINT64, L"Bytes sent");
      table->addColumn(L"BYTES_RCVD", DCI_DT_UINT64, L"Bytes received");
      table->addColumn(L"FLOWS", DCI_DT_UINT64, L"Flows");

      static const char *criteria[] = { "client", "server" };
      for (int c = 0; c < 2; c++)
      {
         char path[256];
         snprintf(path, sizeof(path), "/lua/rest/v2/get/flow/aggregated_live_flows.lua?ifid=%s&host=%s&aggregation_criteria=%s",
            pointId, hostKey, criteria[c]);
         json_t *response = NtopngApiGet(credentials, path);
         if (response == nullptr)
            continue;
         json_t *data = json_object_get(response, "rsp");
         if (json_is_object(data))
            data = json_object_get(data, "data");
         if (json_is_array(data))
         {
            size_t i;
            json_t *row;
            json_array_foreach(data, i, row)
            {
               // Peer identity is nested under the aggregation criteria key
               json_t *endpoint = json_object_get(row, criteria[c]);
               if (!json_is_object(endpoint))
                  continue;

               const char *ip = json_object_get_string_utf8(endpoint, "ip", "");
               int32_t vlan = json_object_get_int32(endpoint, "vlan_id", 0);
               char peerKey[128];
               snprintf(peerKey, sizeof(peerKey), "%s@%d", ip, vlan);
               if (!strcmp(peerKey, hostKey))
                  continue;   // aggregation row for the host itself, not a peer

               table->addRow();
               table->set(0, json_object_get_string_utf8(endpoint, "label", ip));
               table->set(1, vlan);
               table->set(2, criteria[c]);
               table->set(3, json_object_get_uint64(row, "bytes_sent", 0));
               table->set(4, json_object_get_uint64(row, "bytes_rcvd", 0));
               table->set(5, json_object_get_uint64(row, "num_flows", json_object_get_uint64(row, "flows", 0)));
            }
         }
         json_decref(response);
      }
      *value = table;
      return DCE_SUCCESS;
   }

   return DCE_NOT_SUPPORTED;
}

/* ------------------------------------------------------------------------- *
 *  Metric catalog                                                           *
 * ------------------------------------------------------------------------- */

/**
 * Static metric catalog entry
 */
struct MetricCatalogEntry
{
   const wchar_t *name;
   const wchar_t *displayName;
   const wchar_t *unit;
   const wchar_t *description;
   bool isTable;
   uint64_t requiredCapability;
};

static const MetricCatalogEntry s_observerCatalog[] =
{
   { L"Version", L"Version", L"", L"Backend software version", false, 0 },
   { L"Uptime", L"Uptime", L"s", L"Backend process uptime", false, 0 },
   { nullptr, nullptr, nullptr, nullptr, false, 0 }
};

static const MetricCatalogEntry s_pointCatalog[] =
{
   { L"ThroughputIn", L"Throughput (in)", L"bps", L"Download throughput relative to monitored network", false, 0 },
   { L"ThroughputOut", L"Throughput (out)", L"bps", L"Upload throughput relative to monitored network", false, 0 },
   { L"PacketsIn", L"Packets (in)", L"", L"Cumulative download packets", false, 0 },
   { L"PacketsOut", L"Packets (out)", L"", L"Cumulative upload packets", false, 0 },
   { L"BytesIn", L"Bytes (in)", L"B", L"Cumulative download bytes", false, 0 },
   { L"BytesOut", L"Bytes (out)", L"B", L"Cumulative upload bytes", false, 0 },
   { L"ActiveHosts", L"Active hosts", L"", L"Number of active hosts", false, 0 },
   { L"ActiveFlows", L"Active flows", L"", L"Number of active flows", false, 0 },
   { L"Drops", L"Drops", L"", L"Dropped packets", false, 0 },
   { L"TcpRetransmits", L"TCP retransmits", L"", L"TCP retransmissions", false, 0 },
   { L"L7Breakdown", L"L7 breakdown", L"", L"Per-application traffic breakdown", true, TRAFFIC_CAPABILITY_POINT_L7 },
   { L"TopTalkers", L"Top talkers", L"", L"Top hosts by traffic", true, TRAFFIC_CAPABILITY_POINT_TOP_TALKERS },
   { L"DSCPBreakdown", L"DSCP breakdown", L"", L"Traffic by DSCP class", true, TRAFFIC_CAPABILITY_POINT_DSCP },
   { nullptr, nullptr, nullptr, nullptr, false, 0 }
};

static const MetricCatalogEntry s_hostCatalog[] =
{
   { L"Host.BytesIn({instance})", L"Bytes (in)", L"B", L"Bytes received by host", false, 0 },
   { L"Host.BytesOut({instance})", L"Bytes (out)", L"B", L"Bytes sent by host", false, 0 },
   { L"Host.PacketsIn({instance})", L"Packets (in)", L"", L"Packets received by host", false, 0 },
   { L"Host.PacketsOut({instance})", L"Packets (out)", L"", L"Packets sent by host", false, 0 },
   { L"Host.ActiveFlows({instance})", L"Active flows", L"", L"Active flows for host", false, 0 },
   { L"Host.TcpRetransmits({instance})", L"TCP retransmits", L"", L"TCP retransmissions for host", false, 0 },
   { L"Host.Alerts({instance})", L"Alerts", L"", L"Engaged alerts for host", false, 0 },
   { L"Host.Present({instance})", L"Present", L"", L"1 if host is in the active table, 0 otherwise", false, TRAFFIC_CAPABILITY_HOST_SET_AUTHORITATIVE },
   { L"Host.L7Breakdown({instance})", L"L7 breakdown", L"", L"Per-application breakdown for host", true, TRAFFIC_CAPABILITY_HOST_L7 },
   { L"Host.Peers({instance})", L"Peers", L"", L"Top peers for host", true, TRAFFIC_CAPABILITY_HOST_PEERS },
   { nullptr, nullptr, nullptr, nullptr, false, 0 }
};

/**
 * Metric catalog for the DCI editor. Credentials are unused - the catalog is
 * static and the caller filters by the observer's capability set.
 */
ObjectArray<TrafficMetricDefinition> *NtopngGetMetricDefinitions(TrafficMetricLevel level, json_t *credentials)
{
   const MetricCatalogEntry *catalog;
   switch (level)
   {
      case TrafficMetricLevel::OBSERVER:
         catalog = s_observerCatalog;
         break;
      case TrafficMetricLevel::POINT:
         catalog = s_pointCatalog;
         break;
      case TrafficMetricLevel::HOST:
         catalog = s_hostCatalog;
         break;
      default:
         return nullptr;
   }

   auto result = new ObjectArray<TrafficMetricDefinition>(16, 16, Ownership::True);
   for (int i = 0; catalog[i].name != nullptr; i++)
   {
      auto def = new TrafficMetricDefinition();
      wcslcpy(def->name, catalog[i].name, MAX_PARAM_NAME);
      wcslcpy(def->displayName, catalog[i].displayName, 256);
      wcslcpy(def->unit, catalog[i].unit, 64);
      wcslcpy(def->description, catalog[i].description, 1024);
      def->isTable = catalog[i].isTable;
      def->requiredCapability = catalog[i].requiredCapability;
      result->add(def);
   }
   return result;
}

/**
 * Drop all cached host data
 */
/**
 * Remove all completed entries from a cache map. Entries with a fetch in flight
 * are left in place - GetCachedDocument holds borrowed pointers to them across
 * lock releases, so deleting them here would be a use-after-free.
 */
static void ClearCacheMap(StringObjectMap<JsonCacheEntry> *cache)
{
   StringList keys = cache->keys();
   for (int i = 0; i < keys.size(); i++)
   {
      JsonCacheEntry *entry = cache->get(keys.get(i));
      if ((entry != nullptr) && !entry->processing)
         cache->remove(keys.get(i));
   }
}

void NtopngClearCache()
{
   LockGuard lockGuard(s_cacheLock);
   ClearCacheMap(&s_interfaceDataCache);
   ClearCacheMap(&s_pointHostCache);
   ClearCacheMap(&s_hostDetailCache);
}
