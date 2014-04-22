/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2011 Victor Kirhenshtein
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

#include <nxstat.h>

/**
 * Handler for System.CurrentTime parameter
 */
LONG H_SystemTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   ret_int64(value, (INT64)time(NULL));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.Uptime parameter
 */
LONG H_AgentUptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   ret_uint(value, (UINT32)(time(NULL) - g_tmAgentStartTime));
   return SYSINFO_RC_SUCCESS;
}

/**
 * File filter for GetDirInfo
 */
static bool MatchFileFilter(const TCHAR *fileName, NX_STAT_STRUCT &fileInfo, const TCHAR *pattern, int ageFilter, INT64 sizeFilter)
{
	if (!MatchString(pattern, fileName, FALSE))
		return false;

	if (ageFilter != 0)
	{
		time_t now = time(NULL);
		if (ageFilter < 0)
		{
			if (fileInfo.st_mtime < now + ageFilter)
				return false;
		}
		else
		{
			if (fileInfo.st_mtime > now - ageFilter)
				return false;
		}
	}

	if (sizeFilter != 0)
	{
		if (sizeFilter < 0)
		{
			if (fileInfo.st_size > -sizeFilter)
				return false;
		}
		else
		{
			if (fileInfo.st_size < sizeFilter)
				return false;
		}
	}

	return true;
}


//
// Helper function for H_DirInfo
// 

static LONG GetDirInfo(TCHAR *szPath, TCHAR *szPattern, bool bRecursive,
                       unsigned int &uFileCount, QWORD &llFileSize,
                       int ageFilter, INT64 sizeFilter)
{
   _TDIR *pDir = NULL;
   struct _tdirent *pFile;
	NX_STAT_STRUCT fileInfo;
   TCHAR szFileName[MAX_PATH];
   LONG nRet = SYSINFO_RC_SUCCESS;

   if (CALL_STAT(szPath, &fileInfo) == -1) 
       return SYSINFO_RC_ERROR;

   // if this is just a file than simply return statistics
	// Filters ignored in this case
   if (!S_ISDIR(fileInfo.st_mode))
   {
		llFileSize += (QWORD)fileInfo.st_size;
		uFileCount++;
		return nRet;
   }

   // this is a dir
   pDir = _topendir(szPath);
   if (pDir != NULL)
   {
      while(1)
      {
         pFile = _treaddir(pDir);
         if (pFile == NULL)
            break;

         if (!_tcscmp(pFile->d_name, _T(".")) || !_tcscmp(pFile->d_name, _T("..")))
            continue;
         
			size_t len = _tcslen(szPath) + _tcslen(pFile->d_name) + 2;
			if (len > MAX_PATH)
				continue;	// Full file name is too long

         _tcscpy(szFileName, szPath);
         _tcscat(szFileName, FS_PATH_SEPARATOR);
         _tcscat(szFileName, pFile->d_name);

         // skip unaccessible entries
         if (CALL_STAT(szFileName, &fileInfo) == -1)
            continue;
         
         // skip symlinks
#ifndef _WIN32         
         if (S_ISLNK(fileInfo.st_mode))
         	continue;
#endif         	

         if (S_ISDIR(fileInfo.st_mode) && bRecursive)
         {
            nRet = GetDirInfo(szFileName, szPattern, bRecursive, uFileCount, llFileSize, ageFilter, sizeFilter);
            
            if (nRet != SYSINFO_RC_SUCCESS)
                break;
         }

         if (!S_ISDIR(fileInfo.st_mode) && MatchFileFilter(pFile->d_name, fileInfo, szPattern, ageFilter, sizeFilter))
         {
             llFileSize += (QWORD)fileInfo.st_size;
             uFileCount++;
         }
      }
      _tclosedir(pDir);
   }
   
   return nRet;
}

