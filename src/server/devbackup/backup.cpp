/*
** NetXMS native device configuration backup module
** Copyright (C) 2025-2026 Raden Solutions
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
** File: backup.cpp
**/

#include "devbackup.h"
#include <nxevent.h>

/**
 * Backup record ID counter
 */
static VolatileCounter64 s_nextId = 0;

/**
 * Active job counter
 */
VolatileCounter g_activeBackupJobs = 0;

/**
 * Initialize backup ID counter from database
 */
static void InitializeIdCounter()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (hdb != nullptr)
   {
      DB_RESULT hResult = DBSelect(hdb, L"SELECT max(id) FROM device_configs");
      if (hResult != nullptr)
      {
         s_nextId = DBGetFieldInt64(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   nxlog_debug_tag(DEBUG_TAG, 3, L"Backup ID counter initialized: starting from " INT64_FMT, static_cast<int64_t>(s_nextId));
}

/**
 * Module-level init called from NxDevBackupInitializeInterface
 */
void InitializeBackupSubsystem()
{
   InitializeIdCounter();
}

/**
 * Convert hash bytes to hex string
 */
static void HashToString(const BYTE *hash, WCHAR *str)
{
   BinToStrW(hash, SHA256_DIGEST_SIZE, str);
}

/**
 * Execute backup job for a single node
 */
void ExecuteBackupJob(const shared_ptr<Node>& node)
{
   InterlockedIncrement(&g_activeBackupJobs);
   nxlog_debug_tag(DEBUG_TAG, 5, L"ExecuteBackupJob(%s [%u]): started", node->getName(), node->getId());

   NetworkDeviceDriver *driver = node->getDriver();
   if ((driver == nullptr) || !driver->isConfigBackupSupported())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"ExecuteBackupJob(%s [%u]): driver does not support config backup", node->getName(), node->getId());
      node->setCustomAttribute(L"$devbackup.lastBackupTime", static_cast<int64_t>(time(nullptr)));
      node->setCustomAttribute(L"$devbackup.lastJobStatus", static_cast<int32_t>(DeviceBackupJobStatus::FAILED));
      node->setCustomAttribute(L"$devbackup.lastJobMessage", L"Device driver does not support configuration backup");
      node->updateConfigBackupStatus();
      InterlockedDecrement(&g_activeBackupJobs);
      SignalScheduler();
      return;
   }

   NodeDeviceContext ctx(node);

   // Get running config
   ByteStream runningOutput;
   bool hasRunning = driver->getRunningConfig(&ctx, &runningOutput);
   wchar_t runningHash[SHA256_DIGEST_SIZE * 2 + 1] = L"";
   wchar_t *runningConfig = nullptr;
   if (hasRunning && runningOutput.size() > 0)
   {
      runningOutput.write('\0');
      BYTE hash[SHA256_DIGEST_SIZE];
      CalculateSHA256Hash(runningOutput.buffer(), runningOutput.size() - 1, hash);
      HashToString(hash, runningHash);
      runningConfig = WideStringFromUTF8String(reinterpret_cast<const char*>(runningOutput.buffer()));
      nxlog_debug_tag(DEBUG_TAG, 6, L"ExecuteBackupJob(%s [%u]): running config retrieved (%d bytes, hash=%s)",
         node->getName(), node->getId(), static_cast<int>(runningOutput.size() - 1), runningHash);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"ExecuteBackupJob(%s [%u]): running config not available", node->getName(), node->getId());
   }

   // Get startup config
   ByteStream startupOutput;
   bool hasStartup = driver->getStartupConfig(&ctx, &startupOutput);
   wchar_t startupHash[SHA256_DIGEST_SIZE * 2 + 1] = L"";
   wchar_t *startupConfig = nullptr;
   if (hasStartup && startupOutput.size() > 0)
   {
      startupOutput.write('\0');
      BYTE hash[SHA256_DIGEST_SIZE];
      CalculateSHA256Hash(startupOutput.buffer(), startupOutput.size() - 1, hash);
      HashToString(hash, startupHash);
      startupConfig = WideStringFromUTF8String(reinterpret_cast<const char*>(startupOutput.buffer()));
      nxlog_debug_tag(DEBUG_TAG, 6, L"ExecuteBackupJob(%s [%u]): startup config retrieved (%d bytes, hash=%s)",
         node->getName(), node->getId(), static_cast<int>(startupOutput.size() - 1), startupHash);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"ExecuteBackupJob(%s [%u]): startup config not available", node->getName(), node->getId());
   }

   // If neither config was retrieved, mark as failed
   if ((runningConfig == nullptr) && (startupConfig == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"ExecuteBackupJob(%s [%u]): no configuration data retrieved, job failed", node->getName(), node->getId());
      node->setCustomAttribute(L"$devbackup.lastBackupTime", static_cast<int64_t>(time(nullptr)));
      node->setCustomAttribute(L"$devbackup.lastJobStatus", static_cast<int32_t>(DeviceBackupJobStatus::FAILED));
      node->setCustomAttribute(L"$devbackup.lastJobMessage", L"No configuration data could be retrieved from device");
      node->updateConfigBackupStatus();
      InterlockedDecrement(&g_activeBackupJobs);
      SignalScheduler();
      return;
   }

   // Change detection - query previous backup
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = false;

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT id,running_config_hash,startup_config_hash FROM device_configs WHERE node_id=? ORDER BY backup_time DESC");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, node->getId());
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         bool hasExisting = (DBGetNumRows(hResult) > 0);
         WCHAR oldRunningHash[SHA256_DIGEST_SIZE * 2 + 1] = L"";
         WCHAR oldStartupHash[SHA256_DIGEST_SIZE * 2 + 1] = L"";
         int64_t existingId = 0;

         if (hasExisting)
         {
            existingId = DBGetFieldInt64(hResult, 0, 0);
            DBGetField(hResult, 0, 1, oldRunningHash, SHA256_DIGEST_SIZE * 2 + 1);
            DBGetField(hResult, 0, 2, oldStartupHash, SHA256_DIGEST_SIZE * 2 + 1);
         }
         DBFreeResult(hResult);
         DBFreeStatement(hStmt);

         bool runningChanged = (runningConfig != nullptr) && wcscmp(runningHash, oldRunningHash);
         bool startupChanged = (startupConfig != nullptr) && wcscmp(startupHash, oldStartupHash);

         if (hasExisting && !runningChanged && !startupChanged)
         {
            // No change - just update last check time on existing row
            hStmt = DBPrepare(hdb, L"UPDATE device_configs SET last_check_time=? WHERE id=?");
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int32_t>(time(nullptr)));
               DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, existingId);
               success = DBExecute(hStmt);
               DBFreeStatement(hStmt);
            }
            nxlog_debug_tag(DEBUG_TAG, 5, L"ExecuteBackupJob(%s [%u]): no configuration changes detected, updated last check time",
               node->getName(), node->getId());
         }
         else
         {
            // Insert new backup record
            int64_t newId = static_cast<int64_t>(InterlockedIncrement64(&s_nextId));
            hStmt = DBPrepare(hdb, L"INSERT INTO device_configs (id,node_id,backup_time,last_check_time,running_config,running_config_hash,startup_config,startup_config_hash) VALUES (?,?,?,?,?,?,?,?)");
            if (hStmt != nullptr)
            {
               int32_t now = static_cast<int32_t>(time(nullptr));
               DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, newId);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, node->getId());
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, now);
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, now);
               DBBind(hStmt, 5, DB_SQLTYPE_TEXT, runningConfig, DB_BIND_STATIC);
               DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, (runningConfig != nullptr) ? runningHash : nullptr, DB_BIND_STATIC);
               DBBind(hStmt, 7, DB_SQLTYPE_TEXT, startupConfig, DB_BIND_STATIC);
               DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, (startupConfig != nullptr) ? startupHash : nullptr, DB_BIND_STATIC);
               success = DBExecute(hStmt);
               DBFreeStatement(hStmt);
            }

            if (success)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, L"ExecuteBackupJob(%s [%u]): new backup stored (id=" INT64_FMT L", runningChanged=%s, startupChanged=%s)",
                  node->getName(), node->getId(), newId, runningChanged ? L"yes" : L"no", startupChanged ? L"yes" : L"no");

               // Fire change events
               if (runningChanged)
               {
                  EventBuilder(EVENT_RUNNING_CONFIG_CHANGED, *node)
                     .param(L"previousHash", oldRunningHash)
                     .param(L"newHash", runningHash)
                     .post();
               }
               if (startupChanged)
               {
                  EventBuilder(EVENT_STARTUP_CONFIG_CHANGED, *node)
                     .param(L"previousHash", oldStartupHash)
                     .param(L"newHash", startupHash)
                     .post();
               }
            }
         }
      }
      else
      {
         DBFreeStatement(hStmt);
      }
   }

   DBConnectionPoolReleaseConnection(hdb);
   MemFree(runningConfig);
   MemFree(startupConfig);

   // Update node attributes
   node->setCustomAttribute(L"$devbackup.lastJobStatus", static_cast<int32_t>(success ? DeviceBackupJobStatus::SUCCESSFUL : DeviceBackupJobStatus::FAILED));
   node->setCustomAttribute(L"$devbackup.lastJobMessage", success ? L"" : L"Failed to store backup in database");
   if (success)
   {
      node->setCustomAttribute(L"$devbackup.lastBackupTime", static_cast<int64_t>(time(nullptr)));
   }
   node->updateConfigBackupStatus();

   nxlog_debug_tag(DEBUG_TAG, 5, L"ExecuteBackupJob(%s [%u]): completed (success=%s)", node->getName(), node->getId(), success ? L"yes" : L"no");
   InterlockedDecrement(&g_activeBackupJobs);
   SignalScheduler();
}

