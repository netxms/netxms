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
** File: hk.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("housekeeper")

/**
 * Housekeeper wakeup condition
 */
static CONDITION s_wakeupCondition = INVALID_CONDITION_HANDLE;

/**
 * Housekeeper shutdown flag
 */
static bool s_shutdown = false;

/**
 * Throttling parameters
 */
static size_t s_throttlingHighWatermark = 250000;
static size_t s_throttlingLowWatermark = 50000;

/**
 * Throttle housekeeper if needed. Returns false if shutdown time has arrived and housekeeper process should be aborted.
 */
bool ThrottleHousekeeper()
{
   size_t qsize = g_dbWriterQueue.size() + GetIDataWriterQueueSize() + GetRawDataWriterQueueSize();
   if (qsize < s_throttlingHighWatermark)
      return true;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Housekeeper paused (queue size %d, high watermark %d, low watermark %d)"), qsize, s_throttlingHighWatermark, s_throttlingLowWatermark);
   while((qsize >= s_throttlingLowWatermark) && !s_shutdown)
   {
      ConditionWait(s_wakeupCondition, 30000);
      qsize = g_dbWriterQueue.size() + GetIDataWriterQueueSize() + GetRawDataWriterQueueSize();
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Housekeeper resumed (queue size %d)"), qsize);
   return !s_shutdown;
}

/**
 * Delete empty subnets from given list
 */
static void DeleteEmptySubnetsFromList(SharedObjectArray<NetObj> *subnets)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: %d subnets to check"), subnets->size());
   for(int i = 0; i < subnets->size(); i++)
   {
      NetObj *object = subnets->get(i);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: checking subnet %s [%d] (children: %d, parents: %d)"),
                object->getName(), object->getId(), object->getChildCount(), object->getParentCount());
      if (object->isEmpty())
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("DeleteEmptySubnets: delete subnet %s [%d] (children: %d, parents: %d)"),
                   object->getName(), object->getId(), object->getChildCount(), object->getParentCount());
         object->deleteObject();
      }
   }
}

/**
 * Delete empty subnets
 */
static void DeleteEmptySubnets()
{
   if (IsZoningEnabled())
   {
      SharedObjectArray<NetObj> *zones = g_idxZoneByUIN.getObjects();
      nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: %d zones to check"), zones->size());
      for(int i = 0; i < zones->size(); i++)
      {
         Zone *zone = static_cast<Zone*>(zones->get(i));
         nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: processing zone %s (UIN=%u)"), zone->getName(), zone->getUIN());
         SharedObjectArray<NetObj> *subnets = zone->getSubnets();
         DeleteEmptySubnetsFromList(subnets);
         delete subnets;
         nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: zone processing completed"));
      }
      delete zones;
   }
   else
   {
      SharedObjectArray<NetObj> *subnets = g_idxSubnetByAddr.getObjects();
      DeleteEmptySubnetsFromList(subnets);
      delete subnets;
   }
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

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing alarm log (retention time %d days)"), retentionTime);
	retentionTime *= 86400;	// Convert days to seconds
	time_t ts = time(NULL) - (time_t)retentionTime;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT alarm_id FROM alarms WHERE alarm_state=3 AND last_change_time<?"));
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
            if (!ThrottleHousekeeper())
               break;
            DeleteAlarmEvents(hdb, alarmId);
            if (!ThrottleHousekeeper())
               break;
         }
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	if (!s_shutdown)
	{
      hStmt = DBPrepare(hdb, _T("DELETE FROM alarms WHERE alarm_state=3 AND last_change_time<?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)ts);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      ThrottleHousekeeper();
	}
}

/**
 * DCI cutoff times
 */
struct CutoffTimes
{
   time_t cutoffTimeIData[static_cast<size_t>(DCObjectStorageClass::OTHER)];
   time_t cutoffTimeTData[static_cast<size_t>(DCObjectStorageClass::OTHER)];
};

/**
 * Drop timescale DB chunks for given object type and storage class
 */
static void DropChunksForStorageClass(DB_HANDLE hdb, time_t cutoffTime, TCHAR objectType, DCObjectStorageClass storageClass)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT drop_chunks(to_timestamp(") INT64_FMT _T("), '%cdata_sc_%s')"),
            static_cast<int64_t>(cutoffTime), objectType, DCObject::getStorageClassName(storageClass));
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Executing query \"%s\""), query);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
      DBFreeResult(hResult);
}

/**
 * Callback for calculating cutoff time for TimescaleDB drop_chunks()
 */
static void CalculateDciCutoffTimes(NetObj *object, CutoffTimes *data)
{
   static_cast<DataCollectionTarget*>(object)->calculateDciCutoffTimes(data->cutoffTimeIData, data->cutoffTimeTData);
}

/**
 * Clean collected data in Timescale database
 */
