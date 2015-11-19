/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
 * Constants
 */
#define NUMBER_OF_GROUPS   25

/**
 * Static data
 */
static MUTEX s_mutexTableAccess;
static UINT32 s_freeIdTable[NUMBER_OF_GROUPS] = { 100, 1, FIRST_USER_EVENT_ID, 1, 1,
                                                   1, 1, 0x80000000,
                                                   1, 1, 0x80000001, 1, 1, 1, 1,
                                                   10000, 10000, 1, 1, 1, 1, 1, 1, 1, 1
                                                 };
static UINT32 s_idLimits[NUMBER_OF_GROUPS] = { 0xFFFFFFFE, 0xFFFFFFFE, 0x7FFFFFFF, 0x7FFFFFFF,
                                                0x7FFFFFFF, 0xFFFFFFFE, 0x7FFFFFFF, 0xFFFFFFFF,
                                                0x7FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFE, 0xFFFFFFFE,
                                                0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
                                                0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
																0xFFFFFFFE, 0x7FFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
																0xFFFFFFFE
                                              };
static UINT64 m_freeEventId = 1;
static const TCHAR *m_pszGroupNames[NUMBER_OF_GROUPS] =
{
   _T("Network Objects"),
   _T("Container Categories"),
   _T("Events"),
   _T("Data Collection Items"),
   _T("SNMP Trap"),
   _T("Jobs"),
   _T("Actions"),
   _T("Event Groups"),
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
	_T("Certificates"),
	_T("Situations"),
	_T("Table Columns"),
	_T("Mapping Tables"),
   _T("DCI Summary Tables"),
   _T("Scheduled Tasks")
};

/**
 * Initialize ID table
 */
