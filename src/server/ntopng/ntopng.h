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
** File: ntopng.h
**/

#ifndef _ntopng_h_
#define _ntopng_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <traffic-connector.h>
#include <nxlibcurl.h>

#define DEBUG_TAG L"ntopng"

/**
 * Execute GET request against ntopng REST API v2. Returns parsed response document
 * (envelope with "rc"/"rsp"; caller owns the reference) or nullptr on failure.
 * Path must start with "/" and include query string if needed. Optional status
 * output receives detailed error classification.
 *
 * Note: authentication failures and unknown endpoints are both reported by ntopng
 * as a 302 redirect to the login page - both map to AUTH_ERROR here; TestConnection
 * probes a known-good endpoint, so unknown-endpoint 302s only occur on version drift.
 */
json_t *NtopngApiGet(json_t *credentials, const char *path, TrafficConnectorStatus *status = nullptr);

/**
 * Execute POST request with JSON body against ntopng REST API v2. Same response
 * envelope handling and error classification as NtopngApiGet.
 */
json_t *NtopngApiPost(json_t *credentials, const char *path, json_t *body, TrafficConnectorStatus *status = nullptr);

/**
 * Traffic connector interface functions
 */
TrafficConnectorStatus NtopngTestConnection(json_t *credentials, TrafficBackendInfo *info);
uint64_t NtopngGetCapabilities(json_t *credentials);
ObservationPointDescriptor *NtopngDiscoverPoints(json_t *credentials);
DataCollectionError NtopngGetObserverMetric(const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
DataCollectionError NtopngGetPointMetric(const char *pointId, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
DataCollectionError NtopngGetPointTable(const char *pointId, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials);
ObjectArray<TrafficHostEntry> *NtopngGetActiveHosts(const char *pointId, json_t *credentials);
DataCollectionError NtopngGetHostMetric(const char *pointId, const char *hostKey, const wchar_t *metric, wchar_t *value, size_t size, json_t *credentials);
DataCollectionError NtopngGetHostTable(const char *pointId, const char *hostKey, const wchar_t *metric, shared_ptr<Table> *value, json_t *credentials);
ObjectArray<TrafficMetricDefinition> *NtopngGetMetricDefinitions(TrafficMetricLevel level, json_t *credentials);
TrafficConnectorStatus NtopngSyncHostAliases(const StringMap& aliases, json_t *credentials);

/**
 * Drop all cached host data (called on connector shutdown)
 */
void NtopngClearCache();

#endif
