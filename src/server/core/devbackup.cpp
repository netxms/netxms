/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: devbackup.cpp
**
**/

#include "nxcore.h"
#include <nddrv.h>
#include <device-backup.h>
#include <nxtask.h>

#define DEBUG_TAG _T("backup")

/**
 * Device backup interface
 */
static DeviceBackupInterface *s_provider;

/**
 * Get error message for given status
 */
const TCHAR *GetDeviceBackupApiErrorMessage(DeviceBackupApiStatus status)
{
   static const TCHAR *messages[] =
   {
      _T("No errors"),
      _T("Service provider unavailable"),
      _T("Function not implemented"),
      _T("External API error"),
      _T("Device not registered for backups")
   };
   return messages[static_cast<int>(status)];
}

/**
 * Register device for backup
 */
DeviceBackupApiStatus NXCORE_EXPORTABLE DevBackupRegisterDevice(Node *node)
{
   return (s_provider != nullptr) ? s_provider->RegisterDevice(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Delete device from backup service
 */
DeviceBackupApiStatus NXCORE_EXPORTABLE DevBackupDeleteDevice(Node *node)
{
   return (s_provider != nullptr) ? s_provider->DeleteDevice(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Check if device is registered for backup
 */
bool NXCORE_EXPORTABLE DevBackupIsDeviceRegistered(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->IsDeviceRegistered(node) : false;
}

/**
 * Validate device registration
 */
DeviceBackupApiStatus NXCORE_EXPORTABLE DevBackupValidateDeviceRegistration(Node *node)
{
   return (s_provider != nullptr) ? s_provider->ValidateDeviceRegistration(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Initiate immediate device backup
 */
DeviceBackupApiStatus NXCORE_EXPORTABLE DevBackupStartJob(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->StartJob(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Get status of last backup job
 */
std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> NXCORE_EXPORTABLE DevBackupGetLastJobStatus(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetLastJobStatus(node) : std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, DeviceBackupJobInfo());
}

/**
 * Get latest device backup
 */
std::pair<DeviceBackupApiStatus, BackupData> NXCORE_EXPORTABLE DevBackupGetLatestBackup(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetLatestBackup(node) : std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, BackupData());
}

/**
 * Get list of device backups (metadata only)
 */
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> NXCORE_EXPORTABLE DevBackupGetBackupList(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetBackupList(node) : std::pair<DeviceBackupApiStatus, std::vector<BackupData>>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, std::vector<BackupData>());
}

/**
 * Get device backup by ID (with full config content)
 */
std::pair<DeviceBackupApiStatus, BackupData> NXCORE_EXPORTABLE DevBackupGetBackupById(const Node& node, int64_t id)
{
   return (s_provider != nullptr) ? s_provider->GetBackupById(node, id) : std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, BackupData());
}

/**
 * Initialize device backup interface
 */
void InitializeDeviceBackupInterface()
{
   const TCHAR *providerName;
   ENUMERATE_MODULES(deviceBackupInterface)
   {
      DeviceBackupApiStatus status = CURRENT_MODULE.deviceBackupInterface->Initialize();
      if (status == DeviceBackupApiStatus::SUCCESS)
      {
         s_provider = CURRENT_MODULE.deviceBackupInterface;
         providerName = CURRENT_MODULE.name;
         break;
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Network device backup interface provided by module %s cannot be initialized (%s)"),
            CURRENT_MODULE.name, GetDeviceBackupApiErrorMessage(status));
      }
   }

   if (s_provider != nullptr)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Network device backup interface is provided by module %s"), providerName);
      RegisterComponent(_T("DEVBACKUP"));
      InterlockedOr64(&g_flags, AF_DEVICE_BACKUP_API_ENABLED);
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Network device backup interface is not available"));
   }
}

/**
 * Restore device configuration by feeding it to device via driver's restore hook.
 * Intended to be executed as background task body.
 */
bool RestoreDeviceConfig(const shared_ptr<Node>& node, const BYTE *config, size_t size, uint32_t sourceNodeId,
      const wchar_t *sourceNodeName, int64_t backupId, const wchar_t *userName, BackgroundTask *task)
{
   BYTE hash[SHA256_DIGEST_SIZE];
   CalculateSHA256Hash(config, size, hash);
   wchar_t hashText[SHA256_DIGEST_SIZE * 2 + 1];
   BinToStrW(hash, SHA256_DIGEST_SIZE, hashText);

   nxlog_debug_tag(DEBUG_TAG, 4, L"RestoreDeviceConfig(%s [%u]): starting device configuration restore (%u bytes, source node ID %u, backup ID " INT64_FMT ")",
         node->getName(), node->getId(), static_cast<uint32_t>(size), sourceNodeId, backupId);

   EventBuilder(EVENT_DEVICE_CONFIG_RESTORE_STARTED, *node)
      .param(L"sourceNodeId", sourceNodeId)
      .param(L"sourceNodeName", sourceNodeName)
      .param(L"backupId", backupId)
      .param(L"configHash", hashText)
      .param(L"userName", userName)
      .post();

   StringBuffer errorLog;
   bool success;
   wchar_t newHashText[SHA256_DIGEST_SIZE * 2 + 1];
   newHashText[0] = 0;

   {
      NodeDeviceContext ctx(node);
      ByteStream configStream(size);
      configStream.write(config, size);
      success = node->getDriver()->restoreConfig(&ctx, configStream, &errorLog,
         [task] (int done, int total)
         {
            task->markProgress(done * 80 / total);
         });

      if (success)
      {
         // Post-restore verification - re-read running config on the same session
         task->markProgress(85);
         ByteStream verification;
         if (node->getDriver()->getRunningConfig(&ctx, &verification))
         {
            BYTE newHash[SHA256_DIGEST_SIZE];
            CalculateSHA256Hash(verification.buffer(), verification.size(), newHash);
            BinToStrW(newHash, SHA256_DIGEST_SIZE, newHashText);
         }
         else
         {
            errorLog = L"Configuration was pushed and saved, but post-restore verification failed";
            success = false;
         }
      }
   }  // close SSH session before starting post-restore backup job

   if (!success)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"RestoreDeviceConfig(%s [%u]): restore failed (%s)", node->getName(), node->getId(), errorLog.cstr());
      EventBuilder(EVENT_DEVICE_CONFIG_RESTORE_FAILED, *node)
         .param(L"sourceNodeId", sourceNodeId)
         .param(L"sourceNodeName", sourceNodeName)
         .param(L"backupId", backupId)
         .param(L"configHash", hashText)
         .param(L"userName", userName)
         .param(L"errorMessage", errorLog.cstr())
         .post();
      return task->failure(L"%s", errorLog.cstr());
   }

   // Persist result of the restore as a fresh backup (best effort; will not succeed
   // if target node is not yet registered with the backup provider)
   DeviceBackupApiStatus status = DevBackupStartJob(*node);
   if (status != DeviceBackupApiStatus::SUCCESS)
      nxlog_debug_tag(DEBUG_TAG, 4, L"RestoreDeviceConfig(%s [%u]): cannot start post-restore backup job (%s)",
            node->getName(), node->getId(), GetDeviceBackupApiErrorMessage(status));

   task->markProgress(100);
   EventBuilder(EVENT_DEVICE_CONFIG_RESTORE_COMPLETED, *node)
      .param(L"sourceNodeId", sourceNodeId)
      .param(L"sourceNodeName", sourceNodeName)
      .param(L"backupId", backupId)
      .param(L"configHash", hashText)
      .param(L"userName", userName)
      .param(L"newConfigHash", newHashText)
      .post();
   nxlog_debug_tag(DEBUG_TAG, 4, L"RestoreDeviceConfig(%s [%u]): device configuration restore completed", node->getName(), node->getId());
   return true;
}

