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

#ifndef _WIN32

#if HAVE_SYS_STAT_H || defined(_NETWARE)
#include <sys/stat.h>
#endif

#endif   /* _WIN32 */


//
// Handler for Agent.Uptime parameter
//

LONG H_AgentUptime(char *cmd, char *arg, char *value)
{
   ret_uint(value, time(NULL) - g_dwAgentStartTime);
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

   if (!NxGetParameterArg(cmd, 1, szFileName, MAX_PATH - 1))
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
// Calculate MD5 hash for file
//

LONG H_MD5Hash(char *cmd, char *arg, char *value)
{
   char szFileName[MAX_PATH], szHashText[MD5_DIGEST_SIZE * 2 + 1];
   BYTE hash[MD5_DIGEST_SIZE];
   DWORD i;

   if (!NxGetParameterArg(cmd, 1, szFileName, MAX_PATH - 1))
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

   if (!NxGetParameterArg(cmd, 1, szFileName, MAX_PATH - 1))
      return SYSINFO_RC_UNSUPPORTED;

   if (!CalculateFileSHA1Hash(szFileName, hash))
      return SYSINFO_RC_UNSUPPORTED;

   // Convert MD5 hash to text form
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
   return SYSINFO_RC_UNSUPPORTED;
}
