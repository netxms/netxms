/*
** NetXMS subagent for SunOS/Solaris
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
** File: main.cpp
**
**/

#include "sunos_subagent.h"
#include <netxms-version.h>
#include <sys/systeminfo.h>

#if HAVE_ZONE_H
// zone.h includes conflicting declaration of struct uuid
// the following define will hide it
#define uuid ___uuid
#include <zone.h>
#undef uuid
#endif

#include "../../smbios/solaris.cpp"

//
// Hanlder functions
//

LONG H_CPUCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_CPUUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_DiskInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table, AbstractCommSession *session);
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_HypervisorType(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HypervisorVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_IOStats(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_IOStatsTotal(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_IsVirtual(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_KStat(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_LoadAvg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetIfList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session);
LONG H_NetIfNames(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session);
LONG H_NetIfAdminStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_NetInterfaceLink(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_NetIfDescription(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_NetInterfaceStats(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_OSInfo(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);
LONG H_ProcessCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session);
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_HandleCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SysMsgQueue(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SysProcCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);

#ifdef __sparc
LONG H_SystemHardwareInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
#endif

/**
 * Global variables
 */
BOOL g_bShutdown = FALSE;
DWORD g_flags = 0;

/**
 * Static data
 */
static THREAD s_cpuStatThread = INVALID_THREAD_HANDLE;
static THREAD s_ioStatThread = INVALID_THREAD_HANDLE;
static Mutex s_kstatLock(MutexType::FAST);

/**
 * Lock access to kstat API
 */
void kstat_lock()
{
   s_kstatLock.lock();
}

/**
 * Unlock access to kstat API
 */
void kstat_unlock()
{
   s_kstatLock.unlock();
}

/**
 * Detect support for source packages
 */
static LONG H_SourcePkg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   ret_int(pValue, 1);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Configuration file template
 */
static NX_CFG_TEMPLATE s_cfgTemplate[] =
{
   { _T("InterfacesFromAllZones"), CT_BOOLEAN, 0, 0, SF_IF_ALL_ZONES, 0, &g_flags },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Initalization callback
 */
static bool SubAgentInit(Config *config)
{
   if (!config->parseTemplate(_T("SunOS"), s_cfgTemplate))
      return false;
  
   char tmp[64]; 
   if(sysinfo(SI_VERSION, tmp, 64) != -1)
   {
      if(!memcmp(tmp, "11", 2))
         g_flags |= SF_SOLARIS_11;
   }

   // try to determine if we are running in global zone
   if ((access("/dev/dump", F_OK) == 0)
#if HAVE_GETZONEID
       || (getzoneid() == 0)
#endif
      )
   {
      g_flags |= SF_GLOBAL_ZONE;
      AgentWriteDebugLog(2, _T("SunOS: running in global zone"));
   }
   else
   {
      g_flags &= ~SF_GLOBAL_ZONE;
      AgentWriteDebugLog(2, _T("SunOS: running in zone"));
   }

   ReadCPUVendorId();
   SMBIOS_Parse(SMBIOS_Reader);

   s_cpuStatThread = ThreadCreateEx(CPUStatCollector);
   s_ioStatThread = ThreadCreateEx(IOStatCollector);

   return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   g_bShutdown = TRUE;
   ThreadJoin(s_cpuStatThread);
   ThreadJoin(s_ioStatThread);
}

/**
 * Parameters provided by subagent
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Agent.SourcePackageSupport"), H_SourcePkg, nullptr, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

   { _T("Disk.Avail(*)"), H_DiskInfo, (TCHAR*)DISK_AVAIL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.AvailPerc(*)"), H_DiskInfo, (TCHAR*)DISK_AVAIL_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Free(*)"), H_DiskInfo, (TCHAR*)DISK_FREE, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"), H_DiskInfo, (TCHAR*)DISK_FREE_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"), H_DiskInfo, (TCHAR*)DISK_TOTAL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"), H_DiskInfo, (TCHAR*)DISK_USED, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"), H_DiskInfo, (TCHAR*)DISK_USED_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },

   { _T("FileSystem.Avail(*)"), H_DiskInfo, (TCHAR*)DISK_AVAIL, DCI_DT_UINT64, DCIDESC_FS_AVAIL },
   { _T("FileSystem.AvailInodes(*)"), H_DiskInfo, (TCHAR*)DISK_AVAIL_INODES, DCI_DT_UINT64, DCIDESC_FS_AVAILINODES },
   { _T("FileSystem.AvailInodesPerc(*)"), H_DiskInfo, (TCHAR*)DISK_AVAIL_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILINODESPERC },
   { _T("FileSystem.AvailPerc(*)"), H_DiskInfo, (TCHAR*)DISK_AVAIL_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILPERC },
   { _T("FileSystem.Free(*)"), H_DiskInfo, (TCHAR*)DISK_FREE, DCI_DT_UINT64, DCIDESC_FS_FREE },
   { _T("FileSystem.FreeInodes(*)"), H_DiskInfo, (TCHAR*)DISK_FREE_INODES, DCI_DT_UINT64, DCIDESC_FS_FREEINODES },
   { _T("FileSystem.FreeInodesPerc(*)"), H_DiskInfo, (TCHAR*)DISK_FREE_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEINODESPERC },
   { _T("FileSystem.FreePerc(*)"), H_DiskInfo, (TCHAR*)DISK_FREE_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { _T("FileSystem.Total(*)"), H_DiskInfo, (TCHAR*)DISK_TOTAL, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { _T("FileSystem.TotalInodes(*)"), H_DiskInfo, (TCHAR*)DISK_TOTAL_INODES, DCI_DT_UINT64, DCIDESC_FS_TOTALINODES },
   { _T("FileSystem.Type(*)"), H_DiskInfo, (TCHAR*)DISK_FSTYPE, DCI_DT_STRING, DCIDESC_FS_TYPE },
   { _T("FileSystem.Used(*)"), H_DiskInfo, (TCHAR*)DISK_USED, DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedInodes(*)"), H_DiskInfo, (TCHAR*)DISK_USED_INODES, DCI_DT_UINT64, DCIDESC_FS_USEDINODES },
   { _T("FileSystem.UsedInodesPerc(*)"), H_DiskInfo, (TCHAR*)DISK_USED_INODES_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDINODESPERC },
   { _T("FileSystem.UsedPerc(*)"), H_DiskInfo, (TCHAR*)DISK_USED_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

#ifdef __sparc
   { _T("Hardware.System.Manufacturer"), H_SystemHardwareInfo, CAST_TO_POINTER(SI_HW_PROVIDER, TCHAR *), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MANUFACTURER },
   { _T("Hardware.System.Product"), H_SystemHardwareInfo, CAST_TO_POINTER(SI_PLATFORM, TCHAR *), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCT },
#else
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
   { _T("Hardware.MemoryDevice.MaxSpeed(*)"), SMBIOS_MemDevParameterHandler, _T("m"), DCI_DT_STRING, DCIDESC_HARDWARE_MEMORYDEVICE_MAXSPEED },
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
#endif

   { _T("Hypervisor.Type"), H_HypervisorType, nullptr, DCI_DT_STRING, DCIDESC_HYPERVISOR_TYPE },
   { _T("Hypervisor.Version"), H_HypervisorVersion, nullptr, DCI_DT_STRING, DCIDESC_HYPERVISOR_VERSION },

   { _T("Net.Interface.AdminStatus(*)"), H_NetIfAdminStatus, nullptr, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetInterfaceStats, (const TCHAR*)"rbytes", DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesIn64(*)"), H_NetInterfaceStats, (const TCHAR*)"rbytes64", DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetInterfaceStats, (const TCHAR*)"obytes", DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.BytesOut64(*)"), H_NetInterfaceStats, (const TCHAR*)"obytes64", DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.Description(*)"), H_NetIfDescription, nullptr, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { _T("Net.Interface.InErrors(*)"), H_NetInterfaceStats, (const TCHAR*)"ierrors", DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetInterfaceLink, nullptr, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.OperStatus(*)"), H_NetInterfaceLink, nullptr, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.Interface.OutErrors(*)"), H_NetInterfaceStats, (const TCHAR*)"oerrors", DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetInterfaceStats, (const TCHAR*)"ipackets", DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsIn64(*)"), H_NetInterfaceStats, (const TCHAR*)"ipackets64", DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetInterfaceStats, (const TCHAR*)"opackets", DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.PacketsOut64(*)"), H_NetInterfaceStats, (const TCHAR*)"opackets64", DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetInterfaceStats, (const TCHAR*)"ifspeed", DCI_DT_UINT, DCIDESC_NET_INTERFACE_SPEED },

   {_T("Process.Count(*)"), H_ProcessCount, _T("S"), DCI_DT_INT, DCIDESC_PROCESS_COUNT },
   {_T("Process.CountEx(*)"), H_ProcessCount, _T("E"), DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
   {_T("Process.ZombieCount"), H_ProcessCount, _T("Z"), DCI_DT_INT, DCIDESC_PROCESS_ZOMBIE_COUNT },
   {_T("Process.ZombieCount(*)"), H_ProcessCount, _T("Z"), DCI_DT_INT, DCIDESC_PROCESS_ZOMBIE_COUNT },
   {_T("Process.CPUTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_CPUTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_CPUTIME },
   {_T("Process.Handles(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_HANDLES, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_HANDLES },
   {_T("Process.KernelTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_KTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_KERNELTIME },
   {_T("Process.MemoryUsage(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_MEMPERC, const TCHAR *), DCI_DT_FLOAT, DCIDESC_PROCESS_MEMORYUSAGE },
   {_T("Process.PageFaults(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_PF, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_PAGEFAULTS },
   {_T("Process.RSS(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },
   {_T("Process.Syscalls(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_SYSCALLS, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_SYSCALLS },
   {_T("Process.Threads(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_THREADS, const TCHAR *), DCI_DT_INT, DCIDESC_PROCESS_THREADS },
   {_T("Process.UserTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_UTIME, const TCHAR *), DCI_DT_COUNTER64, DCIDESC_PROCESS_USERTIME },
   {_T("Process.VMSize(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_VMSIZE, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_VMSIZE },
   {_T("Process.WkSet(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_RSS, const TCHAR *), DCI_DT_INT64, DCIDESC_PROCESS_WKSET },

   { _T("System.BIOS.Date"), SMBIOS_ParameterHandler, _T("BD"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_DATE },
   { _T("System.BIOS.Vendor"), SMBIOS_ParameterHandler, _T("Bv"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VENDOR },
   { _T("System.BIOS.Version"), SMBIOS_ParameterHandler, _T("BV"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VERSION },

   { _T("System.CPU.Count"), H_CPUCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { _T("System.CPU.LoadAvg"), H_LoadAvg, (TCHAR*)0, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
   { _T("System.CPU.LoadAvg5"), H_LoadAvg, (TCHAR*)1, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { _T("System.CPU.LoadAvg15"), H_LoadAvg, (TCHAR*)2, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },

   /* CPU usage */
   { _T("System.CPU.Usage"), H_CPUUsage, _T("T0T"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
   { _T("System.CPU.Usage5"), H_CPUUsage, _T("T1T"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
   { _T("System.CPU.Usage15"), H_CPUUsage, _T("T2T"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
   { _T("System.CPU.Usage(*)"), H_CPUUsage, _T("C0T"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
   { _T("System.CPU.Usage5(*)"), H_CPUUsage, _T("C1T"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
   { _T("System.CPU.Usage15(*)"), H_CPUUsage, _T("C2T"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },

   /* CPU usage - idle */
   { _T("System.CPU.Usage.Idle"), H_CPUUsage, _T("T0I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE },
   { _T("System.CPU.Usage5.Idle"), H_CPUUsage, _T("T1I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
   { _T("System.CPU.Usage15.Idle"), H_CPUUsage, _T("T2I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
   { _T("System.CPU.Usage.Idle(*)"), H_CPUUsage, _T("C0I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
   { _T("System.CPU.Usage5.Idle(*)"), H_CPUUsage, _T("C1I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
   { _T("System.CPU.Usage15.Idle(*)"), H_CPUUsage, _T("C2I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

   /* CPU usage - iowait */
   { _T("System.CPU.Usage.IoWait"), H_CPUUsage, _T("T0W"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
   { _T("System.CPU.Usage5.IoWait"), H_CPUUsage, _T("T1W"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
   { _T("System.CPU.Usage15.IoWait"), H_CPUUsage, _T("T2W"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
   { _T("System.CPU.Usage.IoWait(*)"), H_CPUUsage, _T("C0W"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
   { _T("System.CPU.Usage5.IoWait(*)"), H_CPUUsage, _T("C1W"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
   { _T("System.CPU.Usage15.IoWait(*)"), H_CPUUsage, _T("C2W"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

   /* CPU usage - system */
   { _T("System.CPU.Usage.System"), H_CPUUsage, _T("T0S"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
   { _T("System.CPU.Usage5.System"), H_CPUUsage, _T("T1S"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
   { _T("System.CPU.Usage15.System"), H_CPUUsage, _T("T2S"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
   { _T("System.CPU.Usage.System(*)"), H_CPUUsage, _T("C0S"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
   { _T("System.CPU.Usage5.System(*)"), H_CPUUsage, _T("C1S"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
   { _T("System.CPU.Usage15.System(*)"), H_CPUUsage, _T("C2S"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

   /* CPU usage - user */
   { _T("System.CPU.Usage.User"), H_CPUUsage, _T("T0U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER },
   { _T("System.CPU.Usage5.User"), H_CPUUsage, _T("T1U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER },
   { _T("System.CPU.Usage15.User"), H_CPUUsage, _T("T2U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER },
   { _T("System.CPU.Usage.User(*)"), H_CPUUsage, _T("C0U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
   { _T("System.CPU.Usage5.User(*)"), H_CPUUsage, _T("C1U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
   { _T("System.CPU.Usage15.User(*)"), H_CPUUsage, _T("C2U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

   { _T("System.HandleCount"), H_HandleCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_HANDLECOUNT },
   { _T("System.Hostname"), H_Hostname, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
   { _T("System.KStat(*)"), H_KStat, nullptr, DCI_DT_STRING, _T("") },

   { _T("System.IO.ReadRate"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
   { _T("System.IO.ReadRate.Min"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_READS_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_MIN },
   { _T("System.IO.ReadRate.Max"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_READS_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_MAX },
   { _T("System.IO.ReadRate(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
   { _T("System.IO.ReadRate.Min(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_READS_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_EX_MIN },
   { _T("System.IO.ReadRate.Max(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_READS_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_EX_MAX },
   { _T("System.IO.WriteRate"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
   { _T("System.IO.WriteRate.Min"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_WRITES_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_MIN },
   { _T("System.IO.WriteRate.Max"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_WRITES_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_MAX },
   { _T("System.IO.WriteRate(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },
   { _T("System.IO.WriteRate.Min(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_WRITES_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_EX_MIN },
   { _T("System.IO.WriteRate.Max(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_WRITES_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_EX_MAX },
   { _T("System.IO.BytesReadRate"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
   { _T("System.IO.BytesReadRate.Min"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_RBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_MIN },
   { _T("System.IO.BytesReadRate.Max"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_RBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_MAX },
   { _T("System.IO.BytesReadRate(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
   { _T("System.IO.BytesReadRate.Min(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_RBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX_MIN },
   { _T("System.IO.BytesReadRate.Max(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_RBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX_MAX },
   { _T("System.IO.BytesWriteRate"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
   { _T("System.IO.BytesWriteRate.Min"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_WBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_MIN },
   { _T("System.IO.BytesWriteRate.Max"), H_IOStatsTotal, (const TCHAR*)IOSTAT_NUM_WBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_MAX },
   { _T("System.IO.BytesWriteRate(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
   { _T("System.IO.BytesWriteRate.Min(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_WBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MIN },
   { _T("System.IO.BytesWriteRate.Max(*)"), H_IOStats, (const TCHAR*)IOSTAT_NUM_WBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MAX },
   { _T("System.IO.DiskQueue"), H_IOStatsTotal, (const TCHAR*)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
   { _T("System.IO.DiskQueue.Min"), H_IOStatsTotal, (const TCHAR*)IOSTAT_QUEUE_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_MIN },
   { _T("System.IO.DiskQueue.Max"), H_IOStatsTotal, (const TCHAR*)IOSTAT_QUEUE_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_MAX },
   { _T("System.IO.DiskQueue(*)"), H_IOStats, (const TCHAR*)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
   { _T("System.IO.DiskQueue.Min(*)"), H_IOStats, (const TCHAR*)IOSTAT_QUEUE_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MIN },
   { _T("System.IO.DiskQueue.Max(*)"), H_IOStats, (const TCHAR*)IOSTAT_QUEUE_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MAX },

   { _T("System.IsVirtual"), H_IsVirtual, NULL, DCI_DT_INT, DCIDESC_SYSTEM_IS_VIRTUAL },

   { _T("System.Memory.Physical.Free"), H_MemoryInfo, (const TCHAR*)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { _T("System.Memory.Physical.FreePerc"), H_MemoryInfo, (const TCHAR*)MEMINFO_PHYSICAL_FREEPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { _T("System.Memory.Physical.Total"), H_MemoryInfo, (const TCHAR*)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { _T("System.Memory.Physical.Used"), H_MemoryInfo, (const TCHAR*)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (const TCHAR*)MEMINFO_PHYSICAL_USEDPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { _T("System.Memory.Swap.Free"), H_MemoryInfo, (const TCHAR*)MEMINFO_SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
   { _T("System.Memory.Swap.FreePerc"), H_MemoryInfo, (const TCHAR*)MEMINFO_SWAP_FREEPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
   { _T("System.Memory.Swap.Total"), H_MemoryInfo, (const TCHAR*)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
   { _T("System.Memory.Swap.Used"), H_MemoryInfo, (const TCHAR*)MEMINFO_SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
   { _T("System.Memory.Swap.UsedPerc"), H_MemoryInfo, (const TCHAR*)MEMINFO_SWAP_USEDPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
   { _T("System.Memory.Virtual.Free"), H_MemoryInfo, (const TCHAR*)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo, (const TCHAR*)MEMINFO_VIRTUAL_FREEPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_MemoryInfo, (const TCHAR*)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { _T("System.Memory.Virtual.Used"), H_MemoryInfo, (const TCHAR*)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo, (const TCHAR*)MEMINFO_VIRTUAL_USEDPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },

   { _T("System.MsgQueue.Bytes(*)"), H_SysMsgQueue, _T("b"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_BYTES },
   { _T("System.MsgQueue.BytesMax(*)"), H_SysMsgQueue, _T("B"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_BYTES_MAX },
   { _T("System.MsgQueue.ChangeTime(*)"), H_SysMsgQueue, _T("c"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_CHANGE_TIME },
   { _T("System.MsgQueue.Messages(*)"), H_SysMsgQueue, _T("m"), DCI_DT_UINT, DCIDESC_SYSTEM_MSGQUEUE_MESSAGES },
   { _T("System.MsgQueue.RecvTime(*)"), H_SysMsgQueue, _T("r"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_RECV_TIME },
   { _T("System.MsgQueue.SendTime(*)"), H_SysMsgQueue, _T("s"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_SEND_TIME },

   { _T("System.OS.ProductName"), H_OSInfo, _T("N"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_NAME },
   { _T("System.OS.Version"), H_OSInfo, _T("V"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_VERSION },

   { _T("System.ProcessCount"), H_SysProcCount, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_PROCESSCOUNT },
   { _T("System.Uname"), H_Uname, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
   { _T("System.Uptime"), H_Uptime, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};

/**
 * Subagent's lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("FileSystem.MountPoints"), H_MountPoints, nullptr },
   { _T("Hardware.Batteries"), SMBIOS_ListHandler, _T("B") },
   { _T("Hardware.MemoryDevices"), SMBIOS_ListHandler, _T("M") },
   { _T("Hardware.Processors"), SMBIOS_ListHandler, _T("P") },
   { _T("Net.InterfaceList"), H_NetIfList, nullptr },
   { _T("Net.InterfaceNames"), H_NetIfNames, nullptr },
   { _T("System.Processes"), H_ProcessList, _T("2") },
   { _T("System.ProcessList"), H_ProcessList, _T("1") }
};

/**
 * Subagent's tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, nullptr, _T("MOUNTPOINT"), DCTDESC_FILESYSTEM_VOLUMES },
   { _T("Hardware.Batteries"), SMBIOS_TableHandler, _T("B"), _T("HANDLE"), DCTDESC_HARDWARE_BATTERIES },
   { _T("Hardware.MemoryDevices"), SMBIOS_TableHandler, _T("M"), _T("HANDLE"), DCTDESC_HARDWARE_MEMORY_DEVICES },
   { _T("Hardware.Processors"), SMBIOS_TableHandler, _T("P"), _T("HANDLE"), DCTDESC_HARDWARE_PROCESSORS },
   { _T("System.InstalledProducts"), H_InstalledProducts, nullptr, _T("NAME"), DCTDESC_SYSTEM_INSTALLED_PRODUCTS },
   { _T("System.Processes"), H_ProcessTable, nullptr, _T("PID"), DCTDESC_SYSTEM_PROCESSES }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("SUNOS"), NETXMS_VERSION_STRING,
   SubAgentInit, // init handler
   SubAgentShutdown, // unload handler
   nullptr, // command handler
   nullptr, // notification handler
   nullptr, // metric filter
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
DECLARE_SUBAGENT_ENTRY_POINT(SUNOS)
{
   *ppInfo = &m_info;
   return true;
}

/**
 * Entry point for server: interface list
 */
extern "C" BOOL __EXPORT __NxSubAgentGetIfList(StringList *pValue)
{
   return H_NetIfList(_T("Net.InterfaceList"), nullptr, pValue, nullptr) == SYSINFO_RC_SUCCESS;
}

/**
 * Entry point for server: arp cache
 */
extern "C" BOOL __EXPORT __NxSubAgentGetArpCache(StringList *pValue)
{
   return SYSINFO_RC_ERROR;
}
