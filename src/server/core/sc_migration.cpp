/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: sc_migration.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("dc.migration")

/**
 * Migration task status codes
 */
#define SCM_STATUS_PENDING    'P'
#define SCM_STATUS_RUNNING    'R'
#define SCM_STATUS_COMPLETED  'C'
#define SCM_STATUS_FAILED     'F'

/**
 * Storage class migration task structure
 */
struct StorageClassMigrationTask
{
   uint32_t id;
   uint32_t dciId;
   char dciType;                    // 'I' = item (idata), 'T' = table (tdata)
   DCObjectStorageClass oldClass;
   DCObjectStorageClass newClass;
   char status;                     // 'P', 'R', 'C', 'F'
   time_t createTime;
   time_t startTime;
   time_t endTime;
   int64_t lastProcessedTimestamp;  // For resumption (in milliseconds)
   int recordsCopied;
   wchar_t errorMessage[256];
};

/**
 * List of pending migration tasks
 */
static ObjectArray<StorageClassMigrationTask> s_migrationTasks(16, 16, Ownership::True);

/**
 * Access mutex for migration tasks list
 */
static Mutex s_migrationTasksMutex(MutexType::FAST);

/**
 * Shutdown flag
 */
static bool s_shutdown = false;

/**
 * Convert storage class to character code for database
 */
static char StorageClassToChar(DCObjectStorageClass storageClass)
{
   switch(storageClass)
   {
      case DCObjectStorageClass::DEFAULT:
         return 'D';
      case DCObjectStorageClass::BELOW_7:
         return '7';
      case DCObjectStorageClass::BELOW_30:
         return '3';
      case DCObjectStorageClass::BELOW_90:
         return '9';
      case DCObjectStorageClass::BELOW_180:
         return '1';
      case DCObjectStorageClass::OTHER:
         return 'O';
      default:
         return 'D';
   }
}

/**
 * Convert character code from database to storage class
 */
static DCObjectStorageClass CharToStorageClass(char c)
{
   switch(c)
   {
      case 'D':
         return DCObjectStorageClass::DEFAULT;
      case '7':
         return DCObjectStorageClass::BELOW_7;
      case '3':
         return DCObjectStorageClass::BELOW_30;
      case '9':
         return DCObjectStorageClass::BELOW_90;
      case '1':
         return DCObjectStorageClass::BELOW_180;
      case 'O':
         return DCObjectStorageClass::OTHER;
      default:
         return DCObjectStorageClass::DEFAULT;
   }
}

/**
 * Update migration task progress in database
 */
static void UpdateMigrationProgress(DB_HANDLE hdb, StorageClassMigrationTask *task)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, L"UPDATE dc_storage_class_migrations SET status=?,start_time=?,end_time=?,last_processed_timestamp=?,records_copied=?,error_message=? WHERE id=?");
   if (hStmt != nullptr)
   {
      wchar_t statusStr[2];
      statusStr[0] = task->status;
      statusStr[1] = 0;
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, statusStr, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(task->startTime));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(task->endTime));
      DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, task->lastProcessedTimestamp);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, task->recordsCopied);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, task->errorMessage, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, task->id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
}

/**
 * Migrate data for single DCI from one storage class table to another
 */