/**
 * Check that target node accepts restore requests: caller has upload right, driver implements restore hook,
 * and at least one transport used by restore hooks (interactive SSH or NETCONF) is available.
 */
static DeviceConfigRestoreStatus CheckRestoreTarget(const GenericClientSession& session, const Node& node)
{
   if (!node.checkAccessRights(session.getUserId(), OBJECT_ACCESS_UPLOAD_DEVICE_CONFIG))
   {
      session.writeAuditLog(AUDIT_OBJECTS, false, node.getId(), L"Access denied on device configuration restore");
      return DeviceConfigRestoreStatus::ACCESS_DENIED;
   }

   if (!node.getDriver()->isConfigRestoreSupported())
      return DeviceConfigRestoreStatus::NOT_SUPPORTED;

   uint64_t capabilities = node.getCapabilities();
   bool sshAvailable = (capabilities & NC_SSH_INTERACTIVE_CHANNEL) && !(node.getFlags() & NF_DISABLE_SSH);
   bool netconfAvailable = (capabilities & NC_IS_NETCONF) && !(node.getFlags() & (NF_DISABLE_SSH | NF_DISABLE_NETCONF));
   if (!sshAvailable && !netconfAvailable)
      return DeviceConfigRestoreStatus::NO_TRANSPORT;

   return DeviceConfigRestoreStatus::SUCCESS;
}

/**
 * Start restore of already validated configuration as serialized background task (one restore at a time per node).
 * Takes ownership of configuration buffer.
 */
