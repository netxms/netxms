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
** File: hk.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("housekeeper")

/**
 * Remove expired jobs
 */
void RemoveExpiredPackageDeploymentJobs(DB_HANDLE hdb);

/**
 * Housekeeper wakeup condition
 */
static Condition s_wakeupCondition(false);

/**
 * Housekeeper run flag
 */
static bool s_running = false;

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
   size_t qsize = g_dbWriterQueue.size() + static_cast<size_t>(GetIDataWriterQueueSize());
   if (qsize < s_throttlingHighWatermark)
      return true;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Housekeeper paused (queue size %d, high watermark %d, low watermark %d)"),
      static_cast<int>(qsize), static_cast<int>(s_throttlingHighWatermark), static_cast<int>(s_throttlingLowWatermark));
   while((qsize >= s_throttlingLowWatermark) && !s_shutdown)
   {
      s_wakeupCondition.wait(30000);
      qsize = g_dbWriterQueue.size() + static_cast<size_t>(GetIDataWriterQueueSize());
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Housekeeper resumed (queue size %d)"), static_cast<int>(qsize));
   return !s_shutdown;
}

/**
 * Execute custom housekeeper scripts
 */
static void ExecuteHousekeeperScripts()
{
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirData, path);
   _tcscat(path, DDIR_HOUSEKEEPER);

   int count = 0;
   nxlog_debug_tag(DEBUG_TAG,  1, _T("Running housekeeper scripts from %s"), path);
   _TDIR *dir = _topendir(path);
   if (dir != nullptr)
   {
      _tcscat(path, FS_PATH_SEPARATOR);
      int insPos = (int)_tcslen(path);

      struct _tdirent *f;
      while((f = _treaddir(dir)) != nullptr)
      {
         if (MatchString(_T("*.nxsl"), f->d_name, false))
         {
            count++;
            _tcscpy(&path[insPos], f->d_name);

            TCHAR *source = NXSLLoadFile(path);
            if (source != nullptr)
            {
               NXSL_CompilationDiagnostic diag;
               NXSL_VM *vm = NXSLCompileAndCreateVM(source, new NXSL_ServerEnv(true), &diag);
               MemFree(source);
               if (vm != nullptr)
               {
                  if (vm->run())
                  {
                     nxlog_debug_tag(DEBUG_TAG,  3, _T("Housekeeper NXSL script %s completed successfully"), f->d_name);
                  }
                  else
                  {
                     nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Runtime error in housekeeper NXSL script %s (%s)"), f->d_name, vm->getErrorText());
                  }
                  delete vm;
               }
               else
               {
                  nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot compile housekeeper NXSL script %s (%s)"), f->d_name, diag.errorText.cstr());
               }
            }
            else
            {
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot load housekeeper NXSL script %s"), f->d_name);
            }
         }
         else if (MatchString(_T("*.sql"), f->d_name, false))
         {
            count++;
            _tcscpy(&path[insPos], f->d_name);
            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
            if (ExecuteSQLCommandFile(path, hdb))
            {
               nxlog_debug_tag(DEBUG_TAG,  3, _T("Housekeeper SQL script %s completed"), f->d_name);
            }
            else
            {
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot load housekeeper SQL script %s"), f->d_name);
            }
            DBConnectionPoolReleaseConnection(hdb);
         }

         if (!ThrottleHousekeeper())
            break;
      }
      _tclosedir(dir);
   }
   nxlog_debug_tag(DEBUG_TAG,  3, _T("%d housekeeper scripts processed"), count);
}

/**
 * Delete empty subnets from given list
 */
static void DeleteEmptySubnetsFromList(const SharedObjectArray<NetObj>& subnets)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: %d subnets to check"), subnets.size());
   for(int i = 0; i < subnets.size(); i++)
   {
      NetObj *object = subnets.get(i);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: checking subnet %s [%u] (children: %d, parents: %d)"),
                object->getName(), object->getId(), object->getChildCount(), object->getParentCount());
      if (object->isEmpty() && !static_cast<Subnet*>(object)->isManuallyCreated())
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("DeleteEmptySubnets: delete subnet %s [%u] (children: %d, parents: %d)"),
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
      unique_ptr<SharedObjectArray<NetObj>> zones = g_idxZoneByUIN.getObjects();
      nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: %d zones to check"), zones->size());
      for(int i = 0; i < zones->size(); i++)
      {
         Zone *zone = static_cast<Zone*>(zones->get(i));
         nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: processing zone %s (UIN=%u)"), zone->getName(), zone->getUIN());
         unique_ptr<SharedObjectArray<NetObj>> subnets = zone->getSubnets();
         DeleteEmptySubnetsFromList(*subnets);
         nxlog_debug_tag(DEBUG_TAG, 7, _T("DeleteEmptySubnets: zone processing completed"));
      }
   }
   else
   {
      unique_ptr<SharedObjectArray<NetObj>> subnets = g_idxSubnetByAddr.getObjects();
      DeleteEmptySubnetsFromList(*subnets);
   }
}

