/*
** Windows platform subagent
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

/**
 * Externlals
 */
LONG H_ActiveUserSessions(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
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
LONG H_ServiceList(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value, AbstractCommSession *session);
LONG H_ServiceState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ServiceTable(const TCHAR *pszCmd, const TCHAR *pArg, Table *value, AbstractCommSession *session);
LONG H_StorageDeviceInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_StorageDeviceList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_StorageDeviceSize(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_StorageDeviceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SystemUname(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SysUpdateTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ThreadCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_WindowStations(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);

void StartCPUStatCollector();
void StopCPUStatCollector();

/**
 * Optional imports
 */
DWORD (__stdcall *imp_GetIfEntry2)(PMIB_IF_ROW2) = NULL;

/**
 * Windows XP/Windows Server 2003 flag
 */
bool g_isWin5 = false;

/**
 * Import symbols
 */
static void ImportSymbols()
{
   HMODULE hModule;

   // IPHLPAPI.DLL
   hModule = LoadLibrary(_T("IPHLPAPI.DLL"));
   if (hModule != NULL)
   {
      imp_GetIfEntry2 = (DWORD (__stdcall *)(PMIB_IF_ROW2))GetProcAddress(hModule, "GetIfEntry2");
   }
   else
   {
		AgentWriteLog(NXLOG_WARNING, _T("Unable to load IPHLPAPI.DLL"));
   }
}

/**
 * Set or clear current privilege
 */
static BOOL SetCurrentPrivilege(LPCTSTR pszPrivilege, BOOL bEnablePrivilege)
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tp, tpPrevious;
	DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
	BOOL bSuccess = FALSE;

	if (!LookupPrivilegeValue(NULL, pszPrivilege, &luid))
		return FALSE;

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
		return FALSE;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
				&tpPrevious, &cbPrevious))
	{
		tpPrevious.PrivilegeCount = 1;
		tpPrevious.Privileges[0].Luid = luid;

		if (bEnablePrivilege)
			tpPrevious.Privileges[0].Attributes |= SE_PRIVILEGE_ENABLED;
		else
			tpPrevious.Privileges[0].Attributes &= ~SE_PRIVILEGE_ENABLED;

		bSuccess = AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, cbPrevious, NULL, NULL);
	}

	CloseHandle(hToken);

	return bSuccess;
}

/**
 * Shutdown system
 */
static LONG H_ActionShutdown(const TCHAR *action, const StringList *args, const TCHAR *data, AbstractCommSession *session)
{
	LONG nRet = ERR_INTERNAL_ERROR;

	if (SetCurrentPrivilege(SE_SHUTDOWN_NAME, TRUE))
	{
		if (InitiateSystemShutdown(NULL, NULL, 0, TRUE, (*data == _T('R')) ? TRUE : FALSE))
			nRet = ERR_SUCCESS;
	}
	return nRet;
}

/**
 * Change user's password
 * Parameters: user new_password
 */
static LONG H_ChangeUserPassword(const TCHAR *action, const StringList *args, const TCHAR *data, AbstractCommSession *session)
{
   if (args->size() < 2)
	   return ERR_INTERNAL_ERROR;

   USER_INFO_1003 ui;
   ui.usri1003_password = (TCHAR *)args->get(1);
   NET_API_STATUS status = NetUserSetInfo(NULL, args->get(0), 1003, (LPBYTE)&ui, NULL);
   AgentWriteDebugLog(2, _T("WINNT: change password for user %s status=%d"), args->get(0), status);
   return (status == NERR_Success) ? ERR_SUCCESS : ERR_INTERNAL_ERROR; 
}

/**
 * BIOS header
 */
struct BiosHeader
{
   BYTE used20CallingMethod;
   BYTE majorVersion;
   BYTE minorVersion;
   BYTE dmiRevision;
   DWORD length;
   BYTE tables[1];
};

/**
 * BIOS reader
 */