static void CleanTimescaleData(DB_HANDLE hdb)
{
   CutoffTimes cutoffTimes;
   memset(&cutoffTimes, 0, sizeof(cutoffTimes));

   g_idxAccessPointById.forEach(CalculateDciCutoffTimes, &cutoffTimes);
   g_idxChassisById.forEach(CalculateDciCutoffTimes, &cutoffTimes);
   g_idxClusterById.forEach(CalculateDciCutoffTimes, &cutoffTimes);
   g_idxMobileDeviceById.forEach(CalculateDciCutoffTimes, &cutoffTimes);
   g_idxNodeById.forEach(CalculateDciCutoffTimes, &cutoffTimes);
   g_idxSensorById.forEach(CalculateDciCutoffTimes, &cutoffTimes);

   // Always run on default storage class
   time_t defaultCutoffTime = time(NULL) - DCObject::m_defaultRetentionTime * 86400;
   DropChunksForStorageClass(hdb, defaultCutoffTime, 'i', DCObjectStorageClass::DEFAULT);
   DropChunksForStorageClass(hdb, defaultCutoffTime, 't', DCObjectStorageClass::DEFAULT);

   for(int c = static_cast<int>(DCObjectStorageClass::BELOW_7); c <= static_cast<int>(DCObjectStorageClass::OTHER); c++)
   {
      if (cutoffTimes.cutoffTimeIData[c - 1] != 0)
         DropChunksForStorageClass(hdb, cutoffTimes.cutoffTimeIData[c - 1], 'i', static_cast<DCObjectStorageClass>(c));
      if (cutoffTimes.cutoffTimeTData[c - 1] != 0)
         DropChunksForStorageClass(hdb, cutoffTimes.cutoffTimeTData[c - 1], 't', static_cast<DCObjectStorageClass>(c));
   }
}

/**
 * Callback for cleaning expired DCI data on node
 */
static void CleanDciData(NetObj *object, void *data)
{
   if (s_shutdown)
      return;
	static_cast<DataCollectionTarget*>(object)->cleanDCIData(static_cast<DB_HANDLE>(data));
	ThrottleHousekeeper();
}

/**
 * Callback for validating template DCIs
 */
static void QueueTemplateUpdate(NetObj *object, void *data)
{
   if (object->getObjectClass() == OBJECT_TEMPLATE)
      static_cast<Template*>(object)->queueUpdate();
}

/**
 * Callback for queuing prediction engines training
 */
static void QueuePredictionEngineTraining(NetObj *object, void *arg)
{
   if (s_shutdown || !object->isDataCollectionTarget())
      return;
   static_cast<DataCollectionTarget*>(object)->queuePredictionEngineTraining();
}

/**
 * Housekeeper thread
 */