/**
 * Remove outdated alarm records
 */
static void CleanAlarmHistory(DB_HANDLE hdb)
{
   time_t retentionTime = ConfigReadULong(_T("Alarms.HistoryRetentionTime"), 180);
	if (retentionTime == 0)
		return;

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing alarm log (retention time %d days)"), static_cast<int>(retentionTime));
	retentionTime *= 86400;	// Convert days to seconds
	time_t ts = time(nullptr) - retentionTime;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT alarm_id FROM alarms WHERE alarm_state=3 AND last_change_time<?"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (uint32_t)ts);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
         {
            uint32_t alarmId = DBGetFieldULong(hResult, i, 0);
            ExecuteQueryOnObject(hdb, alarmId, _T("DELETE FROM alarm_notes WHERE alarm_id=?"));
            if (!ThrottleHousekeeper())
               break;
            ExecuteQueryOnObject(hdb, alarmId, _T("DELETE FROM alarm_events WHERE alarm_id=?"));
            if (!ThrottleHousekeeper())
               break;
            ExecuteQueryOnObject(hdb, alarmId, _T("DELETE FROM alarm_state_changes WHERE alarm_id=?"));
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
      if (hStmt != nullptr)
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
   if (g_flags & AF_TSDB_DROP_CHUNKS_V2)
   {
      _sntprintf(query, 256, _T("SELECT drop_chunks('%cdata_sc_%s', to_timestamp(") INT64_FMT _T("))"),
               objectType, DCObject::getStorageClassName(storageClass), static_cast<int64_t>(cutoffTime));
   }
   else
   {
      _sntprintf(query, 256, _T("SELECT drop_chunks(to_timestamp(") INT64_FMT _T("), '%cdata_sc_%s')"),
               static_cast<int64_t>(cutoffTime), objectType, DCObject::getStorageClassName(storageClass));
   }
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Executing query \"%s\""), query);
   DBQuery(hdb, query);
}

/**
 * Callback for calculating cutoff time for TimescaleDB drop_chunks()
 */
