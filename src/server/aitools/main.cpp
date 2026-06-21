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

std::string F_AcknowledgeAlarm(json_t *arguments, uint32_t userId);
std::string F_AddAlarmComment(json_t *arguments, uint32_t userId);
std::string F_AddIncidentComment(json_t *arguments, uint32_t userId);
std::string F_AlarmList(json_t *arguments, uint32_t userId);
std::string F_GetAlarmDetails(json_t *arguments, uint32_t userId);
std::string F_ResolveAlarm(json_t *arguments, uint32_t userId);
std::string F_TerminateAlarm(json_t *arguments, uint32_t userId);
std::string F_AssignIncident(json_t *arguments, uint32_t userId);
std::string F_ClearNotificationChannelQueue(json_t *arguments, uint32_t userId);
std::string F_CreateIncident(json_t *arguments, uint32_t userId);
std::string F_CreateIncidentFromAlarms(json_t *arguments, uint32_t userId);
std::string F_CreateMetric(json_t *arguments, uint32_t userId);
std::string F_EditMetric(json_t *arguments, uint32_t userId);
std::string F_DeleteMetric(json_t *arguments, uint32_t userId);
std::string F_GetMetricDetails(json_t *arguments, uint32_t userId);
std::string F_GetThresholds(json_t *arguments, uint32_t userId);
std::string F_GetObjectThresholds(json_t *arguments, uint32_t userId);
std::string F_AddThreshold(json_t *arguments, uint32_t userId);
std::string F_EditThreshold(json_t *arguments, uint32_t userId);
std::string F_DeleteThreshold(json_t *arguments, uint32_t userId);
std::string F_EndMaintenance(json_t *arguments, uint32_t userId);
std::string F_ExplainObjectStatus(json_t *arguments, uint32_t userId);
std::string F_FindObjects(json_t *arguments, uint32_t userId);
std::string F_ForcePoll(json_t *arguments, uint32_t userId);
std::string F_CreateEppRule(json_t *arguments, uint32_t userId);
std::string F_CreateEventTemplate(json_t *arguments, uint32_t userId);
std::string F_CreateServerAction(json_t *arguments, uint32_t userId);
std::string F_DeleteEppRule(json_t *arguments, uint32_t userId);
std::string F_DeleteEventTemplate(json_t *arguments, uint32_t userId);
std::string F_DeleteServerAction(json_t *arguments, uint32_t userId);
std::string F_DisableEppRule(json_t *arguments, uint32_t userId);
std::string F_EnableEppRule(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingAction(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingActions(json_t *arguments, uint32_t userId);
std::string F_GetEventProcessingPolicy(json_t *arguments, uint32_t userId);
std::string F_GetEventTemplate(json_t *arguments, uint32_t userId);
std::string F_ModifyEppRule(json_t *arguments, uint32_t userId);
std::string F_ModifyEventTemplate(json_t *arguments, uint32_t userId);
std::string F_ModifyServerAction(json_t *arguments, uint32_t userId);
std::string F_MoveEppRule(json_t *arguments, uint32_t userId);
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
std::string F_GetBackupStatus(json_t *arguments, uint32_t userId);
std::string F_GetBackupList(json_t *arguments, uint32_t userId);
std::string F_GetBackupContent(json_t *arguments, uint32_t userId);
std::string F_StartBackup(json_t *arguments, uint32_t userId);
std::string F_CompareBackups(json_t *arguments, uint32_t userId);
std::string F_ListDashboardElementTypes(json_t *arguments, uint32_t userId);
std::string F_DescribeDashboardElementType(json_t *arguments, uint32_t userId);
std::string F_CreateDashboard(json_t *arguments, uint32_t userId);
std::string F_GetDashboard(json_t *arguments, uint32_t userId);
std::string F_AddDashboardElement(json_t *arguments, uint32_t userId);
std::string F_UpdateDashboardElement(json_t *arguments, uint32_t userId);
std::string F_RemoveDashboardElement(json_t *arguments, uint32_t userId);
std::string F_MoveDashboardElement(json_t *arguments, uint32_t userId);

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
      "Get list of active alarms (alerts) for monitored devices, including each alarm's numeric ID (needed by the alarm-management skill to act on it). Each alarm represents an outstanding problem or condition worth operator attention. Device in this context could mean anything related to IT infrastructure: switch, router, server, workstation, ATM, POS, terminal, printer, scanner, UPS, sensor of any kind.",
      {
         { "object", "optional name or ID of node, device, server, workstation, or container or group of them" },
         { "state", "optional state filter: 'active' (outstanding + acknowledged, the default), 'outstanding', 'acknowledged', 'resolved', or 'all'" }
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
      "explain-object-status",
      "Get detailed list of factors that contributed to the current status of an object (node, device, server, workstation, container, etc.).",
      {
         { "object", "mandatory name or ID of an object" }
      },
      F_ExplainObjectStatus);
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
      "force-poll",
      "Trigger an immediate poll on a monitored object instead of waiting for the next scheduled poll. "
      "Use when fresh state is needed right now — e.g. after a device was fixed or reconfigured, to "
      "refresh status, capabilities, interfaces, or topology. Polls run asynchronously by default; set "
      "wait=true to block until the poll completes (or timeoutSeconds elapses) and receive the poller's "
      "diagnostic output. The poll type must match the object class: 'status'/'configuration' work for "
      "most monitored objects, 'topology'/'routing-table'/'discovery' for nodes, 'autobind' for "
      "containers/templates/maps/network maps, 'instance-discovery' for data collection owners, "
      "'map-update' for network maps.",
      {
         { "object", "name or ID of the object to poll (mandatory)" },
         { "pollType", "one of: status, configuration, instance-discovery, topology, routing-table, discovery, autobind, map-update (mandatory)" },
         { "wait", "if true, block until the poll completes or timeoutSeconds elapses, returning captured poller output (default: false)" },
         { "timeoutSeconds", "max seconds to wait when wait=true; clamped to [1, 300] (default: 30)" }
      },
      F_ForcePoll);
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
      "operational-status",
      "Composite triage view of what is currently broken in the monitored infrastructure: down/critical nodes, "
      "active alarms grouped by source object, and recent critical events — all in a single call. "
      "Use this when the operator asks 'what's broken', 'anything on fire', 'how does it look', or starts "
      "shift/incident triage. Defaults exclude objects in maintenance mode and administratively unmanaged objects. "
      "To act on a specific alarm afterwards, use the alarm-management skill with the alarm IDs returned by this tool.",
      {
         { "detail", "response detail level: 'summary' (counts plus a few representative samples per section) or 'full' (per-section structured list, capped). Default: 'summary'" },
         { "min_severity", "minimum severity to include: 'critical', 'major', 'minor', 'warning'. Default: 'major' (i.e. critical + major)" },
         { "scope", "optional name or ID of an object (container, zone, single device) to limit the view to that subtree. Default: whole infrastructure visible to user" },
         { "event_window_minutes", "time window for the 'recent events' section, in minutes. Clamped to [5, 1440]. Default: 60" },
         { "include_quiet", "if true, also include objects in maintenance mode and administratively unmanaged objects. Default: false" }
      },
      F_OperationalStatus);
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
      "alarm-management",
      "Provides alarm lifecycle management for NetXMS. Use this skill to acknowledge, resolve, and terminate alarms (alerts), add operator comments, and inspect full alarm details including comment history and related events. Essential for alert triage, incident response, on-call workflows, and clearing stale or resolved alarms. To get a list of active alarms (with their IDs) use the alarm-list function; this skill provides the actions you take on those alarms.",
      "@alarm-management.md",
      {
         AssistantFunction(
            "get-alarm-details",
            "Get full details of a single active alarm: state, severity, source object and its status, timestamps, repeat count, helpdesk linkage, the originating EPP rule, the complete operator comment history, and recent related events. Use this before acting on an alarm to understand its context. Terminated alarms are not retained in the active list and cannot be retrieved here.",
            {
               { "alarm", "numeric alarm ID (mandatory)" }
            },
            F_GetAlarmDetails),
         AssistantFunction(
            "acknowledge-alarm",
            "Acknowledge one or more alarms, signaling that an operator is aware of the condition. Only alarms in the outstanding state can be acknowledged. Returns a per-alarm result with success/failure for each ID.",
            {
               { "alarms", "alarm ID, or array of alarm IDs, to acknowledge (mandatory)" },
               { "sticky", "if true, the alarm stays acknowledged even if the originating condition repeats (default: false)" },
               { "timeout_minutes", "for a sticky acknowledgment, automatically revert the alarm to outstanding after this many minutes unless it was resolved (optional; ignored unless sticky is true; 0 = no timeout)" },
               { "include_subordinates", "also apply to subordinate (child) alarms grouped under each alarm (default: true)" }
            },
            F_AcknowledgeAlarm),
         AssistantFunction(
            "resolve-alarm",
            "Resolve one or more alarms. A resolved alarm remains in the active alarm list and will be re-activated automatically if the originating condition occurs again. Use this when the underlying problem has been fixed. Returns a per-alarm result.",
            {
               { "alarms", "alarm ID, or array of alarm IDs, to resolve (mandatory)" },
               { "include_subordinates", "also apply to subordinate (child) alarms grouped under each alarm (default: true)" }
            },
            F_ResolveAlarm),
         AssistantFunction(
            "terminate-alarm",
            "Terminate one or more alarms, removing them from the active alarm list. The alarm will not reappear unless it is raised again. Use this to clear stale alarms or alarms that no longer apply. An alarm that is open in an external helpdesk system cannot be terminated until the helpdesk issue is closed. Returns a per-alarm result.",
            {
               { "alarms", "alarm ID, or array of alarm IDs, to terminate (mandatory)" },
               { "include_subordinates", "also apply to subordinate (child) alarms grouped under each alarm (default: true)" }
            },
            F_TerminateAlarm),
         AssistantFunction(
            "add-alarm-comment",
            "Add an operator comment to an alarm to document analysis findings, actions taken, or context for other operators. The comment is appended to the alarm's comment history.",
            {
               { "alarm", "numeric alarm ID (mandatory)" },
               { "text", "comment text (mandatory)" }
            },
            F_AddAlarmComment)
      }
   );

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
               { "origin", "data collection origin: agent, snmp, script, ssh, push, webService, deviceDriver, mqtt, modbus, internal (default: agent)" },
               { "dataType", "data type: int, unsigned-int, int64, unsigned-int64, counter32, counter64, float, string (default: string)" },
               { "pollingInterval", "polling interval in seconds (optional, uses system default if not specified)" },
               { "retentionTime", "retention time in days (optional, uses system default if not specified)" },
               { "deltaCalculation", "delta calculation: original, delta, averagePerSecond, averagePerMinute (default: original)" },
               { "sampleCount", "number of samples for threshold functions (default: 1)" },
               { "multiplier", "value multiplier (default: 0)" },
               { "unitName", "unit name for display (optional)" },
               { "transformationScript", "NXSL transformation script source code (optional)" },
               { "status", "status: active, disabled (default: active)" },
               { "anomalyDetection", "anomaly detection: none, iforest, ai, both (default: none)" },
               { "comments", "notes or comments about the metric (optional)" },
               { "userTag", "user-defined tag for categorization (optional)" },
               { "sourceNode", "name or ID of proxy node for data collection (optional)" },
               { "snmpPort", "custom SNMP port (optional, uses node default if not set)" },
               { "showOnObjectTooltip", "show value in object tooltip (optional, boolean)" },
               { "showInObjectOverview", "show value in object overview (optional, boolean)" },
               { "calculateNodeStatus", "use for node status calculation (optional, boolean)" },
               { "storeChangesOnly", "store only changed values (optional, boolean)" },
               { "hideOnLastValuesPage", "hide from last values page (optional, boolean)" }
            },
            F_CreateMetric),
         AssistantFunction(
            "edit-metric",
            "Edit existing data collection item (metric) properties. Only specified properties are changed, others remain unchanged. Use get-metric-details first to see current configuration.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric to edit (mandatory)" },
               { "name", "new metric name (optional, renames the metric)" },
               { "description", "new metric description (optional)" },
               { "origin", "new data collection origin: agent, snmp, script, ssh, push, webService, deviceDriver, mqtt, modbus, internal (optional)" },
               { "dataType", "new data type: int, unsigned-int, int64, unsigned-int64, counter32, counter64, float, string (optional)" },
               { "pollingInterval", "new polling interval in seconds (optional)" },
               { "retentionTime", "new retention time in days (optional)" },
               { "deltaCalculation", "new delta calculation: original, delta, averagePerSecond, averagePerMinute (optional)" },
               { "sampleCount", "new number of samples for threshold functions (optional)" },
               { "multiplier", "new value multiplier (optional)" },
               { "unitName", "new unit name for display (optional)" },
               { "transformationScript", "new NXSL transformation script source code (optional)" },
               { "status", "new status: active, disabled (optional)" },
               { "anomalyDetection", "anomaly detection: none, iforest, ai, both (optional)" },
               { "comments", "new notes or comments (optional)" },
               { "userTag", "new user-defined tag (optional)" },
               { "systemTag", "new system tag (optional)" },
               { "sourceNode", "name or ID of proxy node, or 'none' to clear (optional)" },
               { "snmpPort", "custom SNMP port, 0 to reset to default (optional)" },
               { "showOnObjectTooltip", "show value in object tooltip (optional, boolean)" },
               { "showInObjectOverview", "show value in object overview (optional, boolean)" },
               { "calculateNodeStatus", "use for node status calculation (optional, boolean)" },
               { "storeChangesOnly", "store only changed values (optional, boolean)" },
               { "hideOnLastValuesPage", "hide from last values page (optional, boolean)" },
               { "aggregateOnCluster", "aggregate on cluster (optional, boolean)" }
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
            "get-metric-details",
            "Get full configuration details of a single data collection item (metric), including all properties, flags, thresholds count, transformation script, and metadata. Use this before editing a metric to see its current configuration.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric (mandatory)" }
            },
            F_GetMetricDetails),
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
               { "timeTo", "end time (optional, ISO format, defaults to now)" },
               { "maxDataPoints", "optional maximum number of data points to return; if set, data will be aggregated into time buckets with min/max/avg values; default is 500; set to 0 for raw data" }
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
            "get-object-thresholds",
            "Get all thresholds configured across every data collection item and table on an object (node, device, etc.) in a single call. Returns one row per threshold with the data collection item name and ID, threshold ID, condition, and the names of the events generated on activation and deactivation. Use this to answer questions like which events can be generated from thresholds on a given node.",
            {
               { "object", "name or ID of an object (mandatory)" }
            },
            F_GetObjectThresholds),
         AssistantFunction(
            "add-threshold",
            "Add a threshold to a data collection item for alerting when values exceed limits.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric (mandatory)" },
               { "value", "threshold value (mandatory)" },
               { "function", "function: last, average, sum, meanDeviation, diff, error, script, absDeviation, anomaly (default: last)" },
               { "operation", "operation: less, lessOrEqual, equal, greaterOrEqual, greater, notEqual, like, notLike (default: greaterOrEqual)" },
               { "sampleCount", "number of samples for the function (default: 1)" },
               { "deactivationSampleCount", "number of consecutive non-matching polls before deactivation (default: 1)" },
               { "activationEvent", "event name or numeric code generated when threshold activates (default: SYS_THRESHOLD_REACHED)" },
               { "deactivationEvent", "event name or numeric code generated when threshold deactivates (default: SYS_THRESHOLD_REARMED)" },
               { "repeatInterval", "repeat interval in seconds, -1=default, 0=off (default: -1)" },
               { "regenerateOnValueChange", "regenerate activation event when value changes while threshold stays active (optional, boolean)" },
               { "script", "NXSL script for script function (optional)" }
            },
            F_AddThreshold),
         AssistantFunction(
            "edit-threshold",
            "Edit an existing threshold on a data collection item. Only specified properties are changed; others (including runtime state) remain unchanged. Parameters mirror add-threshold. Use get-thresholds or get-object-thresholds first to find the threshold ID.",
            {
               { "object", "name or ID of an object (mandatory)" },
               { "metric", "name or ID of the metric (mandatory)" },
               { "thresholdId", "ID of the threshold to edit (mandatory)" },
               { "value", "new threshold value (optional)" },
               { "function", "new function: last, average, sum, meanDeviation, diff, error, script, absDeviation, anomaly (optional)" },
               { "operation", "new operation: less, lessOrEqual, equal, greaterOrEqual, greater, notEqual, like, notLike (optional)" },
               { "sampleCount", "new number of samples for the function (optional)" },
               { "deactivationSampleCount", "new number of consecutive non-matching polls before deactivation (optional)" },
               { "activationEvent", "new event name or numeric code generated when threshold activates (optional)" },
               { "deactivationEvent", "new event name or numeric code generated when threshold deactivates (optional)" },
               { "repeatInterval", "new repeat interval in seconds, -1=default, 0=off (optional)" },
               { "regenerateOnValueChange", "regenerate activation event when value changes while threshold stays active (optional, boolean)" },
               { "disabled", "disable or enable the threshold (optional, boolean)" },
               { "script", "new NXSL script for script function, empty string to clear (optional)" }
            },
            F_EditThreshold),
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
      "Provides comprehensive event processing management capabilities for NetXMS. Use this skill to CREATE, MODIFY, and DELETE event templates, manage event processing policy (EPP), server-side actions, and understand how events flow through the system. Event templates define the structure of events (name, severity, message format). You can create new custom event templates, modify existing user-defined templates, or delete templates that are no longer needed. Events are generated by various sources (monitoring, SNMP traps, syslog, agents) and processed according to configured rules to generate alarms, send notifications, execute actions, or correlate related events.",
      "@event-processing.md",
      {
         AssistantFunction(
            "create-event-template",
            "Create a new event template. System will auto-assign event code >= 100000.",
            {
               { "name", "event template name (mandatory, alphanumeric with underscores)" },
               { "severity", "severity: normal, warning, minor, major, critical (mandatory)" },
               { "message", "message template with %1, %2 placeholders (mandatory)" },
               { "description", "detailed description (optional)" },
               { "tags", "comma-separated tags (optional)" }
            },
            F_CreateEventTemplate),
         AssistantFunction(
            "modify-event-template",
            "Modify a user-defined event template (code >= 100000).",
            {
               { "code", "event code (provide code or name)" },
               { "name", "event name (provide code or name)" },
               { "newName", "new name (optional)" },
               { "severity", "new severity (optional)" },
               { "message", "new message template (optional)" },
               { "description", "new description (optional)" },
               { "tags", "new tags (optional)" }
            },
            F_ModifyEventTemplate),
         AssistantFunction(
            "delete-event-template",
            "Delete a user-defined event template (code >= 100000).",
            {
               { "code", "event code (provide code or name)" },
               { "name", "event name (provide code or name)" }
            },
            F_DeleteEventTemplate),
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
            F_GetEventProcessingPolicy),
         AssistantFunction(
            "create-epp-rule",
            "Create a new event processing policy rule. Returns the created rule (with its assigned GUID) and the new policy version. Position the rule with after_guid / before_guid / position (default: append at end).",
            {
               { "after_guid", "place the new rule immediately after this rule GUID (optional)" },
               { "before_guid", "place the new rule immediately before this rule GUID (optional)" },
               { "position", "explicit position: 'first' or 'last' (default: 'last' if no after/before given)" },
               { "comments", "human-readable comments describing the rule (optional)" },
               { "events", "array of event codes or names that the rule matches (optional; empty = match any event)" },
               { "sources", "array of object IDs or names; rule applies to events from these objects (optional; empty = any source)" },
               { "source_exclusions", "array of object IDs or names to exclude (optional)" },
               { "match_severities", "array of severity names to match: info, warning, minor, major, critical (optional; absent = match all severities)" },
               { "negated_event_match", "if true, invert the event match (match events NOT in the list) (optional, default false)" },
               { "negated_source_match", "if true, invert the source match (optional, default false)" },
               { "time_frames", "array of {time, date} integer pairs (optional; format documented in skill md)" },
               { "filter_script", "NXSL filter script (optional)" },
               { "actions", "array of {id, timer_delay?, timer_key?, blocking_timer_key?, snooze_time?, active?}; id is action ID or name (optional)" },
               { "action_script", "NXSL action script (optional)" },
               { "timer_cancellations", "array of timer keys to cancel (optional)" },
               { "stop_processing", "if true, stop EPP processing after this rule matches (optional, default false)" },
               { "generate_alarm", "if true, generate an alarm on match (optional, default false)" },
               { "alarm_severity", "alarm severity: info, warning, minor, major, critical, normal, 'same as event', resolve, terminate (optional)" },
               { "alarm_message", "alarm message template (optional)" },
               { "alarm_impact", "alarm impact text (optional)" },
               { "alarm_key", "alarm key for deduplication and resolve/terminate matching (optional)" },
               { "alarm_timeout", "alarm acknowledgment timeout in seconds (optional, 0 = no timeout)" },
               { "alarm_timeout_event", "event code or name to generate when alarm times out (optional)" },
               { "alarm_categories", "array of alarm category IDs or names (optional)" },
               { "create_incident", "if true, create an incident on match (optional, default false)" },
               { "incident_delay", "incident creation delay in seconds (optional, 0 = immediate)" },
               { "incident_title", "incident title template (optional; defaults to alarm message)" },
               { "incident_description", "incident description template (optional)" },
               { "ai_analyze_incident", "if true, run AI analysis on created incident (optional, default false)" },
               { "incident_ai_analysis_depth", "AI analysis depth: quick, standard, thorough (optional)" },
               { "incident_ai_prompt", "custom AI analysis instructions (optional)" },
               { "create_ticket", "if true, create helpdesk ticket on match (optional, default false)" },
               { "terminate_by_regexp", "if true, treat alarm_key as a regexp when resolving/terminating (optional, default false)" },
               { "start_downtime", "if true, start a downtime period on match (optional, default false)" },
               { "end_downtime", "if true, end a downtime period on match (optional, default false)" },
               { "downtime_tag", "downtime tag (optional)" },
               { "request_ai_comment", "if true, request AI-generated comment on alarm (optional, default false)" },
               { "rca_script_name", "name of library script for root cause analysis (optional)" },
               { "ai_agent_instructions", "instructions for AI agent processing this rule (optional)" },
               { "pstorage_set", "object map of persistent storage keys to set on match: {key: value, ...} (optional)" },
               { "pstorage_delete", "array of persistent storage keys to delete on match (optional)" },
               { "custom_attribute_set", "object map of custom attributes to set on the source object: {key: value, ...} (optional)" },
               { "custom_attribute_delete", "array of custom attribute names to delete on the source object (optional)" }
            },
            F_CreateEppRule),
         AssistantFunction(
            "modify-epp-rule",
            "Modify an existing event processing policy rule. Only fields actually provided are changed - any field omitted is left unchanged. Use get-event-processing-policy first to find the rule GUID and review its current settings.",
            {
               { "guid", "GUID of the rule to modify (mandatory)" },
               { "comments", "new comments (optional)" },
               { "events", "new event list (optional, replaces current)" },
               { "sources", "new source list (optional, replaces current)" },
               { "source_exclusions", "new source exclusions list (optional, replaces current)" },
               { "match_severities", "new severity match list (optional, replaces current)" },
               { "negated_event_match", "new negated-event-match flag (optional)" },
               { "negated_source_match", "new negated-source-match flag (optional)" },
               { "time_frames", "new time frames list (optional, replaces current)" },
               { "filter_script", "new NXSL filter script (optional; empty string clears)" },
               { "actions", "new actions list (optional, replaces current)" },
               { "action_script", "new NXSL action script (optional; empty string clears)" },
               { "timer_cancellations", "new timer cancellations list (optional, replaces current)" },
               { "stop_processing", "new stop_processing flag (optional)" },
               { "generate_alarm", "new generate_alarm flag (optional)" },
               { "alarm_severity", "new alarm severity (optional)" },
               { "alarm_message", "new alarm message (optional)" },
               { "alarm_impact", "new alarm impact (optional)" },
               { "alarm_key", "new alarm key (optional)" },
               { "alarm_timeout", "new alarm timeout in seconds (optional)" },
               { "alarm_timeout_event", "new alarm timeout event code or name (optional)" },
               { "alarm_categories", "new alarm categories list (optional, replaces current)" },
               { "create_incident", "new create_incident flag (optional)" },
               { "incident_delay", "new incident delay in seconds (optional)" },
               { "incident_title", "new incident title (optional)" },
               { "incident_description", "new incident description (optional)" },
               { "ai_analyze_incident", "new ai_analyze_incident flag (optional)" },
               { "incident_ai_analysis_depth", "new AI analysis depth (optional)" },
               { "incident_ai_prompt", "new AI analysis prompt (optional)" },
               { "create_ticket", "new create_ticket flag (optional)" },
               { "terminate_by_regexp", "new terminate_by_regexp flag (optional)" },
               { "start_downtime", "new start_downtime flag (optional)" },
               { "end_downtime", "new end_downtime flag (optional)" },
               { "downtime_tag", "new downtime tag (optional)" },
               { "request_ai_comment", "new request_ai_comment flag (optional)" },
               { "rca_script_name", "new RCA script name (optional)" },
               { "ai_agent_instructions", "new AI agent instructions (optional)" },
               { "pstorage_set", "new persistent storage set map (optional, replaces current)" },
               { "pstorage_delete", "new persistent storage delete list (optional, replaces current)" },
               { "custom_attribute_set", "new custom attribute set map (optional, replaces current)" },
               { "custom_attribute_delete", "new custom attribute delete list (optional, replaces current)" }
            },
            F_ModifyEppRule),
         AssistantFunction(
            "delete-epp-rule",
            "Delete an event processing policy rule.",
            {
               { "guid", "GUID of the rule to delete (mandatory)" }
            },
            F_DeleteEppRule),
         AssistantFunction(
            "enable-epp-rule",
            "Enable a previously disabled event processing policy rule.",
            {
               { "guid", "GUID of the rule to enable (mandatory)" }
            },
            F_EnableEppRule),
         AssistantFunction(
            "disable-epp-rule",
            "Disable an event processing policy rule. The rule remains in the policy but is skipped during event processing.",
            {
               { "guid", "GUID of the rule to disable (mandatory)" }
            },
            F_DisableEppRule),
         AssistantFunction(
            "move-epp-rule",
            "Move an event processing policy rule to a new position. Rules are evaluated top-to-bottom, so order can be functionally significant (especially with stop_processing).",
            {
               { "guid", "GUID of the rule to move (mandatory)" },
               { "after_guid", "place the rule immediately after this rule GUID (optional)" },
               { "before_guid", "place the rule immediately before this rule GUID (optional)" },
               { "position", "explicit position: 'first' or 'last' (optional)" }
            },
            F_MoveEppRule),
         AssistantFunction(
            "create-server-action",
            "Create a new server-side action that can be referenced from event processing rules. Action types: local_command (run shell command on server), agent_command (run command on monitored node via agent), ssh_command (run command via SSH), notification (send via configured notification channel), forward_event (forward event to another NetXMS server), nxsl_script (execute NXSL script).",
            {
               { "name", "unique action name (mandatory)" },
               { "type", "action type: local_command, agent_command, ssh_command, notification, forward_event, nxsl_script (optional, default local_command)" },
               { "disabled", "if true, action is created in disabled state (optional, default false)" },
               { "recipient", "recipient address for notification, or destination server name for forward_event (optional)" },
               { "email_subject", "subject line for notification (optional)" },
               { "data", "command line text, NXSL source, or notification message body depending on type (optional)" },
               { "channel", "notification channel name (notification type only) (optional)" }
            },
            F_CreateServerAction),
         AssistantFunction(
            "modify-server-action",
            "Modify an existing server-side action. Only fields actually provided are changed.",
            {
               { "id", "action ID (mandatory)" },
               { "name", "new action name (optional)" },
               { "type", "new action type (optional)" },
               { "disabled", "new disabled flag (optional)" },
               { "recipient", "new recipient address (optional)" },
               { "email_subject", "new email subject (optional)" },
               { "data", "new command line / script / body (optional)" },
               { "channel", "new notification channel name (optional)" }
            },
            F_ModifyServerAction),
         AssistantFunction(
            "delete-server-action",
            "Delete a server-side action. Refused if any event processing rule still references it - modify those rules first.",
            {
               { "id", "action ID (mandatory)" }
            },
            F_DeleteServerAction)
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
               { "origin", "event origin (SYSTEM, AGENT, CLIENT, SYSLOG, SNMP, NXSL, REMOTE_SERVER, WINDOWS_EVENT, OPENTELEMETRY)" },
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

   RegisterAIAssistantSkill(
      "device-backup",
      "Provides device configuration backup management capabilities for NetXMS monitored network devices. Use this skill to check backup registration status, list available backups, retrieve configuration content, trigger on-demand backups, and compare configurations across backup versions for change analysis, compliance auditing, and troubleshooting.",
      "@device-backup.md",
      {
         AssistantFunction(
            "get-backup-status",
            "Check if a device is registered for configuration backup and get last backup job status.",
            {
               { "object", "name or ID of a node (mandatory)" }
            },
            F_GetBackupStatus),
         AssistantFunction(
            "get-backup-list",
            "Get list of available configuration backups for a device with timestamps, sizes, and SHA-256 hashes.",
            {
               { "object", "name or ID of a node (mandatory)" }
            },
            F_GetBackupList),
         AssistantFunction(
            "get-backup-content",
            "Get full configuration content from a specific backup or the latest backup. Returns both running and startup configs.",
            {
               { "object", "name or ID of a node (mandatory)" },
               { "backupId", "backup ID to retrieve (optional, defaults to latest backup)" }
            },
            F_GetBackupContent),
         AssistantFunction(
            "start-backup",
            "Trigger an immediate device configuration backup job. Device must be registered for backup.",
            {
               { "object", "name or ID of a node (mandatory)" }
            },
            F_StartBackup),
         AssistantFunction(
            "compare-backups",
            "Get two backup configurations side by side for diff comparison.",
            {
               { "object", "name or ID of a node (mandatory)" },
               { "backupId1", "first backup ID (mandatory)" },
               { "backupId2", "second backup ID (mandatory)" },
               { "configType", "config type to compare: running or startup (default: running)" }
            },
            F_CompareBackups)
      }
   );

   RegisterAIAssistantSkill(
      "dashboard-building",
      "Provides capabilities for building and editing NetXMS dashboards from natural-language requests. Use this skill to create a new dashboard and incrementally add, configure, reorder, or remove visualization elements such as charts, network maps, alarm viewers, and status maps. The dashboard is built one element at a time with per-element validation. Discover supported element types with list-dashboard-element-types and their configuration schemas with describe-dashboard-element-type before adding elements. Use the existing data collection tools (e.g. get-metrics) to resolve node and DCI identifiers for chart elements.",
      "@dashboard-building.md",
      {
         AssistantFunction(
            "list-dashboard-element-types",
            "List all supported dashboard element types with a one-line description of each. Call this first to discover what can be placed on a dashboard.",
            {},
            F_ListDashboardElementTypes),
         AssistantFunction(
            "describe-dashboard-element-type",
            "Get the full configuration schema and a filled example for one dashboard element type.",
            {
               { "type", "element type name as returned by list-dashboard-element-types (mandatory)" }
            },
            F_DescribeDashboardElementType),
         AssistantFunction(
            "create-dashboard",
            "Create a new empty dashboard. Returns the new dashboard ID used by the other dashboard tools.",
            {
               { "name", "name of the dashboard (mandatory)" },
               { "columns", "number of layout columns, 1-12 (default 2)" },
               { "group", "name or ID of a dashboard group to place the dashboard in (default: top-level Dashboards)" },
               { "associateWith", "name or ID of an object to associate the dashboard with (optional)" }
            },
            F_CreateDashboard),
         AssistantFunction(
            "get-dashboard",
            "Read back a dashboard: its column count and ordered list of elements, each with its stable GUID, type, layout, and configuration. Use the GUIDs to target update/remove/move operations.",
            {
               { "dashboard", "name or ID of the dashboard (mandatory)" }
            },
            F_GetDashboard),
         AssistantFunction(
            "add-dashboard-element",
            "Append a configured element to a dashboard. References to non-existent or inaccessible objects/DCIs are rejected and the element is not added.",
            {
               { "dashboard", "name or ID of the dashboard (mandatory)" },
               { "type", "element type name as returned by list-dashboard-element-types (mandatory)" },
               { "config", "element configuration as a JSON object matching the type's schema (mandatory)" },
               { "width", "layout width intent: full, half, third, or quarter (default full)" },
               { "height", "optional fixed element height in pixels" },
               { "grabVerticalSpace", "whether the element grabs extra vertical space (default true); set false for labels so they take only their natural height" }
            },
            F_AddDashboardElement),
         AssistantFunction(
            "update-dashboard-element",
            "Replace the configuration (and optionally the layout) of an existing element identified by its GUID.",
            {
               { "dashboard", "name or ID of the dashboard (mandatory)" },
               { "guid", "GUID of the element to update (mandatory)" },
               { "config", "new element configuration as a JSON object matching the type's schema (mandatory)" },
               { "width", "optional new layout width intent: full, half, third, or quarter" },
               { "height", "optional new fixed element height in pixels" },
               { "grabVerticalSpace", "optional: whether the element grabs extra vertical space; set false for labels so they take only their natural height" }
            },
            F_UpdateDashboardElement),
         AssistantFunction(
            "remove-dashboard-element",
            "Remove an element from a dashboard by its GUID.",
            {
               { "dashboard", "name or ID of the dashboard (mandatory)" },
               { "guid", "GUID of the element to remove (mandatory)" }
            },
            F_RemoveDashboardElement),
         AssistantFunction(
            "move-dashboard-element",
            "Move an element to a new position in the dashboard's element order. The element GUID is preserved.",
            {
               { "dashboard", "name or ID of the dashboard (mandatory)" },
               { "guid", "GUID of the element to move (mandatory)" },
               { "position", "new zero-based position in the element list (mandatory)" }
            },
            F_MoveDashboardElement)
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
