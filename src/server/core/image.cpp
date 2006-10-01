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

#include "nxcore.h"


//
// Update hashes for image files
//

void UpdateImageHashes(void)
{
   DB_RESULT hResult;
   int i, iNumImages, iPathLen, iFormat;
   char szPath[MAX_PATH], szHashText[MD5_DIGEST_SIZE * 2 + 1], szQuery[1024];
   BYTE hash[MD5_DIGEST_SIZE];
   DWORD dwImageId;
   static char szExt[3][4] = { "err", "png", "ico" };

   strcpy(szPath, g_szDataDir);
   strcat(szPath, DDIR_IMAGES);
   strcat(szPath, FS_PATH_SEPARATOR);
   iPathLen = (int)_tcslen(szPath);

   hResult = DBSelect(g_hCoreDB, "SELECT image_id,file_name_png,file_name_ico FROM images");
   if (hResult != NULL)
   {
      iNumImages = DBGetNumRows(hResult);
      for(i = 0; i < iNumImages; i++)
      {
         dwImageId = DBGetFieldULong(hResult, i, 0);
         for(iFormat = 1; iFormat < 3; iFormat++)
         {
            DBGetField(hResult, i, iFormat, &szPath[iPathLen], MAX_PATH - iPathLen);
            if (CalculateFileMD5Hash(szPath, hash))
            {
               // Convert MD5 hash to text form and update database
               BinToStr(hash, MD5_DIGEST_SIZE, szHashText);
               sprintf(szQuery, "UPDATE images SET file_hash_%s='%s' WHERE image_id=%d",
                       szExt[iFormat], szHashText, dwImageId);
               DBQuery(g_hCoreDB, szQuery);
            }
            else
            {
               WriteLog(MSG_IMAGE_FILE_IO_ERROR, EVENTLOG_ERROR_TYPE, "ds", dwImageId, szPath);
            }
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
// Send default images to client
//

void SendDefaultImageList(ClientSession *pSession, DWORD dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   DWORD i, dwListSize, *pdwClassId, *pdwImageId;

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_DEFAULT_IMAGE_LIST);

   // Load default image list from database
   hResult = DBSelect(g_hCoreDB, "SELECT object_class,image_id FROM default_images");
   if (hResult != NULL)
   {
      dwListSize = DBGetNumRows(hResult);
      msg.SetVariable(VID_NUM_IMAGES, dwListSize);
      
      pdwClassId = (DWORD *)malloc(sizeof(DWORD) * dwListSize);
      pdwImageId = (DWORD *)malloc(sizeof(DWORD) * dwListSize);
      for(i = 0; i < dwListSize; i++)
      {
         pdwClassId[i] = DBGetFieldULong(hResult, i, 0);
         pdwImageId[i] = DBGetFieldULong(hResult, i, 1);
      }
      msg.SetVariableToInt32Array(VID_CLASS_ID_LIST, dwListSize, pdwClassId);
      msg.SetVariableToInt32Array(VID_IMAGE_ID_LIST, dwListSize, pdwImageId);
      free(pdwClassId);
      free(pdwImageId);

      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      DBFreeResult(hResult);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }

   // Send response
   pSession->SendMessage(&msg);
}


//
// Send current image catalogue to client
//

void SendImageCatalogue(ClientSession *pSession, DWORD dwRqId, WORD wFormat)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   DWORD i, dwNumImages, dwId;
   BYTE hash[MD5_DIGEST_SIZE];
   TCHAR szQuery[256], szHashText[MD5_DIGEST_SIZE * 2 + 1], szBuffer[MAX_DB_STRING];

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_IMAGE_LIST);

   // Load image catalogue from database
   _stprintf(szQuery, _T("SELECT image_id,name,file_hash_%s FROM images"),
             (wFormat == IMAGE_FORMAT_PNG) ? _T("png") : _T("ico"));
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      dwNumImages = DBGetNumRows(hResult);
      msg.SetVariable(VID_NUM_IMAGES, dwNumImages);
      
      for(i = 0, dwId = VID_IMAGE_LIST_BASE; i < dwNumImages; i++, dwId += 7)
      {
         msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
         msg.SetVariable(dwId++, DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING));
         DBGetField(hResult, i, 2, szHashText, MD5_DIGEST_SIZE * 2 + 1);
         StrToBin(szHashText, hash, MD5_DIGEST_SIZE);
         msg.SetVariable(dwId++, hash, MD5_DIGEST_SIZE);
      }

      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      DBFreeResult(hResult);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }

   // Send response
   pSession->SendMessage(&msg);
}


//
// Send image file to client
//

void SendImageFile(ClientSession *pSession, DWORD dwRqId, DWORD dwImageId, WORD wFormat)
{
   CSCPMessage msg;
   char szQuery[256], szFileName[MAX_PATH];
   DB_RESULT hResult;
   BYTE *pFile;
   DWORD dwFileSize;

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_IMAGE_FILE);

   sprintf(szQuery, "SELECT file_name_%s FROM images WHERE image_id=%d", 
           (wFormat == IMAGE_FORMAT_PNG) ? "png" : "ico", dwImageId);
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
         DBGetField(hResult, 0, 0, &szFileName[_tcslen(szFileName)], MAX_PATH - _tcslen(szFileName));
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

   // Send response
   pSession->SendMessage(&msg);
}
