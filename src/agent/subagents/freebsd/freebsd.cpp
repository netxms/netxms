/* 
** NetXMS subagent for FreeBSD
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
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <netxms-version.h>
#include "freebsd_subagent.h"

#include "../../smbios/freebsd.cpp"

/**
 * Execute sysctl on named parameter
 */
int ExecSysctl(const char *param, void *buffer, size_t buffSize)
{
   int ret = SYSINFO_RC_ERROR;
   int mib[2];
   size_t nSize = sizeof(mib);

   if (sysctlnametomib(param, mib, &nSize) == 0)
   {
      if (sysctl(mib, nSize, buffer, &buffSize, NULL, 0) == 0)
      {
         ret = SYSINFO_RC_SUCCESS;
      }
      else
      {
         nxlog_debug_tag(SUBAGENT_DEBUG_TAG, 5, _T("sysctl(%hs) failed (%s)"), param, _tcserror(errno));
      }
   }
   else
   {
      nxlog_debug_tag(SUBAGENT_DEBUG_TAG, 5, _T("sysctlnametomib(%hs) failed (%s)"), param, _tcserror(errno));
   }

   return ret;
}

/**
 * Initalization callback
 */
static bool SubAgentInit(Config *config)
{
   ReadCPUVendorId();
   SMBIOS_Parse(SMBIOS_Reader);
   StartCpuUsageCollector();
   nxlog_debug_tag(SUBAGENT_DEBUG_TAG, 1, _T("FreeBSD platform subagent initialized"));
   return true;
}

/**
 * Shutdown callback
 */