static EnumerationCallbackResult CalculateDciCutoffTimes(NetObj *object, CutoffTimes *data)
{
   static_cast<DataCollectionTarget*>(object)->calculateDciCutoffTimes(data->cutoffTimeIData, data->cutoffTimeTData);
   return _CONTINUE;
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
   g_idxCollectorById.forEach(CalculateDciCutoffTimes, &cutoffTimes);
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
 * Construct query with drop_chunks() function
 */
static void BuildDropChunksQuery(const TCHAR *table, time_t cutoffTime, TCHAR *query, size_t size)
{
   if (g_flags & AF_TSDB_DROP_CHUNKS_V2)
      _sntprintf(query, size, _T("SELECT drop_chunks('%s', to_timestamp(") INT64_FMT _T("))"), table, static_cast<int64_t>(cutoffTime));
   else
      _sntprintf(query, size, _T("SELECT drop_chunks(to_timestamp(") INT64_FMT _T("), '%s')"), static_cast<int64_t>(cutoffTime), table);
}

/**
 * Delete expired log records and throttle housekeeper if needed. Returns false if shutdown time has arrived and housekeeper process should be aborted.
 */
static bool DeleteExpiredLogRecords(const TCHAR *logName, const TCHAR *logTable, const TCHAR *timestampColumn, const TCHAR *retentionParameter, DB_HANDLE hdb, time_t cycleStartTime)
{
   uint32_t retentionTime = ConfigReadULong(retentionParameter, 90);
   if (retentionTime <= 0)
      return true;

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing %s (retention time %u days)"), logName, retentionTime);
   retentionTime *= 86400; // Convert days to seconds
   TCHAR query[256];
   if (g_dbSyntax == DB_SYNTAX_TSDB)
      BuildDropChunksQuery(logTable, cycleStartTime - retentionTime, query, sizeof(query) / sizeof(TCHAR));
   else
      _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM %s WHERE %s<") INT64_FMT, logTable, timestampColumn, static_cast<int64_t>(cycleStartTime - retentionTime));
   DBQuery(hdb, query);

   return ThrottleHousekeeper();
}

/**
 * Housekeeper thread
 */
static void HouseKeeper()
{
   ThreadSetName("Housekeeper");

   // Read housekeeping configuration
   int hour;
   int minute;

   TCHAR buffer[64];
   ConfigReadStr(_T("Housekeeper.StartTime"), buffer, 64, _T("02:00"));
   TCHAR *p = _tcschr(buffer, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      minute = _tcstol(p, nullptr, 10);
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
   hour = _tcstol(buffer, nullptr, 10);
   if ((hour < 0) || (hour > 23))
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Invalid hour value %s"), buffer);
      hour = 0;
   }
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Wakeup time is %02d:%02d"), hour, minute);

   // Call policy validation for templates
   g_idxObjectById.forEach(
      [] (NetObj *object) -> EnumerationCallbackResult
      {
         if (object->getObjectClass() == OBJECT_TEMPLATE)
            static_cast<Template*>(object)->initiatePolicyValidation();
         return _CONTINUE;
      });

   int sleepTime = GetSleepTime(hour, minute, 0);
   while(!s_shutdown)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Sleeping for %d seconds"), sleepTime);
      s_wakeupCondition.wait(sleepTime * 1000);
      if (s_shutdown)
         break;

      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Housekeeper run started"));
      s_running = true;
      time_t cycleStartTime = time(nullptr);
      PostSystemEvent(EVENT_HOUSEKEEPER_STARTED, g_dwMgmtNode);

      s_throttlingHighWatermark = ConfigReadInt(_T("Housekeeper.Throttle.HighWatermark"), 250000);
      s_throttlingLowWatermark = ConfigReadInt(_T("Housekeeper.Throttle.LowWatermark"), 50000);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Throttling high watermark = %d, low watermark= %d"), static_cast<int>(s_throttlingHighWatermark), static_cast<int>(s_throttlingLowWatermark));

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		CleanAlarmHistory(hdb);

		// Remove expired log records
		if (!DeleteExpiredLogRecords(_T("event log"), _T("event_log"), _T("event_timestamp"), _T("Events.LogRetentionTime"), hdb, cycleStartTime))
		   break;
      if (!DeleteExpiredLogRecords(_T("syslog"), _T("syslog"), _T("msg_timestamp"), _T("Syslog.RetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("windows event log"), _T("win_event_log"), _T("event_timestamp"), _T("WindowsEvents.LogRetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("SNMP trap log"), _T("snmp_trap_log"), _T("trap_timestamp"), _T("SNMP.Traps.LogRetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("server action execution log"), _T("server_action_execution_log"), _T("action_timestamp"), _T("ActionExecutionLog.RetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("notification log"), _T("notification_log"), _T("notification_timestamp"), _T("NotificationLog.RetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("maintenance journal"), _T("maintenance_journal"), _T("creation_time"), _T("MaintenanceJournal.RetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("asset change log"), _T("asset_change_log"), _T("operation_timestamp"), _T("AssetChangeLog.RetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("certificate action log"), _T("certificate_action_log"), _T("operation_timestamp"), _T("CertificateActionLog.RetentionTime"), hdb, cycleStartTime))
         break;
      if (!DeleteExpiredLogRecords(_T("package deployment log"), _T("package_deployment_log"), _T("execution_time"), _T("PackageDeployment.LogRetentionTime"), hdb, cycleStartTime))
         break;

		// Remove outdated audit log records
		int32_t retentionTime = ConfigReadULong(_T("AuditLog.RetentionTime"), 90);
		if (retentionTime > 0)
		{
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing audit log (retention time %d days)"), retentionTime);
			retentionTime *= 86400;	// Convert days to seconds
         TCHAR query[256];
			_sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM audit_log WHERE timestamp<") INT64_FMT, static_cast<int64_t>(cycleStartTime - retentionTime));
			DBQuery(hdb, query);
         if (!ThrottleHousekeeper())
            break;
		}

      // Remove expired business service history records
      retentionTime = ConfigReadULong(_T("BusinessServices.History.RetentionTime"), 90);
      if (retentionTime > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing business service history (retention time %d days)"), retentionTime);
         retentionTime *= 86400;	// Convert days to seconds
         TCHAR query[256];
         _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM business_service_tickets WHERE close_timestamp>0 AND close_timestamp<") INT64_FMT, static_cast<int64_t>(cycleStartTime - retentionTime));
         DBQuery(hdb, query);
         if (!ThrottleHousekeeper())
            break;
         _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM business_service_downtime WHERE to_timestamp>0 AND to_timestamp<") INT64_FMT, static_cast<int64_t>(cycleStartTime - retentionTime));
         DBQuery(hdb, query);
         if (!ThrottleHousekeeper())
            break;
      }

      // Remove expired downtime log records
      retentionTime = ConfigReadULong(_T("DowntimeLog.RetentionTime"), 90);
      if (retentionTime > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing downtime log (retention time %d days)"), retentionTime);
         retentionTime *= 86400; // Convert days to seconds
         TCHAR query[256];
         _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM downtime_log WHERE end_time>0 AND start_time<") INT64_FMT, static_cast<int64_t>(cycleStartTime - retentionTime));
         DBQuery(hdb, query);
         if (!ThrottleHousekeeper())
            break;
      }

      // Delete old user agent messages
      retentionTime = ConfigReadULong(_T("UserAgent.RetentionTime"), 30);
      if (retentionTime > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing user agent messages log (retention time %d days)"), retentionTime);
         retentionTime *= 86400;  // Convert days to seconds
         DeleteExpiredUserAgentNotifications(hdb, retentionTime);
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
            SharedObjectArray<NetObj> objects(1024, 1024);
            g_idxAccessPointById.getObjects(&objects);
            g_idxChassisById.getObjects(&objects);
            g_idxClusterById.getObjects(&objects);
            g_idxCollectorById.getObjects(&objects);
            g_idxMobileDeviceById.getObjects(&objects);
            g_idxNodeById.getObjects(&objects);
            g_idxSensorById.getObjects(&objects);

            for(int i = 0; (i < objects.size()) && !s_shutdown; i++)
            {
               static_cast<DataCollectionTarget*>(objects.get(i))->cleanDCIData(hdb);
               ThrottleHousekeeper();
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Collected DCI data cleanup disabled"));
      }

      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();

      // Clean geolocation data
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Clearing geolocation data"));
      retentionTime = ConfigReadULong(_T("Geolocation.History.RetentionTime"), 90);
      retentionTime *= 86400;   // Convert days to seconds
      int64_t latestTimestamp = static_cast<int64_t>(cycleStartTime - retentionTime);
      for(int i = 0; i < objects->size(); i++)
      {
         objects->get(i)->cleanGeoLocationHistoryTable(hdb, latestTimestamp);
         ThrottleHousekeeper();
      }

      // Call policy validation for templates
      g_idxObjectById.forEach(
         [] (NetObj *object) -> EnumerationCallbackResult
         {
            if (object->getObjectClass() == OBJECT_TEMPLATE)
               static_cast<Template*>(object)->initiatePolicyValidation();
            return _CONTINUE;
         });

	   // Save object runtime data
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Saving object runtime data"));
	   for(int i = 0; i < objects->size(); i++)
	   {
	      objects->get(i)->saveRuntimeData(hdb);
	   }

	   // Delete old completed package deployment jobs
	   RemoveExpiredPackageDeploymentJobs(hdb);

		DBConnectionPoolReleaseConnection(hdb);

		// Validate template DCIs
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Queue template updates"));
		g_idxObjectById.forEach(
		   [] (NetObj *object) -> EnumerationCallbackResult
		   {
            if (object->getObjectClass() == OBJECT_TEMPLATE)
               static_cast<Template*>(object)->queueUpdate();
            return _CONTINUE;
		   });

	   // Validate scripts in script library
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Validate server NXSL scripts"));
	   ValidateScripts();

      // Call hooks in loaded modules
		ENUMERATE_MODULES(pfHousekeeperHook)
		{
		   nxlog_debug_tag(DEBUG_TAG, 3, _T("Housekeeper: calling hook in module %s"), CURRENT_MODULE.name);
		   CURRENT_MODULE.pfHousekeeperHook();
      }

		ExecuteHousekeeperScripts();

		SaveCurrentFreeId();

		// Run training on prediction engines
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Queue prediction engines training"));
		g_idxObjectById.forEach(
		   [] (NetObj *object) -> EnumerationCallbackResult
		   {
            if (!s_shutdown && object->isDataCollectionTarget())
               static_cast<DataCollectionTarget*>(object)->queuePredictionEngineTraining();
            return _CONTINUE;
		   });

      g_pEventPolicy->validateConfig();

      uint32_t elapsedTime = static_cast<uint32_t>(time(nullptr) - cycleStartTime);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Housekeeper run completed (elapsed time %u milliseconds)"), elapsedTime);
      EventBuilder(EVENT_HOUSEKEEPER_COMPLETED, g_dwMgmtNode)
         .param(_T("elapsedTime"), elapsedTime)
         .post();

      ThreadSleep(1);   // to prevent multiple executions if processing took less then 1 second
      sleepTime = GetSleepTime(hour, minute, 0);
      s_running = false;
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Housekeeper thread terminated"));
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
   s_thread = ThreadCreateEx(HouseKeeper);
}

/**
 * Stop housekeeper
 */
void StopHouseKeeper()
{
   s_shutdown = true;
   s_wakeupCondition.set();
   ThreadJoin(s_thread);
}

/**
 * Run housekeeper
 */
void RunHouseKeeper(ServerConsole *console)
{
   if (s_running)
   {
      console->print(_T("Housekeeper process already running\n"));
      return;
   }
   console->print(_T("Starting housekeeper\n"));
   s_wakeupCondition.set();
}
