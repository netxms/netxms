/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Constants
//

#define NUMBER_OF_GROUPS   21


//
// Static data
//

static MUTEX m_mutexTableAccess;
static DWORD m_dwFreeIdTable[NUMBER_OF_GROUPS] = { 100, 1, FIRST_USER_EVENT_ID, 1, 1, 
                                                   1, 1, 0x80000000,
                                                   1, 1, 0x80000001, 1, 1, 1, 1,
                                                   10000, 10000, 1, 1, 1, 1
                                                 };
static DWORD m_dwIdLimits[NUMBER_OF_GROUPS] = { 0xFFFFFFFE, 0xFFFFFFFE, 0x7FFFFFFF, 0x7FFFFFFF, 
                                                0x7FFFFFFF, 0xFFFFFFFE, 0x7FFFFFFF, 0xFFFFFFFF,
                                                0x7FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFE, 0xFFFFFFFE,
                                                0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
                                                0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE,
																0xFFFFFFFE
                                              };
static QWORD m_qwFreeEventId = 1;
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
   _T("--Log Processing Policies"),
   _T("Object Tools"),
   _T("Scripts"),
   _T("Agent Configs"),
	_T("Graphs"),
	_T("Certificates"),
	_T("Situations")
};


//
// Initialize ID table
//

BOOL InitIdTable()
{
   DB_RESULT hResult;

   m_mutexTableAccess = MutexCreate();

   // Get first available network object ID
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM nodes"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM subnets"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM interfaces"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM containers"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM templates"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM network_services"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM conditions"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM clusters"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM ap_common"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM network_maps"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM dashboards"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM reports"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(object_id) FROM deleted_objects"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available container category id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(category) FROM container_categories"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_CONTAINER_CAT] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event code
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(event_code) FROM event_cfg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_EVENT] = max(m_dwFreeIdTable[IDG_EVENT], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection item id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(item_id) FROM items"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ITEM] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available server job id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM job_history"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_JOB] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available SNMP trap configuration record id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(trap_id) FROM snmp_trap_cfg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_SNMP_TRAP] = max(m_dwFreeIdTable[IDG_SNMP_TRAP], DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available action id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(action_id) FROM actions"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ACTION] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event group id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM event_groups"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_EVENT_GROUP] = max(0x80000000, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available threshold id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(threshold_id) FROM thresholds"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_THRESHOLD] = max(m_dwFreeIdTable[IDG_THRESHOLD], 
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM users"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_USER] = max(m_dwFreeIdTable[IDG_USER], 
                                         DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user group id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM user_groups"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_USER_GROUP] = max(m_dwFreeIdTable[IDG_USER_GROUP], 
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(alarm_id) FROM alarms"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ALARM] = max(m_dwFreeIdTable[IDG_ALARM], 
                                          DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm note id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(note_id) FROM alarm_notes"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ALARM_NOTE] = max(m_dwFreeIdTable[IDG_ALARM_NOTE], 
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event identifier
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(event_id) FROM event_log"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_qwFreeEventId = max(m_qwFreeEventId, DBGetFieldUInt64(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available package id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(pkg_id) FROM agent_pkg"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_PACKAGE] = max(m_dwFreeIdTable[IDG_PACKAGE], 
                                            DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available object tool id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(tool_id) FROM object_tools"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_OBJECT_TOOL] = max(m_dwFreeIdTable[IDG_OBJECT_TOOL], 
                                                DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available script id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(script_id) FROM script_library"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_SCRIPT] = max(m_dwFreeIdTable[IDG_SCRIPT], 
                                           DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available agent config id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(config_id) FROM agent_configs"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_AGENT_CONFIG] = max(m_dwFreeIdTable[IDG_AGENT_CONFIG], 
                                                 DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available graph id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(graph_id) FROM graphs"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_GRAPH] = max(m_dwFreeIdTable[IDG_GRAPH],
                                          DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available certificate id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(cert_id) FROM certificates"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_CERTIFICATE] = max(m_dwFreeIdTable[IDG_CERTIFICATE],
                                                DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available situation id
   hResult = DBSelect(g_hCoreDB, _T("SELECT max(id) FROM situations"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_SITUATION] = max(m_dwFreeIdTable[IDG_SITUATION],
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   return TRUE;
}


//
// Create unique ID
//

DWORD CreateUniqueId(int iGroup)
{
   DWORD dwId;

   MutexLock(m_mutexTableAccess, INFINITE);
   if (m_dwFreeIdTable[iGroup] == m_dwIdLimits[iGroup])
   {
      dwId = 0;   // ID zero means _T("no unique ID available")
      nxlog_write(MSG_NO_UNIQUE_ID, EVENTLOG_ERROR_TYPE, "s", m_pszGroupNames[iGroup]);
   }
   else
   {
      dwId = m_dwFreeIdTable[iGroup];
      m_dwFreeIdTable[iGroup]++;
   }
   MutexUnlock(m_mutexTableAccess);
   return dwId;
}


//
// Create unique ID for event log record
//

QWORD CreateUniqueEventId()
{
   QWORD qwId;

   MutexLock(m_mutexTableAccess, INFINITE);
   qwId = m_qwFreeEventId++;
   MutexUnlock(m_mutexTableAccess);
   return qwId;
}
