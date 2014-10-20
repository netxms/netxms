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
** File: hk.cpp
**
**/

#include "nxcore.h"

/**
 * Delete empty subnets
 */
static void DeleteEmptySubnets()
{
	ObjectArray<NetObj> *subnets = g_idxSubnetByAddr.getObjects(true);
	for(int i = 0; i < subnets->size(); i++)
	{
		NetObj *object = subnets->get(i);
		if (object->isEmpty())
		{
		   DbgPrintf(5, _T("DeleteEmptySubnets: subnet %s [%d] has %d refs, children: %d, parents: %d"),
            object->getName(), object->getId(), object->getRefCount(), object->getChildCount(), object->getParentCount());
			object->deleteObject();
		}
      object->decRefCount();
	}
	delete subnets;
}

/**
 * Delete notes for alarm
 */
void DeleteAlarmNotes(DB_HANDLE hdb, UINT32 alarmId)
{
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_notes WHERE alarm_id=?"));
	if (hStmt == NULL)
		return;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
	DBExecute(hStmt);
	DBFreeStatement(hStmt);
}

/**
 * Delete realted events for alarm
 */
void DeleteAlarmEvents(DB_HANDLE hdb, UINT32 alarmId)
{
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_events WHERE alarm_id=?"));
	if (hStmt == NULL)
		return;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
	DBExecute(hStmt);
	DBFreeStatement(hStmt);
}

/**
 * Remove outdated alarm records
 */
static void CleanAlarmHistory(DB_HANDLE hdb)
{
	UINT32 retentionTime = ConfigReadULong(_T("AlarmHistoryRetentionTime"), 180);
	if (retentionTime == 0)
		return;

	retentionTime *= 86400;	// Convert days to seconds
	time_t ts = time(NULL) - (time_t)retentionTime;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT alarm_id FROM alarms WHERE alarm_state=2 AND last_change_time<?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)ts);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
         {
            UINT32 alarmId = DBGetFieldULong(hResult, i, 0);
				DeleteAlarmNotes(hdb, alarmId);
            DeleteAlarmEvents(hdb, alarmId);
         }
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	hStmt = DBPrepare(hdb, _T("DELETE FROM alarms WHERE alarm_state=2 AND last_change_time<?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)ts);
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
}

/**
 * Callback for cleaning expired DCI data on node
 */
static void CleanDciData(NetObj *object, void *data)
{
	((DataCollectionTarget *)object)->cleanDCIData();
}

/**
 * Housekeeper thread
 */
THREAD_RESULT THREAD_CALL HouseKeeper(void *pArg)
{
   time_t currTime;
   TCHAR szQuery[256];
   UINT32 dwRetentionTime, dwInterval;

   // Load configuration
   dwInterval = ConfigReadULong(_T("HouseKeepingInterval"), 3600);

   // Housekeeping loop
   while(!IsShutdownInProgress())
   {
      currTime = time(NULL);
      if (SleepAndCheckForShutdown(dwInterval - (UINT32)(currTime % dwInterval)))
         break;      // Shutdown has arrived

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

		CleanAlarmHistory(hdb);

		// Remove outdated event log records
		dwRetentionTime = ConfigReadULong(_T("EventLogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
			dwRetentionTime *= 86400;	// Convert days to seconds
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM event_log WHERE event_timestamp<%ld"), (long)(currTime - dwRetentionTime));
			DBQuery(hdb, szQuery);
		}

		// Remove outdated syslog records
		dwRetentionTime = ConfigReadULong(_T("SyslogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
			dwRetentionTime *= 86400;	// Convert days to seconds
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM syslog WHERE msg_timestamp<%ld"), (long)(currTime - dwRetentionTime));
			DBQuery(hdb, szQuery);
		}

		// Remove outdated audit log records
		dwRetentionTime = ConfigReadULong(_T("AuditLogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
			dwRetentionTime *= 86400;	// Convert days to seconds
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM audit_log WHERE timestamp<%ld"), (long)(currTime - dwRetentionTime));
			DBQuery(hdb, szQuery);
		}

		// Remove outdated SNMP trap log records
		dwRetentionTime = ConfigReadULong(_T("SNMPTrapLogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
			dwRetentionTime *= 86400;	// Convert days to seconds
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM snmp_trap_log WHERE trap_timestamp<%ld"), (long)(currTime - dwRetentionTime));
			DBQuery(hdb, szQuery);
		}

		// Delete empty subnets if needed
		if (g_flags & AF_DELETE_EMPTY_SUBNETS)
			DeleteEmptySubnets();

		// Remove expired DCI data
		g_idxNodeById.forEach(CleanDciData, NULL);
		g_idxClusterById.forEach(CleanDciData, NULL);
		g_idxMobileDeviceById.forEach(CleanDciData, NULL);

		DBConnectionPoolReleaseConnection(hdb);

		SaveCurrentFreeId();
   }

   DbgPrintf(1, _T("Housekeeper thread terminated"));
   return THREAD_OK;
}
