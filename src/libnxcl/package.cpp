/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
** $module: package.cpp
**
**/

#include "libnxcl.h"


//
// Lock/unlock package database
//

static DWORD LockPackageDB(NXCL_Session *pSession, BOOL bLock)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = pSession->CreateRqId();

   msg.SetCode(bLock ? CMD_LOCK_PACKAGE_DB : CMD_UNLOCK_PACKAGE_DB);
   msg.SetId(dwRqId);
   pSession->SendMsg(&msg);

   return pSession->WaitForRCC(dwRqId);
}


//
// Client interface: lock package database
//

DWORD LIBNXCL_EXPORTABLE NXCLockPackageDB(NXC_SESSION hSession)
{
   return LockPackageDB((NXCL_Session *)hSession, TRUE);
}


//
// Client interface: unlock package database
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockPackageDB(NXC_SESSION hSession)
{
   return LockPackageDB((NXCL_Session *)hSession, FALSE);
}


//
// Retrieve package list from server
//

DWORD LIBNXCL_EXPORTABLE NXCGetPackageList(NXC_SESSION hSession, DWORD *pdwNumPackages, 
                                           NXC_PACKAGE_INFO **ppList)
{
   CSCPMessage msg, *pResponce;
   DWORD dwResult, dwRqId, dwPkgId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_PACKAGE_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   *pdwNumPackages = 0;
   *ppList = NULL;
   if (dwResult == RCC_SUCCESS)
   {
      *pdwNumPackages = 0;
      do
      {
         pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_PACKAGE_INFO, dwRqId);
         if (pResponce != NULL)
         {
            dwPkgId = pResponce->GetVariableLong(VID_PACKAGE_ID);
            if (dwPkgId != 0)
            {
               *ppList = (NXC_PACKAGE_INFO *)realloc(*ppList, sizeof(NXC_PACKAGE_INFO) * (*pdwNumPackages + 1));
               (*ppList)[*pdwNumPackages].dwId = dwPkgId;
               pResponce->GetVariableStr(VID_PACKAGE_NAME, 
                                         (*ppList)[*pdwNumPackages].szName, 
                                         MAX_PACKAGE_NAME_LEN);
               pResponce->GetVariableStr(VID_FILE_NAME, 
                                         (*ppList)[*pdwNumPackages].szFileName, 
                                         MAX_DB_STRING);
               pResponce->GetVariableStr(VID_PLATFORM_NAME, 
                                         (*ppList)[*pdwNumPackages].szPlatform, 
                                         MAX_PLATFORM_NAME_LEN);
               pResponce->GetVariableStr(VID_PACKAGE_VERSION, 
                                         (*ppList)[*pdwNumPackages].szVersion, 
                                         MAX_AGENT_VERSION_LEN);
               pResponce->GetVariableStr(VID_DESCRIPTION, 
                                         (*ppList)[*pdwNumPackages].szDescription, 
                                         MAX_DB_STRING);
               (*pdwNumPackages)++;
            }
            delete pResponce;
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

DWORD LIBNXCL_EXPORTABLE NXCRemovePackage(NXC_SESSION hSession, DWORD dwPkgId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_REMOVE_PACKAGE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PACKAGE_ID, dwPkgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Install package to server
//

DWORD LIBNXCL_EXPORTABLE NXCInstallPackage(NXC_SESSION hSession, NXC_PACKAGE_INFO *pInfo,
                                           TCHAR *pszFullPkgPath)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_INSTALL_PACKAGE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PACKAGE_NAME, pInfo->szName);
   msg.SetVariable(VID_DESCRIPTION, pInfo->szDescription);
   msg.SetVariable(VID_FILE_NAME, pInfo->szFileName);
   msg.SetVariable(VID_PLATFORM_NAME, pInfo->szPlatform);
   msg.SetVariable(VID_PACKAGE_VERSION, pInfo->szVersion);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponce != NULL)
   {
      dwResult = pResponce->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         // Get id assigned to installed package and
         // update provided package information structure
         pInfo->dwId = pResponce->GetVariableLong(VID_PACKAGE_ID);
      }
      delete pResponce;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }

   // If everything is OK, send package file to server
   if (dwResult == RCC_SUCCESS)
   {
      dwResult = ((NXCL_Session *)hSession)->SendFile(dwRqId, pszFullPkgPath);
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

DWORD LIBNXCL_EXPORTABLE NXCParseNPIFile(TCHAR *pszInfoFile, NXC_PACKAGE_INFO *pInfo)
{
   FILE *fp;
   DWORD dwResult = RCC_SUCCESS;
   TCHAR szBuffer[256], szTag[256], *ptr;

   fp = _tfopen(pszInfoFile, _T("r"));
   if (fp != NULL)
   {
      while(1)
      {
         fgets(szBuffer, 256, fp);
         if (feof(fp))
            break;
         ptr = _tcschr(szBuffer, _T('\n'));
         if (ptr != NULL)
            *ptr = 0;
         StrStrip(szBuffer);
         if ((szBuffer[0] == _T('#')) || (szBuffer[0] == 0))
            continue;   // Empty line or comment

         ptr = ExtractWord(szBuffer, szTag);
         StrStrip(ptr);
         _tcsupr(szTag);

         if (!_tcscmp(szTag, _T("NAME")))
         {
            _tcsncpy(pInfo->szName, ptr, MAX_PACKAGE_NAME_LEN);
         }
         else if (!_tcscmp(szTag, _T("PLATFORM")))
         {
            _tcsncpy(pInfo->szPlatform, ptr, MAX_PLATFORM_NAME_LEN);
         }
         else if (!_tcscmp(szTag, _T("VERSION")))
         {
            _tcsncpy(pInfo->szVersion, ptr, MAX_AGENT_VERSION_LEN);
         }
         else if (!_tcscmp(szTag, _T("DESCRIPTION")))
         {
            _tcsncpy(pInfo->szDescription, ptr, MAX_DB_STRING);
         }
         else if (!_tcscmp(szTag, _T("FILE")))
         {
            _tcsncpy(pInfo->szFileName, GetCleanFileName(ptr), MAX_DB_STRING);
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

DWORD LIBNXCL_EXPORTABLE NXCDeployPackage(NXC_SESSION hSession, DWORD dwPkgId,
                                          DWORD dwNumObjects, DWORD *pdwObjectList,
                                          DWORD *pdwRqId)
{
   CSCPMessage msg, *pInfo;
   DWORD dwRqId, dwResult;
   NXC_DEPLOYMENT_STATUS status;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   *pdwRqId = dwRqId;

   msg.SetCode(CMD_DEPLOY_PACKAGE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PACKAGE_ID, dwPkgId);
   msg.SetVariable(VID_NUM_OBJECTS, dwNumObjects);
   msg.SetVariableToInt32Array(VID_OBJECT_LIST, dwNumObjects, pdwObjectList);
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
            status.dwStatus = pInfo->GetVariableShort(VID_DEPLOYMENT_STATUS);
            if (status.dwStatus == DEPLOYMENT_STATUS_FINISHED)
            {
               delete pInfo;
               break;   // Deployment job finished
            }

            status.dwNodeId = pInfo->GetVariableLong(VID_OBJECT_ID);
            status.pszErrorMessage = pInfo->GetVariableStr(VID_ERROR_MESSAGE);
            ((NXCL_Session *)hSession)->CallEventHandler(NXC_EVENT_DEPLOYMENT_STATUS, dwRqId, &status);
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
