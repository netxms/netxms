/* 
** Windows NT+ NetXMS subagent
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

#include "winnt_subagent.h"
#include <winternl.h>

/**
 * Convert process time from FILETIME structure (100-nanosecond units) to __uint64 (milliseconds)
 */
static unsigned __int64 ConvertProcessTime(FILETIME *lpft)
{
   unsigned __int64 i;

   memcpy(&i, lpft, sizeof(unsigned __int64));
   i /= 10000;      // Convert 100-nanosecond units to milliseconds
   return i;
}

/**
 * Get specific process attribute
 */
static unsigned __int64 GetProcessAttribute(HANDLE hProcess, int attr, int type, int count,
                                            unsigned __int64 lastValue)
{
   unsigned __int64 value;  
   PROCESS_MEMORY_COUNTERS mc;
   IO_COUNTERS ioCounters;
   FILETIME ftCreate, ftExit, ftKernel, ftUser;
   DWORD handles;

   // Get value for current process instance
   switch(attr)
   {
      case PROCINFO_VMSIZE:
         GetProcessMemoryInfo(hProcess, &mc, sizeof(PROCESS_MEMORY_COUNTERS));
         value = mc.PagefileUsage;
         break;
      case PROCINFO_WKSET:
         GetProcessMemoryInfo(hProcess, &mc, sizeof(PROCESS_MEMORY_COUNTERS));
         value = mc.WorkingSetSize;
         break;
      case PROCINFO_PF:
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
      case PROCINFO_HANDLES:
         if (GetProcessHandleCount(hProcess, &handles))
            value = handles;
         else
            value = 0;
         break;
      default:       // Unknown attribute
         AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("GetProcessAttribute(): Unexpected attribute: 0x%02X"), attr);
         value = 0;
         break;
   }

   // Recalculate final value according to selected type
   if (count == 1)     // First instance
   {
      return value;
   }

   switch(type)
   {
      case INFOTYPE_MIN:
         return std::min(lastValue, value);
      case INFOTYPE_MAX:
         return std::max(lastValue, value);
      case INFOTYPE_AVG:
         return (lastValue * (count - 1) + value) / count;
      case INFOTYPE_SUM:
         return lastValue + value;
      default:
         AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("GetProcessAttribute(): Unexpected type: 0x%02X"), type);
         return 0;
   }
}

/**
 * Get command line of the process, specified by process ID
 */
static BOOL GetProcessCommandLine(DWORD pid, TCHAR *pszCmdLine, DWORD dwLen)
{
	SIZE_T dummy;
	BOOL bRet = FALSE;
#ifdef __64BIT__
	const DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
#else
	const DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | 
	                            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION;
#endif

	HANDLE hProcess = OpenProcess(desiredAccess, FALSE, pid);
	if(hProcess == INVALID_HANDLE_VALUE)
		return FALSE;

#ifdef __64BIT__
	PROCESS_BASIC_INFORMATION pbi;
	PEB peb;
	RTL_USER_PROCESS_PARAMETERS pp;
	WCHAR *buffer;
	NTSTATUS status;
	ULONG size;
	
	status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(PROCESS_BASIC_INFORMATION), &size);
	if (status == 0)	// STATUS_SUCCESS
	{
		if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(PEB), &dummy))
		{
			if (ReadProcessMemory(hProcess, peb.ProcessParameters, &pp, sizeof(RTL_USER_PROCESS_PARAMETERS), &dummy))
			{
				size_t bufSize = (pp.CommandLine.Length + 1) * sizeof(WCHAR);
				buffer = (WCHAR *)malloc(bufSize);
				if (ReadProcessMemory(hProcess, pp.CommandLine.Buffer, buffer, bufSize, &dummy))
				{
#ifdef UNICODE
					nx_strncpy(pszCmdLine, buffer, dwLen);
#else
					WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, buffer, pp.CommandLine.Length, pszCmdLine, dwLen, NULL, NULL);
					pszCmdLine[dwLen - 1] = 0;
#endif
					bRet = TRUE;
				}
				free(buffer);
			}
		}
	}

