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
** File: cloud-connector.h
**
**/

#ifndef _cloud_connector_h_
#define _cloud_connector_h_

/**
 * Status codes for cloud connector functions
 */
enum class CloudConnectorStatus
{
   SUCCESS = 0,
   CONNECTOR_UNAVAILABLE = 1,
   NOT_IMPLEMENTED = 2,
   API_ERROR = 3,
   AUTH_ERROR = 4
};

/**
 * Dimension definition for a metric
 */
struct DimensionDefinition
{
   TCHAR name[256];
   TCHAR displayName[256];
};

/**
 * Metric definition returned by GetMetricDefinitions
 */
struct MetricDefinition
{
   TCHAR name[MAX_PARAM_NAME];         // Metric name (e.g. "Percentage CPU")
   TCHAR displayName[256];             // UI display name
   TCHAR unit[64];                     // Unit (Percent, Bytes, Count, etc.)
   TCHAR description[1024];            // Provider description
   uint32_t supportedAggregations;     // Bitmask of METRIC_AGGREGATION_* flags
   int16_t defaultAggregation;         // METRIC_AGGREGATION_* value
   uint32_t minInterval;               // Min collection interval in seconds
   ObjectArray<DimensionDefinition> *dimensions; // nullptr = scalar metric

   MetricDefinition()
   {
      name[0] = 0;
      displayName[0] = 0;
      unit[0] = 0;
      description[0] = 0;
      supportedAggregations = 0;
      defaultAggregation = METRIC_AGGREGATION_AVERAGE;
      minInterval = 300;
      dimensions = nullptr;
   }

   ~MetricDefinition()
   {
      delete dimensions;
   }
};

/**
 * Resource descriptor returned by Discover
 */
struct ResourceDescriptor
{
   char resourceId[1024];              // Provider's unique resource ID
   char name[MAX_OBJECT_NAME];         // Display name
   char type[256];                     // Provider-specific type
   char region[128];                   // Region/location
   int16_t state;                      // RESOURCE_STATE_* value
   char providerState[256];            // Raw provider state string
   StringMap *tags;                    // Key-value tag pairs (caller owns)
   char linkHint[256];                 // Hostname/IP/FQDN for Node matching
   ResourceDescriptor *children;       // First child (linked list via next)
   ResourceDescriptor *next;           // Next sibling

   ResourceDescriptor()
   {
      resourceId[0] = 0;
      name[0] = 0;
      type[0] = 0;
      region[0] = 0;
      state = RESOURCE_STATE_UNKNOWN;
      providerState[0] = 0;
      tags = nullptr;
      linkHint[0] = 0;
      children = nullptr;
      next = nullptr;
   }

   ~ResourceDescriptor()
   {
      delete tags;
      delete children;
      delete next;
   }
};

/**
 * Module interface for cloud connector
 */
struct CloudConnectorInterface
{
   CloudConnectorStatus (*Initialize)();
   void (*Shutdown)();

   ResourceDescriptor *(*Discover)(
      json_t *credentials,
      const TCHAR *filter);

   ObjectArray<MetricDefinition> *(*GetMetricDefinitions)(
      const TCHAR *resourceType,
      const TCHAR *resourceId,
      json_t *credentials);

   DataCollectionError (*Collect)(
      const TCHAR *resourceId,
      const TCHAR *metric,
      int16_t aggregation,
      TCHAR *value,
      size_t bufLen,
      json_t *credentials);

   DataCollectionError (*CollectTable)(
      const TCHAR *resourceId,
      const TCHAR *metric,
      int16_t aggregation,
      shared_ptr<Table> *value,
      json_t *credentials);

   int16_t (*QueryState)(
      const TCHAR *resourceId,
      char *providerState,
      size_t bufLen,
      json_t *credentials);

   // Optional batch collection (nullptr if not supported)
   DataCollectionError (*BatchCollect)(
      const ObjectArray<TCHAR> *resourceIds,
      const ObjectArray<TCHAR> *metrics,
      const IntegerArray<int16_t> *aggregations,
      StringList *values,
      json_t *credentials);
};

/**
 * Find cloud connector by name
 */
CloudConnectorInterface NXCORE_EXPORTABLE *FindCloudConnector(const wchar_t *name);

/**
 * Get error message for given status
 */
const wchar_t NXCORE_EXPORTABLE *GetCloudConnectorErrorMessage(CloudConnectorStatus status);

#endif   /* _cloud_connector_h_ */
