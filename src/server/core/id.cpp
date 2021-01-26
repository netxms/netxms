/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
int32_t GetLastAuditRecordId();
int64_t GetLastEventId();
int64_t GetLastSnmpTrapId();
uint64_t GetNextSyslogId();
uint64_t GetNextWinEventId();
void LoadLastEventId(DB_HANDLE hdb);

/**
 * Constants
 */
#define NUMBER_OF_GROUPS   29

/**
 * Static data
 */
static MUTEX s_mutexTableAccess;
static uint32_t s_freeIdTable[NUMBER_OF_GROUPS] =
   {
      100, FIRST_USER_EVENT_ID, 1, 1,
      1, 1, 1, 1,
      0x40000001, 1, 1, 1,
      1, 10000, 10000, 1,
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 1,
      1
   };
static uint32_t s_idLimits[NUMBER_OF_GROUPS] =
   {
      0xFFFFFFFE, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
      0xFFFFFFFE, 0x7FFFFFFF, 0x7FFFFFFF, 0x3FFFFFFF,
      0x7FFFFFFF, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0x7FFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
      0xFFFFFFFE
   };
static const TCHAR *m_pszGroupNames[NUMBER_OF_GROUPS] =
{
   _T("Network Objects"),
   _T("Events"),
   _T("Data Collection Items"),
   _T("SNMP Trap"),
   _T("Jobs"),
   _T("Actions"),
   _T("Data Collection Thresholds"),
   _T("Users"),
   _T("User Groups"),
   _T("Alarms"),
   _T("Alarm Notes"),
   _T("Packages"),
   _T("SLM Ticket"),
   _T("Object Tools"),
   _T("Scripts"),
   _T("Agent Configs"),
   _T("Graphs"),
   _T("Table Columns"),
   _T("Mapping Tables"),
   _T("DCI Summary Tables"),
   _T("Scheduled Tasks"),
   _T("Alarm Categories"),
   _T("User Agent Messages"),
   _T("Passive Rack Elements"),
   _T("Physical Links"),
   _T("Web Service Definitions"),
   _T("Object Categories"),
   _T("Geographical Areas"),
   _T("SSH Key id")
};

/**
 * Initialize ID table
 */
