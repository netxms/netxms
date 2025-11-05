/*
** Windows platform subagent
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#include "winnt_subagent.h"
#include "lm.h"
#include <netxms-version.h>

#include "../../smbios/windows.cpp"

/**
 * Externlals
 */
LONG H_ActiveUserSessionsList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ActiveUserSessionsTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_AgentDesktop(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_AppAddressSpace(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ArpCache(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ConnectedUsers(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_CpuContextSwitches(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuInterrupts(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CpuVendorId(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_Desktops(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_FileSystemInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_FileSystemType(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HandleCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HypervisorType(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HypervisorVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_InterfaceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_InterfaceNames(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_IoDeviceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_IoStats(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IoStatsTotal(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IPRoutingTable(const TCHAR *cmd, const TCHAR *arg, StringList *pValue, AbstractCommSession *session);
LONG H_MemoryInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_MemoryInfo2(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetInterface64bitSupport(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetInterfaceStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetIPStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_RemoteShareStatus(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_ProcCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ProcCountSpecific(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ProcInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_RegistryKeyList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_RegistryValue(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_RegistryValueList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
uint32_t H_ServiceControl(const shared_ptr<ActionExecutionContext>& context);
LONG H_ServiceList(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value, AbstractCommSession *session);
LONG H_ServiceState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ServiceTable(const TCHAR *pszCmd, const TCHAR *pArg, Table *value, AbstractCommSession *session);
LONG H_StorageDeviceInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_StorageDeviceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_StorageDeviceSize(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_StorageDeviceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SystemArchitecture(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemIsRestartPending(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemProductInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemUname(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SystemVersionInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SysUpdateTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
uint32_t H_TerminateProcess(const shared_ptr<ActionExecutionContext>& context);
uint32_t H_TerminateUserSession(const shared_ptr<ActionExecutionContext>& context);
LONG H_ThreadCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
uint32_t H_UninstallProduct(const shared_ptr<ActionExecutionContext>& context);
LONG H_Uptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_UsbConnectedCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_WindowsFirewallCurrentProfile(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_WindowsFirewallProfileState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_WindowsFirewallState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_WindowStations(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);


void StartCPUStatCollector();
void StopCPUStatCollector();
void StartIOStatCollector();
void StopIOStatCollector();

/**
 * Set or clear current privilege
 */
static BOOL SetCurrentPrivilege(LPCTSTR privilege, BOOL enable)
{
   LUID luid;
   if (!LookupPrivilegeValue(nullptr, privilege, &luid))
		return FALSE;

   HANDLE hToken;
   if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
		return FALSE;

   TOKEN_PRIVILEGES tp;
   tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

   BOOL success = FALSE;
   TOKEN_PRIVILEGES tpPrevious;
   DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
   if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious))
	{
		tpPrevious.PrivilegeCount = 1;
		tpPrevious.Privileges[0].Luid = luid;

		if (enable)
			tpPrevious.Privileges[0].Attributes |= SE_PRIVILEGE_ENABLED;
		else
			tpPrevious.Privileges[0].Attributes &= ~SE_PRIVILEGE_ENABLED;

		success = AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, cbPrevious, nullptr, nullptr);
	}

	CloseHandle(hToken);

	return success;
}

/**
 * Shutdown system
 */
static uint32_t H_ActionShutdown(const shared_ptr<ActionExecutionContext>& context)
{
   if (!SetCurrentPrivilege(SE_SHUTDOWN_NAME, TRUE))
      return ERR_INTERNAL_ERROR;

   return InitiateSystemShutdown(nullptr, nullptr, 0, TRUE, (*context->getData<TCHAR>() == _T('R')) ? TRUE : FALSE) ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
}

/**
 * Change user's password
 * Parameters: user new_password
 */
static uint32_t H_ChangeUserPassword(const shared_ptr<ActionExecutionContext>& context)
{
   if (context->getArgCount() < 2)
      return ERR_BAD_ARGUMENTS;

   USER_INFO_1003 ui;
   ui.usri1003_password = (TCHAR *)context->getArg(1);
   NET_API_STATUS status = NetUserSetInfo(NULL, context->getArg(0), 1003, (LPBYTE)&ui, nullptr);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Change password for user %s status=%d"), context->getArg(0), status);
   return (status == NERR_Success) ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   ReadCPUVendorId();
   SMBIOS_Parse(SMBIOS_Reader);
   StartCPUStatCollector();
   StartIOStatCollector();
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   StopCPUStatCollector();
   StopIOStatCollector();
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("Agent.Desktop"), H_AgentDesktop, nullptr, DCI_DT_STRING, _T("Desktop associated with agent process") },

   { _T("Disk.Free(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_FREE_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_FREE_SPACE_PCT, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_TOTAL_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_USED_BYTES, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_USED_SPACE_PCT, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },

   { _T("FileSystem.Free(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_FREE_BYTES, DCI_DT_UINT64, DCIDESC_FS_FREE },
   { _T("FileSystem.FreePerc(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_FREE_SPACE_PCT, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { _T("FileSystem.Total(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_TOTAL_BYTES, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { _T("FileSystem.Type(*)"), H_FileSystemType, NULL, DCI_DT_STRING, DCIDESC_FS_TYPE },
   { _T("FileSystem.Used(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_USED_BYTES, DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedPerc(*)"), H_FileSystemInfo, (TCHAR *)FSINFO_USED_SPACE_PCT, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

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
   { _T("Hardware.MemoryDevice.Size(*)"), SMBIOS_MemDevParameterHandler, _T("S"), DCI_DT_UINT64, DCIDESC_HARDWARE_MEMORYDEVICE_SIZE },
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
   { _T("Hardware.StorageDevice.BusType(*)"), H_StorageDeviceInfo, _T("B"), DCI_DT_STRING, DCIDESC_HARDWARE_STORAGEDEVICE_BUSTYPE },
   { _T("Hardware.StorageDevice.IsRemovable(*)"), H_StorageDeviceInfo, _T("r"), DCI_DT_INT, DCIDESC_HARDWARE_STORAGEDEVICE_ISREMOVABLE },
   { _T("Hardware.StorageDevice.Manufacturer(*)"), H_StorageDeviceInfo, _T("M"), DCI_DT_STRING, DCIDESC_HARDWARE_STORAGEDEVICE_MANUFACTURER },
   { _T("Hardware.StorageDevice.Product(*)"), H_StorageDeviceInfo, _T("P"), DCI_DT_STRING, DCIDESC_HARDWARE_STORAGEDEVICE_PRODUCT },
   { _T("Hardware.StorageDevice.Revision(*)"), H_StorageDeviceInfo, _T("R"), DCI_DT_STRING, DCIDESC_HARDWARE_STORAGEDEVICE_REVISION },
   { _T("Hardware.StorageDevice.SerialNumber(*)"), H_StorageDeviceInfo, _T("S"), DCI_DT_STRING, DCIDESC_HARDWARE_STORAGEDEVICE_SERIALNUMBER },
   { _T("Hardware.StorageDevice.Size(*)"), H_StorageDeviceSize, NULL, DCI_DT_UINT64, DCIDESC_HARDWARE_STORAGEDEVICE_SIZE },
   { _T("Hardware.StorageDevice.Type(*)"), H_StorageDeviceInfo, _T("T"), DCI_DT_INT, DCIDESC_HARDWARE_STORAGEDEVICE_TYPE },
   { _T("Hardware.StorageDevice.TypeDescription(*)"), H_StorageDeviceInfo, _T("t"), DCI_DT_STRING, DCIDESC_HARDWARE_STORAGEDEVICE_TYPEDESCR },
   { _T("Hardware.System.MachineId"), SMBIOS_ParameterHandler, _T("HU"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MACHINEID },
   { _T("Hardware.System.Manufacturer"), SMBIOS_ParameterHandler, _T("HM"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MANUFACTURER },
   { _T("Hardware.System.Product"), SMBIOS_ParameterHandler, _T("HP"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCT },
   { _T("Hardware.System.ProductCode"), SMBIOS_ParameterHandler, _T("HC"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCTCODE },
   { _T("Hardware.System.Version"), SMBIOS_ParameterHandler, _T("HV"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_VERSION },
   { _T("Hardware.WakeUpEvent"), SMBIOS_ParameterHandler, _T("W"), DCI_DT_STRING, DCIDESC_HARDWARE_WAKEUPEVENT },

   { _T("Hypervisor.Type"), H_HypervisorType, nullptr, DCI_DT_STRING, DCIDESC_HYPERVISOR_TYPE },
   { _T("Hypervisor.Version"), H_HypervisorVersion, nullptr, DCI_DT_STRING, DCIDESC_HYPERVISOR_VERSION },

   { _T("Net.Interface.64BitCounters"), H_NetInterface64bitSupport, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_64BITCOUNTERS },
   { _T("Net.Interface.AdminStatus(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesIn64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_IN_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.BytesOut64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_OUT_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.Description(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_DESCR, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { _T("Net.Interface.InErrors(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_IN_ERRORS, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OPER_STATUS, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.MTU(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_MTU, DCI_DT_UINT, DCIDESC_NET_INTERFACE_MTU },
   { _T("Net.Interface.OperStatus(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.Interface.OutErrors(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OUT_ERRORS, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_IN, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsIn64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_IN_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_OUT, DCI_DT_COUNTER32, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.PacketsOut64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_OUT_64, DCI_DT_COUNTER64, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_SPEED, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_SPEED },

   { _T("Net.IP.Forwarding"), H_NetIPStats, (TCHAR *)NETINFO_IP_FORWARDING, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },

	{ _T("Net.RemoteShareStatus(*)"), H_RemoteShareStatus, _T("C"), DCI_DT_INT, _T("Status of remote shared resource") },
	{ _T("Net.RemoteShareStatusText(*)"), H_RemoteShareStatus, _T("T"), DCI_DT_STRING, _T("Status of remote shared resource as text") },

	{ _T("Process.Count(*)"), H_ProcCountSpecific, _T("N"), DCI_DT_INT, DCIDESC_PROCESS_COUNT },
	{ _T("Process.CountEx(*)"), H_ProcCountSpecific, _T("E"), DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
	{ _T("Process.CPUTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_CPUTIME, DCI_DT_COUNTER64, DCIDESC_PROCESS_CPUTIME },
   { _T("Process.GDIObjects(*)"), H_ProcInfo, (TCHAR *)PROCINFO_GDI_OBJ, DCI_DT_INT64, DCIDESC_PROCESS_GDIOBJ },
   { _T("Process.Handles(*)"), H_ProcInfo, (TCHAR *)PROCINFO_HANDLES, DCI_DT_INT, DCIDESC_PROCESS_HANDLES },
	{ _T("Process.IO.OtherB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_OTHER_B, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_OTHERB },
	{ _T("Process.IO.OtherOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_OTHER_OP, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_OTHEROP },
	{ _T("Process.IO.ReadB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_READ_B, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_READB },
	{ _T("Process.IO.ReadOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_READ_OP, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_READOP },
	{ _T("Process.IO.WriteB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_WRITE_B, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_WRITEB },
	{ _T("Process.IO.WriteOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_WRITE_OP, DCI_DT_COUNTER64, DCIDESC_PROCESS_IO_WRITEOP },
   { _T("Process.KernelTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_KTIME, DCI_DT_COUNTER64, DCIDESC_PROCESS_KERNELTIME },
   { _T("Process.MemoryUsage(*)"), H_ProcInfo, (TCHAR *)PROCINFO_MEMPERC, DCI_DT_FLOAT, DCIDESC_PROCESS_MEMORYUSAGE },
   { _T("Process.PageFaults(*)"), H_ProcInfo, (TCHAR *)PROCINFO_PAGE_FAULTS, DCI_DT_COUNTER64, DCIDESC_PROCESS_PAGEFAULTS },
   { _T("Process.RSS(*)"), H_ProcInfo, (TCHAR *)PROCINFO_WKSET, DCI_DT_INT64, DCIDESC_PROCESS_WKSET },
   { _T("Process.Threads(*)"), H_ProcInfo, (TCHAR *)PROCINFO_THREADS, DCI_DT_INT, DCIDESC_PROCESS_THREADS },
   { _T("Process.UserObjects(*)"), H_ProcInfo, (TCHAR *)PROCINFO_USER_OBJ, DCI_DT_INT64, DCIDESC_PROCESS_USEROBJ },
   { _T("Process.UserTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_UTIME, DCI_DT_COUNTER64, DCIDESC_PROCESS_USERTIME },
	{ _T("Process.VMSize(*)"), H_ProcInfo, (TCHAR *)PROCINFO_VMSIZE, DCI_DT_INT64, DCIDESC_PROCESS_VMSIZE },
	{ _T("Process.WkSet(*)"), H_ProcInfo, (TCHAR *)PROCINFO_WKSET, DCI_DT_INT64, DCIDESC_PROCESS_WKSET },

	{ _T("System.AppAddressSpace"), H_AppAddressSpace, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_APPADDRESSSPACE },
   { _T("System.Architecture"), H_SystemArchitecture, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_ARCHITECTURE },
   { _T("System.BIOS.Date"), SMBIOS_ParameterHandler, _T("BD"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_DATE },
   { _T("System.BIOS.Vendor"), SMBIOS_ParameterHandler, _T("Bv"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VENDOR },
   { _T("System.BIOS.Version"), SMBIOS_ParameterHandler, _T("BV"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VERSION },
   { _T("System.ConnectedUsers"), H_ConnectedUsers, NULL, DCI_DT_INT, DCIDESC_SYSTEM_CONNECTEDUSERS },

	{ _T("System.CPU.Interrupts"), H_CpuInterrupts, _T("T"), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_INTERRUPTS },
	{ _T("System.CPU.Interrupts(*)"), H_CpuInterrupts, _T("C"), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_INTERRUPTS_EX },
	{ _T("System.CPU.ContextSwitches"), H_CpuContextSwitches, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_CONTEXT_SWITCHES },
   
   { _T("System.CPU.Count"), H_CpuCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },

   { _T("System.CPU.CurrentUsage"), H_CpuUsage, _T("T0U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR },
   { _T("System.CPU.CurrentUsage.Idle"), H_CpuUsage, _T("T0I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_IDLE },
   { _T("System.CPU.CurrentUsage.Irq"), H_CpuUsage, _T("T0q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_IRQ },
   { _T("System.CPU.CurrentUsage.System"), H_CpuUsage, _T("T0s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_SYSTEM },
   { _T("System.CPU.CurrentUsage.User"), H_CpuUsage, _T("T0u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_USER },

   { _T("System.CPU.CurrentUsage(*)"), H_CpuUsage, _T("C0U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_EX },
   { _T("System.CPU.CurrentUsage.Idle(*)"), H_CpuUsage, _T("C0I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_IDLE_EX },
   { _T("System.CPU.CurrentUsage.Irq(*)"), H_CpuUsage, _T("C0q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_IRQ_EX },
   { _T("System.CPU.CurrentUsage.System(*)"), H_CpuUsage, _T("C0s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_SYSTEM_EX },
   { _T("System.CPU.CurrentUsage.User(*)"), H_CpuUsage, _T("C0u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGECURR_USER_EX },

   { _T("System.CPU.Usage"), H_CpuUsage, _T("T1U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
   { _T("System.CPU.Usage.Idle"), H_CpuUsage, _T("T1I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE },
   { _T("System.CPU.Usage.Irq"), H_CpuUsage, _T("T1q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IRQ },
   { _T("System.CPU.Usage.System"), H_CpuUsage, _T("T1s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
   { _T("System.CPU.Usage.User"), H_CpuUsage, _T("T1u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER },
	
   { _T("System.CPU.Usage(*)"), H_CpuUsage, _T("C1U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
   { _T("System.CPU.Usage.Idle(*)"), H_CpuUsage, _T("C1I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
   { _T("System.CPU.Usage.Irq(*)"), H_CpuUsage, _T("C1q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX },
   { _T("System.CPU.Usage.System(*)"), H_CpuUsage, _T("C1s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
   { _T("System.CPU.Usage.User(*)"), H_CpuUsage, _T("C1u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
   
   { _T("System.CPU.Usage5"), H_CpuUsage, _T("T2U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
   { _T("System.CPU.Usage5.Idle"), H_CpuUsage, _T("T2I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
   { _T("System.CPU.Usage5.Irq"), H_CpuUsage, _T("T2q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IRQ },
   { _T("System.CPU.Usage5.System"), H_CpuUsage, _T("T2s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
   { _T("System.CPU.Usage5.User"), H_CpuUsage, _T("T2u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER },
   
   { _T("System.CPU.Usage5(*)"), H_CpuUsage, _T("C2U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
   { _T("System.CPU.Usage5.Idle(*)"), H_CpuUsage, _T("C2I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
   { _T("System.CPU.Usage5.Irq(*)"), H_CpuUsage, _T("C2q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX },
   { _T("System.CPU.Usage5.System(*)"), H_CpuUsage, _T("C2s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
   { _T("System.CPU.Usage5.User(*)"), H_CpuUsage, _T("C2u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
   
   { _T("System.CPU.Usage15"), H_CpuUsage, _T("T3U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
   { _T("System.CPU.Usage15.Idle"), H_CpuUsage, _T("T3I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
   { _T("System.CPU.Usage15.Irq"), H_CpuUsage, _T("T3q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IRQ },
   { _T("System.CPU.Usage15.System"), H_CpuUsage, _T("T3s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
   { _T("System.CPU.Usage15.User"), H_CpuUsage, _T("T3u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER },
   
   { _T("System.CPU.Usage15(*)"), H_CpuUsage, _T("C3U"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },
   { _T("System.CPU.Usage15.Idle(*)"), H_CpuUsage, _T("C3I"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },
   { _T("System.CPU.Usage15.Irq(*)"), H_CpuUsage, _T("C3q"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX },
   { _T("System.CPU.Usage15.System(*)"), H_CpuUsage, _T("C3s"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },
   { _T("System.CPU.Usage15.User(*)"), H_CpuUsage, _T("C3u"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

   { _T("System.CPU.VendorId"), H_CpuVendorId, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_CPU_VENDORID },

   { _T("System.HandleCount"), H_HandleCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_HANDLECOUNT },

   { _T("System.IO.ReadRate"), H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
   { _T("System.IO.ReadRate(*)"), H_IoStats, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
   { _T("System.IO.WriteRate"), H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
   { _T("System.IO.WriteRate(*)"), H_IoStats, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },
   { _T("System.IO.BytesReadRate"), H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_SREADS, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
   { _T("System.IO.BytesReadRate(*)"), H_IoStats, (const TCHAR *)IOSTAT_NUM_SREADS, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
   { _T("System.IO.BytesWriteRate"), H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_SWRITES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
   { _T("System.IO.BytesWriteRate(*)"), H_IoStats, (const TCHAR *)IOSTAT_NUM_SWRITES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
   { _T("System.IO.DiskQueue"), H_IoStatsTotal, (const TCHAR *)IOSTAT_DISK_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
   { _T("System.IO.DiskQueue(*)"), H_IoStats, (const TCHAR *)IOSTAT_DISK_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
   { _T("System.IO.DiskReadTime"), H_IoStatsTotal, (const TCHAR *)IOSTAT_IO_READ_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKREADTIME },
   { _T("System.IO.DiskReadTime(*)"), H_IoStats, (const TCHAR *)IOSTAT_IO_READ_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKREADTIME_EX },
   { _T("System.IO.DiskTime"), H_IoStatsTotal, (const TCHAR *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME },
   { _T("System.IO.DiskTime(*)"), H_IoStats, (const TCHAR *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME_EX },
   { _T("System.IO.DiskWriteTime"), H_IoStatsTotal, (const TCHAR *)IOSTAT_IO_WRITE_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKWRITETIME },
   { _T("System.IO.DiskWriteTime(*)"), H_IoStats, (const TCHAR *)IOSTAT_IO_WRITE_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKWRITETIME_EX },

   { _T("System.IsRestartPending"), H_SystemIsRestartPending, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_IS_RESTART_PENDING },

   { _T("System.Memory.Physical.Available"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_AVAIL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE },
   { _T("System.Memory.Physical.AvailablePerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_AVAIL_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT },
   { _T("System.Memory.Physical.Cached"), H_MemoryInfo2, (TCHAR *)MEMINFO_PHYSICAL_CACHE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED },
   { _T("System.Memory.Physical.CachedPerc"), H_MemoryInfo2, (TCHAR *)MEMINFO_PHYSICAL_CACHE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED_PCT },
   { _T("System.Memory.Physical.Free"), H_MemoryInfo2, (TCHAR *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { _T("System.Memory.Physical.FreePerc"), H_MemoryInfo2, (TCHAR *)MEMINFO_PHYSICAL_FREE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { _T("System.Memory.Physical.Total"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { _T("System.Memory.Physical.Used"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { _T("System.Memory.Virtual.Free"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { _T("System.Memory.Virtual.Used"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },

   { _T("System.OS.Build"), H_SystemVersionInfo, _T("B"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_BUILD },
   { _T("System.OS.LicenseKey"), H_SystemProductInfo, _T("DigitalProductId"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_LICENSE_KEY },
   { _T("System.OS.ProductId"), H_SystemProductInfo, _T("ProductId"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_ID },
   { _T("System.OS.ProductName"), H_SystemProductInfo, _T("ProductName"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_NAME },
   { _T("System.OS.ProductType"), H_SystemVersionInfo, _T("T"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_PRODUCT_TYPE },
   { _T("System.OS.ServicePack"), H_SystemVersionInfo, _T("S"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_SERVICE_PACK },
   { _T("System.OS.Version"), H_SystemVersionInfo, _T("V"), DCI_DT_STRING, DCIDESC_SYSTEM_OS_VERSION },

   { _T("System.ProcessCount"), H_ProcCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_PROCESSCOUNT },
   { _T("System.Registry.Value(*)"), H_RegistryValue, nullptr, DCI_DT_STRING, _T("Value of registry key {instance}") },
   { _T("System.ServiceState(*)"), H_ServiceState, nullptr, DCI_DT_INT, DCIDESC_SYSTEM_SERVICESTATE },
	{ _T("System.ThreadCount"), H_ThreadCount, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_THREADCOUNT },
   { _T("System.Uname"), H_SystemUname, nullptr, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
   { _T("System.Update.LastDetectTime"), H_SysUpdateTime, _T("Detect"), DCI_DT_INT64, _T("System update: last detect time") },
   { _T("System.Update.LastDownloadTime"), H_SysUpdateTime, _T("Download"), DCI_DT_INT64, _T("System update: last download time") },
   { _T("System.Update.LastInstallTime"), H_SysUpdateTime, _T("Install"), DCI_DT_INT64, _T("System update: last install time") },
   { _T("System.Uptime"), H_Uptime, nullptr, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME },

   { _T("USB.ConnectedCount(*)"), H_UsbConnectedCount, nullptr, DCI_DT_INT, _T("Number of connected USB devices matching VID/PID") },

   { _T("WindowsFirewall.CurrentProfile"), H_WindowsFirewallCurrentProfile, nullptr, DCI_DT_STRING, _T("Windows firewall: current profile") },
   { _T("WindowsFirewall.State"), H_WindowsFirewallState, nullptr, DCI_DT_INT, _T("Windows firewall: state of current profile") },
   { _T("WindowsFirewall.State(*)"), H_WindowsFirewallProfileState, nullptr, DCI_DT_INT, _T("Windows firewall: state of {instance} profile") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("FileSystem.MountPoints"), H_MountPoints, nullptr },
   { _T("Hardware.Batteries"), SMBIOS_ListHandler, _T("B") },
   { _T("Hardware.MemoryDevices"), SMBIOS_ListHandler, _T("M") },
   { _T("Hardware.Processors"), SMBIOS_ListHandler, _T("P") },
   { _T("Hardware.StorageDevices"), H_StorageDeviceList, nullptr },
   { _T("Net.ArpCache"), H_ArpCache, nullptr },
   { _T("Net.InterfaceList"), H_InterfaceList, nullptr },
   { _T("Net.InterfaceNames"), H_InterfaceNames, nullptr },
   { _T("Net.IP.RoutingTable"), H_IPRoutingTable, nullptr },
	{ _T("System.ActiveUserSessions"), H_ActiveUserSessionsList, nullptr },
	{ _T("System.Desktops(*)"), H_Desktops, nullptr },
   { _T("System.IO.Devices"), H_IoDeviceList, nullptr },
   { _T("System.Processes"), H_ProcessList, _T("2") },
   { _T("System.ProcessList"), H_ProcessList, _T("1") },
   { _T("System.Registry.Keys(*)"), H_RegistryKeyList, nullptr },
   { _T("System.Registry.Values(*)"), H_RegistryValueList, nullptr },
   { _T("System.Services"), H_ServiceList, nullptr },
	{ _T("System.WindowStations"), H_WindowStations, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, nullptr, _T("VOLUME"), DCTDESC_FILESYSTEM_VOLUMES },
   { _T("Hardware.Batteries"), SMBIOS_TableHandler, _T("B"), _T("HANDLE"), DCTDESC_HARDWARE_BATTERIES },
   { _T("Hardware.MemoryDevices"), SMBIOS_TableHandler, _T("M"), _T("HANDLE"), DCTDESC_HARDWARE_MEMORY_DEVICES },
   { _T("Hardware.Processors"), SMBIOS_TableHandler, _T("P"), _T("HANDLE"), DCTDESC_HARDWARE_PROCESSORS },
   { _T("Hardware.StorageDevices"), H_StorageDeviceTable, nullptr, _T("NUMBER"), DCTDESC_HARDWARE_STORAGE_DEVICES },
   { _T("System.ActiveUserSessions"), H_ActiveUserSessionsTable, nullptr, _T("ID"), DCTDESC_SYSTEM_ACTIVE_USER_SESSIONS },
   { _T("System.InstalledProducts"), H_InstalledProducts, nullptr, _T("NAME"), DCTDESC_SYSTEM_INSTALLED_PRODUCTS },
	{ _T("System.Processes"), H_ProcessTable, nullptr, _T("PID"), DCTDESC_SYSTEM_PROCESSES },
	{ _T("System.Services"), H_ServiceTable, nullptr, _T("Name"), _T("Services") }
};

/**
 * Supported actions
 */
static NETXMS_SUBAGENT_ACTION s_actions[] =
{
	{ _T("Process.Terminate"), H_TerminateProcess, nullptr, _T("Terminate process") },
   { _T("Service.SetStartType.Automatic"), H_ServiceControl, _T("A"), _T("Set service start type to \"automatic\"") },
   { _T("Service.SetStartType.Disabled"), H_ServiceControl, _T("D"), _T("Set service start type to \"disabled\"") },
   { _T("Service.SetStartType.Manual"), H_ServiceControl, _T("M"), _T("Set service start type to \"manual\"") },
   { _T("Service.Start"), H_ServiceControl, _T("s"), _T("Start service") },
   { _T("Service.Stop"), H_ServiceControl, _T("S"), _T("Stop service") },
   { _T("System.Restart"), H_ActionShutdown, _T("R"), _T("Restart system") },
   { _T("System.Shutdown"), H_ActionShutdown, _T("S"), _T("Shutdown system") },
   { _T("System.TerminateUserSession"), H_TerminateUserSession, nullptr, _T("Terminate user session") },
   { _T("System.UninstallProduct"), H_UninstallProduct, nullptr, _T("Uninstall software product") },
   { _T("User.ChangePassword"), H_ChangeUserPassword, nullptr, _T("Change password for given user") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("WinNT"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   s_parameters,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   s_tables,
   sizeof(s_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
   s_actions,
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WINNT)
{
	*ppInfo = &s_info;
	return true;
}

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}
