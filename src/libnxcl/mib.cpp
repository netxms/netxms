/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: mib.cpp
**
**/

#include "libnxcl.h"

#ifdef _WIN32
#include <io.h>
#endif


//
// Get list of available MIB files from server
//

DWORD LIBNXCL_EXPORTABLE NXCGetMIBList(NXC_MIB_LIST **ppMibList)
{
   DWORD i, dwRqId, dwRetCode, dwId1, dwId2;
   CSCPMessage msg, *pResponce;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_MIB_LIST);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_MIB_LIST, dwRqId, 2000);
   if (pResponce != NULL)
   {
      *ppMibList = (NXC_MIB_LIST *)MemAlloc(sizeof(NXC_MIB_LIST));
      (*ppMibList)->dwNumFiles = pResponce->GetVariableLong(VID_NUM_MIBS);
      (*ppMibList)->ppszName = (char **)MemAlloc(sizeof(char *) * (*ppMibList)->dwNumFiles);
      (*ppMibList)->ppHash = (BYTE **)MemAlloc(sizeof(BYTE *) * (*ppMibList)->dwNumFiles);
      for(i = 0, dwId1 = VID_MIB_NAME_BASE, dwId2 = VID_MIB_HASH_BASE; 
          i < (*ppMibList)->dwNumFiles; i++, dwId1++, dwId2++)
      {
         (*ppMibList)->ppszName[i] = pResponce->GetVariableStr(dwId1);
         (*ppMibList)->ppHash[i] = (BYTE *)MemAlloc(MD5_DIGEST_SIZE);
         pResponce->GetVariableBinary(dwId2, (*ppMibList)->ppHash[i], MD5_DIGEST_SIZE);
      }
      delete pResponce;
      dwRetCode = RCC_SUCCESS;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}


//
// Destroy MIB list
//

void LIBNXCL_EXPORTABLE NXCDestroyMIBList(NXC_MIB_LIST *pMibList)
{
   DWORD i;

   for(i = 0; i < pMibList->dwNumFiles; i++)
   {
      MemFree(pMibList->ppHash[i]);
      MemFree(pMibList->ppszName[i]);
   }
   MemFree(pMibList->ppHash);
   MemFree(pMibList->ppszName);
   MemFree(pMibList);
}


//
// Download MIB file from server
//

DWORD LIBNXCL_EXPORTABLE NXCDownloadMIBFile(char *pszName, char *pszDestDir)
{
   DWORD i, dwRqId, dwRetCode, dwFileSize, dwNumBytes;
   CSCPMessage msg, *pResponce;
   BYTE *pBuffer;
   char cLastChar, szFileName[MAX_PATH];
   int fd;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_MIB);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MIB_NAME, pszName);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_MIB, dwRqId, 10000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         dwFileSize = pResponce->GetVariableLong(VID_MIB_FILE_SIZE);
         pBuffer = (BYTE *)MemAlloc(dwFileSize);
         if (pBuffer != NULL)
         {
            pResponce->GetVariableBinary(VID_MIB_FILE, pBuffer, dwFileSize);
            cLastChar = pszDestDir[strlen(pszDestDir) - 1];
            sprintf(szFileName, "%s%s%s", pszDestDir, 
                    (cLastChar == '\\') || (cLastChar == '/') ? "" : "/", pszName);
            fd = open(szFileName, O_CREAT | O_WRONLY | O_BINARY, 0644);
            if (fd != -1)
            {
               for(i = 0; i < dwFileSize; i += dwNumBytes)
               {
                  dwNumBytes = min(16384, dwFileSize - i);
                  write(fd, &pBuffer[i], dwNumBytes);
               }
               close(fd);
            }
            else
            {
               dwRetCode = RCC_IO_ERROR;
            }
            MemFree(pBuffer);
         }
         else
         {
            dwRetCode = RCC_OUT_OF_MEMORY;
         }
      }
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}
