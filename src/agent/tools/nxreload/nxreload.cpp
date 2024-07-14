/* 
** nxreload - helper tool for restarting session agents
** Copyright (C) 2006-2024 Raden Solutions
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

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <Psapi.h>
#include <strsafe.h>
#include <Shlobj.h>

#define MAX_PROCESSES   4096

/**
 * Log file
 */
static TCHAR s_logFile[MAX_PATH];

/**
 * Write debug output
 */
static void DebugPrintf(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   if (s_logFile[0] != 0)
   {
      FILE *fp = _tfopen(s_logFile, _T("a"));
      if (fp != nullptr)
      {
         va_list logArgs;
         va_copy(logArgs, args);
         _vftprintf(fp, format, logArgs);
         va_end(logArgs);
         fclose(fp);
      }
   }
   _vtprintf(format, args);
   va_end(args);
}

/**
 * Wait for reload flag and run command
 */
static void WaitForReloadAndRun()
{
   DebugPrintf(_T("Waiting for reload flag\n"));

   int retryCount = 3600;
   DWORD flag = 0;
   do
   {
      Sleep(1000);

      HKEY hKey;
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Agent"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
      {
         DWORD size = sizeof(DWORD);
         RegQueryValueEx(hKey, _T("ReloadFlag"), nullptr, nullptr, (BYTE *)&flag, &size);
         RegCloseKey(hKey);
      }
   } while ((flag == 0) && (retryCount-- > 0));

   DebugPrintf(_T("Reload flag set to %d\n"), flag);
   if (flag)
   {
      TCHAR *cmdLine = GetCommandLine();
      TCHAR *start = _tcsstr(cmdLine, _T("-- "));
      if (start != nullptr)
      {
         start += 3;
         DebugPrintf(_T("Starting: %s\n"), start);

         STARTUPINFO si;
         memset(&si, 0, sizeof(STARTUPINFO));
         si.cb = sizeof(STARTUPINFO);

         PROCESS_INFORMATION pi;
         memset(&pi, 0, sizeof(PROCESS_INFORMATION));
         if (CreateProcess(NULL, start, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
         {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            DebugPrintf(_T("Process created\n"));
         }
         else
         {
            DebugPrintf(_T("Create process failed\n"));
         }
      }
   }
}

/**
 * Count nxreload.exe processes
 */
static int GetProcessCount()
{
   DWORD pid = GetCurrentProcessId();
   DWORD *procList = (DWORD *)malloc(sizeof(DWORD) * MAX_PROCESSES);
   DWORD size = 0;
   EnumProcesses(procList, sizeof(DWORD) * MAX_PROCESSES, &size);
   int procCount = (int)(size / sizeof(DWORD));
   int count = 0;
   for (int i = 0; i < procCount; i++)
   {
      if (procList[i] == pid)
         continue;   // ignore self

      HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procList[i]);
      if (hProcess != NULL)
      {
         TCHAR image[MAX_PATH];
         if (GetProcessImageFileName(hProcess, image, MAX_PATH) != 0)
         {
            TCHAR *exe = _tcsrchr(image, _T('\\'));
            if (exe != NULL)
               exe++;
            else
               exe = image;
            if (!_tcsicmp(exe, _T("nxreload.exe")))
               count++;
         }
         CloseHandle(hProcess);
      }
   }
   free(procList);
   DebugPrintf(_T("%d running processes\n"), count);
   return count;
}

/**
 * Set completion flag and wait for all outstanding reload requests to complete
 */
static void FinishReload()
{
   HKEY hKey;
   if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Agent"), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
   {
      DWORD flag = 1;
      RegSetValueEx(hKey, _T("ReloadFlag"), 0, REG_DWORD, (BYTE *)&flag, sizeof(DWORD));
      RegCloseKey(hKey);
   }
   else
   {
      DebugPrintf(_T("RegCreateKeyEx failed\n"));
   }

   DebugPrintf(_T("Waiting for outstanding requests\n"));
   while(GetProcessCount() > 0)
      Sleep(2000);
   DebugPrintf(_T("All reload requests completed"));
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   if (SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, s_logFile) == S_OK)
   {
      size_t l = _tcslen(s_logFile);
      _sntprintf(&s_logFile[l], MAX_PATH - l, _T("\\nxreload.%u.log"), GetCurrentProcessId());
   }
   else
   {
      s_logFile[0] = 0;
   }

   DebugPrintf(_T("Command line: %s\n"), GetCommandLine());

   if ((argc > 1) && !strcmp(argv[1], "-finish"))
      FinishReload();
   else
      WaitForReloadAndRun();
	return 0;
}