static void SubAgentShutdown()
{
   ShutdownCpuUsageCollector();
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Agent.SourcePackageSupport"),   H_SourcePkgSupport,NULL,				DCI_DT_INT,	DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

   { _T("Disk.Avail(*)"),                H_DiskInfo, (const TCHAR *)DISK_AVAIL,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { _T("Disk.AvailPerc(*)"),            H_DiskInfo, (const TCHAR *)DISK_AVAIL_PERC,	DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { _T("Disk.Free(*)"),                 H_DiskInfo, (const TCHAR *)DISK_FREE,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"),             H_DiskInfo, (const TCHAR *)DISK_FREE_PERC,	DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"),                H_DiskInfo, (const TCHAR *)DISK_TOTAL,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"),                 H_DiskInfo, (const TCHAR *)DISK_USED,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"),             H_DiskInfo, (const TCHAR *)DISK_USED_PERC,	DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },

   { _T("FileSystem.Avail(*)"),           H_DiskInfo, (const TCHAR *)DISK_AVAIL,             DCI_DT_UINT64,	DCIDESC_FS_AVAIL },
   { _T("FileSystem.AvailInodes(*)"),     H_DiskInfo, (const TCHAR *)DISK_AVAIL_INODES,      DCI_DT_UINT64,	DCIDESC_FS_AVAILINODES },
   { _T("FileSystem.AvailInodesPerc(*)"), H_DiskInfo, (const TCHAR *)DISK_AVAIL_INODES_PERC, DCI_DT_FLOAT,	DCIDESC_FS_AVAILINODESPERC },
   { _T("FileSystem.AvailPerc(*)"),       H_DiskInfo, (const TCHAR *)DISK_AVAIL_PERC,        DCI_DT_FLOAT,	DCIDESC_FS_AVAILPERC },
   { _T("FileSystem.Free(*)"),            H_DiskInfo, (const TCHAR *)DISK_FREE,              DCI_DT_UINT64,	DCIDESC_FS_FREE },
   { _T("FileSystem.FreeInodes(*)"),      H_DiskInfo, (const TCHAR *)DISK_FREE_INODES,       DCI_DT_UINT64,	DCIDESC_FS_FREEINODES },
   { _T("FileSystem.FreePerc(*)"),        H_DiskInfo, (const TCHAR *)DISK_FREE_PERC,         DCI_DT_FLOAT,	DCIDESC_FS_FREEPERC },
   { _T("FileSystem.FreeInodesPerc(*)"),  H_DiskInfo, (const TCHAR *)DISK_FREE_INODES_PERC,  DCI_DT_FLOAT,	DCIDESC_FS_FREEINODESPERC },
   { _T("FileSystem.Total(*)"),           H_DiskInfo, (const TCHAR *)DISK_TOTAL,             DCI_DT_UINT64,	DCIDESC_FS_TOTAL },
   { _T("FileSystem.TotalInodes(*)"),     H_DiskInfo, (const TCHAR *)DISK_TOTAL_INODES,      DCI_DT_UINT64,	DCIDESC_FS_TOTALINODES },
   { _T("FileSystem.Used(*)"),            H_DiskInfo, (const TCHAR *)DISK_USED,              DCI_DT_UINT64,	DCIDESC_FS_USED },
   { _T("FileSystem.UsedInodes(*)"),      H_DiskInfo, (const TCHAR *)DISK_USED_INODES,       DCI_DT_UINT64,	DCIDESC_FS_USEDINODES },
   { _T("FileSystem.UsedInodesPerc(*)"),  H_DiskInfo, (const TCHAR *)DISK_USED_INODES_PERC,  DCI_DT_FLOAT,	DCIDESC_FS_USEDINODESPERC },

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
   { _T("Hardware.System.MachineId"), SMBIOS_ParameterHandler, _T("HU"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MACHINEID },
   { _T("Hardware.System.Manufacturer"), SMBIOS_ParameterHandler, _T("HM"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MANUFACTURER },
   { _T("Hardware.System.Product"), SMBIOS_ParameterHandler, _T("HP"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCT },
   { _T("Hardware.System.ProductCode"), SMBIOS_ParameterHandler, _T("HC"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCTCODE },
   { _T("Hardware.System.Version"), SMBIOS_ParameterHandler, _T("HV"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_VERSION },
   { _T("Hardware.WakeUpEvent"), SMBIOS_ParameterHandler, _T("W"), DCI_DT_STRING, DCIDESC_HARDWARE_WAKEUPEVENT },

   { _T("Hypervisor.Type"), H_HypervisorType, NULL, DCI_DT_STRING, DCIDESC_HYPERVISOR_TYPE },
   { _T("Hypervisor.Version"), H_HypervisorVersion, NULL, DCI_DT_STRING, DCIDESC_HYPERVISOR_VERSION },

   { _T("Net.Interface.64BitCounters"), H_NetInterface64bitSupport, nullptr, DCI_DT_INT, DCIDESC_NET_INTERFACE_64BITCOUNTERS },

   { _T("Net.Interface.AdminStatus(*)"), H_NetIfAdminStatus, nullptr, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_BYTES_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_BYTES_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.InErrors(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_IN_ERRORS, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetIfOperStatus, nullptr, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.OperStatus(*)"), H_NetIfOperStatus, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.Interface.OutErrors(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_OUT_ERRORS, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_PACKETS_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_PACKETS_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetIfInfoSpeed, nullptr, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_SPEED },

#if __FreeBSD__ >= 10
   { _T("Net.Interface.BytesIn64(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_BYTES_IN_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut64(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_BYTES_OUT_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.InErrors64(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_IN_ERRORS_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.OutErrors64(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_OUT_ERRORS_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn64(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_PACKETS_IN_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut64(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_PACKETS_OUT_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSOUT },
#endif	

   { _T("Net.IP.Forwarding"), H_NetIpForwarding, (const TCHAR *)4, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },
   { _T("Net.IP6.Forwarding"), H_NetIpForwarding, (const TCHAR *)6, DCI_DT_INT, DCIDESC_NET_IP6_FORWARDING },

   { _T("Process.Count(*)"), H_ProcessCount, _T("P"), DCI_DT_INT, DCIDESC_PROCESS_COUNT },
   { _T("Process.CountEx(*)"), H_ProcessCount, _T("E"), DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
   { _T("Process.CPUTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_CPUTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_CPUTIME },
   { _T("Process.MemoryUsage(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_MEMPERC, const TCHAR *), DCI_DT_FLOAT, DCIDESC_PROCESS_MEMORYUSAGE },
   { _T("Process.RSS(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },
   { _T("Process.Threads(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_THREADS, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_THREADS },
   { _T("Process.VMSize(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_VMSIZE, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_VMSIZE },
   { _T("Process.WkSet(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },

   { _T("System.BIOS.Date"), SMBIOS_ParameterHandler, _T("BD"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_DATE },
   { _T("System.BIOS.Vendor"), SMBIOS_ParameterHandler, _T("Bv"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VENDOR },
   { _T("System.BIOS.Version"), SMBIOS_ParameterHandler, _T("BV"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VERSION },

	{ _T("System.CPU.Count"), H_CpuInfo, _T("C"), DCI_DT_INT, DCIDESC_SYSTEM_CPU_COUNT },
	{ _T("System.CPU.Frequency"), H_CpuInfo, _T("F"), DCI_DT_INT, DCIDESC_SYSTEM_CPU_FREQUENCY },
	{ _T("System.CPU.LoadAvg"), H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
	{ _T("System.CPU.LoadAvg5"), H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ _T("System.CPU.LoadAvg15"), H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
	{ _T("System.CPU.Model"), H_CpuInfo, _T("M"), DCI_DT_STRING, DCIDESC_SYSTEM_CPU_MODEL },

   { _T("System.CPU.Interrupts"), H_CpuInterrupts, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_INTERRUPTS), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_INTERRUPTS },
   { _T("System.CPU.ContextSwitches"), H_CpuCswitch, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_CONTEXT_SWITCHES), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_CONTEXT_SWITCHES },

	/* usage */
	{ _T("System.CPU.Usage"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE },
	{ _T("System.CPU.Usage5"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5 },
	{ _T("System.CPU.Usage15"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15 },
	{ _T("System.CPU.Usage(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ _T("System.CPU.Usage5(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ _T("System.CPU.Usage15(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_EX },

	/* user */
	{ _T("System.CPU.Usage.User"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER },
	{ _T("System.CPU.Usage5.User"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER },
	{ _T("System.CPU.Usage15.User"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER },
	{ _T("System.CPU.Usage.User(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
	{ _T("System.CPU.Usage5.User(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
	{ _T("System.CPU.Usage15.User(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

	/* nice */
	{ _T("System.CPU.Usage.Nice"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE },
	{ _T("System.CPU.Usage5.Nice"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE },
	{ _T("System.CPU.Usage15.Nice"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE },
	{ _T("System.CPU.Usage.Nice(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE_EX },
	{ _T("System.CPU.Usage5.Nice(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX },
	{ _T("System.CPU.Usage15.Nice(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX },

	/* system */
	{ _T("System.CPU.Usage.System"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
	{ _T("System.CPU.Usage5.System"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
	{ _T("System.CPU.Usage15.System"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
	{ _T("System.CPU.Usage.System(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
	{ _T("System.CPU.Usage5.System(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
	{ _T("System.CPU.Usage15.System(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

	/* idle */
	{ _T("System.CPU.Usage.Idle"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE },
	{ _T("System.CPU.Usage5.Idle"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
	{ _T("System.CPU.Usage15.Idle"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
	{ _T("System.CPU.Usage.Idle(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
	{ _T("System.CPU.Usage5.Idle(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ _T("System.CPU.Usage5.Idle5(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ _T("System.CPU.Usage15.Idle(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

   { _T("System.CPU.VendorId"), H_CpuVendorId, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_CPU_VENDORID },

   { _T("System.IsVirtual"), H_IsVirtual, NULL, DCI_DT_INT, DCIDESC_SYSTEM_IS_VIRTUAL },

   { _T("System.Memory.Physical.Available"),  H_MemoryInfo, (const TCHAR *)PHYSICAL_AVAIL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE },
   { _T("System.Memory.Physical.AvailablePerc"), H_MemoryInfo, (const TCHAR *)PHYSICAL_AVAIL_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT },
   { _T("System.Memory.Physical.Cached"),  H_MemoryInfo, (const TCHAR *)PHYSICAL_CACHED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED },
   { _T("System.Memory.Physical.CachedPerc"), H_MemoryInfo, (const TCHAR *)PHYSICAL_CACHED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED_PCT },
	{ _T("System.Memory.Physical.Free"),  H_MemoryInfo,      (const TCHAR *)PHYSICAL_FREE,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ _T("System.Memory.Physical.FreePerc"), H_MemoryInfo,   (const TCHAR *)PHYSICAL_FREE_PCT,	DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
	{ _T("System.Memory.Physical.Total"), H_MemoryInfo,      (const TCHAR *)PHYSICAL_TOTAL,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ _T("System.Memory.Physical.Used"),  H_MemoryInfo,      (const TCHAR *)PHYSICAL_USED,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo,   (const TCHAR *)PHYSICAL_USED_PCT,	DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
#if HAVE_KVM_GETSWAPINFO
	{ _T("System.Memory.Swap.Free"),      H_MemoryInfo,      (const TCHAR *)SWAP_FREE,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ _T("System.Memory.Swap.FreePerc"),  H_MemoryInfo,      (const TCHAR *)SWAP_FREE_PCT,	DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
	{ _T("System.Memory.Swap.Total"),     H_MemoryInfo,      (const TCHAR *)SWAP_TOTAL,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ _T("System.Memory.Swap.Used"),      H_MemoryInfo,      (const TCHAR *)SWAP_USED,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ _T("System.Memory.Swap.UsedPerc"),      H_MemoryInfo,  (const TCHAR *)SWAP_USED_PCT,	DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
	{ _T("System.Memory.Virtual.Free"),   H_MemoryInfo,      (const TCHAR *)VIRTUAL_FREE,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
	{ _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo,    (const TCHAR *)VIRTUAL_FREE_PCT,	DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
	{ _T("System.Memory.Virtual.Total"),  H_MemoryInfo,      (const TCHAR *)VIRTUAL_TOTAL,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
	{ _T("System.Memory.Virtual.Used"),   H_MemoryInfo,      (const TCHAR *)VIRTUAL_USED,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
	{ _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo,    (const TCHAR *)VIRTUAL_USED_PCT,	DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
#endif

   { _T("System.OS.Build"), H_OSInfo, _T("B"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_BUILD },
   { _T("System.OS.ProductName"), H_OSInfo, _T("N"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_NAME },
   { _T("System.OS.ProductType"), H_OSInfo, _T("T"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_TYPE },
   { _T("System.OS.Version"), H_OSInfo, _T("V"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_VERSION },

	{ _T("System.ProcessCount"),          H_ProcessCount,    _T("S"),			DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },
	{ _T("System.ThreadCount"),           H_ProcessCount,    _T("T"),			DCI_DT_UINT,	DCIDESC_SYSTEM_THREADCOUNT },
	{ _T("System.Uname"),                 H_Uname,           NULL,				DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
	{ _T("System.Uptime"),                H_Uptime,          NULL,				DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("FileSystem.MountPoints"),       H_MountPoints,     NULL },
   { _T("Net.ArpCache"),                 H_NetArpCache,     NULL },
   { _T("Net.InterfaceList"),            H_NetIfList,       NULL },
   { _T("Net.InterfaceNames"),           H_NetIfNames,      NULL },
   { _T("Net.IP.RoutingTable"),          H_NetRoutingTable, NULL },
   { _T("System.ProcessList"),           H_ProcessList,     NULL },
};

/**
 * Subagent's tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("System.InstalledProducts"), H_InstalledProducts, NULL, _T("NAME"), DCTDESC_SYSTEM_INSTALLED_PRODUCTS },
   { _T("System.Processes"), H_ProcessTable, NULL, _T("PID"), DCTDESC_SYSTEM_PROCESSES }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("FreeBSD"),
	NETXMS_VERSION_STRING,
	SubAgentInit, // init handler
	SubAgentShutdown, // shutdown handler
	NULL, // command handler
	NULL, // notification handler
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
        m_tables,
	0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(FREEBSD)
{
	*ppInfo = &m_info;
	return true;
}

/**
 * Entry point for server - get interface list
 */
extern "C" BOOL __EXPORT __NxSubAgentGetIfList(StringList *pValue)
{
	return H_NetIfList(_T("Net.InterfaceList"), NULL, pValue, NULL) == SYSINFO_RC_SUCCESS;
}

/**
 * Entry point for server - get ARP cache
 */
extern "C" BOOL __EXPORT __NxSubAgentGetArpCache(StringList *pValue)
{
	return H_NetArpCache(_T("Net.ArpCache"), NULL, pValue, NULL) == SYSINFO_RC_SUCCESS;
}
