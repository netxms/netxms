/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: package.cpp
**
**/

#include "libnxcl.h"


//
// Lock package database
//

UINT32 LIBNXCL_EXPORTABLE NXCLockPackageDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_LOCK_PACKAGE_DB);
}


//
// Unlock package database
//

UINT32 LIBNXCL_EXPORTABLE NXCUnlockPackageDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_UNLOCK_PACKAGE_DB);
}


//
// Retrieve package list from server
//

UINT32 LIBNXCL_EXPORTABLE NXCGetPackageList(NXC_SESSION hSession, UINT32 *pdwNumPackages, 
                                           NXC_PACKAGE_INFO **ppList)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwResult, dwRqId, dwPkgId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_PACKAGE_LIST);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumPackages = 0;
   *ppList = NULL;

   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwResult == RCC_SUCCESS)
   {
      *pdwNumPackages = 0;
      do
      {
         pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_PACKAGE_INFO, dwRqId);
         if (pResponse != NULL)
         {
            dwPkgId = pResponse->getFieldAsUInt32(VID_PACKAGE_ID);
            if (dwPkgId != 0)
            {
               *ppList = (NXC_PACKAGE_INFO *)realloc(*ppList, sizeof(NXC_PACKAGE_INFO) * (*pdwNumPackages + 1));
               (*ppList)[*pdwNumPackages].dwId = dwPkgId;
               pResponse->getFieldAsString(VID_PACKAGE_NAME, 
                                         (*ppList)[*pdwNumPackages].szName, 
                                         MAX_PACKAGE_NAME_LEN);
               pResponse->getFieldAsString(VID_FILE_NAME, 
                                         (*ppList)[*pdwNumPackages].szFileName, 
                                         MAX_DB_STRING);
               pResponse->getFieldAsString(VID_PLATFORM_NAME, 
                                         (*ppList)[*pdwNumPackages].szPlatform, 
                                         MAX_PLATFORM_NAME_LEN);
               pResponse->getFieldAsString(VID_PACKAGE_VERSION, 
                                         (*ppList)[*pdwNumPackages].szVersion, 
                                         MAX_AGENT_VERSION_LEN);
               pResponse->getFieldAsString(VID_DESCRIPTION, 
                                         (*ppList)[*pdwNumPackages].szDescription, 
                                         MAX_DB_STRING);
               (*pdwNumPackages)++;
            }
            delete pResponse;
         }
         else
         {
            dwResult = RCC_TIMEOUT;
            *pdwNumPackages = 0;
            safe_free(*ppList);
            *ppList = NULL;
            break;
         }
      } while(dwPkgId != 0);
   }

   return dwResult;
}


//
// Remove package from server
//

UINT32 LIBNXCL_EXPORTABLE NXCRemovePackage(NXC_SESSION hSession, UINT32 dwPkgId)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_REMOVE_PACKAGE);
   msg.setId(dwRqId);
   msg.setField(VID_PACKAGE_ID, dwPkgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Install package to server
//

UINT32 LIBNXCL_EXPORTABLE NXCInstallPackage(NXC_SESSION hSession, NXC_PACKAGE_INFO *pInfo,
                                           TCHAR *pszFullPkgPath)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_INSTALL_PACKAGE);
   msg.setId(dwRqId);
   msg.setField(VID_PACKAGE_NAME, pInfo->szName);
   msg.setField(VID_DESCRIPTION, pInfo->szDescription);
   msg.setField(VID_FILE_NAME, pInfo->szFileName);
   msg.setField(VID_PLATFORM_NAME, pInfo->szPlatform);
   msg.setField(VID_PACKAGE_VERSION, pInfo->szVersion);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         // Get id assigned to installed package and
         // update provided package information structure
         pInfo->dwId = pResponse->getFieldAsUInt32(VID_PACKAGE_ID);
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }

   // If everything is OK, send package file to server
   if (dwResult == RCC_SUCCESS)
   {
      dwResult = ((NXCL_Session *)hSession)->SendFile(dwRqId, pszFullPkgPath, 0);
      if (dwResult == RCC_SUCCESS)
      {
         // Wait for final confirmation
         dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
      }
   }

   return dwResult;
}


