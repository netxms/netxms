/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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

#include "nxcore.h"


//
// Check if package with specific parameters already installed
//

BOOL IsPackageInstalled(TCHAR *pszName, TCHAR *pszVersion, TCHAR *pszPlatform)
{
   DB_RESULT hResult;
   TCHAR szQuery[1024], *pszEscName, *pszEscVersion, *pszEscPlatform;
   BOOL bResult = FALSE;

   pszEscName = EncodeSQLString(pszName);
   pszEscVersion = EncodeSQLString(pszVersion);
   pszEscPlatform = EncodeSQLString(pszPlatform);
   _sntprintf(szQuery, 1024, _T("SELECT pkg_id FROM agent_pkg WHERE "
                                "pkg_name='%s' AND version='%s' AND platform='%s'"),
              pszEscName, pszEscVersion, pszEscPlatform);
   free(pszEscName);
   free(pszEscVersion);
   free(pszEscPlatform);

   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      bResult = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }
   return bResult;
}


//
// Check if package file with given name exist
//

BOOL IsPackageFileExist(TCHAR *pszFileName)
{
   TCHAR szFullPath[MAX_PATH];

   _tcscpy(szFullPath, g_szDataDir);
   _tcscat(szFullPath, DDIR_PACKAGES);
   _tcscat(szFullPath, FS_PATH_SEPARATOR);
   _tcscat(szFullPath, pszFileName);
   return (_taccess(szFullPath, 0) == 0);
}


//
// Uninstall (remove) package from server
//

DWORD UninstallPackage(DWORD dwPkgId)
{
   TCHAR szQuery[256], szFileName[MAX_PATH];
   DB_RESULT hResult;
   DWORD dwResult;

   _sntprintf(szQuery, 256, _T("SELECT pkg_file FROM agent_pkg WHERE pkg_id=%ld"), dwPkgId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         // Delete file from directory
         _tcscpy(szFileName, g_szDataDir);
         _tcscat(szFileName, DDIR_PACKAGES);
         _tcscat(szFileName, FS_PATH_SEPARATOR);
         _tcscat(szFileName, CHECK_NULL_EX(DBGetField(hResult, 0, 0)));
         if (_tunlink(szFileName) == 0)
         {
            // Delete record from database
            _sntprintf(szQuery, 256, _T("DELETE FROM agent_pkg WHERE pkg_id=%ld"), dwPkgId);
            DBQuery(g_hCoreDB, szQuery);
            dwResult = RCC_SUCCESS;
         }
         else
         {
            dwResult = RCC_IO_ERROR;
         }
      }
      else
      {
         dwResult = RCC_INVALID_PACKAGE_ID;
      }
      DBFreeResult(hResult);
   }
   else
   {
      dwResult = RCC_DB_FAILURE;
   }
   return dwResult;
}
