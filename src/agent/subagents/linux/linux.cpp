/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2024 Raden Solutions
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
** File: linux.cpp
**
**/

#include "linux_subagent.h"
#include <netxms-version.h>

#if HAVE_SYS_REBOOT_H
#include <sys/reboot.h>
#endif

#include "../../smbios/linux.cpp"

#if HAVE_REBOOT

/**
 * Shutdown/reboot thread
 */
static void RebootThread(const char *mode)
{
   AgentWriteLog(NXLOG_INFO, _T("Reboot thread started - system %s in 2 seconds"), *mode == 'R' ? _T("restart") : _T("shutdown"));
   ThreadSleep(2); // give time for sending response to server
   sync();
   if (*mode == 'R')
   {
#if HAVE_DECL_RB_AUTOBOOT
      reboot(RB_AUTOBOOT);
#endif
   }
   else
   {
#if HAVE_DECL_RB_POWER_OFF
      reboot(RB_POWER_OFF);
#elif HAVE_DECL_RB_HALT_SYSTEM
      reboot(RB_HALT_SYSTEM);
#endif
   }
}

#endif

/**
 * Handler for hard shutdown/restart actions
 */
static uint32_t H_HardShutdown(const shared_ptr<ActionExecutionContext>& context)
{
#if HAVE_REBOOT
   TCHAR mode = *context->getData<TCHAR>();
   if (mode == _T('R'))
   {
#if HAVE_DECL_RB_AUTOBOOT
      ThreadCreate(RebootThread, "R");
      return ERR_SUCCESS;
#else
      return ERR_NOT_IMPLEMENTED;
#endif
   }
   else
   {
#if HAVE_DECL_RB_POWER_OFF || HAVE_DECL_RB_HALT_SYSTEM
      ThreadCreate(RebootThread, "S");
      return ERR_SUCCESS;
#else
      return ERR_NOT_IMPLEMENTED;
#endif
   }
#else
   return ERR_NOT_IMPLEMENTED;
#endif
}

/**
 * Handler for soft shutdown/restart actions
 */
static uint32_t H_SoftShutdown(const shared_ptr<ActionExecutionContext>& context)
{
   TCHAR mode = *context->getData<TCHAR>();
   char cmd[128];
   snprintf(cmd, 128, "shutdown %s now", (mode == _T('R')) ? "-r" : "-h");
   return (system(cmd) >= 0) ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
}

/**
 * Initialization callback
 */
static bool SubAgentInit(Config *config)
{
   ReadCPUVendorId();
   SMBIOS_Parse(SMBIOS_Reader);
   StartCpuUsageCollector();
   StartIoStatCollector();
   InitDrbdCollector();
   return true;
}

/**
 * Shutdown callback
 */
static void SubAgentShutdown()
{
   ShutdownCpuUsageCollector();
   ShutdownIoStatCollector();
   StopDrbdCollector();
}

/**
 * Externals
 */
