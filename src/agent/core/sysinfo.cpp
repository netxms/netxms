/*
** NetXMS multiplatform core agent
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
** File: sysinfo.cpp
**/

#include "nxagentd.h"

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include <nxstat.h>

#define FILEMON_DEBUG_TAG  _T("filemon")

/**
 * Handler for System.CurrentDate metric
 */
LONG H_SystemDate(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   time_t now = time(nullptr);
#if HAVE_LOCALTIME_R
   tm buffer;
   tm *lt = localtime_r(&now, &buffer);
#else
   tm *lt = localtime(&now);
#endif
   _tcsftime(value, MAX_RESULT_LENGTH, _T("%Y%m%d"), lt);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.CurrentTime metric
 */
LONG H_SystemTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_int64(value, static_cast<int64_t>(time(nullptr)));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.CurrentTime.ISO8601.UTC metric
 */
LONG H_SystemTimeISO8601UTC(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   time_t t = time(nullptr);
   struct tm utcTime;
#if HAVE_GMTIME_R
   gmtime_r(&t, &utcTime);
#else
   memcpy(&utcTime, gmtime(&t), sizeof(struct tm));
#endif
   _tcsftime(value, MAX_RESULT_LENGTH, _T("%Y-%m-%dT%H:%M:%SZ"), &utcTime);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.CurrentTime.ISO8601.Local metric
 */
LONG H_SystemTimeISO8601Local(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   time_t t = time(nullptr);
   struct tm localTime;
#if HAVE_LOCALTIME_R
   localtime_r(&t, &localTime);
#else
   memcpy(&localTime, localtime(&t), sizeof(struct tm));
#endif
   _tcsftime(value, MAX_RESULT_LENGTH, _T("%Y-%m-%dT%H:%M:%S"), &localTime);
   GetSystemTimeZone(&value[19], MAX_RESULT_LENGTH - 19, false, true);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.TimeZone metric
 */
LONG H_SystemTimeZone(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GetSystemTimeZone(value, MAX_RESULT_LENGTH, *arg == 'N');
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.Uptime metric
 */
LONG H_AgentUptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, static_cast<uint32_t>((GetMonotonicClockTime() - g_agentStartTime) / 1000));
   return SYSINFO_RC_SUCCESS;
}

/**
 * File filter for GetDirInfo
 */
#ifdef _WIN32
static bool MatchFileFilter(const WCHAR *fileName, const NX_STAT_STRUCT &fileInfo, const WCHAR *pattern, int ageFilter, int64_t sizeFilter, bool searchInverse)
#else
static bool MatchFileFilter(const char *fileName, const NX_STAT_STRUCT &fileInfo, const char *pattern, int ageFilter, int64_t sizeFilter, bool searchInverse)
#endif
{
   if (searchInverse)
   {
#ifdef _WIN32
      if (MatchStringW(pattern, fileName, FALSE))
#else
      if (MatchStringA(pattern, fileName, FALSE))
#endif
         return false;
   }
   else
   {
#ifdef _WIN32
      if (!MatchStringW(pattern, fileName, FALSE))
#else
      if (!MatchStringA(pattern, fileName, FALSE))
#endif
         return false;
   }

   if (ageFilter != 0)
   {
      time_t now = time(nullptr);
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

/**
 * Helper function for H_DirInfo
 */
#ifdef _WIN32
static LONG GetDirInfo(const WCHAR *path, const WCHAR *pattern, bool isRecursive, unsigned int &uFileCount,
                       uint64_t &llFileSize, int ageFilter, int64_t sizeFilter, bool countFiles, bool countFolders, bool searchInverse)
#else
static LONG GetDirInfo(const char *path, const char *pattern, bool isRecursive, unsigned int &uFileCount,
                       uint64_t &llFileSize, int ageFilter, int64_t sizeFilter, bool countFiles, bool countFolders, bool searchInverse)
#endif
{
   NX_STAT_STRUCT fileInfo;
   if (CALL_STAT_A(path, &fileInfo) == -1)
   {
      nxlog_debug(5, _T("GetDirInfo: stat() call on \"%hs\" failed (%s)"), path, _tcserror(errno));
      return SYSINFO_RC_ERROR;
   }

   // if this is just a file than simply return statistics
   // Filters ignored in this case
   if (!S_ISDIR(fileInfo.st_mode))
   {
      llFileSize += static_cast<uint64_t>(fileInfo.st_size);
      uFileCount++;
      return SYSINFO_RC_SUCCESS;
   }

   LONG nRet = SYSINFO_RC_SUCCESS;

   // this is a dir
#ifdef _WIN32
   DIRW *dir = wopendir(path);
#else
   DIR *dir = opendir(path);
#endif
   if (dir != nullptr)
   {
      while(true)
      {
#ifdef _WIN32
         struct dirent_w *pFile = wreaddir(dir);
#else
         struct dirent *pFile = readdir(dir);
#endif
         if (pFile == nullptr)
            break;

#ifdef _WIN32
         if (!wcscmp(pFile->d_name, L".") || !wcscmp(pFile->d_name, L".."))
            continue;

         size_t len = wcslen(path) + wcslen(pFile->d_name) + 2;
         if (len > MAX_PATH)
            continue;   // Full file name is too long

         WCHAR szFileName[MAX_PATH];
         wcscpy(szFileName, path);
         wcscat(szFileName, FS_PATH_SEPARATOR);
         wcscat(szFileName, pFile->d_name);
#else
         if (!strcmp(pFile->d_name, ".") || !strcmp(pFile->d_name, ".."))
            continue;

         size_t len = strlen(path) + strlen(pFile->d_name) + 2;
         if (len > MAX_PATH)
            continue;   // Full file name is too long

         char szFileName[MAX_PATH];
         strcpy(szFileName, path);
         strcat(szFileName, FS_PATH_SEPARATOR_A);
         strcat(szFileName, pFile->d_name);
#endif

         // skip unaccessible entries
         if (CALL_STAT_A(szFileName, &fileInfo) == -1)
            continue;

         // skip symlinks
#ifndef _WIN32
         if (S_ISLNK(fileInfo.st_mode))
            continue;
#endif

         if (S_ISDIR(fileInfo.st_mode) && isRecursive)
         {
            nRet = GetDirInfo(szFileName, pattern, isRecursive, uFileCount, llFileSize, ageFilter, sizeFilter, countFiles, countFolders, searchInverse);
            if (nRet != SYSINFO_RC_SUCCESS)
                break;
         }

         if (((countFiles && !S_ISDIR(fileInfo.st_mode)) || (countFolders && S_ISDIR(fileInfo.st_mode))) && 
             MatchFileFilter(pFile->d_name, fileInfo, pattern, ageFilter, sizeFilter, searchInverse))
         {
             llFileSize += static_cast<uint64_t>(fileInfo.st_size);
             uFileCount++;
         }
      }
#ifdef _WIN32
      wclosedir(dir);
#else
      closedir(dir);
#endif
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
LONG H_DirInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR path[MAX_PATH], patternBuffer[MAX_PATH], buffer[128];
   if (!AgentGetMetricArg(cmd, 1, path, MAX_PATH) ||
       !AgentGetParameterArg(cmd, 2, patternBuffer, MAX_PATH) ||
       !AgentGetParameterArg(cmd, 3, buffer, 128))
      return SYSINFO_RC_UNSUPPORTED;
   bool isRecursive = ((_tcstol(buffer, nullptr, 0) != 0) || !_tcsicmp(buffer, _T("TRUE")));

   if (!AgentGetMetricArg(cmd, 4, buffer, 128))
      return SYSINFO_RC_UNSUPPORTED;
   int64_t sizeFilter = _tcstoll(buffer, nullptr, 0);

   if (!AgentGetMetricArg(cmd, 5, buffer, 128))
      return SYSINFO_RC_UNSUPPORTED;
   int ageFilter = _tcstoul(buffer, nullptr, 0);

   bool searchInverse = false;
   const TCHAR *pattern;

   // If pattern is omited use asterisk
   if (patternBuffer[0] == 0)
   {
      pattern = _T("*");
   }
   else if (patternBuffer[0] == _T('!'))
   {
      // Inverse counting flag
      searchInverse = true;
      pattern = &patternBuffer[1];
   }
   else
   {
      pattern = patternBuffer;
   }

   // Expand strftime macros in the path and in the pattern
   TCHAR realPath[MAX_PATH], realPattern[MAX_PATH];
   if ((ExpandFileName(path, realPath, MAX_PATH, session == nullptr ? false : session->isMasterServer()) == nullptr) ||
       (ExpandFileName(pattern, realPattern, MAX_PATH, session == nullptr ? false : session->isMasterServer()) == nullptr))
      return SYSINFO_RC_UNSUPPORTED;

   // Remove trailing backslash on Windows
#ifdef _WIN32
   size_t i = _tcslen(realPath) - 1;
   if (realPath[i] == _T('\\'))
      realPath[i] = 0;
#endif

   int mode = CAST_FROM_POINTER(arg, int);
   nxlog_debug_tag(FILEMON_DEBUG_TAG, 6, _T("H_DirInfo: path=\"%s\" pattern=\"%s\" recursive=%s mode=%d"), realPath, realPattern, BooleanToString(isRecursive), mode);

   unsigned int uFileCount = 0;
   uint64_t llFileSize = 0;

#if defined(_WIN32) || !defined(UNICODE)
   LONG nRet = GetDirInfo(realPath, realPattern, isRecursive, uFileCount, llFileSize, ageFilter, sizeFilter, mode != DIRINFO_FOLDER_COUNT, mode == DIRINFO_FOLDER_COUNT, searchInverse);
#else
   char mbRealPath[MAX_PATH], mbRealPattern[MAX_PATH];
   WideCharToMultiByteSysLocale(realPath, mbRealPath, MAX_PATH);
   WideCharToMultiByteSysLocale(realPattern, mbRealPattern, MAX_PATH);
   LONG nRet = GetDirInfo(mbRealPath, mbRealPattern, isRecursive, uFileCount, llFileSize, ageFilter, sizeFilter, mode != DIRINFO_FOLDER_COUNT, mode == DIRINFO_FOLDER_COUNT, searchInverse);
#endif

   switch(mode)
   {
      case DIRINFO_FILE_SIZE:
         ret_uint64(value, llFileSize);
         break;
      case DIRINFO_FILE_COUNT:
      case DIRINFO_FOLDER_COUNT:
         ret_uint(value, uFileCount);
         break;
      default:
         nRet = SYSINFO_RC_UNSUPPORTED;
         break;
   }

   return nRet;
}

/**
 * Calculate hash for file
 */
template<size_t HASH_SIZE, bool (*HashFunction)(const TCHAR*, BYTE*, int64_t)>
static inline LONG H_FileHash(const TCHAR *cmd, TCHAR *value, AbstractCommSession *session)
{
   TCHAR fileName[MAX_PATH], realFileName[MAX_PATH];
   if (!AgentGetMetricArg(cmd, 1, fileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   // Expand strftime macros in the path
   if (ExpandFileName(fileName, realFileName, MAX_PATH, session->isMasterServer()) == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   nxlog_debug_tag(FILEMON_DEBUG_TAG, 6, _T("H_FileHash: path=\"%s\""), realFileName);

   BYTE hash[HASH_SIZE];
   if (!HashFunction(realFileName, hash, 0))
      return SYSINFO_RC_UNSUPPORTED;

   BinToStrL(hash, HASH_SIZE, value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Calculate MD5 hash for file
 */
LONG H_MD5Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return H_FileHash<MD5_DIGEST_SIZE, CalculateFileMD5Hash>(cmd, value, session);
}

/**
 * Calculate SHA1 hash for file
 */
LONG H_SHA1Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return H_FileHash<SHA1_DIGEST_SIZE, CalculateFileSHA1Hash>(cmd, value, session);
}

/**
 * Calculate SHA256 hash for file
 */
LONG H_SHA256Hash(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return H_FileHash<SHA256_DIGEST_SIZE, CalculateFileSHA256Hash>(cmd, value, session);
}

/**
 * Calculate CRC32 for file
 */
LONG H_CRC32(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR fileName[MAX_PATH], realFileName[MAX_PATH];
   uint32_t crc32;

   if (!AgentGetMetricArg(cmd, 1, fileName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   // Expand strftime macros in the path
   if (ExpandFileName(fileName, realFileName, MAX_PATH, session->isMasterServer()) == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   nxlog_debug_tag(FILEMON_DEBUG_TAG, 6, _T("H_CRC32: path=\"%s\""), realFileName);

   if (!CalculateFileCRC32(realFileName, &crc32))
      return SYSINFO_RC_UNSUPPORTED;

   ret_uint(value, crc32);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.PlatformName
 */
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
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
      case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64:
         _tcscpy(value, _T("windows-i386"));
         break;
      case PROCESSOR_ARCHITECTURE_AMD64:
         _tcscpy(value, _T("windows-x64"));
         break;
      case PROCESSOR_ARCHITECTURE_ARM64:
         _tcscpy(value, _T("windows-aarch64"));
         break;
      default:
         _tcscpy(value, _T("windows-unknown"));
         break;
   }

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
      DebugPrintf(2, _T("uname() failed: %s"), _tcserror(errno));
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
LONG H_FileTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR path[MAX_PATH];
   if (!AgentGetMetricArg(cmd, 1, path, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   // Expand strftime macros in the path
   TCHAR realPath[MAX_PATH];
   if (ExpandFileName(path, realPath, MAX_PATH, session->isMasterServer()) == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   nxlog_debug_tag(FILEMON_DEBUG_TAG, 6, _T("H_FileTime: path=\"%s\""), realPath);

   NX_STAT_STRUCT fileInfo;
   if (CALL_STAT(realPath, &fileInfo) == -1)
      return (errno == ENOENT) ? SYSINFO_RC_NO_SUCH_INSTANCE : SYSINFO_RC_ERROR;

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
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for File.Type metric
 */
LONG H_FileType(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR path[MAX_PATH];
   if (!AgentGetMetricArg(cmd, 1, path, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   // Expand strftime macros in the path
   TCHAR realPath[MAX_PATH];
   if (ExpandFileName(path, realPath, MAX_PATH, session->isMasterServer()) == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   nxlog_debug_tag(FILEMON_DEBUG_TAG, 6, _T("H_FileType: path=\"%s\""), realPath);

   NX_STAT_STRUCT fileInfo;
   if (CALL_STAT(realPath, &fileInfo) == -1)
   {
      if (errno != ENOENT)
         return SYSINFO_RC_ERROR;

      ret_uint(value, 0);
      return SYSINFO_RC_SUCCESS;
   }

   if (S_ISDIR(fileInfo.st_mode))
      ret_uint(value, 1);
#ifndef _WIN32
   else if (S_ISCHR(fileInfo.st_mode) || S_ISBLK(fileInfo.st_mode))
      ret_uint(value, 2);
#endif
   else if (S_ISREG(fileInfo.st_mode))
      ret_uint(value, 3);
   else
      ret_uint(value, 4);

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.Resolver.AddressByName parameter
 */
LONG H_ResolverAddrByName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetMetricArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR family[32] = _T("");
   if (!AgentGetMetricArg(cmd, 2, family, 32))
      return SYSINFO_RC_UNSUPPORTED;
   _tcslwr(family);
   int af = AF_UNSPEC;
   if (!_tcscmp(family, _T("ipv4")) || !_tcscmp(family, _T("ip4")) || !_tcscmp(family, _T("4")))
      af = AF_INET;
   else if (!_tcscmp(family, _T("ipv6")) || !_tcscmp(family, _T("ip6")) || !_tcscmp(family, _T("6")))
      af = AF_INET6;

   InetAddress addr = InetAddress::resolveHostName(name, af);
   addr.toString(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.Resolver.AddressByName list
 */
LONG H_ResolverAddrByNameList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetMetricArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;

   InetAddressList *list = InetAddressList::resolveHostName(name);
   if (list == nullptr)
      return SYSINFO_RC_ERROR;

   TCHAR buffer[64];
   for(int i = 0; i < list->size(); i++)
   {
      value->add(list->get(i).toString(buffer));
   }
   delete list;

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Net.Resolver.NameByAddress parameter
 */
LONG H_ResolverNameByAddr(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetMetricArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;

   InetAddress addr = InetAddress::parse(name);
   if (!addr.isValid())
      return SYSINFO_RC_UNSUPPORTED;

   if (addr.getHostByAddr(value, MAX_RESULT_LENGTH) == nullptr)
      addr.toString(value);   // return address itself if cannot be back resolved
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get thread pool information
 */
LONG H_ThreadPoolInfo(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR poolName[64], options[64];
   if (!AgentGetMetricArg(metric, 1, poolName, 64) ||
       !AgentGetMetricArg(metric, 2, options, 64))
      return SYSINFO_RC_UNSUPPORTED;

   ThreadPoolInfo info;
   if (!ThreadPoolGetInfo(poolName, &info))
      return SYSINFO_RC_UNSUPPORTED;

   switch(CAST_FROM_POINTER(arg, int))
   {
      case THREAD_POOL_ACTIVE_REQUESTS:
         ret_int(value, info.activeRequests);
         break;
      case THREAD_POOL_AVG_WAIT_TIME:
         ret_int(value, info.waitTimeEMA);
         break;
      case THREAD_POOL_CURR_SIZE:
         ret_int(value, info.curThreads);
         break;
      case THREAD_POOL_LOAD:
         ret_int(value, info.load);
         break;
      case THREAD_POOL_LOADAVG_1:
         if ((options[0] != 0) && _tcstol(options, nullptr, 10))
            ret_double(value, info.loadAvg[0] / info.maxThreads, 2);
         else
            ret_double(value, info.loadAvg[0], 2);
         break;
      case THREAD_POOL_LOADAVG_5:
         if ((options[0] != 0) && _tcstol(options, nullptr, 10))
            ret_double(value, info.loadAvg[1] / info.maxThreads, 2);
         else
            ret_double(value, info.loadAvg[1], 2);
         break;
      case THREAD_POOL_LOADAVG_15:
         if ((options[0] != 0) && _tcstol(options, nullptr, 10))
            ret_double(value, info.loadAvg[2] / info.maxThreads, 2);
         else
            ret_double(value, info.loadAvg[2], 2);
         break;
      case THREAD_POOL_MAX_SIZE:
         ret_int(value, info.maxThreads);
         break;
      case THREAD_POOL_MIN_SIZE:
         ret_int(value, info.minThreads);
         break;
      case THREAD_POOL_SCHEDULED_REQUESTS:
         ret_int(value, info.scheduledRequests);
         break;
      case THREAD_POOL_USAGE:
         ret_int(value, info.usage);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Thread pool list
 */
LONG H_ThreadPoolList(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   value->addAll(ThreadPoolGetAllPools());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Hostname and System.FQDN parameters
 */
LONG H_HostName(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return (GetLocalHostName(value, MAX_RESULT_LENGTH, arg != NULL) != NULL) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for File.LineCount parameter
 */
LONG H_FileLineCount(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR fname[MAX_PATH];
   if (!AgentGetMetricArg(metric, 1, fname, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   int fd = _topen(fname, O_RDONLY);
   if (fd == -1)
      return SYSINFO_RC_ERROR;

   uint32_t lineCount = 0;
   char buffer[8192];
   bool lastCharIsNL = false;
   int bytes;
   while((bytes = _read(fd, buffer, sizeof(buffer))) > 0)
   {
      for(int i = 0; i < bytes; i++)
         if (buffer[i] == '\n')
            lineCount++;
      lastCharIsNL = (buffer[bytes - 1] == '\n');
   }
   _close(fd);

   if (!lastCharIsNL)
      lineCount++; // Last line without '\n' char

   ret_uint(value, lineCount);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for File.Content parameter
 */
LONG H_FileContent(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (!session->isMasterServer())
      return SYSINFO_RC_ACCESS_DENIED;

   char fname[MAX_PATH];
   int32_t offset;
   if (!AgentGetMetricArgA(metric, 1, fname, MAX_PATH) || !AgentGetMetricArgAsInt32(metric, 2, &offset, 1))
      return SYSINFO_RC_UNSUPPORTED;

   if (offset < 1)
      return SYSINFO_RC_UNSUPPORTED;

   FILE *fp = fopen(fname, "r");
   if (fp == nullptr)
      return SYSINFO_RC_ERROR;

   char line[4096];
   while(!feof(fp) && (offset > 0))
   {
      if (fgets(line, sizeof(line), fp) == nullptr)
         break;

      offset--;
      if (offset == 0)
      {
         char *nl = strchr(line, '\n');
         if (nl != nullptr)
            *nl = 0;
#ifdef UNICODE
         char codepage[64] = "";
         AgentGetMetricArgA(metric, 3, codepage, 64);
         mbcp_to_wchar(line, -1, value, MAX_RESULT_LENGTH, (codepage[0] != 0) ? codepage : nullptr);
#else
         ret_string(value, line);
#endif
      }
   }

   fclose(fp);
   return SYSINFO_RC_SUCCESS;
}