static THREAD_RESULT THREAD_CALL HouseKeeper(void *pArg)
{
   ThreadSetName("Housekeeper");

   // Read housekeeping configuration
   int hour;
   int minute;

   TCHAR buffer[64];
   ConfigReadStr(_T("Housekeeper.StartTime"), buffer, 64, _T("02:00"));
   TCHAR *p = _tcschr(buffer, _T(':'));
   if (p != NULL)
   {
      *p = 0;
      p++;
      minute = _tcstol(p, NULL, 10);
      if ((minute < 0) || (minute > 59))
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Invalid minute value %s"), p);
         minute = 0;
      }
   }
   else
   {
      minute = 0;
   }
   hour = _tcstol(buffer, NULL, 10);
   if ((hour < 0) || (hour > 23))
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Invalid hour value %s"), buffer);
      hour = 0;
   }
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Wakeup time is %02d:%02d"), hour, minute);

   int sleepTime = GetSleepTime(hour, minute, 0);
   while(!s_shutdown)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Sleeping for %d seconds"), sleepTime);
      ConditionWait(s_wakeupCondition, sleepTime * 1000);
      if (s_shutdown)
         break;

      nxlog_debug_tag(DEBUG_TAG, 2, _T("Wakeup"));
      time_t cycleStartTime = time(nullptr);
      PostSystemEvent(EVENT_HOUSEKEEPER_STARTED, g_dwMgmtNode, nullptr);

      s_throttlingHighWatermark = ConfigReadInt(_T("Housekeeper.Throttle.HighWatermark"), 250000);
      s_throttlingLowWatermark = ConfigReadInt(_T("Housekeeper.Throttle.LowWatermark"), 50000);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Throttling high watermark = %d, low watermark= %d"), s_throttlingHighWatermark, s_throttlingLowWatermark);

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		CleanAlarmHistory(hdb);

		// Remove outdated event log records
		uint32_t dwRetentionTime = ConfigReadULong(_T("EventLogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
	      nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing event log (retention time %d days)"), dwRetentionTime);
			dwRetentionTime *= 86400;	// Convert days to seconds
         TCHAR query[256];
			_sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM event_log WHERE event_timestamp<%ld"), (long)(cycleStartTime - dwRetentionTime));
			DBQuery(hdb, query);
			if (!ThrottleHousekeeper())
			   break;
		}

		// Remove outdated syslog records
		dwRetentionTime = ConfigReadULong(_T("SyslogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing syslog (retention time %d days)"), dwRetentionTime);
			dwRetentionTime *= 86400;	// Convert days to seconds
         TCHAR query[256];
			_sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM syslog WHERE msg_timestamp<%ld"), (long)(cycleStartTime - dwRetentionTime));
			DBQuery(hdb, query);
         if (!ThrottleHousekeeper())
            break;
		}

		// Remove outdated audit log records
		dwRetentionTime = ConfigReadULong(_T("AuditLogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing audit log (retention time %d days)"), dwRetentionTime);
			dwRetentionTime *= 86400;	// Convert days to seconds
         TCHAR query[256];
			_sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM audit_log WHERE timestamp<%ld"), (long)(cycleStartTime - dwRetentionTime));
			DBQuery(hdb, query);
         if (!ThrottleHousekeeper())
            break;
		}

		// Remove outdated SNMP trap log records
		dwRetentionTime = ConfigReadULong(_T("SNMPTrapLogRetentionTime"), 90);
		if (dwRetentionTime > 0)
		{
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing SNMP trap log (retention time %d days)"), dwRetentionTime);
			dwRetentionTime *= 86400;	// Convert days to seconds
         TCHAR query[256];
			_sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM snmp_trap_log WHERE trap_timestamp<%ld"), (long)(cycleStartTime - dwRetentionTime));
			DBQuery(hdb, query);
         if (!ThrottleHousekeeper())
            break;
		}

      // Delete old user agent messages
      dwRetentionTime = ConfigReadULong(_T("UserAgent.RetentionTime"), 30);
      if (dwRetentionTime > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing user agent messages log (retention time %d days)"), dwRetentionTime);
         dwRetentionTime *= 86400;  // Convert days to seconds
         DeleteExpiredUserAgentNotifications(hdb, dwRetentionTime);
         if (!ThrottleHousekeeper())
            break;
      }

		// Delete empty subnets if needed
		if (g_flags & AF_DELETE_EMPTY_SUBNETS)
		{
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Checking for empty subnets"));
			DeleteEmptySubnets();
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Empty subnet check completed"));
		}

		// Remove expired DCI data
      if (!ConfigReadBoolean(_T("Housekeeper.DisableCollectedDataCleanup"), false))
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing collected DCI data"));
         if ((g_dbSyntax == DB_SYNTAX_TSDB) && (g_flags & AF_SINGLE_TABLE_PERF_DATA))
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Using drop_chunks()"));
            CleanTimescaleData(hdb);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Using DELETE statements"));
            g_idxAccessPointById.forEach(CleanDciData, hdb);
            g_idxChassisById.forEach(CleanDciData, hdb);
            g_idxClusterById.forEach(CleanDciData, hdb);
            g_idxMobileDeviceById.forEach(CleanDciData, hdb);
            g_idxNodeById.forEach(CleanDciData, hdb);
            g_idxSensorById.forEach(CleanDciData, hdb);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Collected DCI data cleanup disabled"));
      }

	   // Save object runtime data
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Saving object runtime data"));
	   SharedObjectArray<NetObj> *objects = g_idxObjectById.getObjects();
	   for(int i = 0; i < objects->size(); i++)
	   {
	      objects->get(i)->saveRuntimeData(hdb);
	   }
	   delete objects;

		DBConnectionPoolReleaseConnection(hdb);

		// Validate template DCIs
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Queue template updates"));
		g_idxObjectById.forEach(QueueTemplateUpdate, NULL);

      // Call hooks in loaded modules
		ENUMERATE_MODULES(pfHousekeeperHook)
		{
		   nxlog_debug_tag(DEBUG_TAG, 3, _T("Housekeeper: calling hook in module %s"), CURRENT_MODULE.szName);
		   CURRENT_MODULE.pfHousekeeperHook();
      }

		SaveCurrentFreeId();

		// Run training on prediction engines
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Queue prediction engines training"));
		g_idxObjectById.forEach(QueuePredictionEngineTraining, NULL);

      PostSystemEvent(EVENT_HOUSEKEEPER_COMPLETED, g_dwMgmtNode, "t", time(nullptr) - cycleStartTime);

      ThreadSleep(1);   // to prevent multiple executions if processing took less then 1 second
      sleepTime = GetSleepTime(hour, minute, 0);
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Housekeeper thread terminated"));
   return THREAD_OK;
}

/**
 * Housekeeper thread handle
 */
static THREAD s_thread = INVALID_THREAD_HANDLE;

/**
 * Start housekeeper
 */
void StartHouseKeeper()
{
   s_wakeupCondition = ConditionCreate(FALSE);
   s_thread = ThreadCreateEx(HouseKeeper, 0, NULL);
}

/**
 * Stop housekeeper
 */
void StopHouseKeeper()
{
   s_shutdown = true;
   ConditionSet(s_wakeupCondition);
   ThreadJoin(s_thread);
   ConditionDestroy(s_wakeupCondition);
}

/**
 * Run housekeeper
 */
void RunHouseKeeper()
{
   ConditionSet(s_wakeupCondition);
}
