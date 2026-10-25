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
** File: devbackup.h
**/

#ifndef _devbackup_h_
#define _devbackup_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <nddrv.h>
#include <device-backup.h>

#define DEBUG_TAG L"devbackup"

// Global configuration
extern int32_t g_backupInterval;      // seconds, default 86400
extern int32_t g_maxConcurrentJobs;   // default 10
extern int32_t g_retentionDays;       // default 90

// Scheduler
void StartBackupScheduler();
void StopBackupScheduler();
void SignalScheduler();

// Backup execution
void InitializeBackupSubsystem();
bool SubmitBackupJob(const shared_ptr<Node>& node);

extern VolatileCounter g_activeBackupJobs;

// Device functions
DeviceBackupApiStatus NxDevBackupRegisterDevice(Node *node);
DeviceBackupApiStatus NxDevBackupDeleteDevice(Node *node);
bool NxDevBackupIsDeviceRegistered(const Node& node);
DeviceBackupApiStatus NxDevBackupValidateDeviceRegistration(Node *node);
DeviceBackupApiStatus NxDevBackupStartJob(const Node& node);
std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> NxDevBackupGetLastJobStatus(const Node& node);
std::pair<DeviceBackupApiStatus, BackupData> NxDevBackupGetLatestBackup(const Node& node);
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> NxDevBackupGetBackupList(const Node& node);
std::pair<DeviceBackupApiStatus, BackupData> NxDevBackupGetBackupById(const Node& node, int64_t id);

#endif