/**
 * Start backup job for a node
 */
DeviceBackupApiStatus NxDevBackupStartJob(const Node& node)
{
   shared_ptr<Node> nodePtr = static_pointer_cast<Node>(FindObjectById(node.getId(), OBJECT_NODE));
   if (nodePtr == nullptr)
      return DeviceBackupApiStatus::DEVICE_NOT_REGISTERED;

   ThreadPoolExecute(g_mainThreadPool, ExecuteBackupJob, nodePtr);
   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * Get last backup job status
 */
std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> NxDevBackupGetLastJobStatus(const Node& node)
{
   if (!node.getCustomAttributeAsBoolean(L"$devbackup.registered", false))
      return std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, DeviceBackupJobInfo());

   DeviceBackupJobInfo info;
   info.status = static_cast<DeviceBackupJobStatus>(node.getCustomAttributeAsInt32(L"$devbackup.lastJobStatus", static_cast<int32_t>(DeviceBackupJobStatus::UNKNOWN)));
   info.timestamp = static_cast<time_t>(node.getCustomAttributeAsInt64(L"$devbackup.lastBackupTime", 0));
   SharedString msg = node.getCustomAttribute(L"$devbackup.lastJobMessage");
   if (!msg.isNull())
      wcslcpy(info.message, msg, 256);
   return std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::SUCCESS, info);
}

