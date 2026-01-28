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
** File: main.cpp
**
**/

#include "aitools.h"
#include <netxms-version.h>

std::string F_AddIncidentComment(json_t *arguments, uint32_t userId);
std::string F_AlarmList(json_t *arguments, uint32_t userId);
std::string F_AssignIncident(json_t *arguments, uint32_t userId);
std::string F_ClearNotificationChannelQueue(json_t *arguments, uint32_t userId);
std::string F_CreateIncident(json_t *arguments, uint32_t userId);
std::string F_CreateIncidentFromAlarms(json_t *arguments, uint32_t userId);
std::string F_CreateMetric(json_t *arguments, uint32_t userId);
std::string F_EditMetric(json_t *arguments, uint32_t userId);
std::string F_DeleteMetric(json_t *arguments, uint32_t userId);
std::string F_GetThresholds(json_t *arguments, uint32_t userId);
std::string F_AddThreshold(json_t *arguments, uint32_t userId);
std::string F_DeleteThreshold(json_t *arguments, uint32_t userId);
std::string F_EndMaintenance(json_t *arguments, uint32_t userId);
std::string F_FindObjects(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingAction(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingActions(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingPolicy(json_t *arguments, uint32_t userId);
std::string F_GetEventTemplate(json_t *arguments, uint32_t userId);
std::string F_GetHistoricalData(json_t *arguments, uint32_t userId);
std::string F_GetIncidentDetails(json_t *arguments, uint32_t userId);
std::string F_GetIncidentHistory(json_t *arguments, uint32_t userId);
std::string F_GetIncidentRelatedEvents(json_t *arguments, uint32_t userId);
std::string F_GetIncidentTopologyContext(json_t *arguments, uint32_t userId);
std::string F_GetOpenIncidents(json_t *arguments, uint32_t userId);
std::string F_LinkAlarmToIncident(json_t *arguments, uint32_t userId);
std::string F_LinkAlarmsToIncident(json_t *arguments, uint32_t userId);
std::string F_GetMetrics(json_t *arguments, uint32_t userId);
std::string F_GetNodeHardwareComponents(json_t *arguments, uint32_t userId);
std::string F_GetNodeInterfaces(json_t *arguments, uint32_t userId);
std::string F_GetNodeSoftwarePackages(json_t *arguments, uint32_t userId);
std::string F_GetNotificationChannels(json_t *arguments, uint32_t userId);
std::string F_GetObject(json_t *arguments, uint32_t userId);
std::string F_GetObjectAIData(json_t *arguments, uint32_t userId);
std::string F_ListObjectAIDataKeys(json_t *arguments, uint32_t userId);
std::string F_RemoveObjectAIData(json_t *arguments, uint32_t userId);
std::string F_SendNotification(json_t *arguments, uint32_t userId);
std::string F_SetObjectAIData(json_t *arguments, uint32_t userId);
std::string F_SNMPWalk(json_t *arguments, uint32_t userId);
std::string F_SNMPRead(json_t *arguments, uint32_t userId);
std::string F_StartMaintenance(json_t *arguments, uint32_t userId);
std::string F_SuggestIncidentAssignee(json_t *arguments, uint32_t userId);
std::string F_ListLogs(json_t *arguments, uint32_t userId);
std::string F_GetLogSchema(json_t *arguments, uint32_t userId);
std::string F_SearchLog(json_t *arguments, uint32_t userId);
std::string F_SearchSyslog(json_t *arguments, uint32_t userId);
std::string F_SearchWindowsEvents(json_t *arguments, uint32_t userId);
std::string F_SearchSnmpTraps(json_t *arguments, uint32_t userId);
std::string F_SearchEvents(json_t *arguments, uint32_t userId);
std::string F_GetLogStatistics(json_t *arguments, uint32_t userId);
std::string F_CorrelateLogs(json_t *arguments, uint32_t userId);
std::string F_AnalyzeLogPatterns(json_t *arguments, uint32_t userId);

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
      "get-object-ai-data",
      "Retrieve AI agent data stored for an object. Can get specific data by key or all data if no key specified.",
      {
         { "object", "mandatory name or ID of an object" },
         { "key", "optional key to retrieve specific data (if not specified, returns all AI data)" }
      },
      F_GetObjectAIData);
   RegisterAIAssistantFunction(
      "list-object-ai-data-keys",
      "List all AI data keys available for an object",
      {
         { "object", "mandatory name or ID of an object" }
      },
      F_ListObjectAIDataKeys);
   RegisterAIAssistantFunction(
      "remove-object-ai-data",
      "Remove AI agent data for an object by key",
      {
         { "object", "mandatory name or ID of an object" },
         { "key", "mandatory key of the data to remove" }
      },
      F_RemoveObjectAIData);
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
      "set-object-ai-data",
      "Store AI agent data for an object with a specified key-value pair. Use this to remember context, analysis results, or other AI-specific information about objects.",
      {
         { "object", "mandatory name or ID of an object" },
         { "key", "mandatory key for the data (e.g., 'analysis_history', 'anomaly_patterns', 'user_notes')" },
         { "value", "mandatory value to store (can be string, number, object, or array)" }
      },
      F_SetObjectAIData);
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
      "Provides comprehensive data collection management capabilities for NetXMS monitored infrastructure. Use this skill to create, edit, and delete metrics, configure thresholds, monitor current values, analyze performance data, analyze historical data, detect trends and anomalies, and optimize performance monitoring across your IT environment. Essential for performance analysis, capacity planning, resource utilization monitoring, and troubleshooting performance issues.",
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
               { "dataType", "data type: int, unsigned-int, int64, unsigned-int64, counter32, counter64, float, string (default: string)" },
               { "pollingInterval", "polling interval in seconds (optional, uses system default if not specified)" },
               { "retentionTime", "retention time in days (optional, uses system default if not specified)" },
               { "deltaCalculation", "delta calculation: original, delta, averagePerSecond, averagePerMinute (default: original)" },
               { "sampleCount", "number of samples for threshold functions (default: 1)" },
               { "multiplier", "value multiplier (default: 0)" },
               { "unitName", "unit name for display (optional)" },
               { "status", "status: active, disabled (default: active)" },
               { "anomalyDetection", "anomaly detection: none, iforest, ai, both (default: none)" }
            },
            F_CreateMetric),
         AssistantFunction(
            "edit-metric",
            "Edit existing data collection item (metric) properties.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric to edit (mandatory)" },
               { "pollingInterval", "new polling interval in seconds (optional)" },
               { "retentionTime", "new retention time in days (optional)" },
               { "status", "new status: active, disabled (optional)" },
               { "unitName", "new unit name for display (optional)" }
            },
            F_EditMetric),
         AssistantFunction(
            "delete-metric",
            "Delete a data collection item (metric) and its historical data.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric to delete (mandatory)" }
            },
            F_DeleteMetric),
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
            F_GetHistoricalData),
         AssistantFunction(
            "get-thresholds",
            "Get all thresholds configured on a data collection item (metric).",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric (mandatory)" }
            },
            F_GetThresholds),
         AssistantFunction(
            "add-threshold",
            "Add a threshold to a data collection item for alerting when values exceed limits.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric (mandatory)" },
               { "value", "threshold value (mandatory)" },
               { "function", "function: last, average, sum, meanDeviation, diff, error, script, absDeviation, anomaly (default: last)" },
               { "operation", "operation: less, lessOrEqual, equal, greaterOrEqual, greater, notEqual, like, notLike (default: greaterOrEqual)" },
               { "sampleCount", "sample count for function (default: 1)" },
               { "activationEvent", "event code or name when threshold activates (default: SYS_THRESHOLD_REACHED)" },
               { "deactivationEvent", "event code or name when threshold deactivates (default: SYS_THRESHOLD_REARMED)" },
               { "repeatInterval", "repeat interval in seconds, -1=default, 0=off (default: -1)" },
               { "script", "NXSL script for script function (optional)" }
            },
            F_AddThreshold),
         AssistantFunction(
            "delete-threshold",
            "Delete a threshold from a data collection item.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric (mandatory)" },
               { "thresholdId", "threshold ID to delete (mandatory)" }
            },
            F_DeleteThreshold)
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

   RegisterAIAssistantSkill(
      "incident-analysis",
      "Provides comprehensive incident management and analysis capabilities for NetXMS. Use this skill to investigate incidents, analyze root causes, correlate related alarms, review historical incidents, assess topology impact, and manage incident lifecycle including comments and assignments.",
      "@incident-analysis.md",
      {
         AssistantFunction(
            "get-incident-details",
            "Get full details of an incident including linked alarms, source object, comments, and state information.",
            {
               { "incident_id", "ID of the incident (mandatory)" }
            },
            F_GetIncidentDetails),
         AssistantFunction(
            "get-incident-related-events",
            "Get events related to an incident within a time window around its creation for correlation analysis.",
            {
               { "incident_id", "ID of the incident (mandatory)" },
               { "time_window", "Time window in seconds before and after incident creation (default: 3600)" }
            },
            F_GetIncidentRelatedEvents),
         AssistantFunction(
            "get-incident-history",
            "Get historical incidents for a specific object to identify recurring issues and patterns.",
            {
               { "object", "Name or ID of the object to get incident history for (mandatory)" },
               { "max_count", "Maximum number of historical incidents to return (default: 20)" }
            },
            F_GetIncidentHistory),
         AssistantFunction(
            "get-incident-topology-context",
            "Get topology context for an incident including upstream and downstream objects with their current status.",
            {
               { "incident_id", "ID of the incident (mandatory)" }
            },
            F_GetIncidentTopologyContext),
         AssistantFunction(
            "add-incident-comment",
            "Add a comment to an incident for documenting analysis findings, updates, or notes.",
            {
               { "incident_id", "ID of the incident (mandatory)" },
               { "text", "Comment text to add (mandatory)" }
            },
            F_AddIncidentComment),
         AssistantFunction(
            "link-alarm-to-incident",
            "Link a single alarm to an incident. Alarms can only be linked to one incident at a time.",
            {
               { "incident_id", "ID of the incident (mandatory)" },
               { "alarm_id", "ID of the alarm to link (mandatory)" }
            },
            F_LinkAlarmToIncident),
         AssistantFunction(
            "link-alarms-to-incident",
            "Link multiple alarms to an incident in a single operation.",
            {
               { "incident_id", "ID of the incident (mandatory)" },
               { "alarm_ids", "JSON array of alarm IDs to link (mandatory)" }
            },
            F_LinkAlarmsToIncident),
         AssistantFunction(
            "assign-incident",
            "Assign an incident to a specific user by user ID or username.",
            {
               { "incident_id", "ID of the incident (mandatory)" },
               { "user_id", "ID of the user to assign to (provide either user_id or user_name)" },
               { "user_name", "Username to assign to (provide either user_id or user_name)" }
            },
            F_AssignIncident),
         AssistantFunction(
            "suggest-incident-assignee",
            "Get AI suggestion for incident assignment based on responsible users configured on the source object.",
            {
               { "incident_id", "ID of the incident (mandatory)" }
            },
            F_SuggestIncidentAssignee),
         AssistantFunction(
            "get-open-incidents",
            "Get all open (not closed) incidents for a specific object. Returns incidents in states: Open, In Progress, Blocked, or Resolved.",
            {
               { "object", "Name or ID of the object to get open incidents for (mandatory)" }
            },
            F_GetOpenIncidents),
         AssistantFunction(
            "create-incident",
            "Create a new incident for tracking an issue. Use when detecting patterns, anomalies, or issues that warrant unified tracking.",
            {
               { "title", "Incident title (mandatory)" },
               { "source_object", "Name or ID of the source object (mandatory)" },
               { "initial_comment", "Initial comment describing the issue (optional)" },
               { "source_alarm_id", "ID of an alarm to link (optional)" }
            },
            F_CreateIncident),
         AssistantFunction(
            "create-incident-from-alarms",
            "Create an incident linking multiple related alarms. Use when multiple alarms stem from a common root cause.",
            {
               { "title", "Incident title (mandatory)" },
               { "alarm_ids", "JSON array of alarm IDs to link (mandatory)" },
               { "initial_comment", "Initial comment describing the correlation (optional)" }
            },
            F_CreateIncidentFromAlarms)
      }
   );

   RegisterAIAssistantSkill(
      "log-analysis",
      "Provides comprehensive log analysis capabilities for NetXMS. Use this skill to search and analyze syslog messages, Windows events, SNMP traps, system events, and custom log tables from server modules. Essential for troubleshooting, root cause analysis, security investigation, compliance auditing, and detecting patterns or anomalies in log data across monitored infrastructure.",
      "@log-analysis.md",
      {
         AssistantFunction(
            "list-logs",
            "List all available logs that the user has permission to access. Use this to discover available log sources including custom logs from server modules.",
            {},
            F_ListLogs),
         AssistantFunction(
            "get-log-schema",
            "Get detailed schema information for a specific log including column names, types, and descriptions.",
            {
               { "log_name", "name of the log to get schema for (required)" }
            },
            F_GetLogSchema),
         AssistantFunction(
            "search-log",
            "Search any log table with flexible filtering. Works with built-in logs and custom module logs.",
            {
               { "log_name", "name of the log to search (required)" },
               { "object", "optional object name or ID to filter by source" },
               { "time_from", "start time (ISO format or relative like '-60m', default: -60m)" },
               { "time_to", "end time (ISO format, default: now)" },
               { "text_pattern", "case-insensitive text search pattern across text columns" },
               { "filters", "optional JSON object with column-specific filters" },
               { "limit", "max results (default: 100, max: 1000)" }
            },
            F_SearchLog),
         AssistantFunction(
            "search-syslog",
            "Search syslog messages with flexible filtering by time, source, severity, facility, and text content.",
            {
               { "object", "optional node name or ID to filter by source" },
               { "time_from", "start time (ISO format or relative like '-60m' for 60 min ago, '-1h' for 1 hour ago)" },
               { "time_to", "end time (ISO format, defaults to now)" },
               { "severity", "syslog severity (0-7) or array of severities" },
               { "facility", "syslog facility (0-23) or array" },
               { "text_pattern", "case-insensitive text search pattern" },
               { "tag", "syslog tag filter" },
               { "limit", "max results (default: 100, max: 1000)" }
            },
            F_SearchSyslog),
         AssistantFunction(
            "search-windows-events",
            "Search Windows event log entries collected from monitored Windows hosts.",
            {
               { "object", "optional node name or ID" },
               { "time_from", "start time (default: -60m)" },
               { "time_to", "end time (default: now)" },
               { "log_name", "log name (System, Application, Security, etc.)" },
               { "event_source", "event source filter" },
               { "event_code", "event ID or array of event IDs" },
               { "severity", "severity (1=Error, 2=Warning, 4=Info, 8=AuditSuccess, 16=AuditFailure)" },
               { "text_pattern", "text search pattern" },
               { "limit", "max results (default: 100)" }
            },
            F_SearchWindowsEvents),
         AssistantFunction(
            "search-snmp-traps",
            "Search SNMP traps received from network devices.",
            {
               { "object", "optional node name or ID" },
               { "time_from", "start time (default: -60m)" },
               { "time_to", "end time (default: now)" },
               { "trap_oid", "trap OID or OID prefix filter" },
               { "ip_address", "source IP address filter" },
               { "varbind_pattern", "text search in varbind data" },
               { "limit", "max results (default: 100)" }
            },
            F_SearchSnmpTraps),
         AssistantFunction(
            "search-events",
            "Search NetXMS system events generated by thresholds, polling, and scripts.",
            {
               { "object", "optional source object name or ID" },
               { "time_from", "start time (default: -60m)" },
               { "time_to", "end time (default: now)" },
               { "event_code", "event code or name" },
               { "severity", "severity (0=Normal to 4=Critical) or array" },
               { "origin", "event origin (SYSTEM, SNMP, SYSLOG, AGENT, SCRIPT)" },
               { "tags", "event tags filter" },
               { "text_pattern", "text search in message" },
               { "limit", "max results (default: 100)" }
            },
            F_SearchEvents),
         AssistantFunction(
            "get-log-statistics",
            "Get log volume statistics for capacity planning and trend analysis.",
            {
               { "log_type", "log type: syslog, windows_events, snmp_traps, events (required)" },
               { "object", "optional object to filter" },
               { "time_from", "start time (default: -24h)" },
               { "time_to", "end time (default: now)" },
               { "group_by", "grouping: hour, day, source, severity (default: hour)" }
            },
            F_GetLogStatistics),
         AssistantFunction(
            "correlate-logs",
            "Cross-correlate log entries across multiple sources within a time window for root cause analysis.",
            {
               { "object", "primary object for correlation (required)" },
               { "time_from", "correlation window start (required)" },
               { "time_to", "correlation window end (required)" },
               { "include_neighbors", "include L2 neighbors (default: true)" },
               { "log_types", "array of log types to include (default: all)" },
               { "limit_per_source", "max entries per source (default: 50)" }
            },
            F_CorrelateLogs),
         AssistantFunction(
            "analyze-log-patterns",
            "Analyze log patterns for anomaly detection, burst identification, recurring patterns, and new message detection.",
            {
               { "object", "optional object to analyze" },
               { "log_type", "log type to analyze: syslog, windows_events, snmp_traps, events (required)" },
               { "time_from", "analysis start (default: -24h)" },
               { "time_to", "analysis end (default: now)" },
               { "pattern_type", "pattern type: frequency, burst, recurring, new, all (default: all)" }
            },
            F_AnalyzeLogPatterns)
      }
   );
}

/**
 * Module initialization
 */
static bool InitializeModule(Config *config)
{
   AddAIAssistantPromptFromFile(L"overview.md");
   AddAIAssistantPromptFromFile(L"ai-data-storage.md");
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
