/* 
** NetXMS - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: id.cpp
**
**/

#include "nms_core.h"


//
// Constants
//

#define NUMBER_OF_GROUPS   13


//
// Static data
//

static MUTEX m_mutexTableAccess;
static DWORD m_dwFreeIdTable[NUMBER_OF_GROUPS] = { 2, 0x80000000, 10000, 1, 1, 1, 1, 0x80000000,
                                                   1, 1, 0x80000001, 1, 1 };
static DWORD m_dwIdLimits[NUMBER_OF_GROUPS] = { 0x7FFFFFFF, 0xFFFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 
                                                0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
                                                0x7FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFE, 0xFFFFFFFE,
                                                0xFFFFFFFE
                                              };
static char *m_pszGroupNames[] =
{
   "Network Objects",
   "Node Groups",
   "User-defined Events",
   "Data Collection Items",
   "Data Collection Templates",
   "Data Collection Template Items",
   "Actions",
   "Event Groups",
   "Data Collection Thresholds",
   "Users",
   "User Groups",
   "Alarms",
   "Alarm Notes"
};


//
// Initialize ID table
//

BOOL InitIdTable(void)
{
   DB_RESULT hResult;

   m_mutexTableAccess = MutexCreate();

   // Get first available network object ID
   hResult = DBSelect(g_hCoreDB, "SELECT max(id) FROM nodes");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, "SELECT max(id) FROM subnets");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, "SELECT max(id) FROM interfaces");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }
   hResult = DBSelect(g_hCoreDB, "SELECT max(object_id) FROM deleted_objects");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETWORK_OBJECT] = max(m_dwFreeIdTable[IDG_NETWORK_OBJECT],
                                                   DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available network object group id
   hResult = DBSelect(g_hCoreDB, "SELECT max(id) FROM node_groups");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_NETOBJ_GROUP] = max(0x80000000, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event id
   hResult = DBSelect(g_hCoreDB, "SELECT max(event_id) FROM events");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_EVENT] = max(10000, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection item id
   hResult = DBSelect(g_hCoreDB, "SELECT max(item_id) FROM items");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ITEM] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection template id
   hResult = DBSelect(g_hCoreDB, "SELECT max(template_id) FROM dct");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_DCT] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available data collection template item id
   hResult = DBSelect(g_hCoreDB, "SELECT max(item_id) FROM dct_items");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_DCT_ITEM] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available action id
   hResult = DBSelect(g_hCoreDB, "SELECT max(action_id) FROM actions");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ACTION] = max(1, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available event group id
   hResult = DBSelect(g_hCoreDB, "SELECT max(id) FROM event_groups");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_EVENT_GROUP] = max(0x80000000, DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available threshold id
   hResult = DBSelect(g_hCoreDB, "SELECT max(threshold_id) FROM thresholds");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_THRESHOLD] = max(m_dwFreeIdTable[IDG_THRESHOLD], 
                                              DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user id
   hResult = DBSelect(g_hCoreDB, "SELECT max(id) FROM users");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_USER] = max(m_dwFreeIdTable[IDG_USER], 
                                         DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available user group id
   hResult = DBSelect(g_hCoreDB, "SELECT max(id) FROM user_groups");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_USER_GROUP] = max(m_dwFreeIdTable[IDG_USER_GROUP], 
                                               DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm id
   hResult = DBSelect(g_hCoreDB, "SELECT max(alarm_id) FROM alarms");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ALARM] = max(m_dwFreeIdTable[IDG_ALARM], 
                                          DBGetFieldULong(hResult, 0, 0) + 1);
      DBFreeResult(hResult);
   }

   // Get first available alarm note id
   hResult = DBSelect(g_hCoreDB, "SELECT max(note_id) FROM alarm_notes");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_dwFreeIdTable[IDG_ALARM_NOTE] = max(m_dwFreeIdTable[IDG_ALARM_NOTE], 
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
      dwId = 0;   // ID zero means "no unique ID available"
      WriteLog(MSG_NO_UNIQUE_ID, EVENTLOG_ERROR_TYPE, "s", m_pszGroupNames[iGroup]);
   }
   else
   {
      dwId = m_dwFreeIdTable[iGroup];
      m_dwFreeIdTable[iGroup]++;
   }
   MutexUnlock(m_mutexTableAccess);
   return dwId;
}
