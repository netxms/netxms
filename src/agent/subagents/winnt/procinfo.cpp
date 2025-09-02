/* 
** Windows NT+ NetXMS subagent
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: procinfo.cpp
** Win32 specific process information parameters
**
**/

#define UMDF_USING_NTSTATUS
#define MAX_NAME 256
#include <ntstatus.h>

#include "winnt_subagent.h"
#include <winternl.h>
#include <sstream>
#include "aclapi.h"

/**
 * Convert process time from FILETIME structure (100-nanosecond units) to __uint64 (milliseconds)
 */
static uint64_t ConvertProcessTime(FILETIME *lpft)
{
   uint64_t i;
   memcpy(&i, lpft, sizeof(uint64_t));
   i /= 10000;      // Convert 100-nanosecond units to milliseconds
   return i;
}

/**
 * Get process attribute by opening handle to process
 */
static uint64_t GetProcessAttributeFromHandle(DWORD pid, ProcessAttribute attr)
{
   HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
   if (hProcess == nullptr)
      return 0;

   uint64_t value;
   PROCESS_MEMORY_COUNTERS mc;
   IO_COUNTERS ioCounters;
   FILETIME ftCreate, ftExit, ftKernel, ftUser;
   switch (attr)
   {
      case PROCINFO_PAGE_FAULTS:
         GetProcessMemoryInfo(hProcess, &mc, sizeof(PROCESS_MEMORY_COUNTERS));
         value = mc.PageFaultCount;
         break;
      case PROCINFO_KTIME:
      case PROCINFO_UTIME:
         GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser);
         value = ConvertProcessTime(attr == PROCINFO_KTIME ? &ftKernel : &ftUser);
         break;
      case PROCINFO_CPUTIME:
         GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser);
         value = ConvertProcessTime(&ftKernel) + ConvertProcessTime(&ftUser);
         break;
      case PROCINFO_GDI_OBJ:
      case PROCINFO_USER_OBJ:
         value = GetGuiResources(hProcess, attr == PROCINFO_GDI_OBJ ? GR_GDIOBJECTS : GR_USEROBJECTS);
         break;
      case PROCINFO_IO_READ_B:
         GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.ReadTransferCount;
         break;
      case PROCINFO_IO_READ_OP:
         GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.ReadOperationCount;
         break;
      case PROCINFO_IO_WRITE_B:
         GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.WriteTransferCount;
         break;
      case PROCINFO_IO_WRITE_OP:
         GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.WriteOperationCount;
         break;
      case PROCINFO_IO_OTHER_B:
         GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.OtherTransferCount;
         break;
      case PROCINFO_IO_OTHER_OP:
         GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.OtherOperationCount;
         break;
      default:
         value = 0;
         break;
   }

   CloseHandle(hProcess);
   return value;
}

/**
 * Get specific process attribute
 */
static uint64_t GetProcessAttribute(SYSTEM_PROCESS_INFORMATION *process, uint64_t totalMemory, ProcessAttribute attr, ProcessInfoAgregationMethod aggregationMethod, int count, uint64_t lastValue)
{
   uint64_t value;

   // Get value for current process instance
   switch(attr)
   {
      case PROCINFO_VMSIZE:
         value = process->VirtualSize;
         break;
      case PROCINFO_WKSET:
         value = process->WorkingSetSize;
         break;
      case PROCINFO_MEMPERC:
         value = static_cast<uint64_t>(process->WorkingSetSize) * 10000 / totalMemory;
         break;
      case PROCINFO_HANDLES:
         value = process->HandleCount;
         break;
      case PROCINFO_THREADS:
         value = process->NumberOfThreads;
         break;
      default: // Other attributes require handle to process
         value = GetProcessAttributeFromHandle(static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(process->UniqueProcessId)), attr);
         break;
   }

   // Recalculate final value according to selected type
   if (count == 1)     // First instance
      return value;

   switch(aggregationMethod)
   {
      case ProcessInfoAgregationMethod::MIN:
         return std::min(lastValue, value);
      case ProcessInfoAgregationMethod::MAX:
         return std::max(lastValue, value);
      case ProcessInfoAgregationMethod::AVG:
         return (lastValue * (count - 1) + value) / count;
      case ProcessInfoAgregationMethod::SUM:
         return lastValue + value;
      default:
         return 0;
   }
}

