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
// Download image file from server
//

DWORD LIBNXCL_EXPORTABLE NXCLoadImageFile(DWORD dwImageId, char *pszCacheDir)
{
   DWORD i, dwRqId, dwRetCode, dwFileSize, dwNumBytes;
   CSCPMessage msg, *pResponce;
   BYTE *pBuffer;
   char cLastChar, szFileName[MAX_PATH];
   int fd;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_LOAD_IMAGE_FILE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IMAGE_ID, dwImageId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_IMAGE_FILE, dwRqId, 30000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         dwFileSize = pResponce->GetVariableLong(VID_IMAGE_FILE_SIZE);
         pBuffer = (BYTE *)MemAlloc(dwFileSize);
         if (pBuffer != NULL)
         {
            pResponce->GetVariableBinary(VID_IMAGE_FILE, pBuffer, dwFileSize);
            cLastChar = pszCacheDir[strlen(pszCacheDir) - 1];
            sprintf(szFileName, "%s%s%lu.png", pszCacheDir, 
                    (cLastChar == '\\') || (cLastChar == '/') ? "" : "/", dwImageId);
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


//
// Synchronize single image file
//

static DWORD SyncImageFile(char *pszCacheDir, DWORD dwImageId, BYTE *pServerHash)
{
   char szFileName[MAX_PATH];
   BYTE hash[MD5_DIGEST_SIZE];
   DWORD dwRetCode = RCC_SUCCESS;

   sprintf(szFileName, "%s" FS_PATH_SEPARATOR "%lu.png", pszCacheDir, dwImageId);
   memset(hash, 0, MD5_DIGEST_SIZE);
   CalculateFileMD5Hash(szFileName, hash);
   if (memcmp(hash, pServerHash, MD5_DIGEST_SIZE))
      // Hash not match, need to download file
      dwRetCode = NXCLoadImageFile(dwImageId, pszCacheDir);
   return dwRetCode;
}


//
// Synchronize images with client
//

DWORD LIBNXCL_EXPORTABLE NXCSyncImages(NXC_IMAGE_LIST **ppImageList, char *pszCacheDir)
{
   DWORD i, dwRqId, dwRetCode;
   CSCPMessage msg, *pResponce;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_IMAGE_LIST);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_IMAGE_LIST, dwRqId, 5000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppImageList = (NXC_IMAGE_LIST *)MemAlloc(sizeof(NXC_IMAGE_LIST));
         (*ppImageList)->dwNumImages = pResponce->GetVariableLong(VID_NUM_IMAGES);
         if ((*ppImageList)->dwNumImages > 0)
         {
            (*ppImageList)->pImageList = (NXC_IMAGE *)MemAlloc(sizeof(NXC_IMAGE) * (*ppImageList)->dwNumImages);
            pResponce->GetVariableBinary(VID_IMAGE_LIST, (BYTE *)(*ppImageList)->pImageList, 
                                         sizeof(NXC_IMAGE) * (*ppImageList)->dwNumImages);
            for(i = 0; i < (*ppImageList)->dwNumImages; i++)
            {
               (*ppImageList)->pImageList[i].dwId = ntohl((*ppImageList)->pImageList[i].dwId);
               dwRetCode = SyncImageFile(pszCacheDir, (*ppImageList)->pImageList[i].dwId,
                                         (*ppImageList)->pImageList[i].hash);
               if (dwRetCode != RCC_SUCCESS)
               {
                  NXCDestroyImageList(*ppImageList);
                  break;
               }
            }
         }
         else
         {
            (*ppImageList)->pImageList = NULL;
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


//
// Destroy image list
//

void LIBNXCL_EXPORTABLE NXCDestroyImageList(NXC_IMAGE_LIST *pImageList)
{
   if (pImageList != NULL)
   {
      MemFree(pImageList->pImageList);
      MemFree(pImageList);
   }
}
