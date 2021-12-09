/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

/**
 * Package deployment task
 */
struct PackageDeploymentTask
{
	SharedObjectArray<Node> nodeList;
   ClientSession *session;
   uint32_t requestId;
   uint32_t packageId;
   ObjectQueue<Node> queue;  // Used internally by deployment manager
   TCHAR *platform;
   TCHAR *packageFile;
   TCHAR packageType[16];
   TCHAR version[32];
   TCHAR *command;

   PackageDeploymentTask(ClientSession *_session, uint32_t _requestId, uint32_t _packageId,
            const TCHAR *_platform, const TCHAR *_packageFile, const TCHAR *_packageType, const TCHAR *_version, const TCHAR *_command)
   {
      session = _session;
      requestId = _requestId;
      packageId = _packageId;
      platform = MemCopyString(_platform);
      packageFile = MemCopyString(_packageFile);
      _tcslcpy(packageType, _packageType, 16);
      _tcslcpy(version, _version, 32);
      command = MemCopyString(_command);
   }

   ~PackageDeploymentTask()
   {
      session->decRefCount();
      MemFree(platform);
      MemFree(packageFile);
      MemFree(command);
   }
};

/**
 * Package functions
 */
bool IsPackageInstalled(const TCHAR *name, const TCHAR *version, const TCHAR *platform);
bool IsPackageFileExist(const TCHAR *fileName);
bool IsValidPackageId(uint32_t packageId);
uint32_t UninstallPackage(uint32_t packageId);
void DeploymentManager(PackageDeploymentTask *task);

#endif   /* _nms_pkg_h_ */
