/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: sysinfo.cpp
**
**/

#include "nxagentd.h"

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif


//
// Handler for Agent.Uptime parameter
//

LONG H_AgentUptime(char *cmd, char *arg, char *value)
{
   ret_uint(value, (DWORD)(time(NULL) - g_tmAgentStartTime));
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for File.Size(*) parameter
//

LONG H_FileSize(char *cmd, char *arg, char *value)
{
   char szFileName[MAX_PATH];
#ifdef _WIN32
   HANDLE hFind;
   WIN32_FIND_DATA fd;
#else
   struct stat fileInfo;
#endif

   if (!NxGetParameterArg(cmd, 1, szFileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

#ifdef _WIN32
   hFind = FindFirstFile(szFileName, &fd);
   if (hFind == INVALID_HANDLE_VALUE)
      return SYSINFO_RC_UNSUPPORTED;
   FindClose(hFind);

   ret_uint64(value, (unsigned __int64)fd.nFileSizeLow + ((unsigned __int64)fd.nFileSizeHigh << 32));
#else
   if (stat(szFileName, &fileInfo) == -1)
      return SYSINFO_RC_UNSUPPORTED;

   ret_uint(value, fileInfo.st_size);
#endif

   return SYSINFO_RC_SUCCESS;
}


//
// Handler for File.Count(*) parameter
//

LONG H_FileCount(char *pszCmd, char *pszArg, char *pValue)
{
   char szDirName[MAX_PATH], szPattern[MAX_PATH];
   DIR *pDir;
   struct dirent *pFile;
   LONG nResult = SYSINFO_RC_ERROR, nCount = 0;

   if (!NxGetParameterArg(pszCmd, 1, szDirName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   if (!NxGetParameterArg(pszCmd, 2, szPattern, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   // If pattern is omited use asterisk
   if (szPattern[0] == 0)
      strcpy(szPattern, "*");

   pDir = opendir(szDirName);
   if (pDir != NULL)
   {
      while(1)
      {
         pFile = readdir(pDir);
         if (pFile == NULL)
            break;
         if (strcmp(pFile->d_name, ".") && strcmp(pFile->d_name, ".."))
         {
            if (MatchString(szPattern, pFile->d_name, FALSE))
               nCount++;
         }
      }
      closedir(pDir);
      ret_int(pValue, nCount);
      nResult = SYSINFO_RC_SUCCESS;
   }

   return nResult;
}


//
// Calculate MD5 hash for file
//

LONG H_MD5Hash(char *cmd, char *arg, char *value)
{
   char szFileName[MAX_PATH], szHashText[MD5_DIGEST_SIZE * 2 + 1];
   BYTE hash[MD5_DIGEST_SIZE];
   DWORD i;

   if (!NxGetParameterArg(cmd, 1, szFileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   if (!CalculateFileMD5Hash(szFileName, hash))
      return SYSINFO_RC_UNSUPPORTED;

   // Convert MD5 hash to text form
   for(i = 0; i < MD5_DIGEST_SIZE; i++)
      sprintf(&szHashText[i << 1], "%02x", hash[i]);

   ret_string(value, szHashText);
   return SYSINFO_RC_SUCCESS;
}


//
// Calculate SHA1 hash for file
//

LONG H_SHA1Hash(char *cmd, char *arg, char *value)
{
   char szFileName[MAX_PATH], szHashText[SHA1_DIGEST_SIZE * 2 + 1];
   BYTE hash[SHA1_DIGEST_SIZE];
   DWORD i;

   if (!NxGetParameterArg(cmd, 1, szFileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   if (!CalculateFileSHA1Hash(szFileName, hash))
      return SYSINFO_RC_UNSUPPORTED;

   // Convert SHA1 hash to text form
   for(i = 0; i < SHA1_DIGEST_SIZE; i++)
      sprintf(&szHashText[i << 1], "%02x", hash[i]);

   ret_string(value, szHashText);
   return SYSINFO_RC_SUCCESS;
}


//
// Calculate CRC32 for file
//

LONG H_CRC32(char *cmd, char *arg, char *value)
{
   char szFileName[MAX_PATH];
   DWORD dwCRC32;

   if (!NxGetParameterArg(cmd, 1, szFileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   if (!CalculateFileCRC32(szFileName, &dwCRC32))
      return SYSINFO_RC_UNSUPPORTED;

   ret_uint(value, dwCRC32);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.PlatformName
//

LONG H_PlatformName(char *cmd, char *arg, char *value)
{
   LONG nResult = SYSINFO_RC_SUCCESS;

#if defined(_WIN32)

   SYSTEM_INFO sysInfo;

   GetSystemInfo(&sysInfo);
   switch(sysInfo.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
         strcpy(value, "windows-i386");
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         strcpy(value, "windows-mips");
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         strcpy(value, "windows-alpha");
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         strcpy(value, "windows-ppc");
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         strcpy(value, "windows-ia64");
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
         strcpy(value, "windows-i386");
         break;
      case PROCESSOR_ARCHITECTURE_AMD64:
         strcpy(value, "windows-x64");
         break;
      default:
         strcpy(value, "windows-unknown");
         break;
   }

#elif defined(_NETWARE)

   // NetWare only exists on Intel x86 CPU,
   // so there are nothing to detect
   strcpy(value, "netware-i386");

#elif HAVE_UNAME

   struct utsname info;

   if (uname(&info) != -1)
   {
      sprintf(value, "%s-%s", info.sysname, info.machine);
   }
   else
   {
      DebugPrintf(INVALID_INDEX, "uname() failed: %s", strerror(errno));
      nResult = SYSINFO_RC_ERROR;
   }

#else

   // Finally, we don't know a way to detect platform
   strcpy(value, "unknown");

#endif

   // Add user-configurable platform name suffix
   if ((nResult == SYSINFO_RC_SUCCESS) && (g_szPlatformSuffix[0] != 0))
   {
      if (g_szPlatformSuffix[0] != '-')
         strcat(value, "-");
      strcat(value, g_szPlatformSuffix);
   }

   return nResult;
}
