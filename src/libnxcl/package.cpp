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
               pResponce->GetVariableStr(VID_FILE_NAME, 
                                         (*ppList)[*pdwNumPackages].szFileName, 
                                         MAX_DB_STRING);
               pResponce->GetVariableStr(VID_PLATFORM_NAME, 
                                         (*ppList)[*pdwNumPackages].szPlatform, 
                                         MAX_PLATFORM_NAME_LEN);
               pResponce->GetVariableStr(VID_PACKAGE_VERSION, 
                                         (*ppList)[*pdwNumPackages].szVersion, 
                                         MAX_AGENT_VERSION_LEN);
               *pdwNumPackages++;
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

   msg.SetCode(CMD_GET_PACKAGE_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PACKAGE_ID, dwPkgId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Install package to server
//

DWORD LIBNXCL_EXPORTABLE NXCInstallPackage(NXC_SESSION hSession, TCHAR *pszPkgFile, DWORD *pdwPkgId)
{
   return 0;
}
