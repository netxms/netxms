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
#include "msi.h"


//
// Upgrade agent from given package file
//

DWORD UpgradeAgent(TCHAR *pszPkgFile)
{
   TCHAR szOptions[1024];
   HKEY hKey;
   DWORD dwSize;

   // Read current installation directory
   // and set szOptions to TARGETDIR="<install_dir>"
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\NetXMS Agent"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
      return ERR_ACCESS_DENIED;

   _tcscpy(szOptions, _T("TARGETDIR=\""));
   dwSize = sizeof(szOptions) - 11 * sizeof(TCHAR);
   if (RegQueryValueEx(hKey, _T("InstallDir"), NULL, NULL,
                       (BYTE *)&szOptions[11], &dwSize) != ERROR_SUCCESS)
      return ERR_ACCESS_DENIED;
   _tcscat(szOptions, _T("\""));

   // Start installation
   MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
   MsiInstallProduct(pszPkgFile, szOptions);
   
   return ERR_SUCCESS;
}
