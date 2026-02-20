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
** File: device-backup.h
**
**/

#ifndef _device_backup_h_
#define _device_backup_h_

/**
 * Status codes for device backup functions
 */
enum class DeviceBackupApiStatus
{
   SUCCESS = 0,
   PROVIDER_UNAVAILABLE = 1,
   NOT_IMPLEMENTED = 2,
   EXTERNAL_API_ERROR = 3,
   DEVICE_NOT_REGISTERED = 4
};

/**
 * Single backup data
 */
struct NXCORE_EXPORTABLE BackupData
{
   int64_t id;
   time_t timestamp;
   time_t lastCheckTime;
   BYTE *runningConfig;
   size_t runningConfigSize;
   BYTE runningConfigHash[SHA256_DIGEST_SIZE];
   BYTE *startupConfig;
   size_t startupConfigSize;
   BYTE startupConfigHash[SHA256_DIGEST_SIZE];
   bool isBinary;

   BackupData()
   {
      id = -1;
      timestamp = 0;
      lastCheckTime = 0;
      runningConfig = nullptr;
      runningConfigSize = 0;
      memset(runningConfigHash, 0, SHA256_DIGEST_SIZE);
      startupConfig = nullptr;
      startupConfigSize = 0;
      memset(startupConfigHash, 0, SHA256_DIGEST_SIZE);
      isBinary = false;
   }

   BackupData(const BackupData& src)
   {
      id = src.id;
      timestamp = src.timestamp;
      lastCheckTime = src.lastCheckTime;
      runningConfig = (src.runningConfig != nullptr) ? MemCopyBlock(src.runningConfig, src.runningConfigSize) : nullptr;
      runningConfigSize = src.runningConfigSize;
      memcpy(runningConfigHash, src.runningConfigHash, SHA256_DIGEST_SIZE);
      startupConfig = (src.startupConfig != nullptr) ? MemCopyBlock(src.startupConfig, src.startupConfigSize) : nullptr;
      startupConfigSize = src.startupConfigSize;
      memcpy(startupConfigHash, src.startupConfigHash, SHA256_DIGEST_SIZE);
      isBinary = src.isBinary;
   }

   BackupData(BackupData&& src)
   {
      id = src.id;
      timestamp = src.timestamp;
      lastCheckTime = src.lastCheckTime;
      runningConfig = src.runningConfig;
      runningConfigSize = src.runningConfigSize;
      memcpy(runningConfigHash, src.runningConfigHash, SHA256_DIGEST_SIZE);
      startupConfig = src.startupConfig;
      startupConfigSize = src.startupConfigSize;
      memcpy(startupConfigHash, src.startupConfigHash, SHA256_DIGEST_SIZE);
      isBinary = src.isBinary;

      src.runningConfig = nullptr;
      src.runningConfigSize = 0;
      src.startupConfig = nullptr;
      src.startupConfigSize = 0;
   }

   ~BackupData()
   {
      MemFree(runningConfig);
      MemFree(startupConfig);
   }

   BackupData& operator =(const BackupData& src)
   {
      if (this != &src)
      {
         MemFree(runningConfig);
         MemFree(startupConfig);

         id = src.id;
         timestamp = src.timestamp;
         lastCheckTime = src.lastCheckTime;
         runningConfig = (src.runningConfig != nullptr) ? MemCopyBlock(src.runningConfig, src.runningConfigSize) : nullptr;
         runningConfigSize = src.runningConfigSize;
         memcpy(runningConfigHash, src.runningConfigHash, SHA256_DIGEST_SIZE);
         startupConfig = (src.startupConfig != nullptr) ? MemCopyBlock(src.startupConfig, src.startupConfigSize) : nullptr;
         startupConfigSize = src.startupConfigSize;
         memcpy(startupConfigHash, src.startupConfigHash, SHA256_DIGEST_SIZE);
         isBinary = src.isBinary;
      }
      return *this;
   }

