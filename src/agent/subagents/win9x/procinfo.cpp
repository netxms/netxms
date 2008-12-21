/* 
** Windows 95/98/Me NetXMS subagent
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: procinfo.cpp
** Win32 specific process information parameters
**
**/

#include "win9x_subagent.h"


//
// Handler for System.ProcessCount
//

LONG H_ProcCount(const char *pszCmd, const char *pArg, char *pValue)
{
   HANDLE hSnapshot;
   PROCESSENTRY32 pe;
   LONG nCount = 0, nRet = SYSINFO_RC_SUCCESS;

   hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (hSnapshot != INVALID_HANDLE_VALUE)
   {
      pe.dwSize = sizeof(PROCESSENTRY32);
      if (Process32First(hSnapshot, &pe))
      {
         do
         {
            nCount++;
            pe.dwSize = sizeof(PROCESSENTRY32);
         } while(Process32Next(hSnapshot, &pe));
      }
      CloseHandle(hSnapshot);
      ret_int(pValue, nCount);
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}


//
// Handler for Process.Count(*)
//

LONG H_ProcCountSpecific(const char *pszCmd, const char *pArg, char *pValue)
{
   HANDLE hSnapshot;
   PROCESSENTRY32 pe;
   TCHAR *pszFileName, szProcName[MAX_PATH];
   LONG nCount = 0, nRet = SYSINFO_RC_SUCCESS;

   if (!NxGetParameterArg(pszCmd, 1, szProcName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (hSnapshot != INVALID_HANDLE_VALUE)
   {
      pe.dwSize = sizeof(PROCESSENTRY32);
      if (Process32First(hSnapshot, &pe))
      {
         do
         {
            pszFileName = _tcsrchr(pe.szExeFile, _T('\\'));
            if (pszFileName != NULL)
               pszFileName++;
            else
               pszFileName = pe.szExeFile;
            if (!_tcsicmp(szProcName, pszFileName))
               nCount++;
            pe.dwSize = sizeof(PROCESSENTRY32);
         } while(Process32Next(hSnapshot, &pe));
      }
      CloseHandle(hSnapshot);
      ret_int(pValue, nCount);
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}


//
// Handler for System.ProcessList enum
//

LONG H_ProcessList(const char *pszCmd, const char *pArg, NETXMS_VALUES_LIST *pValue)
{
   HANDLE hSnapshot;
   PROCESSENTRY32 pe;
   TCHAR *pszFileName, szBuffer[MAX_PATH + 64];
   LONG nRet = SYSINFO_RC_SUCCESS;

   hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (hSnapshot != INVALID_HANDLE_VALUE)
   {
      pe.dwSize = sizeof(PROCESSENTRY32);
      if (Process32First(hSnapshot, &pe))
      {
         do
         {
            pszFileName = _tcsrchr(pe.szExeFile, _T('\\'));
            if (pszFileName != NULL)
               pszFileName++;
            else
               pszFileName = pe.szExeFile;
            _sntprintf(szBuffer, MAX_PATH + 64, _T("%u %s"), pe.th32ProcessID, pszFileName);
            NxAddResultString(pValue, szBuffer);
            pe.dwSize = sizeof(PROCESSENTRY32);
         } while(Process32Next(hSnapshot, &pe));
      }
      CloseHandle(hSnapshot);
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}


//
// Handler for System.ThreadCount
//

LONG H_ThreadCount(const char *pszCmd, const char *pArg, char *pValue)
{
   HANDLE hSnapshot;
   PROCESSENTRY32 pe;
   LONG nCount = 0, nRet = SYSINFO_RC_SUCCESS;

   hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
   if (hSnapshot != INVALID_HANDLE_VALUE)
   {
      pe.dwSize = sizeof(PROCESSENTRY32);
      if (Process32First(hSnapshot, &pe))
      {
         do
         {
            nCount += pe.cntThreads;
            pe.dwSize = sizeof(PROCESSENTRY32);
         } while(Process32Next(hSnapshot, &pe));
      }
      CloseHandle(hSnapshot);
      ret_int(pValue, nCount);
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}
