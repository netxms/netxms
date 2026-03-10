/*
** NetXMS - Network Management System
** Copyright (C) 2024-2026 Raden Solutions
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
** File: otlp.h
**
**/

#ifndef _otlp_h_
#define _otlp_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <netxms-webapi.h>
#include <map>
#include <unordered_map>

#define DEBUG_TAG_OTLP  _T("otlp")

/**
 * Node property used for matching
 */
enum OTLPNodeProperty
{
   OTLP_MATCH_NODE_NAME = 0,
   OTLP_MATCH_DNS_NAME = 1,
   OTLP_MATCH_IP_ADDRESS = 2,
   OTLP_MATCH_CUSTOM_ATTRIBUTE = 3
};

/**
 * Matching rule
 */
struct OTLPMatchingRule
{
   uint32_t id;
   int sequenceNumber;
   char otelAttribute[256];
   char regexTransform[256];
   OTLPNodeProperty nodeProperty;
   wchar_t customAttrName[128];
};

/**
 * Initialize matching engine (load rules from DB)
 */
void InitMatchingEngine();

/**
 * Shutdown matching engine
 */
void ShutdownMatchingEngine();

/**
 * Reload matching rules from database
 */
void ReloadMatchingRules();

/**
 * Match OTel resource attributes to a NetXMS node.
 * Takes a StringMap of resource attributes (key -> string value).
 * Returns matched node or empty shared_ptr if no match.
 */
shared_ptr<Node> MatchResourceToNode(const std::map<std::string, std::string>& attributes);

/**
 * Parsed OTLP instance discovery configuration
 */
struct OTLPDiscoveryConfig
{
   wchar_t metricPattern[512];     // Metric name glob pattern
   StringList keyAttributes;       // Attribute names forming instance key
   wchar_t nameFormat[512];        // Instance name format string (empty = use key)
};

/**
 * Initialize instance discovery engine
 */
void InitInstanceDiscovery();

/**
 * Shutdown instance discovery engine
 */
void ShutdownInstanceDiscovery();

/**
 * Resolve an OTel data point to an instance DCI.
 * Finds or creates the appropriate instance DCI based on metric name and data point attributes.
 * Returns the DCI to push the value to, or nullptr if no matching IDM_OTLP prototype.
 */
shared_ptr<DCObject> ResolveOTelInstance(const shared_ptr<Node>& node,
   const char *metricName,
   const std::map<std::string, std::string>& dpAttributes);

/**
 * Invalidate instance cache entries for a given node
 */
void InvalidateInstanceCache(uint32_t nodeId);

/**
 * Shutdown counter state tracking
 */
void ShutdownCounterState();

/**
 * Handler for OTLP metrics endpoint
 */
int H_OtlpMetrics(Context *context);

#endif