/**
 * Helper: parse hash hex string from DB into binary hash
 */
static void ParseHashFromDB(DB_RESULT hResult, int row, int column, BYTE *hash)
{
   WCHAR hashStr[SHA256_DIGEST_SIZE * 2 + 1];
   DBGetField(hResult, row, column, hashStr, SHA256_DIGEST_SIZE * 2 + 1);
   if (hashStr[0] != 0)
      StrToBinW(hashStr, hash, SHA256_DIGEST_SIZE);
   else
      memset(hash, 0, SHA256_DIGEST_SIZE);
}

/**
 * Helper: populate BackupData with full config content from DB result row.
 * Columns: id, backup_time, last_check_time, running_config, running_config_hash, startup_config, startup_config_hash
 */
static void FillBackupDataFull(DB_RESULT hResult, int row, BackupData *bd)
{
   bd->id = DBGetFieldInt64(hResult, row, 0);
   bd->timestamp = static_cast<time_t>(DBGetFieldInt64(hResult, row, 1));
   bd->lastCheckTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 2));
   bd->isBinary = false;

   // Running config
   char *running = DBGetFieldUTF8(hResult, row, 3, nullptr, 0);
   if (running != nullptr)
   {
      bd->runningConfigSize = strlen(running);
      bd->runningConfig = reinterpret_cast<BYTE*>(running);
   }
   ParseHashFromDB(hResult, row, 4, bd->runningConfigHash);

   // Startup config
   char *startup = DBGetFieldUTF8(hResult, row, 5, nullptr, 0);
   if (startup != nullptr)
   {
      bd->startupConfigSize = strlen(startup);
      bd->startupConfig = reinterpret_cast<BYTE*>(startup);
   }
   ParseHashFromDB(hResult, row, 6, bd->startupConfigHash);
}

