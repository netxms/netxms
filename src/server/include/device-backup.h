/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
struct BackupData
{
   int64_t id;
   time_t timestamp;
   BYTE *data;
   size_t size;
   bool isBinary;
   
   BackupData()
   {
      id = -1;
      timestamp = 0;
      data = nullptr;
      size = 0;
      isBinary = false;
   }
   
   BackupData(const BackupData& src)
   {
      id = src.id;
      timestamp = src.timestamp;
      data = MemCopyBlock(src.data, src.size);
      size = src.size;
      isBinary = src.isBinary;
   }

   BackupData(BackupData&& src)
   {
      id = src.id;
      timestamp = src.timestamp;
      data = src.data;
      size = src.size;
      isBinary = src.isBinary;
      
      src.data = nullptr;
      src.size = 0;
   }

   ~BackupData()
   {
      MemFree(data);
   }
   
   BackupData& operator =(const BackupData& src)
   {
      MemFree(data);

      id = src.id;
      timestamp = src.timestamp;
      data = MemCopyBlock(src.data, src.size);
      size = src.size;
      isBinary = src.isBinary;

      return *this;
   }

   BackupData& operator =(BackupData&& src)
   {
      MemFree(data);

      id = src.id;
      timestamp = src.timestamp;
      data = MemCopyBlock(src.data, src.size);
      size = src.size;
      isBinary = src.isBinary;

      src.data = nullptr;
      src.size = 0;

      return *this;
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
   std::pair<DeviceBackupApiStatus, DeviceBackupJobStatus> (*GetLastJobStatus)(const Node& node);
   std::pair<DeviceBackupApiStatus, BackupData> (*GetLatestBackup)(const Node& node);
   std::pair<DeviceBackupApiStatus, std::vector<BackupData>> (*GetBackups)(const Node& node);
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
std::pair<DeviceBackupApiStatus, DeviceBackupJobStatus> DevBackupGetLastJobStatus(const Node& node);

/**
 * Get latest device backup
 */
std::pair<DeviceBackupApiStatus, BackupData> DevBackupGetLatestBackup(const Node& node);

/**
 * Get list of device backups
 */
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> DevBackupGetBackups(const Node& node);

#endif   /* _device_backup_h_ */