static BYTE *BIOSReader(size_t *size)
{
   BYTE *buffer = (BYTE *)MemAlloc(16384);
   UINT rc = GetSystemFirmwareTable('RSMB', 0, buffer, 16384);
   if (rc > 16384)
   {
      buffer = (BYTE *)realloc(buffer, rc);
      rc = GetSystemFirmwareTable('RSMB', 0, buffer, rc);
   }
   if (rc == 0)
   {
      TCHAR errorText[1024];
      nxlog_debug_tag(_T("smbios"), 3, _T("Call to GetSystemFirmwareTable failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
      free(buffer);
      return NULL;
   }

   BiosHeader *header = reinterpret_cast<BiosHeader*>(buffer);
   BYTE *bios = (BYTE *)MemAlloc(header->length);
   memcpy(bios, header->tables, header->length);
   *size = header->length;
   MemFree(buffer);

   return bios;
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   ReadCPUVendorId();
   SMBIOS_Parse(BIOSReader);
   StartCPUStatCollector();
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   StopCPUStatCollector();
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Agent.Desktop"), H_AgentDesktop, NULL, DCI_DT_STRING, _T("Desktop associated with agent process") },

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
   { _T("Hardware.System.Manufacturer"), SMBIOS_ParameterHandler, _T("HM"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_MANUFACTURER },
   { _T("Hardware.System.Product"), SMBIOS_ParameterHandler, _T("HP"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_PRODUCT },
   { _T("Hardware.System.SerialNumber"), SMBIOS_ParameterHandler, _T("HS"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_SERIALNUMBER },
   { _T("Hardware.System.Version"), SMBIOS_ParameterHandler, _T("HV"), DCI_DT_STRING, DCIDESC_HARDWARE_SYSTEM_VERSION },
   { _T("Hardware.WakeUpEvent"), SMBIOS_ParameterHandler, _T("W"), DCI_DT_STRING, DCIDESC_HARDWARE_WAKEUPEVENT },

   { _T("Hypervisor.Type"), H_HypervisorType, NULL, DCI_DT_STRING, DCIDESC_HYPERVISOR_TYPE },
   { _T("Hypervisor.Version"), H_HypervisorVersion, NULL, DCI_DT_STRING, DCIDESC_HYPERVISOR_VERSION },

   { _T("Net.Interface.64BitCounters"), H_NetInterface64bitSupport, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_64BITCOUNTERS },
   { _T("Net.Interface.AdminStatus(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesIn64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_IN_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.BytesOut64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_OUT_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.Description(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_DESCR, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { _T("Net.Interface.InErrors(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_IN_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OPER_STATUS, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.MTU(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_MTU, DCI_DT_UINT, DCIDESC_NET_INTERFACE_MTU },
   { _T("Net.Interface.OperStatus(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.Interface.OutErrors(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OUT_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsIn64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_IN_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.PacketsOut64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_OUT_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_SPEED, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_SPEED },

   { _T("Net.IP.Forwarding"), H_NetIPStats, (TCHAR *)NETINFO_IP_FORWARDING, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },

	{ _T("Net.RemoteShareStatus(*)"), H_RemoteShareStatus, _T("C"), DCI_DT_INT, _T("Status of remote shared resource") },
	{ _T("Net.RemoteShareStatusText(*)"), H_RemoteShareStatus, _T("T"), DCI_DT_STRING, _T("Status of remote shared resource as text") },

	{ _T("Process.Count(*)"), H_ProcCountSpecific, _T("N"), DCI_DT_INT, DCIDESC_PROCESS_COUNT },
	{ _T("Process.CountEx(*)"), H_ProcCountSpecific, _T("E"), DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
	{ _T("Process.CPUTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_CPUTIME, DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
	{ _T("Process.GDIObjects(*)"), H_ProcInfo, (TCHAR *)PROCINFO_GDI_OBJ, DCI_DT_UINT64, DCIDESC_PROCESS_GDIOBJ },
	{ _T("Process.Handles(*)"), H_ProcInfo, (TCHAR *)PROCINFO_HANDLES, DCI_DT_UINT, DCIDESC_PROCESS_HANDLES },
	{ _T("Process.IO.OtherB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_OTHER_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_OTHERB },
	{ _T("Process.IO.OtherOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_OTHER_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_OTHEROP },
	{ _T("Process.IO.ReadB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_READ_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READB },
	{ _T("Process.IO.ReadOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READOP },
	{ _T("Process.IO.WriteB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_WRITE_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEB },
	{ _T("Process.IO.WriteOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEOP },
	{ _T("Process.KernelTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
	{ _T("Process.PageFaults(*)"), H_ProcInfo, (TCHAR *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
	{ _T("Process.UserObjects(*)"), H_ProcInfo, (TCHAR *)PROCINFO_USER_OBJ, DCI_DT_UINT64, DCIDESC_PROCESS_USEROBJ },
	{ _T("Process.UserTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
	{ _T("Process.VMSize(*)"), H_ProcInfo, (TCHAR *)PROCINFO_VMSIZE, DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
	{ _T("Process.WkSet(*)"), H_ProcInfo, (TCHAR *)PROCINFO_WKSET, DCI_DT_UINT64, DCIDESC_PROCESS_WKSET },

	{ _T("System.AppAddressSpace"), H_AppAddressSpace, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_APPADDRESSSPACE },
   { _T("System.BIOS.Date"), SMBIOS_ParameterHandler, _T("BD"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_DATE },
   { _T("System.BIOS.Vendor"), SMBIOS_ParameterHandler, _T("Bv"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VENDOR },
   { _T("System.BIOS.Version"), SMBIOS_ParameterHandler, _T("BV"), DCI_DT_STRING, DCIDESC_SYSTEM_BIOS_VERSION },
   { _T("System.ConnectedUsers"), H_ConnectedUsers, NULL, DCI_DT_INT, DCIDESC_SYSTEM_CONNECTEDUSERS },

	{ _T("System.CPU.Interrupts"), H_CpuInterrupts, _T("T"), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_INTERRUPTS },
	{ _T("System.CPU.Interrupts(*)"), H_CpuInterrupts, _T("C"), DCI_DT_UINT, DCIDESC_SYSTEM_CPU_INTERRUPTS_EX },
	{ _T("System.CPU.ContextSwitches"), H_CpuContextSwitches, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_CONTEXT_SWITCHES },
   
   { _T("System.CPU.Count"), H_CpuCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },

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

   { _T("System.HandleCount"), H_HandleCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_HANDLECOUNT },

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

   { _T("System.ProcessCount"), H_ProcCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_PROCESSCOUNT },
	{ _T("System.ServiceState(*)"), H_ServiceState, NULL, DCI_DT_INT, DCIDESC_SYSTEM_SERVICESTATE },
	{ _T("System.ThreadCount"), H_ThreadCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_THREADCOUNT },
   { _T("System.Uname"), H_SystemUname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
   { _T("System.Update.LastDetectTime"), H_SysUpdateTime, _T("Detect"), DCI_DT_INT64, _T("System update: last detect time") },
   { _T("System.Update.LastDownloadTime"), H_SysUpdateTime, _T("Download"), DCI_DT_INT64, _T("System update: last download time") },
   { _T("System.Update.LastInstallTime"), H_SysUpdateTime, _T("Install"), DCI_DT_INT64, _T("System update: last install time") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("FileSystem.MountPoints"), H_MountPoints, NULL },
   { _T("Hardware.MemoryDevices"), SMBIOS_ListHandler, _T("M") },
   { _T("Hardware.Processors"), SMBIOS_ListHandler, _T("P") },
   { _T("Hardware.StorageDevices"), H_StorageDeviceList, NULL },
   { _T("Net.ArpCache"), H_ArpCache, NULL },
   { _T("Net.InterfaceList"), H_InterfaceList, NULL },
   { _T("Net.InterfaceNames"), H_InterfaceNames, NULL },
   { _T("Net.IP.RoutingTable"), H_IPRoutingTable, NULL },
	{ _T("System.ActiveUserSessions"), H_ActiveUserSessions, NULL },
	{ _T("System.Desktops(*)"), H_Desktops, NULL },
	{ _T("System.ProcessList"), H_ProcessList, NULL },
	{ _T("System.Services"), H_ServiceList, NULL },
	{ _T("System.WindowStations"), H_WindowStations, NULL }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, NULL, _T("VOLUME"), DCTDESC_FILESYSTEM_VOLUMES },
   { _T("Hardware.MemoryDevices"), SMBIOS_TableHandler, _T("M"), _T("HANDLE"), DCTDESC_HARDWARE_MEMORY_DEVICES },
   { _T("Hardware.Processors"), SMBIOS_TableHandler, _T("P"), _T("HANDLE"), DCTDESC_HARDWARE_PROCESSORS },
   { _T("Hardware.StorageDevices"), H_StorageDeviceTable, NULL, _T("NUMBER"), DCTDESC_HARDWARE_STORAGE_DEVICES },
   { _T("System.InstalledProducts"), H_InstalledProducts, NULL, _T("NAME"), DCTDESC_SYSTEM_INSTALLED_PRODUCTS },
	{ _T("System.Processes"), H_ProcessTable, NULL, _T("PID"), DCTDESC_SYSTEM_PROCESSES },
	{ _T("System.Services"), H_ServiceTable, NULL, _T("Name"), _T("Services") }
};

/**
 * Supported actions
 */
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ _T("System.Restart"), H_ActionShutdown, _T("R"), _T("Restart system") },
	{ _T("System.Shutdown"), H_ActionShutdown, _T("S"), _T("Shutdown system") },
	{ _T("User.ChangePassword"), H_ChangeUserPassword, NULL, _T("Change password for given user") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("WinNT"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
	m_tables,
	sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
	m_actions,
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WINNT)
{
	*ppInfo = &m_info;
	ImportSymbols();

   OSVERSIONINFO ver;
   ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   if (GetVersionEx(&ver))
   {
      if (ver.dwMajorVersion < 6)
         g_isWin5 = true;
   }
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