/**
 * Helper: populate BackupData with metadata only from DB result row.
 * Columns: id, backup_time, last_check_time, running_config_hash, startup_config_hash, length(running_config), length(startup_config)
 */
static void FillBackupDataMetadata(DB_RESULT hResult, int row, BackupData *bd)
{
   bd->id = DBGetFieldInt64(hResult, row, 0);
   bd->timestamp = static_cast<time_t>(DBGetFieldInt64(hResult, row, 1));
   bd->lastCheckTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 2));
   bd->isBinary = false;

   ParseHashFromDB(hResult, row, 3, bd->runningConfigHash);
   ParseHashFromDB(hResult, row, 4, bd->startupConfigHash);
   bd->runningConfigSize = DBGetFieldUInt32(hResult, row, 5);
   bd->startupConfigSize = DBGetFieldUInt32(hResult, row, 6);
}

/**
 * Get latest backup for a node (with full content)
 */
std::pair<DeviceBackupApiStatus, BackupData> NxDevBackupGetLatestBackup(const Node& node)
{
   BackupData bd;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      L"SELECT id,backup_time,last_check_time,running_config,running_config_hash,startup_config,startup_config_hash"
      L" FROM device_configs WHERE node_id=? ORDER BY backup_time DESC");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, node.getId());
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            FillBackupDataFull(hResult, 0, &bd);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   if (bd.id < 0)
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, BackupData());

   return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::SUCCESS, std::move(bd));
}

/**
 * Get backup list for a node (metadata only, no config content)
 */
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> NxDevBackupGetBackupList(const Node& node)
{
   std::vector<BackupData> backups;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      L"SELECT id,backup_time,last_check_time,running_config_hash,startup_config_hash,"
      L"coalesce(length(running_config),0),coalesce(length(startup_config),0)"
      L" FROM device_configs WHERE node_id=? ORDER BY backup_time DESC");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, node.getId());
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            BackupData bd;
            FillBackupDataMetadata(hResult, i, &bd);
            backups.push_back(std::move(bd));
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return std::pair<DeviceBackupApiStatus, std::vector<BackupData>>(DeviceBackupApiStatus::SUCCESS, std::move(backups));
}

/**
 * Get backup by ID (with full content)
 */
std::pair<DeviceBackupApiStatus, BackupData> NxDevBackupGetBackupById(const Node& node, int64_t id)
{
   BackupData bd;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      L"SELECT id,backup_time,last_check_time,running_config,running_config_hash,startup_config,startup_config_hash"
      L" FROM device_configs WHERE id=? AND node_id=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, id);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, node.getId());
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            FillBackupDataFull(hResult, 0, &bd);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   if (bd.id < 0)
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, BackupData());

   return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::SUCCESS, std::move(bd));
}