#else
	DWORD dwAddressOfCommandLine;
	FARPROC pfnGetCommandLine;
	HANDLE hThread;

	pfnGetCommandLine = GetProcAddress(GetModuleHandle(_T("KERNEL32.DLL")), 
#ifdef UNICODE
		"GetCommandLineW");
#else
		"GetCommandLineA");
#endif

	hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)pfnGetCommandLine, 0, 0, 0);
	if (hThread != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(hThread, INFINITE);
		GetExitCodeThread(hThread, &dwAddressOfCommandLine);
		bRet = ReadProcessMemory(hProcess, (PVOID)dwAddressOfCommandLine, pszCmdLine, dwLen * sizeof(TCHAR), &dummy);
		CloseHandle(hThread);
	}
#endif

	CloseHandle(hProcess);
	return bRet;
}

/**
 * Callback function for GetProcessWindows
 */
static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
   DWORD pid;
	TCHAR szBuffer[MAX_PATH];

   if (GetWindowLong(hWnd,GWL_STYLE) & WS_VISIBLE) 
   {
		WINDOW_LIST *pList = (WINDOW_LIST *)lParam;
		if (pList == NULL)
			return FALSE;

		GetWindowThreadProcessId(hWnd, &pid);
		if (pid == pList->pid)
		{
			GetWindowText(hWnd, szBuffer, MAX_PATH - 1);
			szBuffer[MAX_PATH - 1] = 0;
			pList->pWndList->add(szBuffer);
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
 * Match process to search criteria
 */
static BOOL MatchProcess(DWORD pid, HANDLE hProcess, HMODULE hModule, BOOL bExtMatch,
								 TCHAR *pszModName, TCHAR *pszCmdLine, TCHAR *pszWindowName)
{
   TCHAR szBaseName[MAX_PATH];
	int i;
	BOOL bRet;

   GetModuleBaseName(hProcess, hModule, szBaseName, MAX_PATH);
	if (bExtMatch)	// Extended version
	{
		TCHAR commandLine[8192];
		BOOL bProcMatch, bCmdMatch, bWindowMatch;

		bProcMatch = bCmdMatch = bWindowMatch = TRUE;

		if (pszCmdLine[0] != 0)		// not empty, check if match
		{
			memset(commandLine, 0, sizeof(commandLine));	
			GetProcessCommandLine(pid, commandLine, 8192);
			bCmdMatch = RegexpMatch(commandLine, pszCmdLine, FALSE);
		}

		if (pszModName[0] != 0)
		{
			bProcMatch = RegexpMatch(szBaseName, pszModName, FALSE);
		}

		if (pszWindowName[0] != 0)
		{
			StringList *pWndList;

			pWndList = GetProcessWindows(pid);
			for(i = 0, bWindowMatch = FALSE; i < pWndList->size(); i++)
			{
				if (RegexpMatch(pWndList->get(i), pszWindowName, FALSE))
				{
					bWindowMatch = TRUE;
					break;
				}
			}
			delete pWndList;
		}

	   bRet = bProcMatch && bCmdMatch && bWindowMatch;
	}
	else
	{
		bRet = !_tcsicmp(szBaseName, pszModName);
	}
	return bRet;
}

/**
 * Get process-specific information
 * Parameter has the following syntax:
 *    Process.XXX(<process>,<type>,<cmdline>,<window>)
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
   TCHAR buffer[256], procName[MAX_PATH], cmdLine[MAX_PATH], windowTitle[MAX_PATH];
   int attr, type, i, procCount, counter;
   unsigned __int64 attrVal;
   DWORD *pdwProcList, dwSize;
   HMODULE *modList;
   static TCHAR *typeList[]={ _T("min"), _T("max"), _T("avg"), _T("sum"), NULL };

   // Get parameter type arguments
   AgentGetParameterArg(cmd, 2, buffer, 255);
   if (buffer[0] == 0)     // Omited type
   {
      type = INFOTYPE_SUM;
   }
   else
   {
      for(type = 0; typeList[type] != NULL; type++)
         if (!_tcsicmp(typeList[type], buffer))
            break;
      if (typeList[type] == NULL)
         return SYSINFO_RC_UNSUPPORTED;     // Unsupported type
   }

   // Get process name
   AgentGetParameterArg(cmd, 1, procName, MAX_PATH - 1);
	AgentGetParameterArg(cmd, 3, cmdLine, MAX_PATH - 1);
	AgentGetParameterArg(cmd, 4, windowTitle, MAX_PATH - 1);

   // Gather information
   attr = CAST_FROM_POINTER(arg, int);
   attrVal = 0;
   pdwProcList = (DWORD *)malloc(MAX_PROCESSES * sizeof(DWORD));
   modList = (HMODULE *)malloc(MAX_MODULES * sizeof(HMODULE));
   EnumProcesses(pdwProcList, sizeof(DWORD) * MAX_PROCESSES, &dwSize);
   procCount = dwSize / sizeof(DWORD);
   for(i = 0, counter = 0; i < procCount; i++)
   {
      HANDLE hProcess;

      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pdwProcList[i]);
      if (hProcess != NULL)
      {
         if (EnumProcessModules(hProcess, modList, sizeof(HMODULE) * MAX_MODULES, &dwSize))
         {
            if (dwSize >= sizeof(HMODULE))     // At least one module exist
            {
					if (MatchProcess(pdwProcList[i], hProcess, modList[0],
					                 (cmdLine[0] != 0) || (windowTitle[0] != 0),
					                 procName, cmdLine, windowTitle))
					{
                  counter++;  // Number of processes with specific name
                  attrVal = GetProcessAttribute(hProcess, attr, type, counter, attrVal);
					}
            }
         }
         CloseHandle(hProcess);
      }
   }

   // Cleanup
   free(pdwProcList);
   free(modList);

   if (counter == 0)    // No processes with given name
      return SYSINFO_RC_ERROR;

   ret_uint64(value, attrVal);
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
   TCHAR procName[MAX_PATH], cmdLine[MAX_PATH], windowTitle[MAX_PATH];

   AgentGetParameterArg(cmd, 1, procName, MAX_PATH - 1);
	if (*arg == 'E')
	{
		AgentGetParameterArg(cmd, 2, cmdLine, MAX_PATH - 1);
		AgentGetParameterArg(cmd, 3, windowTitle, MAX_PATH - 1);

		// Check if all 3 parameters are empty
		if ((procName[0] == 0) && (cmdLine[0] == 0) && (windowTitle[0] == 0))
			return SYSINFO_RC_UNSUPPORTED;
	}

   DWORD *procList = (DWORD *)malloc(sizeof(DWORD) * MAX_PROCESSES);
   DWORD size = 0;
   EnumProcesses(procList, sizeof(DWORD) * MAX_PROCESSES, &size);
   int procCount = (int)(size / sizeof(DWORD));
   int count = 0;
   for(int i = 0; i < procCount; i++)
   {
      HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procList[i]);
      if (hProcess != NULL)
      {
         HMODULE modList[MAX_MODULES];
         if (EnumProcessModules(hProcess, modList, sizeof(HMODULE) * MAX_MODULES, &size))
         {
            if (size >= sizeof(HMODULE))     // At least one module exist
            {
					if (MatchProcess(procList[i], hProcess, modList[0], *arg == 'E',
					                 procName, cmdLine, windowTitle))
						count++;
				}
         }
         CloseHandle(hProcess);
      }
   }
   ret_int(value, count);
   free(procList);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ProcessList list
 */
LONG H_ProcessList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   DWORD i, dwSize, dwNumProc, *pdwProcList;
   LONG iResult = SYSINFO_RC_SUCCESS;
   TCHAR szBuffer[MAX_PATH + 64];
   HMODULE phModList[MAX_MODULES];
   HANDLE hProcess;

   pdwProcList = (DWORD *)malloc(sizeof(DWORD) * MAX_PROCESSES);
   if (EnumProcesses(pdwProcList, sizeof(DWORD) * MAX_PROCESSES, &dwSize))
   {
      dwNumProc = dwSize / sizeof(DWORD);
      for(i = 0; i < dwNumProc; i++)
      {
         hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pdwProcList[i]);
         if (hProcess != NULL)
         {
            if (EnumProcessModules(hProcess, phModList, sizeof(HMODULE) * MAX_MODULES, &dwSize))
            {
               if (dwSize >= sizeof(HMODULE))     // At least one module exist
               {
                  TCHAR szBaseName[MAX_PATH];

                  GetModuleBaseName(hProcess, phModList[0], szBaseName, MAX_PATH);
                  _sntprintf(szBuffer, MAX_PATH + 64, _T("%u %s"), pdwProcList[i], szBaseName);
						value->add(szBuffer);
               }
               else
               {
                  _sntprintf(szBuffer, MAX_PATH + 64, _T("%u <unknown>"), pdwProcList[i]);
                  value->add(szBuffer);
               }
            }
            else
            {
               if (pdwProcList[i] == 4)
               {
                  value->add(_T("4 System"));
               }
               else
               {
                  _sntprintf(szBuffer, MAX_PATH + 64, _T("%u <unknown>"), pdwProcList[i]);
                  value->add(szBuffer);
               }
            }
            CloseHandle(hProcess);
         }
         else
         {
            if (pdwProcList[i] == 0)
            {
               value->add(_T("0 System Idle Process"));
            }
            else
            {
               _sntprintf(szBuffer, MAX_PATH + 64, _T("%lu <unaccessible>"), pdwProcList[i]);
               value->add(szBuffer);
            }
         }
      }
   }
   else
   {
      iResult = SYSINFO_RC_ERROR;
   }

   free(pdwProcList);
   return iResult;
}