/**
 * Get command line of the process, specified by process ID
 */
static bool GetProcessCommandLine(DWORD pid, TCHAR *pszCmdLine, DWORD dwLen)
{
#ifdef __64BIT__
	const DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
#else
	const DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | 
	                            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION;
#endif

	HANDLE hProcess = OpenProcess(desiredAccess, FALSE, pid);
	if (hProcess == INVALID_HANDLE_VALUE)
		return false;

   bool success = false;

#ifdef __64BIT__
	ULONG size;
   PROCESS_BASIC_INFORMATION pbi;
   NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(PROCESS_BASIC_INFORMATION), &size);
	if (status == 0)	// STATUS_SUCCESS
	{
      SIZE_T dummy;
      PEB peb;
      if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(PEB), &dummy))
		{
         RTL_USER_PROCESS_PARAMETERS pp;
         if (ReadProcessMemory(hProcess, peb.ProcessParameters, &pp, sizeof(RTL_USER_PROCESS_PARAMETERS), &dummy))
			{
				size_t bufSize = (pp.CommandLine.Length + 1) * sizeof(WCHAR);
            WCHAR *buffer = static_cast<WCHAR*>(MemAlloc(bufSize));
				if (ReadProcessMemory(hProcess, pp.CommandLine.Buffer, buffer, bufSize, &dummy))
				{
#ifdef UNICODE
					wcslcpy(pszCmdLine, buffer, dwLen);
#else
					WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, buffer, pp.CommandLine.Length, pszCmdLine, dwLen, NULL, NULL);
					pszCmdLine[dwLen - 1] = 0;
#endif
					success = true;
				}
				MemFree(buffer);
			}
		}
	}

#else
   FARPROC pfnGetCommandLine = GetProcAddress(GetModuleHandle(_T("KERNEL32.DLL")),
#ifdef UNICODE
		"GetCommandLineW");
#else
		"GetCommandLineA");
#endif
	HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)pfnGetCommandLine, 0, 0, 0);
	if (hThread != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(hThread, INFINITE);
      DWORD dwAddressOfCommandLine;
      GetExitCodeThread(hThread, &dwAddressOfCommandLine);
      SIZE_T dummy;
      success = ReadProcessMemory(hProcess, (PVOID)dwAddressOfCommandLine, pszCmdLine, dwLen * sizeof(TCHAR), &dummy);
		CloseHandle(hThread);
	}
#endif

	CloseHandle(hProcess);
	return success;
}

/**
 * Callback function for GetProcessWindows
 */
static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
   if (GetWindowLong(hWnd,GWL_STYLE) & WS_VISIBLE) 
   {
		WINDOW_LIST *pList = (WINDOW_LIST *)lParam;
		if (pList == NULL)
			return FALSE;

      DWORD pid;
      GetWindowThreadProcessId(hWnd, &pid);
		if (pid == pList->pid)
		{
         TCHAR buffer[MAX_PATH];
         GetWindowText(hWnd, buffer, MAX_PATH - 1);
			buffer[MAX_PATH - 1] = 0;
			pList->pWndList->add(buffer);
		}
	}

   return TRUE;
}

/**
 * Get list of windows belonging to given process
 */
static StringList *GetProcessWindows(DWORD pid)
{
	WINDOW_LIST list;
	list.pid = pid;
	list.pWndList = new StringList;
	EnumWindows(EnumWindowsProc, (LPARAM)&list);
	return list.pWndList;
}

/**
 * Get owner name and domain of given process by process ID
 */
