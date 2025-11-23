/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: id.cpp
**
**/

#include "nxcore.h"

/**
 * External functions
 */
int64_t GetLastActionExecutionLogId();
int32_t GetLastAuditRecordId();
int64_t GetLastEventId();
int64_t GetLastNotificationId();
int64_t GetLastSnmpTrapId();
uint64_t GetNextSyslogId();
uint64_t GetNextWinEventId();
uint64_t GetLastAssetChangeLogId();
void LoadLastEventId(DB_HANDLE hdb);

/**
 * Constants
 */
#define NUMBER_OF_GROUPS   32

/**
 * Static data
 */
static Mutex s_mutexTableAccess(MutexType::FAST);
static uint32_t s_freeIdTable[NUMBER_OF_GROUPS] =
   {
      100, FIRST_USER_EVENT_ID, 1, 1,
      1, 1, 1, 0x40000001,
      1, 1, 1, 1,
      10000, 10000, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 1
   };
static uint32_t s_idLimits[NUMBER_OF_GROUPS] =
   {
      0xFFFFFFFE, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
      0x7FFFFFFF, 0x7FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0x7FFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE
   };
static const TCHAR *s_groupNames[NUMBER_OF_GROUPS] =
{
   _T("Network Objects"),
   _T("Events"),
   _T("Data Collection Items"),
   _T("SNMP Trap"),
   _T("Actions"),
   _T("Data Collection Thresholds"),
   _T("Users"),
   _T("User Groups"),
   _T("Alarms"),
   _T("Alarm Notes"),
   _T("Packages"),
   _T("Business Service Tickets"),
   _T("Object Tools"),
   _T("Scripts"),
   _T("Agent Configurations"),
   _T("Graphs"),
   _T("Authentication Tokens"),
   _T("Mapping Tables"),
   _T("DCI Summary Tables"),
   _T("Alarm Categories"),
   _T("User Agent Messages"),
   _T("Passive Rack Elements"),
   _T("Physical Links"),
   _T("Web Service Definitions"),
   _T("Object Categories"),
   _T("Geographical Areas"),
   _T("SSH Keys"),
   _T("Object Queries"),
   _T("Business Service Checks"),
   _T("Business Service Downtime Records"),
   _T("Maintenance journal"),
   _T("Package Deployment Jobs")
};

/**
 * Initialize ID table
 */
