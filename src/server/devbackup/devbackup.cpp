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
** File: devbackup.cpp
**/

#include "devbackup.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("DEVBACKUP", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Global configuration
 */
int32_t g_backupInterval = 86400;
int32_t g_maxConcurrentJobs = 10;
int32_t g_retentionDays = 90;

/**
 * Module initialization
 */
static bool InitializeModule(Config *config)
{
   g_backupInterval = ConfigReadInt(L"DeviceConfigBackup.Interval", 86400);
   g_maxConcurrentJobs = ConfigReadInt(L"DeviceConfigBackup.MaxJobs", 10);
   g_retentionDays = ConfigReadInt(L"DeviceConfigBackup.RetentionDays", 90);

   nxlog_debug_tag(DEBUG_TAG, 2, L"DEVBACKUP module initialized (interval=%d, maxJobs=%d, retention=%d days)",
      g_backupInterval, g_maxConcurrentJobs, g_retentionDays);
   return true;
}

/**
 * Interface initialization - called when the device backup subsystem is ready
 */
static DeviceBackupApiStatus NxDevBackupInitializeInterface()
{
   InitializeBackupSubsystem();
   StartBackupScheduler();
   nxlog_debug_tag(DEBUG_TAG, 2, L"Backup scheduler started");
   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * Module shutdown
 */
static void ShutdownModule()
{
   StopBackupScheduler();
   nxlog_debug_tag(DEBUG_TAG, 2, L"DEVBACKUP module shut down");
}

/**
 * Housekeeper hook - delete old backups
 */
static void HousekeeperHook()
{
   time_t cutoff = time(nullptr) - static_cast<time_t>(g_retentionDays) * 86400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"DELETE FROM device_configs WHERE last_check_time<?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int32_t>(cutoff));
      if (DBExecute(hStmt))
         nxlog_debug_tag(DEBUG_TAG, 5, L"Housekeeper: cleaned up old device configuration backups (cutoff=%d days)", g_retentionDays);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Network device backup interface
 */
static DeviceBackupInterface s_interface =
{
   NxDevBackupInitializeInterface,        // Initialize
   NxDevBackupRegisterDevice,             // RegisterDevice
   NxDevBackupDeleteDevice,               // DeleteDevice
   NxDevBackupIsDeviceRegistered,         // IsDeviceRegistered
   NxDevBackupValidateDeviceRegistration, // ValidateDeviceRegistration
   NxDevBackupStartJob,                   // StartJob
   NxDevBackupGetLastJobStatus,           // GetLastJobStatus
   NxDevBackupGetLatestBackup,            // GetLatestBackup
   NxDevBackupGetBackupList,              // GetBackupList
   NxDevBackupGetBackupById               // GetBackupById
};

/**
 * Module registration
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"DEVBACKUP");
   module->pfInitialize = InitializeModule;
   module->pfShutdown = ShutdownModule;
   module->pfHousekeeperHook = HousekeeperHook;
   module->deviceBackupInterface = &s_interface;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
