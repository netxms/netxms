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
** File: devices.cpp
**/

#include "devbackup.h"

/**
 * Check if device is registered for native backup
 */
bool NxDevBackupIsDeviceRegistered(const Node& node)
{
   return node.getCustomAttributeAsBoolean(L"$devbackup.registered", false);
}

/**
 * Register device for native backup
 */
DeviceBackupApiStatus NxDevBackupRegisterDevice(Node *node)
{
   if (!node->getDriver()->isConfigBackupSupported())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NxDevBackupRegisterDevice(%s [%u]): driver does not support config backup", node->getName(), node->getId());
      return DeviceBackupApiStatus::SUCCESS;
   }

   node->setCustomAttribute(L"$devbackup.registered", L"true", StateChange::IGNORE);
   nxlog_debug_tag(DEBUG_TAG, 4, L"NxDevBackupRegisterDevice(%s [%u]): device registered", node->getName(), node->getId());
   SignalScheduler();
   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * Delete device from native backup
 */
DeviceBackupApiStatus NxDevBackupDeleteDevice(Node *node)
{
   node->deleteCustomAttribute(L"$devbackup.registered");
   node->deleteCustomAttribute(L"$devbackup.lastBackupTime");

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"DELETE FROM device_configs WHERE node_id=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, node->getId());
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   nxlog_debug_tag(DEBUG_TAG, 4, L"NxDevBackupDeleteDevice(%s [%u]): device unregistered and backup data deleted", node->getName(), node->getId());
   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * Validate device registration
 */
DeviceBackupApiStatus NxDevBackupValidateDeviceRegistration(Node *node)
{
   if (!node->getCustomAttributeAsBoolean(L"$devbackup.registered", false))
      return DeviceBackupApiStatus::DEVICE_NOT_REGISTERED;

   if (!node->getDriver()->isConfigBackupSupported())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"NxDevBackupValidateDeviceRegistration(%s [%u]): driver no longer supports config backup", node->getName(), node->getId());
      return DeviceBackupApiStatus::DEVICE_NOT_REGISTERED;
   }

   return DeviceBackupApiStatus::SUCCESS;
}