bool InitIdTable()
{
   DB_RESULT hResult;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Get first available object ID
	uint32_t id = ConfigReadULong(_T("FirstFreeObjectId"), s_freeIdTable[IDG_NETWORK_OBJECT]);
	if (id > s_freeIdTable[IDG_NETWORK_OBJECT])
		s_freeIdTable[IDG_NETWORK_OBJECT] = id;
   hResult = DBSelect(hdb, _T("SELECT max(object_id) FROM object_properties"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event code
   hResult = DBSelect(hdb, _T("SELECT max(event_code) FROM event_cfg"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_EVENT] = std::max(s_freeIdTable[IDG_EVENT], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection object id
   id = ConfigReadULong(_T("FirstFreeDCIId"), s_freeIdTable[IDG_ITEM]);
   if (id > s_freeIdTable[IDG_ITEM])
      s_freeIdTable[IDG_ITEM] = id;
   hResult = DBSelect(hdb, _T("SELECT max(item_id) FROM items"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = std::max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(item_id) FROM dc_tables"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = std::max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(dci_id) FROM dci_delete_list"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = std::max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SNMP trap configuration record id
   hResult = DBSelect(hdb, _T("SELECT max(trap_id) FROM snmp_trap_cfg"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SNMP_TRAP] = std::max(s_freeIdTable[IDG_SNMP_TRAP], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available action id
   hResult = DBSelect(hdb, _T("SELECT max(action_id) FROM actions"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ACTION] = std::max(s_freeIdTable[IDG_ACTION], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available threshold id
   hResult = DBSelect(hdb, _T("SELECT max(threshold_id) FROM thresholds"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_THRESHOLD] = std::max(s_freeIdTable[IDG_THRESHOLD], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dct_thresholds"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_THRESHOLD] = std::max(s_freeIdTable[IDG_THRESHOLD], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM users"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_USER] = std::max(s_freeIdTable[IDG_USER], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user group id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM user_groups"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_USER_GROUP] = std::max(s_freeIdTable[IDG_USER_GROUP], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm id
   id = ConfigReadULong(_T("FirstFreeAlarmId"), s_freeIdTable[IDG_ALARM]);
   if (id > s_freeIdTable[IDG_ALARM])
      s_freeIdTable[IDG_ALARM] = id;
   hResult = DBSelect(hdb, _T("SELECT max(alarm_id) FROM alarms"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM] = std::max(s_freeIdTable[IDG_ALARM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm note id
   hResult = DBSelect(hdb, _T("SELECT max(note_id) FROM alarm_notes"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM_NOTE] = std::max(s_freeIdTable[IDG_ALARM_NOTE], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available package id
   hResult = DBSelect(hdb, _T("SELECT max(pkg_id) FROM agent_pkg"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_PACKAGE] = std::max(s_freeIdTable[IDG_PACKAGE], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object tool id
   hResult = DBSelect(hdb, _T("SELECT max(tool_id) FROM object_tools"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_OBJECT_TOOL] = std::max(s_freeIdTable[IDG_OBJECT_TOOL], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available script id
   hResult = DBSelect(hdb, _T("SELECT max(script_id) FROM script_library"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SCRIPT] = std::max(s_freeIdTable[IDG_SCRIPT], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available agent config id
   hResult = DBSelect(hdb, _T("SELECT max(config_id) FROM agent_configs"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_AGENT_CONFIG] = std::max(s_freeIdTable[IDG_AGENT_CONFIG], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available graph id
   hResult = DBSelect(hdb, _T("SELECT max(graph_id) FROM graphs"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_GRAPH] = std::max(s_freeIdTable[IDG_GRAPH], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available business service ticket id
   hResult = DBSelect(hdb, _T("SELECT max(ticket_id) FROM business_service_tickets"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_BUSINESS_SERVICE_TICKET] = std::max(s_freeIdTable[IDG_BUSINESS_SERVICE_TICKET], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available mapping table id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM mapping_tables"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_MAPPING_TABLE] = std::max(s_freeIdTable[IDG_MAPPING_TABLE], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available DCI summary table id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dci_summary_tables"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_DCI_SUMMARY_TABLE] = std::max(s_freeIdTable[IDG_DCI_SUMMARY_TABLE], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm category id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM alarm_categories"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM_CATEGORY] = std::max(s_freeIdTable[IDG_ALARM_CATEGORY], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user agent message id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM user_agent_notifications"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_UA_MESSAGE] = std::max(s_freeIdTable[IDG_UA_MESSAGE], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available rack passive element id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM rack_passive_elements"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_RACK_ELEMENT] = std::max(s_freeIdTable[IDG_RACK_ELEMENT], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available physical link id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM physical_links"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_PHYSICAL_LINK] = std::max(s_freeIdTable[IDG_PHYSICAL_LINK], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available web service definition id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM websvc_definitions"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_WEBSVC_DEFINITION] = std::max(s_freeIdTable[IDG_WEBSVC_DEFINITION], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object category id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM object_categories"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_OBJECT_CATEGORY] = std::max(s_freeIdTable[IDG_OBJECT_CATEGORY], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available geo area id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM geo_areas"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_GEO_AREA] = std::max(s_freeIdTable[IDG_GEO_AREA], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SSH key id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM ssh_keys"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SSH_KEY] = std::max(s_freeIdTable[IDG_SSH_KEY], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object query id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM object_queries"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_OBJECT_QUERY] = std::max(s_freeIdTable[IDG_OBJECT_QUERY], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object in business service checks
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM business_service_checks"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_BUSINESS_SERVICE_CHECK] = std::max(s_freeIdTable[IDG_BUSINESS_SERVICE_CHECK], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object in business service downtime records
   hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM business_service_downtime"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_BUSINESS_SERVICE_RECORD] = std::max(s_freeIdTable[IDG_BUSINESS_SERVICE_RECORD], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

    // Get first available object in maintenance journal entries
   hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM maintenance_journal"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_MAINTENANCE_JOURNAL] = std::max(s_freeIdTable[IDG_MAINTENANCE_JOURNAL], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available authentication token id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM auth_tokens"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_AUTHTOKEN] = std::max(s_freeIdTable[IDG_AUTHTOKEN], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available package deployment job id
   id = ConfigReadULong(_T("FirstFreeDeploymentJobId"), s_freeIdTable[IDG_PACKAGE_DEPLOYMENT_JOB]);
   if (id > s_freeIdTable[IDG_PACKAGE_DEPLOYMENT_JOB])
      s_freeIdTable[IDG_PACKAGE_DEPLOYMENT_JOB] = id;
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM package_deployment_jobs"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_PACKAGE_DEPLOYMENT_JOB] = std::max(s_freeIdTable[IDG_PACKAGE_DEPLOYMENT_JOB], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   LoadLastEventId(hdb);

   DBConnectionPoolReleaseConnection(hdb);
   return true;
}

/**
 * Create unique ID
 */
uint32_t NXCORE_EXPORTABLE CreateUniqueId(int group)
{
   uint32_t id;
   s_mutexTableAccess.lock();
   if (s_freeIdTable[group] == s_idLimits[group])
   {
      id = 0;   // ID zero means _T("no unique ID available")
      nxlog_write(NXLOG_ERROR, _T("Unable to assign unique ID to object in group \"%s\". You should perform database optimization to fix that."), s_groupNames[group]);
   }
   else
   {
      id = s_freeIdTable[group];
      s_freeIdTable[group]++;
   }
   s_mutexTableAccess.unlock();
   return id;
}

/**
 * Save current first free IDs
 */
void SaveCurrentFreeId()
{
   s_mutexTableAccess.lock();
   uint32_t objectId = s_freeIdTable[IDG_NETWORK_OBJECT];
   uint32_t dciId = s_freeIdTable[IDG_ITEM];
   uint32_t alarmId = s_freeIdTable[IDG_ALARM];
   uint32_t deploymentJobId = s_freeIdTable[IDG_PACKAGE_DEPLOYMENT_JOB];
   s_mutexTableAccess.unlock();
   ConfigWriteULong(_T("FirstFreeObjectId"), objectId, true, false, true);
   ConfigWriteULong(_T("FirstFreeDCIId"), dciId, true, false, true);
   ConfigWriteULong(_T("FirstFreeAlarmId"), alarmId, true, false, true);
   ConfigWriteULong(_T("FirstFreeDeploymentJobId"), deploymentJobId, true, false, true);
   ConfigWriteInt(_T("LastAuditRecordId"), GetLastAuditRecordId(), true, false, true);
   ConfigWriteInt64(_T("LastEventId"), GetLastEventId(), true, false, true);
   ConfigWriteInt64(_T("LastSNMPTrapId"), GetLastSnmpTrapId(), true, false, true);
   ConfigWriteUInt64(_T("FirstFreeSyslogId"), GetNextSyslogId(), true, false, true);
   ConfigWriteUInt64(_T("FirstFreeWinEventId"), GetNextWinEventId(), true, false, true);
   ConfigWriteInt64(_T("LastActionExecutionLogRecordId"), GetLastActionExecutionLogId(), true, false, true);
   ConfigWriteInt64(_T("LastNotificationId"), GetLastNotificationId(), true, false, true);
   ConfigWriteUInt64(_T("LastAssetChangeLogRecordId"), GetLastAssetChangeLogId(), true, false, true);
}