BOOL InitIdTable()
{
   DB_RESULT hResult;

   s_mutexTableAccess = MutexCreate();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Get first available network object ID
	UINT32 id = ConfigReadULong(_T("FirstFreeObjectId"), s_freeIdTable[IDG_NETWORK_OBJECT]);
	if (id > s_freeIdTable[IDG_NETWORK_OBJECT])
		s_freeIdTable[IDG_NETWORK_OBJECT] = id;
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM nodes"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM subnets"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM interfaces"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM containers"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM templates"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM network_services"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM conditions"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM clusters"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM ap_common"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM network_maps"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dashboards"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM slm_checks"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM mobile_devices"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM access_points"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_NETWORK_OBJECT] = max(s_freeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available container category id
   hResult = DBSelect(hdb, _T("SELECT max(category) FROM container_categories"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_CONTAINER_CAT] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event code
   hResult = DBSelect(hdb, _T("SELECT max(event_code) FROM event_cfg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_EVENT] = max(s_freeIdTable[IDG_EVENT], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection object id
   hResult = DBSelect(hdb, _T("SELECT max(item_id) FROM items"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(item_id) FROM dc_tables"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ITEM] = max(s_freeIdTable[IDG_ITEM], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available server job id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM job_history"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_JOB] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SNMP trap configuration record id
   hResult = DBSelect(hdb, _T("SELECT max(trap_id) FROM snmp_trap_cfg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SNMP_TRAP] = max(s_freeIdTable[IDG_SNMP_TRAP], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available action id
   hResult = DBSelect(hdb, _T("SELECT max(action_id) FROM actions"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ACTION] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event group id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM event_groups"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_EVENT_GROUP] = max(0x80000000, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available threshold id
   hResult = DBSelect(hdb, _T("SELECT max(threshold_id) FROM thresholds"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_THRESHOLD] = max(s_freeIdTable[IDG_THRESHOLD],
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dct_thresholds"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_THRESHOLD] = max(s_freeIdTable[IDG_THRESHOLD],
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM users"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_USER] = max(s_freeIdTable[IDG_USER],
                                         DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user group id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM user_groups"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_USER_GROUP] = max(s_freeIdTable[IDG_USER_GROUP],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm id
   hResult = DBSelect(hdb, _T("SELECT max(alarm_id) FROM alarms"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM] = max(s_freeIdTable[IDG_ALARM],
                                          DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm note id
   hResult = DBSelect(hdb, _T("SELECT max(note_id) FROM alarm_notes"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_ALARM_NOTE] = max(s_freeIdTable[IDG_ALARM_NOTE],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event identifier
   hResult = DBSelect(hdb, _T("SELECT max(event_id) FROM event_log"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_freeEventId = max(m_freeEventId, DBGetFieldUInt64(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available package id
   hResult = DBSelect(hdb, _T("SELECT max(pkg_id) FROM agent_pkg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_PACKAGE] = max(s_freeIdTable[IDG_PACKAGE],
                                            DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object tool id
   hResult = DBSelect(hdb, _T("SELECT max(tool_id) FROM object_tools"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_OBJECT_TOOL] = max(s_freeIdTable[IDG_OBJECT_TOOL],
                                                DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available script id
   hResult = DBSelect(hdb, _T("SELECT max(script_id) FROM script_library"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SCRIPT] = max(s_freeIdTable[IDG_SCRIPT],
                                           DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available agent config id
   hResult = DBSelect(hdb, _T("SELECT max(config_id) FROM agent_configs"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_AGENT_CONFIG] = max(s_freeIdTable[IDG_AGENT_CONFIG],
                                                 DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available graph id
   hResult = DBSelect(hdb, _T("SELECT max(graph_id) FROM graphs"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_GRAPH] = max(s_freeIdTable[IDG_GRAPH],
                                          DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available certificate id
   hResult = DBSelect(hdb, _T("SELECT max(cert_id) FROM certificates"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_CERTIFICATE] = max(s_freeIdTable[IDG_CERTIFICATE],
                                                DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available situation id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM situations"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SITUATION] = max(s_freeIdTable[IDG_SITUATION],
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SLM ticket id
   hResult = DBSelect(hdb, _T("SELECT max(ticket_id) FROM slm_tickets"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SLM_TICKET] = max(s_freeIdTable[IDG_SLM_TICKET],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection table column id
   hResult = DBSelect(hdb, _T("SELECT max(column_id) FROM dct_column_names"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_DCT_COLUMN] = max(s_freeIdTable[IDG_DCT_COLUMN],
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available mapping table id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM mapping_tables"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_MAPPING_TABLE] = max(s_freeIdTable[IDG_MAPPING_TABLE],
                                                  DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available DCI summary table id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM dci_summary_tables"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_DCI_SUMMARY_TABLE] = max(s_freeIdTable[IDG_DCI_SUMMARY_TABLE],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available scheduled_tasks id
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM scheduled_tasks"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         s_freeIdTable[IDG_SCHEDULED_TASK] = max(s_freeIdTable[IDG_SCHEDULED_TASK],
                                                      DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return TRUE;
}

/**
 * Create unique ID
 */
UINT32 CreateUniqueId(int iGroup)
{
   UINT32 dwId;

   MutexLock(s_mutexTableAccess);
   if (s_freeIdTable[iGroup] == s_idLimits[iGroup])
   {
      dwId = 0;   // ID zero means _T("no unique ID available")
      nxlog_write(MSG_NO_UNIQUE_ID, EVENTLOG_ERROR_TYPE, "s", m_pszGroupNames[iGroup]);
   }
   else
   {
      dwId = s_freeIdTable[iGroup];
      s_freeIdTable[iGroup]++;
   }
   MutexUnlock(s_mutexTableAccess);
   return dwId;
}

/**
 * Create unique ID for event log record
 */
QWORD CreateUniqueEventId()
{
   QWORD qwId;

   MutexLock(s_mutexTableAccess);
   qwId = m_freeEventId++;
   MutexUnlock(s_mutexTableAccess);
   return qwId;
}

/**
 * Save current first free IDs
 */
void SaveCurrentFreeId()
{
   MutexLock(s_mutexTableAccess);
   UINT32 id = s_freeIdTable[IDG_NETWORK_OBJECT];
   MutexUnlock(s_mutexTableAccess);
	ConfigWriteULong(_T("FirstFreeObjectId"), id, TRUE, FALSE, TRUE);
}