static bool GetProcessOwner(DWORD pid, TCHAR* buffer, size_t bufferSize)
{
	HANDLE pHandle = OpenProcess(READ_CONTROL, FALSE, pid);
	if (pHandle != nullptr)
	{
      PSID sidUser;
      PSID sidGroup;

      DWORD error = GetSecurityInfo(pHandle, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION, &sidUser, &sidGroup, NULL, NULL, NULL);
      CloseHandle(pHandle);

      if (error == ERROR_SUCCESS)
      {
         TCHAR userName[256];
         DWORD acctNameSize = 256;
         TCHAR domainName[256];
         DWORD domainNameSize = 256;
         SID_NAME_USE use;

         if (LookupAccountSid(nullptr, sidUser, userName, &acctNameSize, domainName, &domainNameSize, &use))
         {
            return _sntprintf(buffer, bufferSize, _T("%s\\%s"), domainName, userName) < static_cast<int>(bufferSize);
         }
      }
	}
   return false;
}

/**
 * Match process to search criteria
 */
static bool MatchProcess(DWORD pid, const WCHAR *imageName, bool extendedMatch, TCHAR *moduleNamePattern, TCHAR *cmdLinePattern, TCHAR *userPattern, TCHAR *windowNamePattern)
{	
	if (!extendedMatch) 
   {
		return (imageName != nullptr) ? wcsicmp(imageName, moduleNamePattern) == 0 : false;
	}

   // command line match
   if ((cmdLinePattern != nullptr) && (cmdLinePattern[0] != 0)) // not empty, check if match
   {
		TCHAR commandLine[8192];
		memset(commandLine, 0, sizeof(commandLine));
      if (!GetProcessCommandLine(pid, commandLine, 8192) || !RegexpMatch(commandLine, cmdLinePattern, false))
         return false;
	}

   // process name match
   if ((moduleNamePattern != nullptr) && (moduleNamePattern[0] != 0))
   {	
      if ((imageName == nullptr) || !RegexpMatch(imageName, moduleNamePattern, false))
         return false;
	}

   // user name match
   if ((userPattern != nullptr) && (userPattern[0] != 0))
   {
		TCHAR userName[256];
      if (!GetProcessOwner(pid, userName, sizeof(userName) / sizeof(TCHAR)) || !RegexpMatch(userName, userPattern, false))
         return false;
   }

   // window name match
   if ((windowNamePattern != nullptr) && (windowNamePattern[0] != 0))
   {
		bool windowMatch = false;
		StringList *windowList = GetProcessWindows(pid);
		for(int i = 0; i < windowList->size(); i++)
		{
			if (RegexpMatch(windowList->get(i), windowNamePattern, false))
			{
				windowMatch = true;
				break;
			}
		}
		delete windowList;
      return windowMatch; //Change this when adding new filters
   }

	return true;
}

/**
 * Get process-specific information
 * Parameter has the following syntax:
 *    Process.XXX(<process>,<type>,<cmdline>,<user>,<window>)
 * where
 *    XXX        - requested process attribute (see documentation for list of valid attributes)
 *    <process>  - process name (same as in Process.Count() parameter)
 *    <type>     - representation type (meaningful when more than one process with the same
 *                 name exists). Valid values are:
 *         min - minimal value among all processes named <process>
 *         max - maximal value among all processes named <process>
 *         avg - average value for all processes named <process>
 *         sum - sum of values for all processes named <process>
 *    <cmdline>  - command line
 *    <window>   - window title
 */