   BackupData& operator =(BackupData&& src)
   {
      if (this != &src)
      {
         MemFree(runningConfig);
         MemFree(startupConfig);

         id = src.id;
         timestamp = src.timestamp;
         lastCheckTime = src.lastCheckTime;
         runningConfig = src.runningConfig;
         runningConfigSize = src.runningConfigSize;
         memcpy(runningConfigHash, src.runningConfigHash, SHA256_DIGEST_SIZE);
         startupConfig = src.startupConfig;
         startupConfigSize = src.startupConfigSize;
         memcpy(startupConfigHash, src.startupConfigHash, SHA256_DIGEST_SIZE);
         isBinary = src.isBinary;

         src.runningConfig = nullptr;
         src.runningConfigSize = 0;
         src.startupConfig = nullptr;
         src.startupConfigSize = 0;
      }
      return *this;
   }
};

/**
 * Last backup job info
 */
struct DeviceBackupJobInfo
{
   DeviceBackupJobStatus status;
   time_t timestamp;
   TCHAR message[256];

   DeviceBackupJobInfo()
   {
      status = DeviceBackupJobStatus::UNKNOWN;
      timestamp = 0;
      message[0] = 0;
   }
};

/**
 * Module interface for network device backup system
 */
struct DeviceBackupInterface
{
   DeviceBackupApiStatus (*Initialize)();
   DeviceBackupApiStatus (*RegisterDevice)(Node *node);
   DeviceBackupApiStatus (*DeleteDevice)(Node *node);
   bool (*IsDeviceRegistered)(const Node& node);
   DeviceBackupApiStatus (*ValidateDeviceRegistration)(Node *node);
   DeviceBackupApiStatus (*StartJob)(const Node& node);
   std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> (*GetLastJobStatus)(const Node& node);
   std::pair<DeviceBackupApiStatus, BackupData> (*GetLatestBackup)(const Node& node);
   std::pair<DeviceBackupApiStatus, std::vector<BackupData>> (*GetBackupList)(const Node& node);
   std::pair<DeviceBackupApiStatus, BackupData> (*GetBackupById)(const Node& node, int64_t id);
};

/**
 * Concrete DeviceBackupContext implementation backed by a Node object.
 * Provides SSH and SNMP access using the node's configured credentials.
 */
class NXCORE_EXPORTABLE NodeBackupContext : public DeviceBackupContext
{
private:
   shared_ptr<Node> m_node;
   shared_ptr<SSHInteractiveChannel> m_sshChannel;
   SNMP_Transport *m_snmpTransport;

public:
   NodeBackupContext(const shared_ptr<Node>& node);
   ~NodeBackupContext();

   bool isSSHCommandChannelAvailable() override;
   bool isSSHInteractiveChannelAvailable() override;
   bool isSNMPAvailable() override;

   SSHInteractiveChannel *getInteractiveSSH() override;
   bool executeSSHCommand(const char *command, ByteStream *output) override;
   SNMP_Transport *getSNMPTransport() override;
};

/**
 * Get error message for given status
 */
const TCHAR *GetDeviceBackupApiErrorMessage(DeviceBackupApiStatus status);

/**
 * Register device for backup
 */
DeviceBackupApiStatus DevBackupRegisterDevice(Node *node);

/**
 * Delete device from backup service
 */
DeviceBackupApiStatus DevBackupDeleteDevice(Node *node);

/**
 * Check if device is registered for backup
 */
bool DevBackupIsDeviceRegistered(const Node& node);

/**
 * Validate device registration
 */
DeviceBackupApiStatus DevBackupValidateDeviceRegistration(Node *node);

/**
 * Start backup job immediately
 */
DeviceBackupApiStatus DevBackupStartJob(const Node& node);

/**
 * Get status of last backup job
 */
std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> DevBackupGetLastJobStatus(const Node& node);

/**
 * Get latest device backup
 */
std::pair<DeviceBackupApiStatus, BackupData> DevBackupGetLatestBackup(const Node& node);

/**
 * Get list of device backups (metadata only, config content pointers will be NULL)
 */
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> DevBackupGetBackupList(const Node& node);

/**
 * Get device backup by ID (with full config content)
 */
std::pair<DeviceBackupApiStatus, BackupData> DevBackupGetBackupById(const Node& node, int64_t id);

#endif   /* _device_backup_h_ */
