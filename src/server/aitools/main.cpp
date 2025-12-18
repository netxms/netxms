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
** File: main.cpp
**
**/

#include "aitools.h"
#include <netxms-version.h>

std::string F_AlarmList(json_t *arguments, uint32_t userId);
std::string F_ClearNotificationChannelQueue(json_t *arguments, uint32_t userId);
std::string F_CreateMetric(json_t *arguments, uint32_t userId);
std::string F_EndMaintenance(json_t *arguments, uint32_t userId);
std::string F_FindObjects(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingAction(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingActions(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingPolicy(json_t *arguments, uint32_t userId);
std::string F_GetEventTemplate(json_t *arguments, uint32_t userId);
std::string F_GetHistoricalData(json_t *arguments, uint32_t userId);
std::string F_GetMetrics(json_t *arguments, uint32_t userId);
std::string F_GetNodeHardwareComponents(json_t *arguments, uint32_t userId);
std::string F_GetNodeInterfaces(json_t *arguments, uint32_t userId);
std::string F_GetNodeSoftwarePackages(json_t *arguments, uint32_t userId);
std::string F_GetNotificationChannels(json_t *arguments, uint32_t userId);
std::string F_GetObject(json_t *arguments, uint32_t userId);
std::string F_SendNotification(json_t *arguments, uint32_t userId);
std::string F_SNMPWalk(json_t *arguments, uint32_t userId);
std::string F_SNMPRead(json_t *arguments, uint32_t userId);
std::string F_StartMaintenance(json_t *arguments, uint32_t userId);

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("AITOOLS", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Create function list
 */
static void CreateAssistantFunctionList()
{
   RegisterAIAssistantFunction(
      "alarm-list",
      "Get list of active (outstanding) alarms (alerts) for monitored devices (each alarm represents outstanding problem or condition worth operator attention). Device in this context could mean anything related to IT infrastructure: switch, router, server, workstation, ATM, POS, terminal, printer, scanner, UPS, sensor of any kind.",
      {
         { "object", "optional name or ID of node, device, server, workstation, or container or group of them" }
      },
      F_AlarmList);
   RegisterAIAssistantFunction(
      "clear-notification-queue",
      "Clear notification channel's queue",
      {
         { "channel", "notification channel name (mandatory)" }
      },
      F_ClearNotificationChannelQueue);
   RegisterAIAssistantFunction(
      "find-objects",
      "Find objects by name, IP address, class, parent object, or zone. At least one search criteria should be set.",
      {
         { "name", "optional part of object name or alias" },
         { "ipAddress", "optional IP address assigned to object" },
         { "classes", "optional array of object classes to search (node, interface, container, etc.)" },
         { "parentId", "optional ID of parent object (container or group)" },
         { "zoneUIN", "optional zone UIN" }
      },
      F_FindObjects);
   RegisterAIAssistantFunction(
      "get-notification-channels",
      "Get list of configured notification channels",
      {},
      F_GetNotificationChannels);
   RegisterAIAssistantFunction(
      "get-object",
      "Find object by its name or ID",
      {
         { "object", "mandatory name or ID of an object (node, device, server, workstation, container, etc.)" }
      },
      F_GetObject);
   RegisterAIAssistantFunction(
      "send-notification",
      "Send notification to given recipient using one of configured notification channels.",
      {
         { "channel", "notification channel name (mandatory)" },
         { "message", "notification message text (mandatory)" },
         { "recipient", "recipient address (mandatory)" },
         { "subject", "notification subject (optional)" }
      },
      F_SendNotification);
   RegisterAIAssistantFunction(
      "server-stats",
      "Get stats (basic internal performance metrics like number of objects, nodes, sensors, interfaces, metrics, data collection items) of NetXMS server itself.",
      {},
      [] (json_t *arguments, uint32_t userId) -> std::string
      {
         return JsonToString(GetServerStats());
      });
   RegisterAIAssistantFunction(
      "snmp-read",
      "Read SNMP variable from given node",
      {
         { "object", "name or ID of a node" },
         { "oid", "SNMP OID of the variable to read" }
      },
      F_SNMPRead);
   RegisterAIAssistantFunction(
      "snmp-walk",
      "Walk SNMP MIB from given node",
      {
          { "object", "name or ID of a node" },
          { "oid", "SNMP OID to start walk from" },
          { "limit", "maximum number of variables to return (default: 1000)" }
      },
      F_SNMPWalk);
}

/**
 * Create skill list
 */
static void CreateAssistantSkillList()
{
   RegisterAIAssistantSkill(
      "data-collection",
      "Provides comprehensive data collection management capabilities for NetXMS monitored infrastructure. Use this skill to create new metrics, monitor current values, analyze historical data, detect trends and anomalies, and optimize performance monitoring across your IT environment.",
      "@data-collection.md",
      {
         AssistantFunction(
            "create-metric",
            "Create new data collection item (metric) for given object (node, device, server, etc.).",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name of the metric to create (mandatory, meaning depends on the origin: metric name for agent, OID for SNMP, script name for script)" },
               { "description", "optional metric description, if not provided then name will be used" },
               { "origin", "data collection origin: agent, snmp, or script (default: agent)" },
               { "dataType", "data type: int, unsigned-int, int64, unsigned-int64, counter32, counter64, float, string (default: string)" }
            },
            F_CreateMetric),
         AssistantFunction(
            "get-metrics",
            "Get data collection items (metrics) and their current values for given object",
            {
               { "object", "name or ID of an object (node, device, server, workstation, container, etc.)" },
               { "filter", "optional filter to select specific metrics by name (partial names allowed)" }
            },
            F_GetMetrics),
         AssistantFunction(
            "get-historical-data",
            "Get historical time series data for analysis and anomaly detection. Returns data points with timestamps and values for specified time period.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name of the metric/DCI to analyze (mandatory)" },
               { "timeFrom", "start time (ISO format or negative number of minutes, like '-60' for an our ago)" },
               { "timeTo", "end time (optional, ISO format, defaults to now)" }
            },
            F_GetHistoricalData)
      }
   );

   RegisterAIAssistantSkill(
      "event-processing",
      "Provides comprehensive event processing management capabilities for NetXMS. Use this skill to manage event templates, event processing policy (EPP), server-side actions, and understand how events flow through the system. Events are generated by various sources (monitoring, SNMP traps, syslog, agents) and processed according to configured rules to generate alarms, send notifications, execute actions, or correlate related events. Server-side actions enable automated responses including: executing commands on management server or remote nodes, sending multi-channel notifications (SMS, email, Teams, etc.), running NXSL scripts for custom logic, and forwarding events to other NetXMS servers for distributed monitoring.",
      "@event-processing.md",
      {
         AssistantFunction(
            "get-event-processing-action",
            "Get specific server-side action used in event processing by its ID. Server-side actions define operations that can be executed in response to events, such as sending notifications, executing commands, or running scripts.",
            {
               { "id", "action ID (numeric)" }
            },
            F_GetEventProcessingAction),
         AssistantFunction(
            "get-event-processing-actions",
            "Get list of server-side actions used in event processing. Server-side actions define operations that can be executed in response to events, such as sending notifications, executing commands, or running scripts.",
            {},
            F_GetEventProcessingActions),
         AssistantFunction(
            "get-event-template",
            "Get event template definition by code or name. Event templates define the structure and properties of events that can be generated in NetXMS.",
            {
               { "code", "event template code (numeric ID)" },
               { "name", "event template name (either code or name must be provided)" }
            },
            F_GetEventTemplate),
         AssistantFunction(
            "get-event-processing-policy",
            "Get the complete event processing policy configuration. The policy defines rules for how events are processed, including conditions, actions, and correlations.",
            {},
            F_GetEventProcessingPolicy)
      }
   );

   RegisterAIAssistantSkill(
      "inventory",
      "Provides comprehensive inventory management capabilities for NetXMS monitored nodes, including hardware components, software packages, and network interfaces. Use this skill for asset discovery, compliance auditing, security assessment, change management, capacity planning, and troubleshooting system configurations.",
      "@inventory.md",
      {
         AssistantFunction(
            "get-node-hardware-components",
            "Get list of hardware components for given node (servers, workstations, network devices, etc.). Returns information about physical components like CPUs, memory modules, storage devices, power supplies, fans, etc.",
            {
               { "object", "name or ID of a node" }
            },
            F_GetNodeHardwareComponents),
         AssistantFunction(
            "get-node-interfaces",
            "Get list of network interfaces for given node",
            {
               { "object", "name or ID of a node" }
            },
            F_GetNodeInterfaces),
         AssistantFunction(
            "get-node-software-packages",
            "Get list of software packages for given node (servers, workstations, network devices, etc.). Returns information about installed software packages including name, version, vendor, installation date, etc.",
            {
               { "object", "name or ID of a node" },
               { "filter", "optional filter to select specific packages by name or description (partial names allowed)" }
            },
            F_GetNodeSoftwarePackages)
      }
   );

   RegisterAIAssistantSkill(
      "maintenance",
      "Provides comprehensive maintenance management capabilities for NetXMS monitored infrastructure. Use this skill to manage planned maintenance windows, suppress alerts during maintenance activities, and ensure proper coordination of maintenance operations across your IT environment.",
      "@maintenance.md",
      {
         AssistantFunction(
            "start-maintenance",
            "Start maintenance period for given object (node, device, server, workstation, container, etc.). While in maintenance mode, object will be monitored but alerts and notifications will be suppressed.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "message", "optional maintenance message" }
            },
            F_StartMaintenance),
         AssistantFunction(
            "end-maintenance",
            "End maintenance period for given object (node, device, server, workstation, container, etc.).",
            {
               { "object", "name or ID of an object (mandatory)" }
            },
            F_EndMaintenance)
      }
   );
}

/**
 * Module initialization
 */
static bool InitializeModule(Config *config)
{
   wchar_t path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   wcslcat(path, L"/aitools/prompt-overview.md", MAX_PATH);
   char *prompt = LoadFileAsUTF8String(path);
   if (prompt != nullptr)
   {
      AddAIAssistantPrompt(prompt);
      MemFree(prompt);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Cannot load AI assistant prompt file \"%s\"", path);
   }

   CreateAssistantFunctionList();
   CreateAssistantSkillList();
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"AI assistant tools module version " NETXMS_VERSION_STRING L" initialized");
   return true;
}

/**
 * Module registration
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"AITOOLS");
   module->pfInitialize = InitializeModule;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
