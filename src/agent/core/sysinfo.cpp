/* $Id: sysinfo.cpp,v 1.25 2008-04-19 19:12:41 victor Exp $ */
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
** File: sysinfo.cpp
**/

#include "nxagentd.h"

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
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
// Helper function to GetDirInfo
// 

LONG GetDirInfo(char *szPath, char *szPattern, bool bRecursive,
                unsigned int &uFileCount, QWORD &llFileSize)
{
   DIR *pDir = NULL;
   struct dirent *pFile = NULL;
   struct stat fileInfo;
   char szFileName[MAX_PATH];
   LONG nRet = SYSINFO_RC_SUCCESS;

   if (stat(szPath, &fileInfo) == -1) 
       return SYSINFO_RC_ERROR;

   // if this is just a file than simply return statistics
   if (!S_ISDIR(fileInfo.st_mode))
   {
       if (MatchString(szPattern, szPath, FALSE))
       {
           llFileSize += (QWORD)fileInfo.st_size;
           uFileCount++;
       }

       return nRet;
   }

   // this is a dir
   pDir = opendir(szPath);
   if (pDir != NULL)
   {
      while(1)
      {
         pFile = readdir(pDir);
         if (pFile == NULL)
            break;

         if (!strcmp(pFile->d_name, ".") || !strcmp(pFile->d_name, ".."))
            continue;
         
         strcpy(szFileName, szPath);
         strcat(szFileName, "/" );
         strcat(szFileName, pFile->d_name);

         // skip unaccessible entries
#ifdef _WIN32
         if (stat(szFileName, &fileInfo) == -1)
#else
         if (lstat(szFileName, &fileInfo) == -1)
#endif
            continue;
         
         // skip symlinks
#ifndef _WIN32         
         if (S_ISLNK(fileInfo.st_mode))
         	continue;
#endif         	

         if (S_ISDIR(fileInfo.st_mode) && bRecursive)
         {
            nRet = GetDirInfo(szFileName, szPattern, bRecursive, uFileCount, llFileSize);
            
            if (nRet != SYSINFO_RC_SUCCESS)
                break;
         }

         if (!S_ISDIR(fileInfo.st_mode) && MatchString(szPattern, pFile->d_name, FALSE))
         {
             llFileSize += (QWORD)fileInfo.st_size;
             uFileCount++;
         }
      }
      closedir(pDir);
   }
   
   return nRet;
}


//
// Handler for File.Size(*) and File.Count(*)
//

LONG H_DirInfo(char *cmd, char *arg, char *value)
{
   char szPath[MAX_PATH], szPattern[MAX_PATH], szRecursive[10];
   bool bRecursive = false;

   unsigned int uFileCount = 0;
   QWORD llFileSize = 0;
   LONG nRet;

   if (!NxGetParameterArg(cmd, 1, szPath, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;
   if (!NxGetParameterArg(cmd, 2, szPattern, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;
   if (!NxGetParameterArg(cmd, 3, szRecursive, 10))
      return SYSINFO_RC_UNSUPPORTED;

	// Recursion flag
	bRecursive = ((atoi(szRecursive) != 0) || !_tcsicmp(szRecursive, _T("TRUE")));

   // If pattern is omited use asterisk
   if (szPattern[0] == 0)
      strcpy(szPattern, "*");

   nRet = GetDirInfo(szPath, szPattern, bRecursive, uFileCount, llFileSize);

   switch(CAST_FROM_POINTER(arg, int))
   {
   	case DIRINFO_FILE_SIZE:
           ret_uint64(value, llFileSize);
           break;
   	case DIRINFO_FILE_COUNT:
           ret_uint(value, uFileCount);
           break;
   	default:
           nRet = SYSINFO_RC_UNSUPPORTED;
			break;
   }

   return nRet;
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
#ifdef _AIX
      // Assume that we are running on PowerPC
      sprintf(value, "%s-powerpc", info.sysname);
#else
      sprintf(value, "%s-%s", info.sysname, info.machine);
#endif
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


//
// Handler for File.Time.* parameters
//

LONG H_FileTime(char *cmd, char *arg, char *value)
{
	char szFilePath[MAX_PATH];
	LONG nRet = SYSINFO_RC_SUCCESS;
	struct stat fileInfo;

	if (!NxGetParameterArg(cmd, 1, szFilePath, MAX_PATH))
		return SYSINFO_RC_UNSUPPORTED;

	if (stat(szFilePath, &fileInfo) == -1) 
		return SYSINFO_RC_ERROR;

	switch(CAST_FROM_POINTER(arg, int))
	{
		case FILETIME_ATIME:
#ifdef _NETWARE
			ret_uint64(value, fileInfo.st_atime.tv_sec);
#else
			ret_uint64(value, fileInfo.st_atime);
#endif
			break;
		case FILETIME_MTIME:
#ifdef _NETWARE
			ret_uint64(value, fileInfo.st_mtime.tv_sec);
#else
			ret_uint64(value, fileInfo.st_mtime);
#endif
			break;
		case FILETIME_CTIME:
#ifdef _NETWARE
			ret_uint64(value, fileInfo.st_ctime.tv_sec);
#else
			ret_uint64(value, fileInfo.st_ctime);
#endif
			break;
		default:
			nRet = SYSINFO_RC_UNSUPPORTED;
			break;
	}

	return nRet;
}