/**
 * Handler for File.Size(*) and File.Count(*)
 * Accepts the following arguments:
 *    path, pattern, recursive, size, age
 * where
 *    path    : path to directory or file to check
 *    pattern : pattern for file name matching
 *    recursive : recursion flag; if set to 1 or true, agent will scan subdirectories
 *    size      : size filter; if < 0, only files with size less than abs(value) will match;
 *                if > 0, only files with size greater than value will match
 *    age       : age filter; if < 0, only files created after now - abs(value) will match;
 *                if > 0, only files created before now - value will match
 */
LONG H_DirInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   TCHAR szPath[MAX_PATH], szRealPath[MAX_PATH], szPattern[MAX_PATH], szRealPattern[MAX_PATH], szRecursive[10], szBuffer[128];
   bool bRecursive = false;

   unsigned int uFileCount = 0;
   QWORD llFileSize = 0;
   LONG nRet;

   if (!AgentGetParameterArg(cmd, 1, szPath, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;
   if (!AgentGetParameterArg(cmd, 2, szPattern, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;
   if (!AgentGetParameterArg(cmd, 3, szRecursive, 10))
      return SYSINFO_RC_UNSUPPORTED;

	if (!AgentGetParameterArg(cmd, 4, szBuffer, 128))
      return SYSINFO_RC_UNSUPPORTED;
	INT64 sizeFilter = _tcstoll(szBuffer, NULL, 0);

	if (!AgentGetParameterArg(cmd, 5, szBuffer, 128))
      return SYSINFO_RC_UNSUPPORTED;
	int ageFilter = _tcstoul(szBuffer, NULL, 0);

	// Recursion flag
	bRecursive = ((_tcstol(szRecursive, NULL, 0) != 0) || !_tcsicmp(szRecursive, _T("TRUE")));

   // If pattern is omited use asterisk
   if (szPattern[0] == 0)
      _tcscpy(szPattern, _T("*"));

	// Expand strftime macros in the path and in the pattern
	if ((ExpandFileName(szPath, szRealPath, MAX_PATH) == NULL) ||
	    (ExpandFileName(szPattern, szRealPattern, MAX_PATH) == NULL))
		return SYSINFO_RC_UNSUPPORTED;

   DebugPrintf(INVALID_INDEX, 6, _T("H_DirInfo: path=\"%s\" pattern=\"%s\" recursive=%s"), szRealPath, szRealPattern, bRecursive ? _T("true") : _T("false"));
   nRet = GetDirInfo(szRealPath, szRealPattern, bRecursive, uFileCount, llFileSize, ageFilter, sizeFilter);

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

/**
 * Calculate MD5 hash for file
 */
LONG H_MD5Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   TCHAR szFileName[MAX_PATH], szRealFileName[MAX_PATH];
	TCHAR szHashText[MD5_DIGEST_SIZE * 2 + 1];
   BYTE hash[MD5_DIGEST_SIZE];
   UINT32 i;

   if (!AgentGetParameterArg(cmd, 1, szFileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

	// Expand strftime macros in the path
	if (ExpandFileName(szFileName, szRealFileName, MAX_PATH) == NULL)
		return SYSINFO_RC_UNSUPPORTED;

   if (!CalculateFileMD5Hash(szRealFileName, hash))
      return SYSINFO_RC_UNSUPPORTED;

   // Convert MD5 hash to text form
   for(i = 0; i < MD5_DIGEST_SIZE; i++)
      _sntprintf(&szHashText[i << 1], 4, _T("%02x"), hash[i]);

   ret_string(value, szHashText);
   return SYSINFO_RC_SUCCESS;
}


//
// Calculate SHA1 hash for file
//

LONG H_SHA1Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   TCHAR szFileName[MAX_PATH], szRealFileName[MAX_PATH]; 
	TCHAR szHashText[SHA1_DIGEST_SIZE * 2 + 1];
   BYTE hash[SHA1_DIGEST_SIZE];
   UINT32 i;

   if (!AgentGetParameterArg(cmd, 1, szFileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

	// Expand strftime macros in the path
	if (ExpandFileName(szFileName, szRealFileName, MAX_PATH) == NULL)
		return SYSINFO_RC_UNSUPPORTED;

   if (!CalculateFileSHA1Hash(szRealFileName, hash))
      return SYSINFO_RC_UNSUPPORTED;

   // Convert SHA1 hash to text form
   for(i = 0; i < SHA1_DIGEST_SIZE; i++)
      _sntprintf(&szHashText[i << 1], 4, _T("%02x"), hash[i]);

   ret_string(value, szHashText);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Calculate CRC32 for file
 */
LONG H_CRC32(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   TCHAR szFileName[MAX_PATH], szRealFileName[MAX_PATH];
   UINT32 dwCRC32;

   if (!AgentGetParameterArg(cmd, 1, szFileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

	// Expand strftime macros in the path
	if (ExpandFileName(szFileName, szRealFileName, MAX_PATH) == NULL)
		return SYSINFO_RC_UNSUPPORTED;

   if (!CalculateFileCRC32(szRealFileName, &dwCRC32))
      return SYSINFO_RC_UNSUPPORTED;

   ret_uint(value, dwCRC32);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.PlatformName
 */
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   LONG nResult = SYSINFO_RC_SUCCESS;

#if defined(_WIN32)

   SYSTEM_INFO sysInfo;

   GetSystemInfo(&sysInfo);
   switch(sysInfo.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
         _tcscpy(value, _T("windows-i386"));
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         _tcscpy(value, _T("windows-mips"));
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         _tcscpy(value, _T("windows-alpha"));
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         _tcscpy(value, _T("windows-ppc"));
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         _tcscpy(value, _T("windows-ia64"));
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
         _tcscpy(value, _T("windows-i386"));
         break;
      case PROCESSOR_ARCHITECTURE_AMD64:
         _tcscpy(value, _T("windows-x64"));
         break;
      default:
         _tcscpy(value, _T("windows-unknown"));
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
      _sntprintf(value, MAX_RESULT_LENGTH, _T("%hs-powerpc"), info.sysname);
#else
      _sntprintf(value, MAX_RESULT_LENGTH, _T("%hs-%hs"), info.sysname, info.machine);
#endif
   }
   else
   {
      DebugPrintf(INVALID_INDEX, 2, _T("uname() failed: %s"), _tcserror(errno));
      nResult = SYSINFO_RC_ERROR;
   }

#else

   // Finally, we don't know a way to detect platform
   _tcscpy(value, _T("unknown"));

#endif

   // Add user-configurable platform name suffix
   if ((nResult == SYSINFO_RC_SUCCESS) && (g_szPlatformSuffix[0] != 0))
   {
      if (g_szPlatformSuffix[0] != _T('-'))
         _tcscat(value, _T("-"));
      _tcscat(value, g_szPlatformSuffix);
   }

   return nResult;
}

/**
 * Handler for File.Time.* parameters
 */
LONG H_FileTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
	TCHAR szFilePath[MAX_PATH], szRealFilePath[MAX_PATH];
	LONG nRet = SYSINFO_RC_SUCCESS;
	NX_STAT_STRUCT fileInfo;

	if (!AgentGetParameterArg(cmd, 1, szFilePath, MAX_PATH))
		return SYSINFO_RC_UNSUPPORTED;

	// Expand strftime macros in the path
	if (ExpandFileName(szFilePath, szRealFilePath, MAX_PATH) == NULL)
		return SYSINFO_RC_UNSUPPORTED;

	if (CALL_STAT(szRealFilePath, &fileInfo) == -1) 
		return SYSINFO_RC_ERROR;

	switch(CAST_FROM_POINTER(arg, int))
	{
		case FILETIME_ATIME:
			ret_uint64(value, fileInfo.st_atime);
			break;
		case FILETIME_MTIME:
			ret_uint64(value, fileInfo.st_mtime);
			break;
		case FILETIME_CTIME:
			ret_uint64(value, fileInfo.st_ctime);
			break;
		default:
			nRet = SYSINFO_RC_UNSUPPORTED;
			break;
	}

	return nRet;
}