static bool MigrateStorageClassData(StorageClassMigrationTask *task, DB_HANDLE hdb)
{
   int maxRecords = ConfigReadInt(L"DBWriter.MaxRecordsPerTransaction", 1000);

   const wchar_t *oldClassName = DCObject::getStorageClassName(task->oldClass);
   const wchar_t *newClassName = DCObject::getStorageClassName(task->newClass);

   wchar_t sourceTable[64], destTable[64];
   const wchar_t *timestampCol;

   if (task->dciType == 'I')
   {
      _sntprintf(sourceTable, 64, _T("idata_sc_%s"), oldClassName);
      _sntprintf(destTable, 64, _T("idata_sc_%s"), newClassName);
      timestampCol = L"idata_timestamp";
   }
   else
   {
      _sntprintf(sourceTable, 64, _T("tdata_sc_%s"), oldClassName);
      _sntprintf(destTable, 64, _T("tdata_sc_%s"), newClassName);
      timestampCol = L"tdata_timestamp";
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"Starting migration for DCI %u from %s to %s (last processed: " INT64_FMT L")",
      task->dciId, sourceTable, destTable, task->lastProcessedTimestamp);

   bool success = true;
   int batchCount = 0;

   while (success && !s_shutdown)
   {
      // Select a batch of records from source table
      wchar_t selectQuery[1024];
      if (task->dciType == 'I')
      {
         _sntprintf(selectQuery, 1024,
            _T("SELECT item_id, timestamptz_to_ms(%s), idata_value, raw_value FROM %s ")
            _T("WHERE item_id=%u AND %s > ms_to_timestamptz(") INT64_FMT _T(") ")
            _T("ORDER BY %s LIMIT %d"),
            timestampCol, sourceTable, task->dciId, timestampCol,
            task->lastProcessedTimestamp, timestampCol, maxRecords);
      }
      else
      {
         _sntprintf(selectQuery, 1024,
            _T("SELECT item_id, timestamptz_to_ms(%s), tdata_value FROM %s ")
            _T("WHERE item_id=%u AND %s > ms_to_timestamptz(") INT64_FMT _T(") ")
            _T("ORDER BY %s LIMIT %d"),
            timestampCol, sourceTable, task->dciId, timestampCol,
            task->lastProcessedTimestamp, timestampCol, maxRecords);
      }

      DB_RESULT hResult = DBSelect(hdb, selectQuery);
      if (hResult == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Migration select failed for DCI %u"), task->dciId);
         wcslcpy(task->errorMessage, L"SELECT query failed", 256);
         success = false;
         break;
      }

      int rowCount = DBGetNumRows(hResult);
      if (rowCount == 0)
      {
         // No more rows to migrate
         DBFreeResult(hResult);
         nxlog_debug_tag(DEBUG_TAG, 4, L"Migration completed for DCI %u, total records: %d", task->dciId, task->recordsCopied);
         break;
      }

      // Prepare INSERT statement
      DB_STATEMENT hStmt;
      wchar_t insertQuery[512];
      if (task->dciType == 'I')
      {
         _sntprintf(insertQuery, 512,
            _T("INSERT INTO %s (item_id, idata_timestamp, idata_value, raw_value) VALUES (?, ms_to_timestamptz(?), ?, ?) ON CONFLICT (item_id, idata_timestamp) DO NOTHING"),
            destTable);
      }
      else
      {
         _sntprintf(insertQuery, 512,
            _T("INSERT INTO %s (item_id, tdata_timestamp, tdata_value) VALUES (?, ms_to_timestamptz(?), ?) ON CONFLICT (item_id, tdata_timestamp) DO NOTHING"),
            destTable);
      }
      hStmt = DBPrepare(hdb, insertQuery, rowCount > 1);
      if (hStmt == nullptr)
      {
         DBFreeResult(hResult);
         nxlog_debug_tag(DEBUG_TAG, 3, L"Migration INSERT prepare failed for DCI %u", task->dciId);
         wcslcpy(task->errorMessage, L"INSERT prepare failed", 256);
         success = false;
         break;
      }

      DBBegin(hdb);

      int64_t maxTimestamp = task->lastProcessedTimestamp;
      int rowsInserted = 0;

      wchar_t value[MAX_RESULT_LENGTH], rawValue[MAX_RESULT_LENGTH];
      for (int i = 0; i < rowCount; i++)
      {
         uint32_t itemId = DBGetFieldULong(hResult, i, 0);
         int64_t timestamp = DBGetFieldInt64(hResult, i, 1);
         DBGetField(hResult, i, 2, value, MAX_RESULT_LENGTH);

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, itemId);
         DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, timestamp);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);

         if (task->dciType == 'I')
         {
            DBGetField(hResult, i, 3, rawValue, MAX_RESULT_LENGTH);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, rawValue, DB_BIND_STATIC);
         }

         if (!DBExecute(hStmt))
         {
            success = false;
            wcslcpy(task->errorMessage, L"INSERT execute failed", 256);
            break;
         }

         rowsInserted++;

         if (timestamp > maxTimestamp)
            maxTimestamp = timestamp;
      }

      DBFreeStatement(hStmt);
      DBFreeResult(hResult);

      if (success)
      {
         task->lastProcessedTimestamp = maxTimestamp;
         task->recordsCopied += rowsInserted;

         // Save progress to database within the same transaction
         UpdateMigrationProgress(hdb, task);

         DBCommit(hdb);
         batchCount++;

         nxlog_debug_tag(DEBUG_TAG, 5, L"Migration batch %d completed for DCI %u, %d records in batch, %d total",
            batchCount, task->dciId, rowsInserted, task->recordsCopied);
      }
      else
      {
         DBRollback(hdb);
      }
   }

   if (s_shutdown && success)
   {
      // Not an error, just interrupted
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Migration for DCI %u interrupted by shutdown"), task->dciId);
   }

   return success;
}

