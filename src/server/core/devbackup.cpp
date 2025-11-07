/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
#include <device-backup.h>

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
DeviceBackupApiStatus DevBackupRegisterDevice(Node *node)
{
   return (s_provider != nullptr) ? s_provider->RegisterDevice(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Delete device from backup service
 */
DeviceBackupApiStatus DevBackupDeleteDevice(Node *node)
{
   return (s_provider != nullptr) ? s_provider->DeleteDevice(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Check if device is registered for backup
 */
bool DevBackupIsDeviceRegistered(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->IsDeviceRegistered(node) : false;
}

/**
 * Validate device registration
 */
DeviceBackupApiStatus DevBackupValidateDeviceRegistration(Node *node)
{
   return (s_provider != nullptr) ? s_provider->ValidateDeviceRegistration(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Initiate immediate device backup
 */
DeviceBackupApiStatus DevBackupStartJob(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->StartJob(node) : DeviceBackupApiStatus::PROVIDER_UNAVAILABLE;
}

/**
 * Get status of last backup job
 */
std::pair<DeviceBackupApiStatus, DeviceBackupJobStatus> DevBackupGetLastJobStatus(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetLastJobStatus(node) : std::pair<DeviceBackupApiStatus, DeviceBackupJobStatus>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, DeviceBackupJobStatus::UNKNOWN);
}

/**
 * Get latest device backup
 */
std::pair<DeviceBackupApiStatus, BackupData> DevBackupGetLatestBackup(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetLatestBackup(node) : std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, BackupData());
}

/**
 * Get list of device backups
 */
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> DevBackupGetBackups(const Node& node)
{
   return (s_provider != nullptr) ? s_provider->GetBackups(node) : std::pair<DeviceBackupApiStatus, std::vector<BackupData>>(DeviceBackupApiStatus::PROVIDER_UNAVAILABLE, std::vector<BackupData>());
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
