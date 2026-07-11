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
** File: traffic-connector.h
**
**/

#ifndef _traffic_connector_h_
#define _traffic_connector_h_

/**
 * Status codes for traffic connector functions
 */
enum class TrafficConnectorStatus
{
   SUCCESS = 0,
   CONNECTOR_UNAVAILABLE = 1,
   NOT_IMPLEMENTED = 2,
   API_ERROR = 3,
   AUTH_ERROR = 4
};

/**
 * Metric level for metric catalog queries
 */
enum class TrafficMetricLevel
{
   OBSERVER = 0,
   POINT = 1,
   HOST = 2
};

/**
 * Traffic connector capabilities
 */
#define TRAFFIC_CAPABILITY_HOST_L7               _ULL(0x00000001)
#define TRAFFIC_CAPABILITY_HOST_PEERS            _ULL(0x00000002)
#define TRAFFIC_CAPABILITY_POINT_L7              _ULL(0x00000004)
#define TRAFFIC_CAPABILITY_POINT_TOP_TALKERS     _ULL(0x00000008)
#define TRAFFIC_CAPABILITY_POINT_DSCP            _ULL(0x00000010)
#define TRAFFIC_CAPABILITY_HISTORICAL_TIMESERIES _ULL(0x00000020)
#define TRAFFIC_CAPABILITY_SYNC_HOST_ALIASES     _ULL(0x00000040)
#define TRAFFIC_CAPABILITY_HOST_SET_AUTHORITATIVE _ULL(0x00000080)  // GetActiveHosts returns the complete active host set, not a top-N approximation

/**
 * Backend information filled by TestConnection
 */
struct TrafficBackendInfo
{
   char product[64];          // e.g. "ntopng"
   char version[64];          // e.g. "6.7.260504"
   char edition[64];          // e.g. "Community"
   uint64_t capabilities;     // TRAFFIC_CAPABILITY_* bitmask

   TrafficBackendInfo()
   {
      product[0] = 0;
      version[0] = 0;
      edition[0] = 0;
      capabilities = 0;
   }
};

/**
 * Observation point descriptor returned by DiscoverPoints
 */
struct ObservationPointDescriptor
{
   char externalId[128];      // connector-synthesized STABLE point identity (see contract below)
   char name[MAX_OBJECT_NAME];
   char type[32];             // "packet" / "zmq" / "pcap" / "view" / ...
   int16_t state;             // OBSERVATION_POINT_STATE_*
   char providerState[64];    // raw backend state string
   uint32_t samplingRate;     // 0 = unknown/not reported, 1 = unsampled, N = 1:N sampling
   StringList *localNetworks; // CIDRs reported by the backend (owned)
   ObservationPointDescriptor *next;   // next sibling (linked list, caller owns)

   // externalId contract: this is a durable identity the connector synthesizes, not
   // necessarily a backend-native handle. Point reconciliation and DCI instances key on
   // it, so it must survive backend restarts and renumbering (e.g. a flow collector
   // should compose exporter IP + ifName rather than use a raw ifIndex). Changing a
   // connector's composition rule is a breaking change for existing objects.

   ObservationPointDescriptor()
   {
      externalId[0] = 0;
      name[0] = 0;
      type[0] = 0;
      state = OBSERVATION_POINT_STATE_UNKNOWN;
      providerState[0] = 0;
      samplingRate = 0;
      localNetworks = nullptr;
      next = nullptr;
   }

   ~ObservationPointDescriptor()
   {
      delete localNetworks;
      delete next;
   }
};

/**
 * TrafficHostEntry flags
 */
#define TRAFFIC_HOST_LOCAL        0x0001
#define TRAFFIC_HOST_MULTICAST    0x0002
#define TRAFFIC_HOST_BROADCAST    0x0004
#define TRAFFIC_HOST_BLACKLISTED  0x0008

/**
 * Active host entry returned by GetActiveHosts
 */
struct TrafficHostEntry
{
   char hostKey[128];         // connector-defined opaque host identity, ntopng: "ip@vlan".
                              // The authoritative qualifier for host records and DCI instance
                              // keys - a connector encodes any extra identity dimensions
                              // (VLAN, VRF, tenant, ...) into it; core never parses it and
                              // matches on ipAddr only. Same stability contract as
                              // ObservationPointDescriptor::externalId.
   InetAddress ipAddr;
   int32_t vlan;
   MacAddress macAddr;
   wchar_t name[192];         // backend label (alias or resolved name)
   uint32_t flags;            // TRAFFIC_HOST_*
   time_t firstSeen;
   time_t lastSeen;

   TrafficHostEntry() : ipAddr(), macAddr()
   {
      hostKey[0] = 0;
      vlan = 0;
      name[0] = 0;
      flags = 0;
      firstSeen = 0;
      lastSeen = 0;
   }
};

/**
 * Metric definition returned by GetMetricDefinitions
 */
struct TrafficMetricDefinition
{
   wchar_t name[MAX_PARAM_NAME];
   wchar_t displayName[256];
   wchar_t unit[64];
   wchar_t description[1024];
   bool isTable;
   uint64_t requiredCapability;  // 0 = always available

   TrafficMetricDefinition()
   {
      name[0] = 0;
      displayName[0] = 0;
      unit[0] = 0;
      description[0] = 0;
      isTable = false;
      requiredCapability = 0;
   }
};

/**
 * Module interface for traffic connector
 */
struct TrafficConnectorInterface
{
   TrafficConnectorStatus (*Initialize)();   // cleanup, if needed, goes into the providing module's pfShutdown hook

   // Instance level
   TrafficConnectorStatus (*TestConnection)(json_t *credentials, TrafficBackendInfo *info);
   uint64_t (*GetCapabilities)(json_t *credentials);

   // Discovery
   ObservationPointDescriptor *(*DiscoverPoints)(json_t *credentials);   // linked list, caller owns

   // Observer / point level data
   DataCollectionError (*GetObserverMetric)(const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
   DataCollectionError (*GetPointMetric)(const char *pointId, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
   DataCollectionError (*GetPointTable)(const char *pointId, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials);

   // Host level (pointId + hostKey addressing); connector pages and caches internally
   ObjectArray<TrafficHostEntry> *(*GetActiveHosts)(const char *pointId, json_t *credentials);
   DataCollectionError (*GetHostMetric)(const char *pointId, const char *hostKey, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
   DataCollectionError (*GetHostTable)(const char *pointId, const char *hostKey, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials);

   // Metric catalog for the DCI editor
   ObjectArray<TrafficMetricDefinition> *(*GetMetricDefinitions)(TrafficMetricLevel level, json_t *credentials);

   // Push direction (may be null = unsupported). Deliberately the only sync surface in v1;
   // further surfaces (host pools, local networks) are added to the interface when a
   // connector that can actually serve them lands - no dead entry points.
   TrafficConnectorStatus (*SyncHostAliases)(const StringMap& aliases, json_t *credentials);   // hostKey -> name
};

/**
 * Find traffic connector by name
 */
TrafficConnectorInterface NXCORE_EXPORTABLE *FindTrafficConnector(const wchar_t *name);

/**
 * Get names of available traffic connectors
 */
StringList NXCORE_EXPORTABLE GetTrafficConnectorNames();

/**
 * Get error message for given status
 */
const wchar_t NXCORE_EXPORTABLE *GetTrafficConnectorErrorMessage(TrafficConnectorStatus status);

#endif   /* _traffic_connector_h_ */