LONG H_ProcInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR buffer[256], procName[MAX_PATH], cmdLine[MAX_PATH], user[MAX_PATH], windowTitle[MAX_PATH];

   // Get parameter type arguments
   AgentGetParameterArg(cmd, 2, buffer, 255);
   ProcessInfoAgregationMethod aggregationMethod;
   if (buffer[0] == 0)     // Omited type
   {
      aggregationMethod = ProcessInfoAgregationMethod::SUM;
   }
   else
   {
      static TCHAR *typeList[] = { _T("min"), _T("max"), _T("avg"), _T("sum"), nullptr };
      static ProcessInfoAgregationMethod methods[] = { ProcessInfoAgregationMethod::MIN, ProcessInfoAgregationMethod::MAX, ProcessInfoAgregationMethod::AVG, ProcessInfoAgregationMethod::SUM };
      
      int i;
      for(i = 0; typeList[i] != nullptr; i++)
         if (!_tcsicmp(typeList[i], buffer))
            break;
      if (typeList[i] == nullptr)
         return SYSINFO_RC_UNSUPPORTED;     // Unsupported method

      aggregationMethod = methods[i];
   }
   // Get process name
   AgentGetParameterArg(cmd, 1, procName, MAX_PATH - 1);
   AgentGetParameterArg(cmd, 3, cmdLine, MAX_PATH - 1);
   AgentGetParameterArg(cmd, 4, user, MAX_PATH - 1);
   AgentGetParameterArg(cmd, 5, windowTitle, MAX_PATH - 1);

   ULONG allocated = 1024 * 1024;  // 1MB by default
   void *processInfoBuffer = MemAlloc(allocated);
   ULONG bytes;
   NTSTATUS rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   while (rc == STATUS_INFO_LENGTH_MISMATCH)
   {
      allocated += 10254 * 1024;
      processInfoBuffer = MemRealloc(processInfoBuffer, allocated);
      rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   }
   if (rc != STATUS_SUCCESS)
   {
      MemFree(processInfoBuffer);
      return SYSINFO_RC_ERROR;
   }

   MEMORYSTATUSEX mse;
   mse.dwLength = sizeof(MEMORYSTATUSEX);
   if (!GlobalMemoryStatusEx(&mse))
   {
      MemFree(processInfoBuffer);
      return SYSINFO_RC_ERROR;
   }

   // Gather information
   ProcessAttribute attribute = CAST_FROM_POINTER(arg, ProcessAttribute);
   uint64_t attributeValue = 0;
   int counter = 0;

   SYSTEM_PROCESS_INFORMATION *process = static_cast<SYSTEM_PROCESS_INFORMATION*>(processInfoBuffer);
   do
   {
      if (MatchProcess(static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(process->UniqueProcessId)), process->ImageName.Buffer,
          (cmdLine[0] != 0) || (user[0] != 0) || (windowTitle[0] != 0), procName, cmdLine, user, windowTitle))
      {
         counter++;  // Number of processes with specific name
         attributeValue = GetProcessAttribute(process, mse.ullTotalPhys, attribute, aggregationMethod, counter, attributeValue);
      }
      process = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(reinterpret_cast<char*>(process) + process->NextEntryOffset);
   } while (process->NextEntryOffset != 0);

   MemFree(processInfoBuffer);

   if (counter == 0)    // No processes with given name
      return SYSINFO_RC_ERROR;

   if (attribute == PROCINFO_MEMPERC)
      ret_double(value, static_cast<double>(attributeValue) / 100.0, 2);
   else
      ret_uint64(value, attributeValue);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ProcessCount
 */
