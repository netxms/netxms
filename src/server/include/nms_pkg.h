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
** $module: nms_pkg.h
**
**/

#ifndef _nms_pkg_h_
#define _nms_pkg_h_

//
// Deployment thread startup info
//

typedef struct
{
   MUTEX mutex;    // Synchronization mutex
	ObjectArray<Node> *nodeList;
   ClientSession *pSession;
   DWORD dwRqId;
   DWORD dwPackageId;
   Queue *pQueue;  // Used internally by deployment manager
   TCHAR szPlatform[MAX_PLATFORM_NAME_LEN];
   TCHAR szPkgFile[MAX_PATH];
   TCHAR szVersion[MAX_AGENT_VERSION_LEN];
} DT_STARTUP_INFO;


//
// Package functions
//

BOOL IsPackageInstalled(TCHAR *pszName, TCHAR *pszVersion, TCHAR *pszPlatform);
BOOL IsPackageFileExist(const TCHAR *pszFileName);
BOOL IsValidPackageId(DWORD dwPkgId);
DWORD UninstallPackage(DWORD dwPkgId);
THREAD_RESULT THREAD_CALL DeploymentManager(void *pArg);


#endif   /* _nms_pkg_h_ */