bool InitIdTable()
{
   DB_RESULT hResult;

   s_mutexTableAccess = MutexCreateFast();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Get first available network object ID
	uint32_t id = ConfigReadULong(_T("FirstFreeObjectId"), s_freeIdTable[IDG_NETWORK_OBJECT]);
	if (id > s_freeIdTable[IDG_NETWORK_OBJECT])
		s_freeIdTable[IDG_NETWORK_OBJECT] = id;
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM nodes"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM subnets"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM interfaces"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM object_containers"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM templates"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM network_services"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM conditions"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM clusters"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM network_maps"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dashboards"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM slm_checks"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM mobile_devices"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM access_points"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM vpn_connectors"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(object_id) FROM object_properties"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = std::max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event code
   hResult = DBSelect(hdb, _T("SELECT max(event_code) FROM event_cfg"));
   if (hResult != NULL)
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
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = std::max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(item_id) FROM dc_tables"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = std::max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(dci_id) FROM dci_delete_list"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = std::max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available server job id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM job_history"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_JOB] = std::max(s_freeIdTable[IDG_JOB], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SNMP trap configuration record id
   hResult = DBSelect(hdb, _T("SELECT max(trap_id) FROM snmp_trap_cfg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SNMP_TRAP] = std::max(s_freeIdTable[IDG_SNMP_TRAP], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available action id
   hResult = DBSelect(hdb, _T("SELECT max(action_id) FROM actions"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ACTION] = std::max(s_freeIdTable[IDG_ACTION], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available threshold id
   hResult = DBSelect(hdb, _T("SELECT max(threshold_id) FROM thresholds"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_THRESHOLD] = std::max(s_freeIdTable[IDG_THRESHOLD],
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dct_thresholds"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_THRESHOLD] = std::max(s_freeIdTable[IDG_THRESHOLD],
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM users"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_USER] = std::max(s_freeIdTable[IDG_USER],
                                         DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user group id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM user_groups"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_USER_GROUP] = std::max(s_freeIdTable[IDG_USER_GROUP],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm id
   id = ConfigReadULong(_T("FirstFreeAlarmId"), s_freeIdTable[IDG_ALARM]);
   if (id > s_freeIdTable[IDG_ALARM])
      s_freeIdTable[IDG_ALARM] = id;
   hResult = DBSelect(hdb, _T("SELECT max(alarm_id) FROM alarms"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM] = std::max(s_freeIdTable[IDG_ALARM],
                                          DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm note id
   hResult = DBSelect(hdb, _T("SELECT max(note_id) FROM alarm_notes"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM_NOTE] = std::max(s_freeIdTable[IDG_ALARM_NOTE],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available package id
   hResult = DBSelect(hdb, _T("SELECT max(pkg_id) FROM agent_pkg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_PACKAGE] = std::max(s_freeIdTable[IDG_PACKAGE],
                                            DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object tool id
   hResult = DBSelect(hdb, _T("SELECT max(tool_id) FROM object_tools"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_OBJECT_TOOL] = std::max(s_freeIdTable[IDG_OBJECT_TOOL],
                                                DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available script id
   hResult = DBSelect(hdb, _T("SELECT max(script_id) FROM script_library"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SCRIPT] = std::max(s_freeIdTable[IDG_SCRIPT],
                                           DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available agent config id
   hResult = DBSelect(hdb, _T("SELECT max(config_id) FROM agent_configs"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_AGENT_CONFIG] = std::max(s_freeIdTable[IDG_AGENT_CONFIG],
                                                 DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available graph id
   hResult = DBSelect(hdb, _T("SELECT max(graph_id) FROM graphs"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_GRAPH] = std::max(s_freeIdTable[IDG_GRAPH],
                                          DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SLM ticket id
   hResult = DBSelect(hdb, _T("SELECT max(ticket_id) FROM slm_tickets"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SLM_TICKET] = std::max(s_freeIdTable[IDG_SLM_TICKET],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection table column id
   hResult = DBSelect(hdb, _T("SELECT max(column_id) FROM dct_column_names"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_DCT_COLUMN] = std::max(s_freeIdTable[IDG_DCT_COLUMN],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available mapping table id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM mapping_tables"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_MAPPING_TABLE] = std::max(s_freeIdTable[IDG_MAPPING_TABLE],
                                                  DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available DCI summary table id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dci_summary_tables"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_DCI_SUMMARY_TABLE] = std::max(s_freeIdTable[IDG_DCI_SUMMARY_TABLE],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available scheduled_tasks id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM scheduled_tasks"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SCHEDULED_TASK] = std::max(s_freeIdTable[IDG_SCHEDULED_TASK],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm category id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM alarm_categories"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM_CATEGORY] = std::max(s_freeIdTable[IDG_ALARM_CATEGORY],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user agent message id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM user_agent_notifications"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_UA_MESSAGE] = std::max(s_freeIdTable[IDG_UA_MESSAGE],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available rack passive element id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM rack_passive_elements"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_RACK_ELEMENT] = std::max(s_freeIdTable[IDG_RACK_ELEMENT],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available physical link id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM physical_links"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_PHYSICAL_LINK] = std::max(s_freeIdTable[IDG_PHYSICAL_LINK],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available web service definition id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM websvc_definitions"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_WEBSVC_DEFINITION] = std::max(s_freeIdTable[IDG_WEBSVC_DEFINITION],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object category id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM object_categories"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_OBJECT_CATEGORIES] = std::max(s_freeIdTable[IDG_OBJECT_CATEGORIES],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available geo area id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM geo_areas"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_GEO_AREAS] = std::max(s_freeIdTable[IDG_GEO_AREAS], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SSH key id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM ssh_keys"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SSH_KEYS] = std::max(s_freeIdTable[IDG_SSH_KEYS], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   LoadLastEventId(hdb);

   DBConnectionPoolReleaseConnection(hdb);
   return true;
}

/**
 * Create unique ID
 */
uint32_t CreateUniqueId(int iGroup)
{
   uint32_t id;
   MutexLock(s_mutexTableAccess);
   if (s_freeIdTable[iGroup] == s_idLimits[iGroup])
   {
      id = 0;   // ID zero means _T("no unique ID available")
      nxlog_write(NXLOG_ERROR, _T("Unable to assign unique ID to object in group \"%s\". You should perform database optimization to fix that."), m_pszGroupNames[iGroup]);
   }
   else
   {
      id = s_freeIdTable[iGroup];
      s_freeIdTable[iGroup]++;
   }
   MutexUnlock(s_mutexTableAccess);
   return id;
}

/**
 * Save current first free IDs
 */
void SaveCurrentFreeId()
{
   MutexLock(s_mutexTableAccess);
   uint32_t objectId = s_freeIdTable[IDG_NETWORK_OBJECT];
   uint32_t dciId = s_freeIdTable[IDG_ITEM];
   uint32_t alarmId = s_freeIdTable[IDG_ALARM];
   MutexUnlock(s_mutexTableAccess);
   ConfigWriteULong(_T("FirstFreeObjectId"), objectId, true, false, true);
   ConfigWriteULong(_T("FirstFreeDCIId"), dciId, true, false, true);
   ConfigWriteULong(_T("FirstFreeAlarmId"), alarmId, true, false, true);
   ConfigWriteInt(_T("LastAuditRecordId"), GetLastAuditRecordId(), true, false, true);
   ConfigWriteInt64(_T("LastEventId"), GetLastEventId(), true, false, true);
   ConfigWriteInt64(_T("LastSNMPTrapId"), GetLastSnmpTrapId(), true, false, true);
   ConfigWriteUInt64(_T("FirstFreeSyslogId"), GetNextSyslogId(), true, false, true);
   ConfigWriteUInt64(_T("FirstFreeWinEventId"), GetNextWinEventId(), true, false, true);
}
