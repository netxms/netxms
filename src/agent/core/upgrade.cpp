/* 
** NetXMS multiplatform core agent
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
** $module: upgrade.cpp
** Agent self-upgrade module
**
**/

#include "nxagentd.h"


//
// Upgrade agent from given package file
//

DWORD UpgradeAgent(TCHAR *pszPkgFile)
{
#if defined(_WIN32)

   TCHAR szInstallDir[MAX_PATH], szCmdLine[1024];
   HKEY hKey;
   DWORD dwSize;

   // Read current installation directory
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\NetXMS Agent"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
      return ERR_ACCESS_DENIED;

   dwSize = MAX_PATH * sizeof(TCHAR);
   if (RegQueryValueEx(hKey, _T("InstallDir"), NULL, NULL,
                       (BYTE *)szInstallDir, &dwSize) != ERROR_SUCCESS)
      return ERR_INTERNAL_ERROR;

   // Start installation
   _sntprintf(szCmdLine, 1024, _T("msiexec.exe /quiet /qn /i %s TARGETDIR=\"%s\""),
              pszPkgFile, szInstallDir);
   return ExecuteCommand(szCmdLine, NULL);

#else
   chmod(pszPkgFile, 0700);   // Set execute permissions on package file
   return ExecuteCommand(pszPkgFile, NULL);

   //return ERR_NOT_IMPLEMENTED;

#endif
}
