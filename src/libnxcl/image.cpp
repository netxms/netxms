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

#if defined(_WIN32) && !defined(UNDER_CE)
#include <io.h>
#endif


//
// Download image file from server
//

DWORD LIBNXCL_EXPORTABLE NXCLoadImageFile(DWORD dwImageId, TCHAR *pszCacheDir, WORD wFormat)
{
   DWORD i, dwRqId, dwRetCode, dwFileSize, dwNumBytes;
   CSCPMessage msg, *pResponce;
   BYTE *pBuffer;
   TCHAR cLastChar, szFileName[MAX_PATH];
   FILE *hFile;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_LOAD_IMAGE_FILE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IMAGE_ID, dwImageId);
   msg.SetVariable(VID_IMAGE_FORMAT, wFormat);
   SendMsg(&msg);

   // Loading  file can take time, so we use 60 sec. timeout instead of default
   pResponce = WaitForMessage(CMD_IMAGE_FILE, dwRqId, 60000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         dwFileSize = pResponce->GetVariableLong(VID_IMAGE_FILE_SIZE);
         pBuffer = (BYTE *)malloc(dwFileSize);
         if (pBuffer != NULL)
         {
            pResponce->GetVariableBinary(VID_IMAGE_FILE, pBuffer, dwFileSize);
            cLastChar = pszCacheDir[_tcslen(pszCacheDir) - 1];
            _stprintf(szFileName, _T("%s%s%08x.%s"), pszCacheDir, 
                    (cLastChar == _T('\\')) || (cLastChar == _T('/')) ? _T("") : FS_PATH_SEPARATOR, 
                    dwImageId, (wFormat == IMAGE_FORMAT_PNG) ? _T("png") : _T("ico"));
#ifndef _WIN32
			//umask(0потом_посчитаю);
#endif
            hFile = _tfopen(szFileName, _T("wb"));
            if (hFile != NULL)
            {
               for(i = 0; i < dwFileSize; i += dwNumBytes)
               {
                  dwNumBytes = min(16384, dwFileSize - i);
                  fwrite(&pBuffer[i], 1, dwNumBytes, hFile);
               }
               fclose(hFile);
            }
            else
            {
               dwRetCode = RCC_IO_ERROR;
            }
            free(pBuffer);
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

static DWORD SyncImageFile(TCHAR *pszCacheDir, DWORD dwImageId, BYTE *pServerHash, WORD wFormat)
{
   TCHAR szFileName[MAX_PATH];
   BYTE hash[MD5_DIGEST_SIZE];
   DWORD dwRetCode = RCC_SUCCESS;

   _stprintf(szFileName, _T("%s") FS_PATH_SEPARATOR _T("%08x.%s"), pszCacheDir, dwImageId,
           (wFormat == IMAGE_FORMAT_PNG) ? _T("png") : _T("ico"));
   memset(hash, 0, MD5_DIGEST_SIZE);
   CalculateFileMD5Hash(szFileName, hash);
   if (memcmp(hash, pServerHash, MD5_DIGEST_SIZE))
      // Hash not match, need to download file
      dwRetCode = NXCLoadImageFile(dwImageId, pszCacheDir, wFormat);
   return dwRetCode;
}


//
// Synchronize images with client
//

DWORD LIBNXCL_EXPORTABLE NXCSyncImages(NXC_IMAGE_LIST **ppImageList, TCHAR *pszCacheDir, WORD wFormat)
{
   DWORD i, dwRqId, dwRetCode;
   CSCPMessage msg, *pResponce;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_IMAGE_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IMAGE_FORMAT, wFormat);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_IMAGE_LIST, dwRqId, g_dwCommandTimeout * 2);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppImageList = (NXC_IMAGE_LIST *)malloc(sizeof(NXC_IMAGE_LIST));
         (*ppImageList)->dwNumImages = pResponce->GetVariableLong(VID_NUM_IMAGES);
         if ((*ppImageList)->dwNumImages > 0)
         {
            (*ppImageList)->pImageList = (NXC_IMAGE *)malloc(sizeof(NXC_IMAGE) * (*ppImageList)->dwNumImages);
            pResponce->GetVariableBinary(VID_IMAGE_LIST, (BYTE *)(*ppImageList)->pImageList, 
                                         sizeof(NXC_IMAGE) * (*ppImageList)->dwNumImages);
            for(i = 0; i < (*ppImageList)->dwNumImages; i++)
            {
               (*ppImageList)->pImageList[i].dwId = ntohl((*ppImageList)->pImageList[i].dwId);
               dwRetCode = SyncImageFile(pszCacheDir, (*ppImageList)->pImageList[i].dwId,
                                         (*ppImageList)->pImageList[i].hash, wFormat);
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
      safe_free(pImageList->pImageList);
      free(pImageList);
   }
}


//
// Load default image list
//

DWORD LIBNXCL_EXPORTABLE NXCLoadDefaultImageList(DWORD *pdwListSize, 
                                                 DWORD **ppdwClassId, DWORD **ppdwImageId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_DEFAULT_IMAGE_LIST);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_DEFAULT_IMAGE_LIST, dwRqId, g_dwCommandTimeout);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwListSize = pResponce->GetVariableLong(VID_NUM_IMAGES);
         *ppdwClassId = (DWORD *)malloc(sizeof(DWORD) * *pdwListSize);
         *ppdwImageId = (DWORD *)malloc(sizeof(DWORD) * *pdwListSize);
         pResponce->GetVariableInt32Array(VID_CLASS_ID_LIST, *pdwListSize, *ppdwClassId);
         pResponce->GetVariableInt32Array(VID_IMAGE_ID_LIST, *pdwListSize, *ppdwImageId);
      }
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}