/**
 * Handler for System.Processes table
 */
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *)
{
   DWORD i, dwSize, dwNumProc, *pdwProcList;
   LONG iResult = SYSINFO_RC_SUCCESS;
   HMODULE phModList[MAX_MODULES];
   HANDLE hProcess;

   value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"), true);
	value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
	value->addColumn(_T("CMDLINE"), DCI_DT_STRING, _T("Command Line"));

   pdwProcList = (DWORD *)malloc(sizeof(DWORD) * MAX_PROCESSES);
   if (EnumProcesses(pdwProcList, sizeof(DWORD) * MAX_PROCESSES, &dwSize))
   {
      dwNumProc = dwSize / sizeof(DWORD);
      for(i = 0; i < dwNumProc; i++)
      {
			value->addRow();
			value->set(0, (UINT32)pdwProcList[i]);    // PID

         // Get process name (executable name)
         hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pdwProcList[i]);
         if (hProcess != NULL)
         {
            if (EnumProcessModules(hProcess, phModList, sizeof(HMODULE) * MAX_MODULES, &dwSize))
            {
               if (dwSize >= sizeof(HMODULE))     // At least one module exist
               {
                  TCHAR szBaseName[MAX_PATH];

                  GetModuleBaseName(hProcess, phModList[0], szBaseName, MAX_PATH);
						value->set(1, szBaseName);
               }
               else
               {
						value->set(1, _T("<unknown>"));
               }
            }
            else
            {
               if (pdwProcList[i] == 4)
               {
						value->set(1, _T("System"));
               }
               else
               {
						value->set(1, _T("<unknown>"));
               }
            }
            CloseHandle(hProcess);
         }
         else
         {
            if (pdwProcList[i] == 0)
            {
					value->set(1, _T("System Idle Process"));
            }
            else
            {
					value->set(1, _T("<unaccessible>"));
            }
         }

         // Get command line
         TCHAR cmdLine[1024];
         if (GetProcessCommandLine(pdwProcList[i], cmdLine, 1024))
         {
            value->set(2, cmdLine);
         }
      }
   }
   else
   {
      iResult = SYSINFO_RC_ERROR;
   }

   free(pdwProcList);
   return iResult;
}
