/* 
** NetXMS - Network Management System
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
** $module: image.cpp
**
**/

#include "nms_core.h"


//
// Update hashes for image files
//

void UpdateImageHashes(void)
{
   DB_RESULT hResult;
   int i, j, iNumImages, iPathLen;
   char szPath[MAX_PATH], szHashText[MD5_DIGEST_SIZE * 2 + 1], szQuery[1024];
   BYTE hash[MD5_DIGEST_SIZE];
   DWORD dwImageId;

   strcpy(szPath, g_szDataDir);
   strcat(szPath, DDIR_IMAGES);
   strcat(szPath, FS_PATH_SEPARATOR);
   iPathLen = strlen(szPath);

   hResult = DBSelect(g_hCoreDB, "SELECT image_id,file_name FROM images");
   if (hResult != NULL)
   {
      iNumImages = DBGetNumRows(hResult);
      for(i = 0; i < iNumImages; i++)
      {
         strcpy(&szPath[iPathLen], DBGetField(hResult, i, 1));
         dwImageId = DBGetFieldULong(hResult, i, 0);
         if (CalculateFileMD5Hash(szPath, hash))
         {
            // Convert MD5 hash to text form
            for(j = 0; j < MD5_DIGEST_SIZE; j++)
               sprintf(&szHashText[j << 1], "%02x", hash[j]);

            sprintf(szQuery, "UPDATE images SET file_hash='%s' WHERE image_id=%ld",
                    szHashText, dwImageId);
            DBQuery(g_hCoreDB, szQuery);
         }
         else
         {
            WriteLog(MSG_IMAGE_FILE_IO_ERROR, EVENTLOG_ERROR_TYPE, "ds", dwImageId, szPath);
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      WriteLog(MSG_ERROR_READ_IMAGE_CATALOG, EVENTLOG_ERROR_TYPE, NULL);
   }
}


//
// Send current image catalogue to client
//

void SendImageCatalogue(ClientSession *pSession, DWORD dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   DWORD i, j, k, dwNumImages;
   NXC_IMAGE *pImageList;
   char szHashText[MD5_DIGEST_SIZE * 2 + 1];

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_IMAGE_LIST);

   // Load image catalogue from database
   hResult = DBSelect(g_hCoreDB, "SELECT image_id,name,file_hash FROM images");
   if (hResult != NULL)
   {
      dwNumImages = DBGetNumRows(hResult);
      msg.SetVariable(VID_NUM_IMAGES, dwNumImages);
      
      pImageList = (NXC_IMAGE *)malloc(sizeof(NXC_IMAGE) * dwNumImages);
      for(i = 0; i < dwNumImages; i++)
      {
         pImageList[i].dwId = htonl(DBGetFieldULong(hResult, i, 0));
         strncpy(pImageList[i].szName, DBGetField(hResult, i, 1), MAX_OBJECT_NAME);
         strncpy(szHashText, DBGetField(hResult, i, 2), MD5_DIGEST_SIZE * 2 + 1);
         for(j = 0, k = 0; j < MD5_DIGEST_SIZE; j++)
         {
            char ch1, ch2;

            ch1 = szHashText[k++];
            ch2 = szHashText[k++];
            pImageList[i].hash[j] = (hex2bin(ch1) << 4) | hex2bin(ch2);
         }
      }
      msg.SetVariable(VID_IMAGE_LIST, (BYTE *)pImageList, sizeof(NXC_IMAGE) * dwNumImages);
      free(pImageList);

      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      DBFreeResult(hResult);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }

   // Send responce
   pSession->SendMessage(&msg);
}


//
// Send image file to client
//

void SendImageFile(ClientSession *pSession, DWORD dwRqId, DWORD dwImageId)
{
   CSCPMessage msg;
   char szQuery[256], szFileName[MAX_PATH];
   DB_RESULT hResult;
   BYTE *pFile;
   DWORD dwFileSize;

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_IMAGE_FILE);

   sprintf(szQuery, "SELECT file_name FROM images WHERE image_id=%ld", dwImageId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         // Load file into memory
         strcpy(szFileName, g_szDataDir);
         strcat(szFileName, DDIR_IMAGES);
#ifdef _WIN32
         strcat(szFileName, "\\");
#else
         strcat(szFileName, "/");
#endif
         strcat(szFileName, DBGetField(hResult, 0, 0));
         pFile = LoadFile(szFileName, &dwFileSize);
         if (pFile != NULL)
         {
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            msg.SetVariable(VID_IMAGE_FILE_SIZE, dwFileSize);
            msg.SetVariable(VID_IMAGE_FILE, pFile, dwFileSize);
            free(pFile);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_SYSTEM_FAILURE);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
      DBFreeResult(hResult);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }

   // Send responce
   pSession->SendMessage(&msg);
}
