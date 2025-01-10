/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
   uint32_t userId;
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
      userId = (session != nullptr) ? session->getUserId() : 0;
      packageId = _packageId;
      platform = MemCopyString(_platform);
      packageFile = MemCopyString(_packageFile);
      _tcslcpy(packageType, _packageType, 16);
      _tcslcpy(version, _version, 32);
      command = MemCopyString(_command);
   }

   ~PackageDeploymentTask()
   {
      if (session != nullptr)
         session->decRefCount();
      MemFree(platform);
      MemFree(packageFile);
      MemFree(command);
   }

   void sendMessage(const NXCPMessage& msg)
   {
      if (session != nullptr)
         session->sendMessage(msg);
   }
};

/**
 * Package functions
 */
bool IsPackageInstalled(const TCHAR *name, const TCHAR *version, const TCHAR *platform);
bool IsDuplicatePackageFileName(const TCHAR *fileName);
bool IsValidPackageId(uint32_t packageId);
PackageDeploymentTask *CreatePackageDeploymentTask(uint32_t packageId, ClientSession *session, uint32_t requestId, uint32_t *rcc);
uint32_t UninstallPackage(uint32_t packageId);
void DeploymentManager(PackageDeploymentTask *task);

#endif   /* _nms_pkg_h_ */