/**
 * Load pending migration tasks from database
 */
void InitStorageClassMigration()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"Initializing storage class migration subsystem");

   s_shutdown = false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load pending and running tasks (running tasks may have been interrupted by server restart)
   DB_RESULT hResult = DBSelect(hdb, L"SELECT id,dci_id,dci_type,old_storage_class,new_storage_class,status,create_time,start_time,end_time,last_processed_timestamp,records_copied,error_message FROM dc_storage_class_migrations WHERE status IN ('P','R') ORDER BY id");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         auto task = new StorageClassMigrationTask();
         task->id = DBGetFieldULong(hResult, i, 0);
         task->dciId = DBGetFieldULong(hResult, i, 1);

         wchar_t typeStr[2];
         DBGetField(hResult, i, 2, typeStr, 2);
         task->dciType = static_cast<char>(typeStr[0]);

         wchar_t classStr[2];
         DBGetField(hResult, i, 3, classStr, 2);
         task->oldClass = CharToStorageClass(static_cast<char>(classStr[0]));

         DBGetField(hResult, i, 4, classStr, 2);
         task->newClass = CharToStorageClass(static_cast<char>(classStr[0]));

         wchar_t statusStr[2];
         DBGetField(hResult, i, 5, statusStr, 2);
         task->status = static_cast<char>(statusStr[0]);

         task->createTime = static_cast<time_t>(DBGetFieldULong(hResult, i, 6));
         task->startTime = static_cast<time_t>(DBGetFieldULong(hResult, i, 7));
         task->endTime = static_cast<time_t>(DBGetFieldULong(hResult, i, 8));
         task->lastProcessedTimestamp = DBGetFieldInt64(hResult, i, 9);
         task->recordsCopied = DBGetFieldLong(hResult, i, 10);
         DBGetField(hResult, i, 11, task->errorMessage, 256);

         // Reset running tasks back to pending (they will be restarted)
         if (task->status == SCM_STATUS_RUNNING)
         {
            task->status = SCM_STATUS_PENDING;
         }

         s_migrationTasks.add(task);
         nxlog_debug_tag(DEBUG_TAG, 4, L"Loaded pending migration task %u for DCI %u (from %s to %s)",
            task->id, task->dciId,
            DCObject::getStorageClassName(task->oldClass),
            DCObject::getStorageClassName(task->newClass));
      }
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Loaded %d pending migration tasks"), count);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Shutdown storage class migration subsystem
 */
void ShutdownStorageClassMigration()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"Shutting down storage class migration subsystem");
   s_shutdown = true;

   s_migrationTasksMutex.lock();
   s_migrationTasks.clear();
   s_migrationTasksMutex.unlock();
}

/**
 * Queue a new storage class migration task
 */