LONG H_ProcCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   PERFORMANCE_INFORMATION pi;
   pi.cb = sizeof(PERFORMANCE_INFORMATION);
   if (!GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
      return SYSINFO_RC_ERROR;
   ret_uint(value, pi.ProcessCount);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Process.Count(*) and Process.CountEx(*)
 */
LONG H_ProcCountSpecific(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR procName[MAX_PATH], cmdLine[MAX_PATH], user[MAX_PATH], windowTitle[MAX_PATH];

   AgentGetParameterArg(cmd, 1, procName, MAX_PATH - 1);
	if (*arg == 'E')
	{
		AgentGetParameterArg(cmd, 2, cmdLine, MAX_PATH - 1);
		AgentGetParameterArg(cmd, 3, user, MAX_PATH - 1);
		AgentGetParameterArg(cmd, 4, windowTitle, MAX_PATH - 1);

		// Check if all 4 parameters are empty
		if ((procName[0] == 0) && (cmdLine[0] == 0) && (user[0] == 0) && (windowTitle[0] == 0))
			return SYSINFO_RC_UNSUPPORTED;
	}

   ULONG allocated = 1024 * 1024;  // 1MB by default
   void *processInfoBuffer = MemAlloc(allocated);
   ULONG bytes;
   NTSTATUS rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   while (rc == STATUS_INFO_LENGTH_MISMATCH)
   {
      allocated += 10254 * 1024;
      processInfoBuffer = MemRealloc(processInfoBuffer, allocated);
      rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   }
   if (rc != STATUS_SUCCESS)
   {
      MemFree(processInfoBuffer);
      return SYSINFO_RC_ERROR;
   }

   int count = 0;

   SYSTEM_PROCESS_INFORMATION *process = static_cast<SYSTEM_PROCESS_INFORMATION*>(processInfoBuffer);
   do
   {
	   if (MatchProcess(static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(process->UniqueProcessId)), process->ImageName.Buffer, *arg == 'E', procName, cmdLine, user, windowTitle))
	   {
		   count++;        
	   }
      process = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(reinterpret_cast<char*>(process) + process->NextEntryOffset);
   } while (process->NextEntryOffset != 0);

   MemFree(processInfoBuffer);
   ret_int(value, count);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ProcessList list
 */
LONG H_ProcessList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   ULONG allocated = 1024 * 1024;  // 1MB by default
   void *processInfoBuffer = MemAlloc(allocated);
   ULONG bytes;
   NTSTATUS rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   while (rc == STATUS_INFO_LENGTH_MISMATCH)
   {
      allocated += 10254 * 1024;
      processInfoBuffer = MemRealloc(processInfoBuffer, allocated);
      rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   }
   if (rc != STATUS_SUCCESS)
   {
      MemFree(processInfoBuffer);
      return SYSINFO_RC_ERROR;
   }

   WCHAR buffer[4096];
   SYSTEM_PROCESS_INFORMATION *process = static_cast<SYSTEM_PROCESS_INFORMATION*>(processInfoBuffer);
   do
   {
      uint32_t pid = static_cast<uint32_t>(reinterpret_cast<ULONG_PTR>(process->UniqueProcessId));
      if (*arg == '2')
      {
         TCHAR cmdLine[4096];
         if (GetProcessCommandLine(pid, cmdLine, 4096))
         {
            snwprintf(buffer, 4096, L"%u %s", pid, (cmdLine[0] != 0) ? cmdLine : process->ImageName.Buffer);
         }
         else if (process->ImageName.Buffer != nullptr)
         {
            snwprintf(buffer, 4096, L"%u %s", pid, process->ImageName.Buffer);
         }
         else
         {
            snwprintf(buffer, 4096, L"%u System Idle Process", pid);
         }
      }
      else
      {
         snwprintf(buffer, 4096, L"%u %s", pid, (process->ImageName.Buffer == nullptr) ? L"System Idle Process" : process->ImageName.Buffer);
      }
      value->add(buffer);
      process = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(reinterpret_cast<char*>(process) + process->NextEntryOffset);
   } while (process->NextEntryOffset != 0);

   MemFree(processInfoBuffer);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Processes table
 */
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *)
{
   ULONG allocated = 1024 * 1024;  // 1MB by default
   void *processInfoBuffer = MemAlloc(allocated);
   ULONG bytes;
   NTSTATUS rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   while (rc == STATUS_INFO_LENGTH_MISMATCH)
   {
      allocated += 10254 * 1024;
      processInfoBuffer = MemRealloc(processInfoBuffer, allocated);
      rc = NtQuerySystemInformation(SystemProcessInformation, processInfoBuffer, allocated, &bytes);
   }
   if (rc != STATUS_SUCCESS)
   {
      MemFree(processInfoBuffer);
      return SYSINFO_RC_ERROR;
   }

   MEMORYSTATUSEX mse;
   mse.dwLength = sizeof(MEMORYSTATUSEX);
   if (!GlobalMemoryStatusEx(&mse))
   {
      MemFree(processInfoBuffer);
      return SYSINFO_RC_ERROR;
   }

   value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"), true);
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
   value->addColumn(_T("USER"), DCI_DT_STRING, _T("User"));
   value->addColumn(_T("THREADS"), DCI_DT_UINT, _T("Threads"));
   value->addColumn(_T("HANDLES"), DCI_DT_UINT, _T("Handles"));
   value->addColumn(_T("KTIME"), DCI_DT_UINT64, _T("Kernel Time"));
   value->addColumn(_T("UTIME"), DCI_DT_UINT64, _T("User Time"));
   value->addColumn(_T("VMSIZE"), DCI_DT_UINT64, _T("VM Size"));
   value->addColumn(_T("RSS"), DCI_DT_UINT64, _T("RSS"));
   value->addColumn(_T("MEMORY_USAGE"), DCI_DT_FLOAT, _T("memory Usage"));
   value->addColumn(_T("PAGE_FAULTS"), DCI_DT_UINT64, _T("Page Faults"));
   value->addColumn(_T("CMDLINE"), DCI_DT_STRING, _T("Command Line"));

   SYSTEM_PROCESS_INFORMATION *process = static_cast<SYSTEM_PROCESS_INFORMATION*>(processInfoBuffer);
   do
   {
      DWORD pid = static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(process->UniqueProcessId));

		value->addRow();
		value->set(0, static_cast<uint32_t>(pid));
      value->set(1, (process->ImageName.Buffer == nullptr) ? L"System Idle Process" : process->ImageName.Buffer);
      value->set(3, static_cast<uint32_t>(process->NumberOfThreads));
      value->set(4, static_cast<uint32_t>(process->HandleCount));
      value->set(7, static_cast<uint64_t>(process->VirtualSize));
      value->set(8, static_cast<uint64_t>(process->WorkingSetSize));
      value->set(9, static_cast<double>(static_cast<uint64_t>(process->WorkingSetSize) * 10000 / mse.ullTotalPhys) / 100.0, 2);

      HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
      if (hProcess != nullptr)
      {
         FILETIME ftCreate, ftExit, ftKernel, ftUser;
         if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser))
         {
            value->set(5, ConvertProcessTime(&ftKernel));
            value->set(6, ConvertProcessTime(&ftUser));
         }

         PROCESS_MEMORY_COUNTERS mc;
         if (GetProcessMemoryInfo(hProcess, &mc, sizeof(PROCESS_MEMORY_COUNTERS)))
         {
            value->set(10, static_cast<uint32_t>(mc.PageFaultCount));
         }

         CloseHandle(hProcess);
      }

      // Get user
      TCHAR userName[256];
      if (GetProcessOwner(pid, userName, 256))
      {
         value->set(2, userName);
      }

      // Get command line
      TCHAR cmdLine[4096];
      if (GetProcessCommandLine(pid, cmdLine, 4096))
      {
         value->set(11, cmdLine);
      }

      process = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(reinterpret_cast<char*>(process) + process->NextEntryOffset);
   } while (process->NextEntryOffset != 0);

   MemFree(processInfoBuffer);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Process.Terminate action
 */
uint32_t H_TerminateProcess(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   DWORD pid = _tcstoul(context->getArg(0), nullptr, 0);
   if (pid == 0)
      return ERR_BAD_ARGUMENTS;

   HANDLE hp = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
   if (hp == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_TerminateProcess: call to OpenProcess failed for PID %u (%s)"), pid, GetSystemErrorText(GetLastError()).cstr());
      return ERR_SYSCALL_FAILED;
   }
 
   BOOL success = TerminateProcess(hp, 127);
   if (success)
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_TerminateProcess: call to TerminateProcess for PID %u successfull"), pid);
   else
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_TerminateProcess: call to TerminateProcess for PID %u failed (%s)"), pid, GetSystemErrorText(GetLastError()).cstr());
   CloseHandle(hp);
   return success ? ERR_SUCCESS : ERR_SYSCALL_FAILED;
}