//
// Parse NPI file
//

UINT32 LIBNXCL_EXPORTABLE NXCParseNPIFile(TCHAR *pszInfoFile, NXC_PACKAGE_INFO *pInfo)
{
   FILE *fp;
   UINT32 dwResult = RCC_SUCCESS;
   TCHAR szBuffer[256], szTag[256], *ptr;

   fp = _tfopen(pszInfoFile, _T("r"));
   if (fp != NULL)
   {
      while(!feof(fp))
      {
         if (_fgetts(szBuffer, 256, fp) == NULL)
				break;
         ptr = _tcschr(szBuffer, _T('\n'));
         if (ptr != NULL)
            *ptr = 0;
         StrStrip(szBuffer);
         if ((szBuffer[0] == _T('#')) || (szBuffer[0] == 0))
            continue;   // Empty line or comment

         ptr = (TCHAR *)ExtractWord(szBuffer, szTag);
         StrStrip(ptr);
         _tcsupr(szTag);

         if (!_tcscmp(szTag, _T("NAME")))
         {
            nx_strncpy(pInfo->szName, ptr, MAX_PACKAGE_NAME_LEN);
         }
         else if (!_tcscmp(szTag, _T("PLATFORM")))
         {
            nx_strncpy(pInfo->szPlatform, ptr, MAX_PLATFORM_NAME_LEN);
         }
         else if (!_tcscmp(szTag, _T("VERSION")))
         {
            nx_strncpy(pInfo->szVersion, ptr, MAX_AGENT_VERSION_LEN);
         }
         else if (!_tcscmp(szTag, _T("DESCRIPTION")))
         {
            nx_strncpy(pInfo->szDescription, ptr, MAX_DB_STRING);
         }
         else if (!_tcscmp(szTag, _T("FILE")))
         {
            nx_strncpy(pInfo->szFileName, GetCleanFileName(ptr), MAX_DB_STRING);
         }
         else
         {
            dwResult = RCC_NPI_PARSE_ERROR;
            break;
         }
      }
      fclose(fp);
   }
   else
   {
      dwResult = RCC_IO_ERROR;
   }

   return dwResult;
}


//
// Start package deployment
//

UINT32 LIBNXCL_EXPORTABLE NXCDeployPackage(NXC_SESSION hSession, UINT32 dwPkgId,
                                          UINT32 dwNumObjects, UINT32 *pdwObjectList,
                                          UINT32 *pdwRqId)
{
   NXCPMessage msg, *pInfo;
   UINT32 dwRqId, dwResult;
   NXC_DEPLOYMENT_STATUS status;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   *pdwRqId = dwRqId;

   msg.setCode(CMD_DEPLOY_PACKAGE);
   msg.setId(dwRqId);
   msg.setField(VID_PACKAGE_ID, dwPkgId);
   msg.setField(VID_NUM_OBJECTS, dwNumObjects);
   msg.setFieldFromInt32Array(VID_OBJECT_LIST, dwNumObjects, pdwObjectList);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwResult == RCC_SUCCESS)
   {
      while(1)
      {
         // Wait up to 10 minutes for notification message from server
         pInfo = ((NXCL_Session *)hSession)->WaitForMessage(CMD_INSTALLER_INFO, dwRqId, 600000);
         if (pInfo != NULL)
         {
            status.dwStatus = pInfo->getFieldAsUInt16(VID_DEPLOYMENT_STATUS);
            if (status.dwStatus == DEPLOYMENT_STATUS_FINISHED)
            {
               delete pInfo;
               break;   // Deployment job finished
            }

            status.dwNodeId = pInfo->getFieldAsUInt32(VID_OBJECT_ID);
            status.pszErrorMessage = pInfo->getFieldAsString(VID_ERROR_MESSAGE);
            ((NXCL_Session *)hSession)->callEventHandler(NXC_EVENT_DEPLOYMENT_STATUS, dwRqId, &status);
            safe_free(status.pszErrorMessage);

            delete pInfo;
         }
         else
         {
            dwResult = RCC_TIMEOUT;
            break;
         }
      }
   }

   return dwResult;
}
