/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: tools.cpp
**
**/

#include "nms_core.h"

#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
# include <io.h>
#endif


//
// Get system error string by call to FormatMessage
//

#ifdef _WIN32

char *GetSystemErrorText(DWORD error)
{
   char *msgBuf;
   static char staticBuffer[1024];

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_SYSTEM | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL,error,
                     MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
                     (LPSTR)&msgBuf,0,NULL)>0)
   {
      msgBuf[strcspn(msgBuf,"\r\n")]=0;
      strncpy(staticBuffer,msgBuf,1023);
      LocalFree(msgBuf);
   }
   else
   {
      sprintf(staticBuffer,"MSG 0x%08X - Unable to find message text",error);
   }

   return staticBuffer;
}

#endif   /* _WIN32 */


//
// Clean interface list from unneeded entries
//

void CleanInterfaceList(INTERFACE_LIST *pIfList)
{
   int i;

   if (pIfList == NULL)
      return;

   // Delete loopback interface(s) from list
   for(i = 0; i < pIfList->iNumEntries; i++)
      if ((pIfList->pInterfaces[i].dwIpAddr & pIfList->pInterfaces[i].dwIpNetMask) == 0x0000007F)
      {
         pIfList->iNumEntries--;
         memmove(&pIfList->pInterfaces[i], &pIfList->pInterfaces[i + 1],
                 sizeof(INTERFACE_INFO) * (pIfList->iNumEntries - i));
         i--;
      }
}


//
// Convert byte array to text representation
//

void BinToStr(BYTE *pData, DWORD dwSize, char *pStr)
{
   DWORD i;
   char *pCurr;

   for(i = 0, pCurr = pStr; i < dwSize; i++)
   {
      *pCurr++ = bin2hex(pData[i] >> 4);
      *pCurr++ = bin2hex(pData[i] & 15);
   }
   *pCurr = 0;
}


//
// Convert string of hexadecimal digits to byte array
//

DWORD StrToBin(char *pStr, BYTE *pData, DWORD dwSize)
{
   DWORD i;
   char *pCurr;

   memset(pData, 0, dwSize);
   for(i = 0, pCurr = pStr; (i < dwSize) && (*pCurr != 0); i++)
   {
      pData[i] = hex2bin(*pCurr) << 4;
      pCurr++;
      pData[i] |= hex2bin(*pCurr);
      pCurr++;
   }
   return i;
}


//
// Get system information string
//

void GetSysInfoStr(char *pszBuffer)
{
#ifdef _WIN32
   DWORD dwSize;
   char computerName[MAX_COMPUTERNAME_LENGTH + 1], osVersion[256];
   SYSTEM_INFO sysInfo;
   OSVERSIONINFO versionInfo;

   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(computerName, &dwSize);

   versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&versionInfo);
   GetSystemInfo(&sysInfo);

   switch(versionInfo.dwPlatformId)
   {
      case VER_PLATFORM_WIN32_WINDOWS:
         sprintf(osVersion,"Windows %s-%s",versionInfo.dwMinorVersion == 0 ? "95" :
            (versionInfo.dwMinorVersion == 10 ? "98" :
               (versionInfo.dwMinorVersion == 90 ? "Me" : "Unknown")),versionInfo.szCSDVersion);
         break;
      case VER_PLATFORM_WIN32_NT:
         if (versionInfo.dwMajorVersion != 5)
            sprintf(osVersion,"Windows NT %d.%d %s",versionInfo.dwMajorVersion,
                    versionInfo.dwMinorVersion,versionInfo.szCSDVersion);
         else      // Windows 2000, Windows XP or Windows Server 2003
            sprintf(osVersion,"Windows %s%s%s",versionInfo.dwMinorVersion == 0 ? "2000" : 
                    (versionInfo.dwMinorVersion == 1 ? "XP" : "Server 2003"),
                       versionInfo.szCSDVersion[0] == 0 ? "" : " ", versionInfo.szCSDVersion);
         break;
      default:
         strcpy(osVersion,"Windows [Unknown Version]");
         break;
   }

   sprintf(pszBuffer, "%s %s Build %d", computerName, osVersion, versionInfo.dwBuildNumber);
#else
   /* TODO: add UNIX code here */
   printf("GetSysInfoStr: code not implemented\n");
   strcpy(pszBuffer, "UNIX");
#endif
}


//
// Get IP address for local machine
//

DWORD GetLocalIpAddr(void)
{
   INTERFACE_LIST *pIfList;
   DWORD dwAddr = 0;
   int i;

   pIfList = GetLocalInterfaceList();
   if (pIfList != NULL)
   {
      CleanInterfaceList(pIfList);
      
      // Find first interface with IP address
      for(i = 0; i < pIfList->iNumEntries; i++)
         if (pIfList->pInterfaces[i].dwIpAddr != 0)
         {
            dwAddr = pIfList->pInterfaces[i].dwIpAddr;
            break;
         }
      DestroyInterfaceList(pIfList);
   }
   return dwAddr;
}


//
// Execute external command via shell
//

BOOL ExecCommand(char *pszCommand)
{
   BOOL bSuccess = TRUE;

#ifdef _WIN32
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   // Fill in process startup info structure
   memset(&si, 0, sizeof(STARTUPINFO));
   si.cb = sizeof(STARTUPINFO);
   si.dwFlags = 0;

   // Create new process
   if (!CreateProcess(NULL, pszCommand, NULL, NULL, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi))
   {
      WriteLog(MSG_CREATE_PROCESS_FAILED, EVENTLOG_ERROR_TYPE, "se", pszCommand, GetLastError());
      bSuccess = FALSE;
   }
   else
   {
      // Close all handles
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
   }
#else
   /* TODO: add UNIX code here */
#endif

   return bSuccess;
}


//
// Generate SHA1 hash for text string
//

void CreateSHA1Hash(char *pszSource, BYTE *pBuffer)
{
   EVP_MD_CTX ctx;

   EVP_MD_CTX_init(&ctx);
   EVP_Digest(pszSource, (unsigned long)strlen(pszSource), pBuffer, NULL, EVP_sha1(), NULL);
   EVP_MD_CTX_cleanup(&ctx);
}


//
// Load file into memory
//

BYTE *LoadFile(char *pszFileName, DWORD *pdwFileSize)
{
   int fd, iBufPos, iNumBytes, iBytesRead;
   BYTE *pBuffer = NULL;
   struct stat fs;

   DbgPrintf(AF_DEBUG_MISC, "Loading file \"%s\" into memory\n", pszFileName);
   fd = open(pszFileName, O_RDONLY | O_BINARY);
   if (fd != -1)
   {
      if (fstat(fd, &fs) != -1)
      {
         pBuffer = (BYTE *)malloc(fs.st_size + 1);
         if (pBuffer != NULL)
         {
            *pdwFileSize = fs.st_size;
            for(iBufPos = 0; iBufPos < fs.st_size; iBufPos += iBytesRead)
            {
               iNumBytes = min(16384, fs.st_size - iBufPos);
               if ((iBytesRead = read(fd, &pBuffer[iBufPos], iNumBytes)) < 0)
               {
                  DbgPrintf(AF_DEBUG_MISC, "File read operation failed\n");
                  free(pBuffer);
                  pBuffer = NULL;
                  break;
               }
            }
         }
      }
      close(fd);
   }
   return pBuffer;
}