static shared_ptr<BackgroundTask> StartRestoreTask(const GenericClientSession& session, const shared_ptr<Node>& node,
      BYTE *config, size_t size, uint32_t sourceNodeId, const wchar_t *sourceNodeName, int64_t backupId)
{
   wchar_t key[64];
   nx_swprintf(key, 64, L"restore-config-%u", node->getId());
   wchar_t description[256];
   nx_swprintf(description, 256, L"Restore device configuration on %s", node->getName());
   String userName(session.getLoginName());
   String sourceName(sourceNodeName);
   return CreateSerializedBackgroundTask(g_mainThreadPool, key,
      [node, config, size, sourceNodeId, sourceName, backupId, userName] (BackgroundTask *task) -> bool
      {
         bool success = RestoreDeviceConfig(node, config, size, sourceNodeId, sourceName.cstr(), backupId, userName.cstr(), task);
         MemFree(config);
         return success;
      }, description);
}

/**
 * Validate restore request for configuration taken from stored backup of source node (possibly a different
 * node than the target), write audit record, and start restore as background task on success.
 */
DeviceConfigRestoreStatus NXCORE_EXPORTABLE StartDeviceConfigRestoreFromBackup(const GenericClientSession& session,
      const shared_ptr<Node>& node, const shared_ptr<Node>& sourceNode, int64_t backupId, bool useStartupConfig,
      bool forceApply, shared_ptr<BackgroundTask> *task)
{
   DeviceConfigRestoreStatus status = CheckRestoreTarget(session, *node);
   if (status != DeviceConfigRestoreStatus::SUCCESS)
      return status;

   if (!sourceNode->checkAccessRights(session.getUserId(), OBJECT_ACCESS_READ_DEVICE_CONFIG))
   {
      session.writeAuditLog(AUDIT_OBJECTS, false, sourceNode->getId(), L"Access denied on reading device configuration backup for restore");
      return DeviceConfigRestoreStatus::ACCESS_DENIED;
   }

   if (wcscmp(sourceNode->getDriverName(), node->getDriverName()) && !forceApply)
      return DeviceConfigRestoreStatus::DRIVER_MISMATCH;

   auto result = DevBackupGetBackupById(*sourceNode, backupId);
   if (result.first != DeviceBackupApiStatus::SUCCESS)
      return (result.first == DeviceBackupApiStatus::NOT_IMPLEMENTED) ? DeviceConfigRestoreStatus::BACKUP_NOT_SUPPORTED : DeviceConfigRestoreStatus::BACKUP_UNAVAILABLE;

   if (result.second.isBinary)
      return DeviceConfigRestoreStatus::BINARY_BACKUP;

   const BYTE *config = useStartupConfig ? result.second.startupConfig : result.second.runningConfig;
   size_t size = useStartupConfig ? result.second.startupConfigSize : result.second.runningConfigSize;
   if ((config == nullptr) || (size == 0))
      return DeviceConfigRestoreStatus::EMPTY_CONFIG;

   BYTE hash[SHA256_DIGEST_SIZE];
   CalculateSHA256Hash(config, size, hash);
   wchar_t hashText[SHA256_DIGEST_SIZE * 2 + 1];
   BinToStrW(hash, SHA256_DIGEST_SIZE, hashText);
   session.writeAuditLog(AUDIT_OBJECTS, true, node->getId(), L"Device configuration restore initiated (source node \"%s\" [%u], backup ID " INT64_FMT L", config SHA-256 %s%s)",
         sourceNode->getName(), sourceNode->getId(), backupId, hashText, forceApply ? L", driver mismatch override" : L"");

   *task = StartRestoreTask(session, node, MemCopyBlock(config, size), size, sourceNode->getId(), sourceNode->getName(), backupId);
   return DeviceConfigRestoreStatus::SUCCESS;
}

/**
 * Validate restore request for client-supplied configuration text, write audit record, and start restore
 * as background task on success.
 */
DeviceConfigRestoreStatus NXCORE_EXPORTABLE StartDeviceConfigRestoreFromText(const GenericClientSession& session,
      const shared_ptr<Node>& node, const BYTE *config, size_t size, shared_ptr<BackgroundTask> *task)
{
   DeviceConfigRestoreStatus status = CheckRestoreTarget(session, *node);
   if (status != DeviceConfigRestoreStatus::SUCCESS)
      return status;

   if ((config == nullptr) || (size == 0))
      return DeviceConfigRestoreStatus::EMPTY_CONFIG;

   BYTE hash[SHA256_DIGEST_SIZE];
   CalculateSHA256Hash(config, size, hash);
   wchar_t hashText[SHA256_DIGEST_SIZE * 2 + 1];
   BinToStrW(hash, SHA256_DIGEST_SIZE, hashText);
   session.writeAuditLog(AUDIT_OBJECTS, true, node->getId(), L"Device configuration restore initiated (client-supplied configuration, config SHA-256 %s)", hashText);

   *task = StartRestoreTask(session, node, MemCopyBlock(config, size), size, 0, L"", 0);
   return DeviceConfigRestoreStatus::SUCCESS;
}