uint32_t QueueStorageClassMigration(uint32_t dciId, char dciType, DCObjectStorageClass oldClass, DCObjectStorageClass newClass)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Queuing storage class migration for DCI %u (type=%c) from %s to %s"),
      dciId, dciType, DCObject::getStorageClassName(oldClass), DCObject::getStorageClassName(newClass));

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (hdb == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Failed to acquire database connection for migration queue"));
      return 0;
   }

   // Check if there's already a pending migration for this DCI
   s_migrationTasksMutex.lock();
   for (int i = 0; i < s_migrationTasks.size(); i++)
   {
      StorageClassMigrationTask *existing = s_migrationTasks.get(i);
      if ((existing->dciId == dciId) && (existing->dciType == dciType) &&
          (existing->status == SCM_STATUS_PENDING || existing->status == SCM_STATUS_RUNNING))
      {
         // Update the new storage class for existing pending task
         existing->newClass = newClass;
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Updated existing migration task %u for DCI %u to new target storage class %s"),
            existing->id, dciId, DCObject::getStorageClassName(newClass));
         s_migrationTasksMutex.unlock();
         DBConnectionPoolReleaseConnection(hdb);
         return existing->id;
      }
   }
   s_migrationTasksMutex.unlock();

   uint32_t taskId = CreateUniqueId(IDG_SC_MIGRATION_TASK);

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dc_storage_class_migrations (id,dci_id,dci_type,old_storage_class,new_storage_class,status,create_time,start_time,end_time,last_processed_timestamp,records_copied,error_message) VALUES (?,?,?,?,?,?,?,0,0,0,0,'')"));
   if (hStmt != nullptr)
   {
      wchar_t typeStr[2], oldClassStr[2], newClassStr[2], statusStr[2];
      typeStr[0] = dciType;
      typeStr[1] = 0;
      oldClassStr[0] = StorageClassToChar(oldClass);
      oldClassStr[1] = 0;
      newClassStr[0] = StorageClassToChar(newClass);
      newClassStr[1] = 0;
      statusStr[0] = SCM_STATUS_PENDING;
      statusStr[1] = 0;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, taskId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, dciId);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, typeStr, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, oldClassStr, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, newClassStr, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, statusStr, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));

      if (DBExecute(hStmt))
      {
         auto task = new StorageClassMigrationTask();
         task->id = taskId;
         task->dciId = dciId;
         task->dciType = dciType;
         task->oldClass = oldClass;
         task->newClass = newClass;
         task->status = SCM_STATUS_PENDING;
         task->createTime = time(nullptr);
         task->startTime = 0;
         task->endTime = 0;
         task->lastProcessedTimestamp = 0;
         task->recordsCopied = 0;
         task->errorMessage[0] = 0;

         s_migrationTasksMutex.lock();
         s_migrationTasks.add(task);
         s_migrationTasksMutex.unlock();

         nxlog_debug_tag(DEBUG_TAG, 4, _T("Created migration task %u for DCI %u"), taskId, dciId);
      }
      else
      {
         taskId = 0;
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Failed to insert migration task for DCI %u"), dciId);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      taskId = 0;
   }

   DBConnectionPoolReleaseConnection(hdb);
   return taskId;
}

/**
 * Process pending storage class migrations (called by housekeeper)
 */
void ProcessStorageClassMigrations()
{
   if (s_shutdown)
      return;

   s_migrationTasksMutex.lock();
   int taskCount = s_migrationTasks.size();
   s_migrationTasksMutex.unlock();

   if (taskCount == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("No pending storage class migrations"));
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Processing %d pending storage class migrations"), taskCount);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (hdb == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Failed to acquire database connection for migration processing"));
      return;
   }

   // Process tasks one at a time
   while (!s_shutdown)
   {
      // Get the next pending task
      StorageClassMigrationTask *task = nullptr;
      s_migrationTasksMutex.lock();
      for (int i = 0; i < s_migrationTasks.size(); i++)
      {
         StorageClassMigrationTask *t = s_migrationTasks.get(i);
         if (t->status == SCM_STATUS_PENDING)
         {
            task = t;
            task->status = SCM_STATUS_RUNNING;
            task->startTime = time(nullptr);
            break;
         }
      }
      s_migrationTasksMutex.unlock();

      if (task == nullptr)
         break;  // No more pending tasks

      nxlog_debug_tag(DEBUG_TAG, 3, _T("Processing migration task %u for DCI %u"), task->id, task->dciId);

      // Update status in database
      UpdateMigrationProgress(hdb, task);

      // Perform the migration
      bool success = MigrateStorageClassData(task, hdb);

      // Update final status
      if (success && !s_shutdown)
      {
         task->status = SCM_STATUS_COMPLETED;
         task->endTime = time(nullptr);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Migration task %u completed successfully (%d records copied)"),
            task->id, task->recordsCopied);
      }
      else if (!success)
      {
         task->status = SCM_STATUS_FAILED;
         task->endTime = time(nullptr);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Migration task %u failed: %s"), task->id, task->errorMessage);
      }
      // If shutdown and success, keep as RUNNING - will be restarted on next server start

      UpdateMigrationProgress(hdb, task);

      // Remove completed/failed tasks from in-memory list
      if (task->status == SCM_STATUS_COMPLETED || task->status == SCM_STATUS_FAILED)
      {
         s_migrationTasksMutex.lock();
         for (int i = 0; i < s_migrationTasks.size(); i++)
         {
            if (s_migrationTasks.get(i)->id == task->id)
            {
               s_migrationTasks.remove(i);
               break;
            }
         }
         s_migrationTasksMutex.unlock();
      }

      // Throttle to prevent overloading the database
      if (!s_shutdown)
         ThreadSleep(1);
   }

   DBConnectionPoolReleaseConnection(hdb);

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Storage class migration processing completed"));
}
