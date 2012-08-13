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
   TCHAR szCmdLine[1024];

#if defined(_WIN32)

   // Start installation
   _sntprintf(szCmdLine, 1024, _T("\"%s\" /VERYSILENT /SUPPRESSMSGBOXES"), pszPkgFile);
   return ExecuteCommand(szCmdLine, NULL, NULL);

#elif defined(_NETWARE)

   return ERR_NOT_IMPLEMENTED;

#else

   _tchmod(pszPkgFile, 0700);   // Set execute permissions on package file
   _sntprintf(szCmdLine, 1024, _T("\"%s\" version=") NETXMS_VERSION_STRING
                               _T(" prefix=") PREFIX
#ifdef __DISABLE_ICONV
										 _T(" opt=--disable-iconv")
#endif
                               _T(" config=%s"), pszPkgFile, g_szConfigFile);
   return ExecuteCommand(szCmdLine, NULL, NULL);

#endif
}