LONG H_DRBDDeviceList(const TCHAR *pszParam, const TCHAR *pszArg, StringList *pValue, AbstractCommSession *session);
LONG H_DRBDDeviceInfo(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_DRBDVersion(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_HandleCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_OpenFilesTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);

/**
 * Parameters provided by subagent
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] = 
{
   { _T("Agent.SourcePackageSupport"), H_SourcePkgSupport, nullptr, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

   { _T("Disk.Avail(*)"), H_FileSystemInfo, (TCHAR *)DISK_AVAIL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.AvailPerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_AVAIL_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Free(*)"), H_FileSystemInfo, (TCHAR *)DISK_FREE, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_FREE_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"), H_FileSystemInfo, (TCHAR *)DISK_TOTAL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"), H_FileSystemInfo, (TCHAR *)DISK_USED, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_USED_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },

   { _T("FileSystem.Avail(*)"), H_FileSystemInfo, (TCHAR *)DISK_AVAIL, DCI_DT_UINT64, DCIDESC_FS_AVAIL },
   { _T("FileSystem.AvailInodes(*)"), H_FileSystemInfo, (TCHAR *)DISK_AVAIL_INODES, DCI_DT_UINT64, DCIDESC_FS_AVAILINODES },
   { _T("FileSystem.AvailInodesPerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_AVAIL_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILINODESPERC },
   { _T("FileSystem.AvailPerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_AVAIL_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILPERC },
   { _T("FileSystem.Free(*)"), H_FileSystemInfo, (TCHAR *)DISK_FREE, DCI_DT_UINT64, DCIDESC_FS_FREE },
   { _T("FileSystem.FreeInodes(*)"), H_FileSystemInfo, (TCHAR *)DISK_FREE_INODES, DCI_DT_UINT64, DCIDESC_FS_FREEINODES },
   { _T("FileSystem.FreeInodesPerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_FREE_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEINODESPERC },
   { _T("FileSystem.FreePerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_FREE_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { _T("FileSystem.Total(*)"), H_FileSystemInfo, (TCHAR *)DISK_TOTAL, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { _T("FileSystem.TotalInodes(*)"), H_FileSystemInfo, (TCHAR *)DISK_TOTAL_INODES, DCI_DT_UINT64, DCIDESC_FS_TOTALINODES },
   { _T("FileSystem.Type(*)"), H_FileSystemType, nullptr, DCI_DT_STRING, DCIDESC_FS_TYPE },
   { _T("FileSystem.Used(*)"), H_FileSystemInfo, (TCHAR *)DISK_USED, DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedInodes(*)"), H_FileSystemInfo, (TCHAR *)DISK_USED_INODES, DCI_DT_UINT64, DCIDESC_FS_USEDINODES },
   { _T("FileSystem.UsedInodesPerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_USED_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDINODESPERC },
   { _T("FileSystem.UsedPerc(*)"), H_FileSystemInfo, (TCHAR *)DISK_USED_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

   { _T("DRBD.ConnState(*)"), H_DRBDDeviceInfo, _T("c"), DCI_DT_STRING, _T("Connection state of DRBD device {instance}") },
   { _T("DRBD.DataState(*)"), H_DRBDDeviceInfo, _T("d"), DCI_DT_STRING, _T("Data state of DRBD device {instance}") },
   { _T("DRBD.DeviceState(*)"), H_DRBDDeviceInfo, _T("s"), DCI_DT_STRING, _T("State of DRBD device {instance}") },
   { _T("DRBD.PeerDataState(*)"), H_DRBDDeviceInfo, _T("D"), DCI_DT_STRING, _T("Data state of DRBD peer device {instance}") },
   { _T("DRBD.PeerDeviceState(*)"), H_DRBDDeviceInfo, _T("S"), DCI_DT_STRING, _T("State of DRBD peer device {instance}") },
   { _T("DRBD.Protocol(*)"), H_DRBDDeviceInfo, _T("p"), DCI_DT_STRING, _T("Protocol type used by DRBD device {instance}") },
   { _T("DRBD.Version.API"), H_DRBDVersion, _T("a"), DCI_DT_STRING, _T("DRBD API version") },
   { _T("DRBD.Version.Driver"), H_DRBDVersion, _T("v"), DCI_DT_STRING, _T("DRBD driver version") },
   { _T("DRBD.Version.Protocol"), H_DRBDVersion, _T("p"), DCI_DT_STRING, _T("DRBD protocol version") },

   { _T("Hardware.Baseboard.Manufacturer"), SMBIOS_ParameterHandler, _T("bM"), DCI_DT_STRING, DCIDESC_HARDWARE_BASEBOARD_MANUFACTURER },
   { _T("Hardware.Baseboard.Product"), SMBIOS_ParameterHandler, _T("bP"), DCI_DT_STRING, DCIDESC_HARDWARE_BASEBOARD_PRODUCT },
   { _T("Hardware.Baseboard.SerialNumber"), SMBIOS_ParameterHandler, _T("bS"), DCI_DT_STRING, DCIDESC_HARDWARE_BASEBOARD_SERIALNUMBER },
   { _T("Hardware.Baseboard.Type"), SMBIOS_ParameterHandler, _T("bT"), DCI_DT_STRING, DCIDESC_HARDWARE_BASEBOARD_TYPE },
   { _T("Hardware.Baseboard.Version"), SMBIOS_ParameterHandler, _T("bV"), DCI_DT_STRING, DCIDESC_HARDWARE_BASEBOARD_VERSION },
   { _T("Hardware.Battery.Capacity(*)"), SMBIOS_BatteryParameterHandler, _T("c"), DCI_DT_UINT, DCIDESC_HARDWARE_BATTERY_CAPACITY },
   { _T("Hardware.Battery.Chemistry(*)"), SMBIOS_BatteryParameterHandler, _T("C"), DCI_DT_STRING, DCIDESC_HARDWARE_BATTERY_CHEMISTRY },
   { _T("Hardware.Battery.Location(*)"), SMBIOS_BatteryParameterHandler, _T("L"), DCI_DT_STRING, DCIDESC_HARDWARE_BATTERY_LOCATION },
   { _T("Hardware.Battery.ManufactureDate(*)"), SMBIOS_BatteryParameterHandler, _T("D"), DCI_DT_STRING, DCIDESC_HARDWARE_BATTERY_MANUFACTURE_DATE },
   { _T("Hardware.Battery.Manufacturer(*)"), SMBIOS_BatteryParameterHandler, _T("M"), DCI_DT_STRING, DCIDESC_HARDWARE_BATTERY_MANUFACTURER },
   { _T("Hardware.Battery.Name(*)"), SMBIOS_BatteryParameterHandler, _T("N"), DCI_DT_STRING, DCIDESC_HARDWARE_BATTERY_NAME },
   { _T("Hardware.Battery.SerialNumber(*)"), SMBIOS_BatteryParameterHandler, _T("s"), DCI_DT_STRING, DCIDESC_HARDWARE_BATTERY_SERIALNUMBER },
   { _T("Hardware.Battery.Voltage(*)"), SMBIOS_BatteryParameterHandler, _T("V"), DCI_DT_UINT, DCIDESC_HARDWARE_BATTERY_VOLTAGE },
   { _T("Hardware.MemoryDevice.Bank(*)"), SMBIOS_MemDevParameterHandler, _T("B"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_BANK },
   { _T("Hardware.MemoryDevice.ConfiguredSpeed(*)"), SMBIOS_MemDevParameterHandler, _T("c"), DCI_DT_UINT, DCIDESC_HARDWARE_MEMORYDEVICE_CONFSPEED },
   { _T("Hardware.MemoryDevice.FormFactor(*)"), SMBIOS_MemDevParameterHandler, _T("F"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_FORMFACTOR },
   { _T("Hardware.MemoryDevice.Location(*)"), SMBIOS_MemDevParameterHandler, _T("L"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_LOCATION },
   { _T("Hardware.MemoryDevice.Manufacturer(*)"), SMBIOS_MemDevParameterHandler, _T("M"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_MANUFACTURER },
   { _T("Hardware.MemoryDevice.MaxSpeed(*)"), SMBIOS_MemDevParameterHandler, _T("m"), DCI_DT_UINT, DCIDESC_HARDWARE_MEMORYDEVICE_MAXSPEED },
   { _T("Hardware.MemoryDevice.PartNumber(*)"), SMBIOS_MemDevParameterHandler, _T("P"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_PARTNUMBER },
   { _T("Hardware.MemoryDevice.SerialNumber(*)"), SMBIOS_MemDevParameterHandler, _T("s"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_SERIALNUMBER },
   { _T("Hardware.MemoryDevice.Size(*)"), SMBIOS_MemDevParameterHandler, _T("S"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_SIZE },
   { _T("Hardware.MemoryDevice.Type(*)"), SMBIOS_MemDevParameterHandler, _T("T"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_TYPE },
   { _T("Hardware.Processor.Cores(*)"), SMBIOS_ProcessorParameterHandler, _T("C"), DCI_DT_UINT, DCIDESC_HARDWARE_PROCESSOR_CORES },
   { _T("Hardware.Processor.CurrentSpeed(*)"), SMBIOS_ProcessorParameterHandler, _T("c"), DCI_DT_UINT, DCIDESC_HARDWARE_PROCESSOR_CURRSPEED },
   { _T("Hardware.Processor.Family(*)"), SMBIOS_ProcessorParameterHandler, _T("F"), DCI_DT_STRING, DCIDESC_HARDWARE_PROCESSOR_FAMILY },
   { _T("Hardware.Processor.Manufacturer(*)"), SMBIOS_ProcessorParameterHandler, _T("M"), DCI_DT_STRING, DCIDESC_HARDWARE_PROCESSOR_MANUFACTURER },
   { _T("Hardware.Processor.MaxSpeed(*)"), SMBIOS_ProcessorParameterHandler, _T("m"), DCI_DT_UINT, DCIDESC_HARDWARE_PROCESSOR_MAXSPEED },
   { _T("Hardware.Processor.PartNumber(*)"), SMBIOS_ProcessorParameterHandler, _T("P"), DCI_DT_STRING, DCIDESC_HARDWARE_PROCESSOR_PARTNUMBER },
   { _T("Hardware.Processor.SerialNumber(*)"), SMBIOS_ProcessorParameterHandler, _T("s"), DCI_DT_STRING, DCIDESC_HARDWARE_PROCESSOR_SERIALNUMBER },
   { _T("Hardware.Processor.Socket(*)"), SMBIOS_ProcessorParameterHandler, _T("S"), DCI_DT_STRING, DCIDESC_HARDWARE_PROCESSOR_SOCKET },
   { _T("Hardware.Processor.Threads(*)"), SMBIOS_ProcessorParameterHandler, _T("t"), DCI_DT_UINT, DCIDESC_HARDWARE_PROCESSOR_THREADS },
   { _T("Hardware.Processor.Type(*)"), SMBIOS_ProcessorParameterHandler, _T("T"), DCI_DT_STRING, DCIDESC_HARDWARE_PROCESSOR_TYPE },
   { _T("Hardware.Processor.Version(*)"), SMBIOS_ProcessorParameterHandler, _T("V"), DCI_DT_STRING, DCIDESC_HARDWARE_PROCESSOR_VERSION },
   { _T("Hardware.System.MachineId"), H_HardwareSystemInfo, _T("HU"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MACHINEID },
   { _T("Hardware.System.Manufacturer"), H_HardwareSystemInfo, _T("HM"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MANUFACTURER },
   { _T("Hardware.System.Product"), H_HardwareSystemInfo, _T("HP"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCT },
   { _T("Hardware.System.ProductCode"), H_HardwareSystemInfo, _T("HC"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCTCODE },
   { _T("Hardware.System.Version"), H_HardwareSystemInfo, _T("HV"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_VERSION },
   { _T("Hardware.WakeUpEvent"), SMBIOS_ParameterHandler, _T("W"), DCI_DT_STRING, DCIDESC_HARDWARE_WAKEUPEVENT },

   { _T("Hypervisor.Type"), H_HypervisorType, nullptr, DCI_DT_STRING, DCIDESC_HYPERVISOR_TYPE },
   { _T("Hypervisor.Version"), H_HypervisorVersion, nullptr, DCI_DT_STRING, DCIDESC_HYPERVISOR_VERSION },

   { _T("Net.Interface.AdminStatus(*)"), H_NetIfInfoFromIOCTL, (TCHAR *)IF_INFO_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_BYTES_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_BYTES_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.Description(*)"), H_NetIfInfoFromIOCTL, (TCHAR *)IF_INFO_DESCRIPTION, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { _T("Net.Interface.InErrors(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_ERRORS_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetIfInfoFromIOCTL, (TCHAR *)IF_INFO_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_LINK },
   { _T("Net.Interface.OutErrors(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_ERRORS_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_PACKETS_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_PACKETS_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetIfInfoSpeed, nullptr, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_SPEED },
   { _T("Net.IP.Forwarding"), H_NetIpForwarding, (TCHAR*)4, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },
   { _T("Net.IP6.Forwarding"), H_NetIpForwarding, (TCHAR*)6, DCI_DT_INT, DCIDESC_NET_IP6_FORWARDING },

#if SIZEOF_LONG > 4
   { _T("Net.Interface.BytesIn64(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_BYTES_IN_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut64(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_BYTES_OUT_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.InErrors64(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_ERRORS_IN_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.OutErrors64(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_ERRORS_OUT_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn64(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_PACKETS_IN_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut64(*)"), H_NetIfInfoFromProc, (TCHAR *)IF_INFO_PACKETS_OUT_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSOUT },
#endif

   { _T("Process.Count(*)"), H_ProcessCount, _T("S"), DCI_DT_INT, DCIDESC_PROCESS_COUNT },
   { _T("Process.CountEx(*)"), H_ProcessCount, _T("E"), DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
   { _T("Process.CPUTime(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_CPUTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_CPUTIME },
   { _T("Process.Handles(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_HANDLES, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_HANDLES },
   { _T("Process.KernelTime(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_KTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_KERNELTIME },
   { _T("Process.MemoryUsage(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_MEMPERC, const TCHAR *), DCI_DT_FLOAT, DCIDESC_PROCESS_MEMORYUSAGE },
   { _T("Process.PageFaults(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_PAGEFAULTS, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_PAGEFAULTS },
   { _T("Process.RSS(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },
   { _T("Process.Threads(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_THREADS, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_THREADS },
   { _T("Process.UserTime(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_UTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_USERTIME },
   { _T("Process.VMRegions(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_VMREGIONS, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_VMREGIONS },
   { _T("Process.VMSize(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_VMSIZE, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_VMSIZE },
   { _T("Process.WkSet(*)"), H_ProcessDetails, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },

   { _T("System.BIOS.Date"), SMBIOS_ParameterHandler, _T("BD"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_DATE },
   { _T("System.BIOS.Vendor"), SMBIOS_ParameterHandler, _T("Bv"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VENDOR },
   { _T("System.BIOS.Version"), SMBIOS_ParameterHandler, _T("BV"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VERSION },
   { _T("System.ConnectedUsers"), H_ConnectedUsers, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_CONNECTEDUSERS },

   { _T("System.CPU.CacheSize(*)"), H_CpuInfo, _T("S"), DCI_DT_INT, DCIDESC_SYSTEM_CPU_CACHE_SIZE },
   { _T("System.CPU.ContextSwitches"), H_CpuCswitch, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_CONTEXT_SWITCHES), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_CONTEXT_SWITCHES },
   { _T("System.CPU.CoreId(*)"), H_CpuInfo, _T("C"), DCI_DT_INT, DCIDESC_SYSTEM_CPU_CORE_ID },
   { _T("System.CPU.Count"), H_CpuCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { _T("System.CPU.Frequency(*)"), H_CpuInfo, _T("F"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_FREQUENCY },
   { _T("System.CPU.Interrupts"), H_CpuInterrupts, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_INTERRUPTS), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_INTERRUPTS },
   { _T("System.CPU.LoadAvg"), H_CpuLoad, (TCHAR *)INTERVAL_1MIN, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
   { _T("System.CPU.LoadAvg5"), H_CpuLoad, (TCHAR *)INTERVAL_5MIN, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { _T("System.CPU.LoadAvg15"), H_CpuLoad, (TCHAR *)INTERVAL_15MIN, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
   { _T("System.CPU.Model(*)"), H_CpuInfo, _T("M"), DCI_DT_STRING, DCIDESC_SYSTEM_CPU_MODEL },
   { _T("System.CPU.PhysicalId(*)"), H_CpuInfo, _T("P"), DCI_DT_INT, DCIDESC_SYSTEM_CPU_PHYSICAL_ID },

   /* usage */
   { _T("System.CPU.Usage"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
   { _T("System.CPU.Usage5"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
   { _T("System.CPU.Usage15"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
   { _T("System.CPU.Usage(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
   { _T("System.CPU.Usage5(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
   { _T("System.CPU.Usage15(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },

   /* user */
   { _T("System.CPU.Usage.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER },
   { _T("System.CPU.Usage5.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER },
   { _T("System.CPU.Usage15.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER },
   { _T("System.CPU.Usage.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
   { _T("System.CPU.Usage5.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
   { _T("System.CPU.Usage15.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

   /* nice */
   { _T("System.CPU.Usage.Nice"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_NICE },
   { _T("System.CPU.Usage5.Nice"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_NICE },
   { _T("System.CPU.Usage15.Nice"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_NICE },
   { _T("System.CPU.Usage.Nice(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_NICE_EX },
   { _T("System.CPU.Usage5.Nice(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX },
   { _T("System.CPU.Usage15.Nice(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX },

   /* system */
   { _T("System.CPU.Usage.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
   { _T("System.CPU.Usage5.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
   { _T("System.CPU.Usage15.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
   { _T("System.CPU.Usage.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
   { _T("System.CPU.Usage5.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
   { _T("System.CPU.Usage15.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

   /* idle */
   { _T("System.CPU.Usage.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE },
   { _T("System.CPU.Usage5.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
   { _T("System.CPU.Usage15.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
   { _T("System.CPU.Usage.Idle(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
   { _T("System.CPU.Usage5.Idle(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
   { _T("System.CPU.Usage5.Idle5(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
   { _T("System.CPU.Usage15.Idle(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

   /* iowait */
   { _T("System.CPU.Usage.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
   { _T("System.CPU.Usage5.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
   { _T("System.CPU.Usage15.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
   { _T("System.CPU.Usage.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
   { _T("System.CPU.Usage5.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
   { _T("System.CPU.Usage15.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

   /* irq */
   { _T("System.CPU.Usage.Irq"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IRQ },
   { _T("System.CPU.Usage5.Irq"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IRQ },
   { _T("System.CPU.Usage15.Irq"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IRQ },
   { _T("System.CPU.Usage.Irq(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX },
   { _T("System.CPU.Usage5.Irq(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX },
   { _T("System.CPU.Usage15.Irq(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX },

   /* softirq */
   { _T("System.CPU.Usage.SoftIrq"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ },
   { _T("System.CPU.Usage5.SoftIrq"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ },
   { _T("System.CPU.Usage15.SoftIrq"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ },
   { _T("System.CPU.Usage.SoftIrq(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ_EX },
   { _T("System.CPU.Usage5.SoftIrq(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ_EX },
   { _T("System.CPU.Usage15.SoftIrq(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ_EX },

   /* steal */
   { _T("System.CPU.Usage.Steal"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_STEAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_STEAL },
   { _T("System.CPU.Usage5.Steal"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_STEAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_STEAL },
   { _T("System.CPU.Usage15.Steal"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_STEAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_STEAL },
   { _T("System.CPU.Usage.Steal(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_STEAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_STEAL_EX },
   { _T("System.CPU.Usage5.Steal(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_STEAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_STEAL_EX },
   { _T("System.CPU.Usage15.Steal(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_STEAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_STEAL_EX },

   /* Guest */
   { _T("System.CPU.Usage.Guest"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_GUEST), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_GUEST },
   { _T("System.CPU.Usage5.Guest"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_GUEST), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_GUEST },
   { _T("System.CPU.Usage15.Guest"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_GUEST), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_GUEST },
   { _T("System.CPU.Usage.Guest(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_GUEST), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_GUEST_EX },
   { _T("System.CPU.Usage5.Guest(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_GUEST), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_GUEST_EX },
   { _T("System.CPU.Usage15.Guest(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_GUEST), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_GUEST_EX },

   { _T("System.CPU.VendorId"), H_CpuVendorId, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_CPU_VENDORID },

   { _T("System.HandleCount"), H_HandleCount, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_HANDLECOUNT },

   /* iostat */
   { _T("System.IO.ReadRate"), H_IoStatsTotalFloat, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
   { _T("System.IO.ReadRate(*)"), H_IoStatsCumulative, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
   { _T("System.IO.ReadWaitTime"), H_IoStatsTotalNonCumulativeInteger, (const TCHAR *)IOSTAT_READ_WAIT_TIME, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READWAITTIME },
   { _T("System.IO.ReadWaitTime(*)"), H_IoStatsNonCumulative, (const TCHAR *)IOSTAT_READ_WAIT_TIME, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READWAITTIME_EX },
   { _T("System.IO.WaitTime"), H_IoStatsTotalNonCumulativeInteger, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WAITTIME },
   { _T("System.IO.WaitTime(*)"), H_IoStatsNonCumulative, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WAITTIME_EX },
   { _T("System.IO.WriteRate"), H_IoStatsTotalFloat, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
   { _T("System.IO.WriteRate(*)"), H_IoStatsCumulative, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },
   { _T("System.IO.WriteWaitTime"), H_IoStatsTotalNonCumulativeInteger, (const TCHAR *)IOSTAT_WRITE_WAIT_TIME, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITEWAITTIME },
   { _T("System.IO.WriteWaitTime(*)"), H_IoStatsNonCumulative, (const TCHAR *)IOSTAT_WRITE_WAIT_TIME, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITEWAITTIME_EX },
   { _T("System.IO.BytesReadRate"), H_IoStatsTotalSectors, (const TCHAR *)IOSTAT_NUM_SREADS, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
   { _T("System.IO.BytesReadRate(*)"), H_IoStatsCumulative, (const TCHAR *)IOSTAT_NUM_SREADS, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
   { _T("System.IO.BytesWriteRate"), H_IoStatsTotalSectors, (const TCHAR *)IOSTAT_NUM_SWRITES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
   { _T("System.IO.BytesWriteRate(*)"), H_IoStatsCumulative, (const TCHAR *)IOSTAT_NUM_SWRITES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
   { _T("System.IO.DiskQueue(*)"), H_IoStatsNonCumulative, (const TCHAR *)IOSTAT_DISK_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
   { _T("System.IO.DiskQueue"), H_IoStatsTotalNonCumulativeFloat, (const TCHAR *)IOSTAT_DISK_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
   { _T("System.IO.DiskTime"), H_IoStatsTotalTimePct, (const TCHAR *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME },
   { _T("System.IO.DiskTime(*)"), H_IoStatsCumulative, (const TCHAR *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME_EX },

   { _T("System.IsVirtual"), H_IsVirtual, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_IS_VIRTUAL },

   { _T("System.Memory.Physical.Available"), H_MemoryInfo, (TCHAR *)PHYSICAL_AVAILABLE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE },
   { _T("System.Memory.Physical.AvailablePerc"), H_MemoryInfo, (TCHAR *)PHYSICAL_AVAILABLE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT },
   { _T("System.Memory.Physical.Buffers"), H_MemoryInfo, (TCHAR *)PHYSICAL_BUFFERS, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_BUFFERS },
   { _T("System.Memory.Physical.BuffersPerc"), H_MemoryInfo, (TCHAR *)PHYSICAL_BUFFERS_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_BUFFERS_PCT },
   { _T("System.Memory.Physical.Cached"), H_MemoryInfo, (TCHAR *)PHYSICAL_CACHED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED },
   { _T("System.Memory.Physical.CachedPerc"), H_MemoryInfo, (TCHAR *)PHYSICAL_CACHED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED_PCT },
   { _T("System.Memory.Physical.Free"), H_MemoryInfo, (TCHAR *)PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { _T("System.Memory.Physical.FreePerc"), H_MemoryInfo, (TCHAR *)PHYSICAL_FREE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { _T("System.Memory.Physical.Total"), H_MemoryInfo, (TCHAR *)PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { _T("System.Memory.Physical.Used"), H_MemoryInfo, (TCHAR *)PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (TCHAR *)PHYSICAL_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { _T("System.Memory.Swap.Free"), H_MemoryInfo, (TCHAR *)SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
   { _T("System.Memory.Swap.FreePerc"), H_MemoryInfo, (TCHAR *)SWAP_FREE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
   { _T("System.Memory.Swap.Total"), H_MemoryInfo, (TCHAR *)SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
   { _T("System.Memory.Swap.Used"), H_MemoryInfo, (TCHAR *)SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
   { _T("System.Memory.Swap.UsedPerc"), H_MemoryInfo, (TCHAR *)SWAP_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
   { _T("System.Memory.Virtual.Free"), H_MemoryInfo, (TCHAR *)VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo, (TCHAR *)VIRTUAL_FREE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_MemoryInfo, (TCHAR *)VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { _T("System.Memory.Virtual.Used"), H_MemoryInfo, (TCHAR *)VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo, (TCHAR *)VIRTUAL_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
   { _T("System.Memory.Virtual.Available"), H_MemoryInfo, (TCHAR *)VIRTUAL_AVAILABLE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE },
   { _T("System.Memory.Virtual.AvailablePerc"), H_MemoryInfo, (TCHAR *)VIRTUAL_AVAILABLE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE_PCT },

   { _T("System.MsgQueue.Bytes(*)"), H_SysMsgQueue, _T("b"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_BYTES },
   { _T("System.MsgQueue.BytesMax(*)"), H_SysMsgQueue, _T("B"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_BYTES_MAX },
   { _T("System.MsgQueue.ChangeTime(*)"), H_SysMsgQueue, _T("c"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_CHANGE_TIME },
   { _T("System.MsgQueue.Messages(*)"), H_SysMsgQueue, _T("m"), DCI_DT_UINT, DCIDESC_SYSTEM_MSGQUEUE_MESSAGES },
   { _T("System.MsgQueue.RecvTime(*)"), H_SysMsgQueue, _T("r"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_RECV_TIME },
   { _T("System.MsgQueue.SendTime(*)"), H_SysMsgQueue, _T("s"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_SEND_TIME },

   { _T("System.OS.Build"), H_OSInfo, _T("B"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_BUILD },
   { _T("System.OS.ProductName"), H_OSInfo, _T("N"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_NAME },
   { _T("System.OS.ProductType"), H_OSInfo, _T("T"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_TYPE },
   { _T("System.OS.Version"), H_OSInfo, _T("V"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_VERSION },

   { _T("System.ProcessCount"), H_SystemProcessCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_PROCESSCOUNT },
   { _T("System.ThreadCount"), H_ThreadCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_THREADCOUNT },

   { _T("System.Uname"), H_Uname, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
   { _T("System.Uptime"), H_Uptime, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};

/**
 * Subagent's lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("DRBD.DeviceList"), H_DRBDDeviceList, nullptr },
   { _T("FileSystem.MountPoints"), H_MountPoints, nullptr },
   { _T("Hardware.Batteries"), SMBIOS_ListHandler, _T("B") },
   { _T("Hardware.MemoryDevices"), SMBIOS_ListHandler, _T("M") },
   { _T("Hardware.Processors"), SMBIOS_ListHandler, _T("P") },
   { _T("Net.ArpCache"), H_NetArpCache, nullptr },
   { _T("Net.IP.Neighbors"), H_NetIpNeighborsList, nullptr },
   { _T("Net.IP.RoutingTable"), H_NetRoutingTable, nullptr },
   { _T("Net.InterfaceList"), H_NetIfList, nullptr },
   { _T("Net.InterfaceNames"), H_NetIfNames, nullptr },
   { _T("System.ActiveUserSessions"), H_UserSessionList, nullptr },
   { _T("System.IO.Devices"), H_IoDevices, nullptr },
   { _T("System.IO.LogicalDevices"), H_IoLogicalDevices, nullptr },
   { _T("System.ProcessList"), H_ProcessList, nullptr }
};

/**
 * Subagent's tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, nullptr, _T("MOUNTPOINT"), DCTDESC_FILESYSTEM_VOLUMES },
   { _T("Hardware.Batteries"), SMBIOS_TableHandler, _T("B"), _T("HANDLE"), DCTDESC_HARDWARE_BATTERIES },
   { _T("Hardware.MemoryDevices"), SMBIOS_TableHandler, _T("M"), _T("HANDLE"), DCTDESC_HARDWARE_MEMORY_DEVICES },
   { _T("Hardware.NetworkAdapters"), H_NetworkAdaptersTable, nullptr, _T("INDEX"), DCTDESC_HARDWARE_NETWORK_ADAPTERS },
   { _T("Hardware.Processors"), SMBIOS_TableHandler, _T("P"), _T("HANDLE"), DCTDESC_HARDWARE_PROCESSORS },
   { _T("Hardware.StorageDevices"), H_StorageDeviceTable, nullptr, _T("NUMBER"), DCTDESC_HARDWARE_STORAGE_DEVICES },
   { _T("Net.Interfaces"), H_NetIfTable, nullptr, _T("INDEX"), DCTDESC_NETWORK_INTERFACES },
   { _T("Net.IP.Neighbors"), H_NetIpNeighborsTable, nullptr, _T("IP_ADDRESS"), DCTDESC_NET_IP_NEIGHBORS },
   { _T("System.ActiveUserSessions"), H_UserSessionTable, nullptr, _T("ID"), DCTDESC_SYSTEM_ACTIVE_USER_SESSIONS },
   { _T("System.InstalledProducts"), H_InstalledProducts, nullptr, _T("NAME"), DCTDESC_SYSTEM_INSTALLED_PRODUCTS },
   { _T("System.OpenFiles"), H_OpenFilesTable, nullptr, _T("PID,HANDLE"), DCTDESC_SYSTEM_OPEN_FILES },
   { _T("System.Processes"), H_ProcessTable, nullptr, _T("PID"), DCTDESC_SYSTEM_PROCESSES }
};

/**
 * Subagent's actions
 */
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
   { _T("System.HardRestart"), H_HardShutdown, _T("R"), _T("Restart system (hard reset)") },
   { _T("System.HardShutdown"), H_HardShutdown, _T("S"), _T("Shutdown system (hard shutdown/power off)") },
   { _T("System.Restart"), H_SoftShutdown, _T("R"), _T("Restart system") },
   { _T("System.Shutdown"), H_SoftShutdown, _T("S"), _T("Shutdown system") }
};

/**
 * Subagent info
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("Linux"), NETXMS_VERSION_STRING,
   SubAgentInit,     /* initialization handler */
   SubAgentShutdown, /* unload handler */
   nullptr,          /* command handler */
   nullptr,          /* notification handler */
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   m_lists,
   sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   m_tables,
   sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
   m_actions,
   0, nullptr // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(LINUX)
{
   *ppInfo = &s_info;
   return true;
}

/**
 * Entry point for server: interface list
 */
extern "C" BOOL __EXPORT __NxSubAgentGetIfList(StringList *value)
{
   return H_NetIfList(_T("Net.InterfaceList"), nullptr, value, nullptr) == SYSINFO_RC_SUCCESS;
}

/**
 * Entry point for server: arp cache
 */
extern "C" BOOL __EXPORT __NxSubAgentGetArpCache(StringList *value)
{
   return H_NetArpCache(_T("Net.ArpCache"), nullptr, value, nullptr) == SYSINFO_RC_SUCCESS;
}
