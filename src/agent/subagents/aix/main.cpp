/*
** NetXMS subagent for AIX
** Copyright (C) 2005-2025 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "aix_subagent.h"
#include <netxms-version.h>

/**
 * Hanlder functions
 */
LONG H_CpuCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuUsageEx(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_FileSystemInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_HardwareMachineId(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *);
LONG H_HardwareManufacturer(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *);
LONG H_HardwareProduct(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *);
LONG H_Hostname(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_IOStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IOStatsTotal(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_LoadAvg(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_LvmAllLogicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_LvmAllPhysicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_LvmLogicalVolumeInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_LvmLogicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_LvmLogicalVolumesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_LvmPhysicalVolumeInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_LvmPhysicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_LvmPhysicalVolumesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_LvmVolumeGroupInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_LvmVolumeGroups(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_LvmVolumeGroupsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_MemoryInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetInterfaceStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetInterfaceInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetInterfaceList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetInterfaceNames(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetRoutingTable(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_OSInfo(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);
LONG H_OSServicePack(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);
LONG H_ProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ProcessInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ProcessList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_HandleCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SysMsgQueue(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SysProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SysThreadCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_Uname(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_Uptime(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_VirtualMemoryInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Shutdown flag
 */
BOOL g_bShutdown = FALSE;

/**
 * Detect support for source packages
 */
static LONG H_SourcePkg(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	ret_int(value, 1);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Initalization callback
 */
static bool SubAgentInit(Config *config)
{
	StartCpuUsageCollector();
	StartIOStatCollector();
	return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
	g_bShutdown = TRUE;
	ShutdownCpuUsageCollector();
	ShutdownIOStatCollector();
	ClearLvmData();
	ClearNetworkData();
}

/**
 * Subagent's parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Agent.SourcePackageSupport"), H_SourcePkg, nullptr, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

   { _T("Disk.Avail(*)"), H_FileSystemInfo, (TCHAR *)FS_AVAIL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.AvailPerc(*)"), H_FileSystemInfo, (TCHAR *)FS_AVAIL_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Free(*)"), H_FileSystemInfo, (TCHAR *)FS_FREE, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"), H_FileSystemInfo, (TCHAR *)FS_FREE_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"), H_FileSystemInfo, (TCHAR *)FS_TOTAL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"), H_FileSystemInfo, (TCHAR *)FS_USED, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"), H_FileSystemInfo, (TCHAR *)FS_USED_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },

   { _T("FileSystem.Avail(*)"), H_FileSystemInfo, (TCHAR *)FS_AVAIL, DCI_DT_UINT64, DCIDESC_FS_AVAIL },
   { _T("FileSystem.AvailInodes(*)"), H_FileSystemInfo, (TCHAR *)FS_AVAIL_INODES, DCI_DT_UINT64, DCIDESC_FS_AVAILINODES },
   { _T("FileSystem.AvailInodesPerc(*)"), H_FileSystemInfo, (TCHAR *)FS_AVAIL_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILINODESPERC },
   { _T("FileSystem.AvailPerc(*)"), H_FileSystemInfo, (TCHAR *)FS_AVAIL_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILPERC },
   { _T("FileSystem.Free(*)"), H_FileSystemInfo, (TCHAR *)FS_FREE, DCI_DT_UINT64, DCIDESC_FS_FREE },
   { _T("FileSystem.FreeInodes(*)"), H_FileSystemInfo, (TCHAR *)FS_FREE_INODES, DCI_DT_UINT64, DCIDESC_FS_FREEINODES },
   { _T("FileSystem.FreeInodesPerc(*)"), H_FileSystemInfo, (TCHAR *)FS_FREE_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEINODESPERC },
   { _T("FileSystem.FreePerc(*)"), H_FileSystemInfo, (TCHAR *)FS_FREE_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { _T("FileSystem.Total(*)"), H_FileSystemInfo, (TCHAR *)FS_TOTAL, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { _T("FileSystem.TotalInodes(*)"), H_FileSystemInfo, (TCHAR *)FS_TOTAL_INODES, DCI_DT_UINT64, DCIDESC_FS_TOTALINODES },
   { _T("FileSystem.Type(*)"), H_FileSystemInfo, (TCHAR *)FS_FSTYPE, DCI_DT_STRING, DCIDESC_FS_TYPE },
   { _T("FileSystem.Used(*)"), H_FileSystemInfo, (TCHAR *)FS_USED, DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedInodes(*)"), H_FileSystemInfo, (TCHAR *)FS_USED_INODES, DCI_DT_UINT64, DCIDESC_FS_USEDINODES },
   { _T("FileSystem.UsedInodesPerc(*)"), H_FileSystemInfo, (TCHAR *)FS_USED_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDINODESPERC },
   { _T("FileSystem.UsedPerc(*)"), H_FileSystemInfo, (TCHAR *)FS_USED_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

   { _T("Hardware.System.MachineId"), H_HardwareMachineId, nullptr, DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MACHINEID },
   { _T("Hardware.System.Manufacturer"), H_HardwareManufacturer, nullptr, DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MANUFACTURER },
   { _T("Hardware.System.Product"), H_HardwareProduct, nullptr, DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCT },

   { _T("LVM.LogicalVolume.Size(*)"), H_LvmLogicalVolumeInfo, (TCHAR *)LVM_LV_SIZE, DCI_DT_UINT64, DCIDESC_LVM_LV_SIZE },
   { _T("LVM.LogicalVolume.Status(*)"), H_LvmLogicalVolumeInfo, (TCHAR *)LVM_LV_STATUS, DCI_DT_STRING, DCIDESC_LVM_LV_STATUS },
   { _T("LVM.PhysicalVolume.Free(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_FREE, DCI_DT_UINT64, DCIDESC_LVM_PV_FREE },
   { _T("LVM.PhysicalVolume.FreePerc(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_FREE_PERC, DCI_DT_FLOAT, DCIDESC_LVM_PV_FREEPERC },
   { _T("LVM.PhysicalVolume.ResyncPartitions(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_RESYNC, DCI_DT_UINT64, DCIDESC_LVM_PV_RESYNC_PART },
   { _T("LVM.PhysicalVolume.StalePartitions(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_STALE, DCI_DT_UINT64, DCIDESC_LVM_PV_STALE_PART },
   { _T("LVM.PhysicalVolume.Status(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_STATUS, DCI_DT_STRING, DCIDESC_LVM_PV_STATUS },
   { _T("LVM.PhysicalVolume.Total(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_TOTAL, DCI_DT_UINT64, DCIDESC_LVM_PV_TOTAL },
   { _T("LVM.PhysicalVolume.Used(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_USED, DCI_DT_UINT64, DCIDESC_LVM_PV_USED },
   { _T("LVM.PhysicalVolume.UsedPerc(*)"), H_LvmPhysicalVolumeInfo, (TCHAR *)LVM_PV_USED_PERC, DCI_DT_FLOAT, DCIDESC_LVM_PV_USEDPERC },
   { _T("LVM.VolumeGroup.ActivePhysicalVolumes(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_PVOL_ACTIVE, DCI_DT_UINT, DCIDESC_LVM_VG_PVOL_ACTIVE },
   { _T("LVM.VolumeGroup.Free(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_FREE, DCI_DT_UINT64, DCIDESC_LVM_VG_FREE },
   { _T("LVM.VolumeGroup.FreePerc(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_FREE_PERC, DCI_DT_FLOAT, DCIDESC_LVM_VG_FREEPERC },
   { _T("LVM.VolumeGroup.LogicalVolumes(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_LVOL_TOTAL, DCI_DT_UINT, DCIDESC_LVM_VG_LVOL_TOTAL },
   { _T("LVM.VolumeGroup.PhysicalVolumes(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_PVOL_TOTAL, DCI_DT_UINT, DCIDESC_LVM_VG_PVOL_TOTAL },
   { _T("LVM.VolumeGroup.ResyncPartitions(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_RESYNC, DCI_DT_UINT64, DCIDESC_LVM_VG_RESYNC_PART },
   { _T("LVM.VolumeGroup.StalePartitions(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_STALE, DCI_DT_UINT64, DCIDESC_LVM_VG_STALE_PART },
   { _T("LVM.VolumeGroup.Total(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_TOTAL, DCI_DT_UINT64, DCIDESC_LVM_VG_TOTAL },
   { _T("LVM.VolumeGroup.Used(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_USED, DCI_DT_UINT64, DCIDESC_LVM_VG_USED },
   { _T("LVM.VolumeGroup.UsedPerc(*)"), H_LvmVolumeGroupInfo, (TCHAR *)LVM_VG_USED_PERC, DCI_DT_FLOAT, DCIDESC_LVM_VG_USEDPERC },

   { _T("Net.Interface.AdminStatus(*)"), H_NetInterfaceStatus, (TCHAR *)IF_INFO_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_BYTES_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_BYTES_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.Description(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_DESCRIPTION, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { _T("Net.Interface.InErrors(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_IN_ERRORS, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetInterfaceStatus, (TCHAR *)IF_INFO_OPER_STATUS, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.OperStatus(*)"), H_NetInterfaceStatus, (TCHAR *)IF_INFO_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.Interface.MTU(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_MTU, DCI_DT_INT, DCIDESC_NET_INTERFACE_MTU },
   { _T("Net.Interface.OutErrors(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_OUT_ERRORS, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_PACKETS_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_PACKETS_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_SPEED, DCI_DT_INT, DCIDESC_NET_INTERFACE_SPEED },

   { _T("Process.Count(*)"), H_ProcessCount, _T("S"), DCI_DT_INT, DCIDESC_PROCESS_COUNT },
   { _T("Process.CountEx(*)"), H_ProcessCount, _T("E"), DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
   { _T("Process.CPUTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_CPUTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_CPUTIME },
   { _T("Process.Handles(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_HANDLES, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_HANDLES },
   { _T("Process.IO.ReadOp(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_IO_READ_OP, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_READOP },
   { _T("Process.IO.WriteOp(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_IO_WRITE_OP, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_WRITEOP },
   { _T("Process.KernelTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_KTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_KERNELTIME },
   { _T("Process.MemoryUsage(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_MEMPERC, const TCHAR *), DCI_DT_FLOAT, DCIDESC_PROCESS_MEMORYUSAGE },
   { _T("Process.PageFaults(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_PF, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_PAGEFAULTS },
   { _T("Process.RSS(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },
   { _T("Process.Threads(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_THREADS, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_THREADS },
   { _T("Process.UserTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_UTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_USERTIME },
   { _T("Process.VMSize(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_VMSIZE, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_VMSIZE },
   { _T("Process.WkSet(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },

   { _T("System.CPU.Count"), H_CpuCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { _T("System.CPU.LoadAvg"), H_LoadAvg, _T("0"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
   { _T("System.CPU.LoadAvg5"), H_LoadAvg, _T("1"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { _T("System.CPU.LoadAvg15"), H_LoadAvg, _T("2"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },

   { _T("System.CPU.Usage"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
   { _T("System.CPU.Usage5"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
   { _T("System.CPU.Usage15"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
   { _T("System.CPU.Usage(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
   { _T("System.CPU.Usage5(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
   { _T("System.CPU.Usage15(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },

	{ _T("System.CPU.Usage.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE },
	{ _T("System.CPU.Usage5.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
	{ _T("System.CPU.Usage15.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
	{ _T("System.CPU.Usage.Idle(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
	{ _T("System.CPU.Usage5.Idle5(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ _T("System.CPU.Usage15.Idle(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

	{ _T("System.CPU.Usage.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
	{ _T("System.CPU.Usage5.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
	{ _T("System.CPU.Usage15.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
	{ _T("System.CPU.Usage.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
	{ _T("System.CPU.Usage5.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
	{ _T("System.CPU.Usage15.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

	{ _T("System.CPU.Usage.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
	{ _T("System.CPU.Usage5.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
	{ _T("System.CPU.Usage15.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
	{ _T("System.CPU.Usage.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
	{ _T("System.CPU.Usage5.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
	{ _T("System.CPU.Usage15.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

	{ _T("System.CPU.Usage.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER },
	{ _T("System.CPU.Usage5.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER },
	{ _T("System.CPU.Usage15.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER },
	{ _T("System.CPU.Usage.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
	{ _T("System.CPU.Usage5.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
	{ _T("System.CPU.Usage15.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

   { _T("System.CPU.PhysicalAverage"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_OVERALL), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization for last minute") },
   { _T("System.CPU.PhysicalAverage5"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_OVERALL), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_OVERALL), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_USER), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (user) for last minute") },
   { _T("System.CPU.PhysicalAverage5.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_USER), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (user) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_USER), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (user) for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_SYSTEM), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (system) for last minute") },
   { _T("System.CPU.PhysicalAverage5.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_SYSTEM), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (system) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_SYSTEM), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (syste) for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_IDLE), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (idle) for last minute") },
   { _T("System.CPU.PhysicalAverage5.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_IDLE), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (idle) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_IDLE), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (idle) for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_IOWAIT), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (iowait) for last minute") },
   { _T("System.CPU.PhysicalAverage5.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_IOWAIT), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (iowait) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_IOWAIT), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (iowait) for last 15 minutes") },

   { _T("System.Hostname"), H_Hostname, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },

	{ _T("System.IO.BytesReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
	{ _T("System.IO.BytesReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
	{ _T("System.IO.BytesWriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
	{ _T("System.IO.BytesWriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
	{ _T("System.IO.DiskQueue"), H_IOStatsTotal, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
	{ _T("System.IO.DiskQueue(*)"), H_IOStats, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
	{ _T("System.IO.ReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
	{ _T("System.IO.ReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
	{ _T("System.IO.TransferRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS },
	{ _T("System.IO.TransferRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS_EX },
	{ _T("System.IO.WaitTime"), H_IOStatsTotal, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME },
	{ _T("System.IO.WaitTime(*)"), H_IOStats, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME_EX },
	{ _T("System.IO.WriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
	{ _T("System.IO.WriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },

   { _T("System.HandleCount"), H_HandleCount, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_HANDLECOUNT },

   { _T("System.Memory.Physical.Available"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_AVAILABLE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE },
   { _T("System.Memory.Physical.AvailablePerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_AVAILABLE_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT },
   { _T("System.Memory.Physical.Cached"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_CACHED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED },
   { _T("System.Memory.Physical.CachedPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_CACHED_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED_PCT },
   { _T("System.Memory.Physical.Client"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_CLIENT, DCI_DT_UINT64, _T("Physical memory used for client frames") },
   { _T("System.Memory.Physical.ClientPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_CLIENT_PERC, DCI_DT_FLOAT, _T("Percentage of physical memory used for client frames") },
   { _T("System.Memory.Physical.Computational"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_COMP, DCI_DT_UINT64, _T("Physical memory used for working segments") },
   { _T("System.Memory.Physical.ComputationalPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_COMP_PERC, DCI_DT_FLOAT, _T("Percentage of physical memory used for working segments") },
   { _T("System.Memory.Physical.Free"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { _T("System.Memory.Physical.FreePerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_FREE_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { _T("System.Memory.Physical.Total"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { _T("System.Memory.Physical.Used"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { _T("System.Memory.Swap.Free"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
   { _T("System.Memory.Swap.FreePerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_FREE_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
   { _T("System.Memory.Swap.Total"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
   { _T("System.Memory.Swap.Used"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
   { _T("System.Memory.Swap.UsedPerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_USED_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
   { _T("System.Memory.Virtual.Active"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_ACTIVE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE },
   { _T("System.Memory.Virtual.ActivePerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_ACTIVE_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE_PCT },
   { _T("System.Memory.Virtual.Free"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { _T("System.Memory.Virtual.FreePerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { _T("System.Memory.Virtual.Used"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { _T("System.Memory.Virtual.UsedPerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED_PERC, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },

   { _T("System.MsgQueue.Bytes(*)"), H_SysMsgQueue, _T("b"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_BYTES },
   { _T("System.MsgQueue.BytesMax(*)"), H_SysMsgQueue, _T("B"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_BYTES_MAX },
   { _T("System.MsgQueue.ChangeTime(*)"), H_SysMsgQueue, _T("c"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_CHANGE_TIME },
   { _T("System.MsgQueue.Messages(*)"), H_SysMsgQueue, _T("m"), DCI_DT_UINT, DCIDESC_SYSTEM_MSGQUEUE_MESSAGES },
   { _T("System.MsgQueue.RecvTime(*)"), H_SysMsgQueue, _T("r"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_RECV_TIME },
   { _T("System.MsgQueue.SendTime(*)"), H_SysMsgQueue, _T("s"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_SEND_TIME },

   { _T("System.OS.ProductName"), H_OSInfo, _T("N"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_NAME },
   { _T("System.OS.ServicePack"), H_OSServicePack, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_OS_SERVICE_PACK },
   { _T("System.OS.Version"), H_OSInfo, _T("V"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_VERSION },

   { _T("System.ProcessCount"), H_SysProcessCount, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_PROCESSCOUNT },
   { _T("System.ThreadCount"), H_SysThreadCount, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_THREADCOUNT },
   { _T("System.Uname"), H_Uname, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
   { _T("System.Uptime"), H_Uptime, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};

/**
 * Subagent's lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("FileSystem.MountPoints"), H_MountPoints, nullptr },
   { _T("LVM.LogicalVolumes"), H_LvmAllLogicalVolumes, nullptr },
   { _T("LVM.LogicalVolumes(*)"), H_LvmLogicalVolumes, nullptr },
   { _T("LVM.PhysicalVolumes"), H_LvmAllPhysicalVolumes, nullptr },
   { _T("LVM.PhysicalVolumes(*)"), H_LvmPhysicalVolumes, nullptr },
   { _T("LVM.VolumeGroups"), H_LvmVolumeGroups, nullptr },
   { _T("Net.InterfaceList"), H_NetInterfaceList, nullptr },
   { _T("Net.InterfaceNames"), H_NetInterfaceNames, nullptr },
   { _T("Net.IP.RoutingTable"), H_NetRoutingTable, nullptr },
   { _T("System.Processes"), H_ProcessList, _T("2") },
   { _T("System.ProcessList"), H_ProcessList, _T("1") }
};

/**
 * Subagent's tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, nullptr, _T("MOUNTPOINT"), DCTDESC_FILESYSTEM_VOLUMES },
   { _T("LVM.LogicalVolumes(*)"), H_LvmLogicalVolumesTable, nullptr, _T("NAME"), DCTDESC_LVM_LOGICAL_VOLUMES },
   { _T("LVM.PhysicalVolumes(*)"), H_LvmPhysicalVolumesTable, nullptr, _T("NAME"), DCTDESC_LVM_PHYSICAL_VOLUMES },
   { _T("LVM.VolumeGroups"), H_LvmVolumeGroupsTable, nullptr, _T("NAME"), DCTDESC_LVM_VOLUME_GROUPS },
   { _T("System.InstalledProducts"), H_InstalledProducts, nullptr, _T("NAME"), DCTDESC_SYSTEM_INSTALLED_PRODUCTS },
   { _T("System.Processes"), H_ProcessTable, nullptr, _T("PID"), DCTDESC_SYSTEM_PROCESSES}
};

/**
 * Subagent info
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("AIX"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   m_lists,
   sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   m_tables,
   0, nullptr,	// actions
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(AIX)
{
   *ppInfo = &m_info;
   return true;
}

/**
 * Entry point for server: get interface list
 */
extern "C" BOOL __EXPORT __NxSubAgentGetIfList(StringList *value)
{
   return H_NetInterfaceList(_T("Net.InterfaceList"), nullptr, value, nullptr) == SYSINFO_RC_SUCCESS;
}  

/**
 * Entry point for server: get ARP cache
 */
extern "C" BOOL __EXPORT __NxSubAgentGetArpCache(StringList *value)
{
//   return H_NetArpCache(_T("Net.ArpCache"), nullptr, value) == SYSINFO_RC_SUCCESS;
   return FALSE;
}
