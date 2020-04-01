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
** File: package.cpp
**
**/

#include "nxcore.h"
#include <nms_pkg.h>

/**
 * Check if package with specific parameters already installed
 */
BOOL IsPackageInstalled(TCHAR *pszName, TCHAR *pszVersion, TCHAR *pszPlatform)
{
   DB_RESULT hResult;
   TCHAR szQuery[1024], *pszEscName, *pszEscVersion, *pszEscPlatform;
   BOOL bResult = FALSE;

   pszEscName = EncodeSQLString(pszName);
   pszEscVersion = EncodeSQLString(pszVersion);
   pszEscPlatform = EncodeSQLString(pszPlatform);
   _sntprintf(szQuery, 1024, _T("SELECT pkg_id FROM agent_pkg WHERE ")
                             _T("pkg_name='%s' AND version='%s' AND platform='%s'"),
              pszEscName, pszEscVersion, pszEscPlatform);
   free(pszEscName);
   free(pszEscVersion);
   free(pszEscPlatform);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, szQuery);
   if (hResult != nullptr)
   {
      bResult = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return bResult;
}


//
// Check if given package ID is valid
//

BOOL IsValidPackageId(UINT32 dwPkgId)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bResult = FALSE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   _sntprintf(szQuery, 256, _T("SELECT pkg_name FROM agent_pkg WHERE pkg_id=%d"), dwPkgId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != nullptr)
   {
      bResult = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return bResult;
}


//
// Check if package file with given name exist
//

BOOL IsPackageFileExist(const TCHAR *pszFileName)
{
   TCHAR szFullPath[MAX_PATH];

   _tcscpy(szFullPath, g_netxmsdDataDir);
   _tcscat(szFullPath, DDIR_PACKAGES);
   _tcscat(szFullPath, FS_PATH_SEPARATOR);
   _tcscat(szFullPath, pszFileName);
   return (_taccess(szFullPath, 0) == 0);
}


//
// Uninstall (remove) package from server
//

UINT32 UninstallPackage(UINT32 dwPkgId)
{
   TCHAR szQuery[256], szFileName[MAX_PATH], szBuffer[MAX_DB_STRING];
   DB_RESULT hResult;
   UINT32 dwResult;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   _sntprintf(szQuery, 256, _T("SELECT pkg_file FROM agent_pkg WHERE pkg_id=%d"), dwPkgId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         // Delete file from directory
         _tcscpy(szFileName, g_netxmsdDataDir);
         _tcscat(szFileName, DDIR_PACKAGES);
         _tcscat(szFileName, FS_PATH_SEPARATOR);
         _tcscat(szFileName, CHECK_NULL_EX(DBGetField(hResult, 0, 0, szBuffer, MAX_DB_STRING)));
         if ((_taccess(szFileName, 0) == -1) || (_tunlink(szFileName) == 0))
         {
            // Delete record from database
            _sntprintf(szQuery, 256, _T("DELETE FROM agent_pkg WHERE pkg_id=%d"), dwPkgId);
            DBQuery(hdb, szQuery);
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

   DBConnectionPoolReleaseConnection(hdb);
   return dwResult;
}

/**
 * Package deployment worker thread
 */
static THREAD_RESULT THREAD_CALL DeploymentThread(void *arg)
{
   PackageDeploymentTask *task = static_cast<PackageDeploymentTask*>(arg);

   // Read configuration
   uint32_t dwMaxWait = ConfigReadULong(_T("AgentUpgradeWaitTime"), 600);
   if (dwMaxWait % 20 != 0)
      dwMaxWait += 20 - (dwMaxWait % 20);

   // Prepare notification message
   NXCPMessage msg;
   msg.setCode(CMD_INSTALLER_INFO);
   msg.setId(task->requestId);

   while(true)
   {
      // Get node object for upgrade
      Node *node = task->queue.get();
      if (node == nullptr)
         break;   // Queue is empty, exit

      bool success = false;
		const TCHAR *errorMessage = _T("");

      // Preset node id in notification message
      msg.setField(VID_OBJECT_ID, node->getId());

      // Check if node is a management server itself
      if (!(node->getCapabilities() & NC_IS_LOCAL_MGMT))
      {
         // Change deployment status to "Initializing"
         msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_INITIALIZE);
         task->session->sendMessage(&msg);

         // Create agent connection
         shared_ptr<AgentConnectionEx> agentConn = node->createAgentConnection();
         if (agentConn != nullptr)
         {
            BOOL bCheckOK = FALSE;
            TCHAR szBuffer[MAX_PATH];

            // Check if package can be deployed on target node
            if (!_tcsicmp(task->platform, _T("src")))
            {
               // Source package, check if target node
               // supports source packages
               if (agentConn->getParameter(_T("Agent.SourcePackageSupport"), 32, szBuffer) == ERR_SUCCESS)
               {
                  bCheckOK = (_tcstol(szBuffer, nullptr, 0) != 0);
               }
            }
            else
            {
               // Binary package, check target platform
               if (agentConn->getParameter(_T("System.PlatformName"), MAX_PATH, szBuffer) == ERR_SUCCESS)
               {
                  bCheckOK = !_tcsicmp(szBuffer, task->platform);
               }
            }

            if (bCheckOK)
            {
               // Change deployment status to "File Transfer"
               msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_TRANSFER);
               task->session->sendMessage(&msg);

               // Upload package file to agent
               _tcslcpy(szBuffer, g_netxmsdDataDir, MAX_PATH);
               _tcslcat(szBuffer, DDIR_PACKAGES, MAX_PATH);
               _tcslcat(szBuffer, FS_PATH_SEPARATOR, MAX_PATH);
               _tcslcat(szBuffer, task->packageFile, MAX_PATH);
               if (agentConn->uploadFile(szBuffer) == ERR_SUCCESS)
               {
                  if (agentConn->startUpgrade(task->packageFile) == ERR_SUCCESS)
                  {
                     BOOL bConnected = FALSE;

                     // Delete current connection
                     agentConn.reset();

                     // Change deployment status to "Package installation"
                     msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_INSTALLATION);
                     task->session->sendMessage(&msg);

                     // Wait for agent's restart
                     ThreadSleep(20);
                     for(uint32_t i = 20; i < dwMaxWait; i += 20)
                     {
                        ThreadSleep(20);
                        agentConn = node->createAgentConnection();
                        if (agentConn != nullptr)
                        {
                           bConnected = TRUE;
                           break;   // Connected successfully
                        }
                     }

                     // Last attempt to reconnect
                     if (!bConnected)
                     {
                        agentConn = node->createAgentConnection();
                        if (agentConn != nullptr)
                           bConnected = TRUE;
                     }

                     if (bConnected)
                     {
                        // Check version
                        if (agentConn->getParameter(_T("Agent.Version"), MAX_AGENT_VERSION_LEN, szBuffer) == ERR_SUCCESS)
                        {
                           if (!_tcsicmp(szBuffer, task->version))
                           {
                              success = true;
                           }
                           else
                           {
                              errorMessage = _T("Agent's version doesn't match package version after upgrade");
                           }
                        }
                        else
                        {
                           errorMessage = _T("Unable to get agent's version after upgrade");
                        }
                     }
                     else
                     {
                        errorMessage = _T("Unable to contact agent after upgrade");
                     }
                  }
                  else
                  {
                     errorMessage = _T("Unable to start upgrade process");
                  }
               }
               else
               {
                  errorMessage = _T("File transfer failed");
               }
            }
            else
            {
               errorMessage = _T("Package is not compatible with target machine");
            }
         }
         else
         {
            errorMessage = _T("Unable to connect to agent");
         }
      }
      else
      {
         errorMessage = _T("Management server cannot deploy agent to itself");
      }

      // Finish node processing
      msg.setField(VID_DEPLOYMENT_STATUS, 
         success ? (WORD)DEPLOYMENT_STATUS_COMPLETED : (WORD)DEPLOYMENT_STATUS_FAILED);
      msg.setField(VID_ERROR_MESSAGE, errorMessage);
      task->session->sendMessage(&msg);
   }
   return THREAD_OK;
}

/**
 * Package deployment thread
 */
THREAD_RESULT THREAD_CALL DeploymentManager(void *arg)
{
   PackageDeploymentTask *task = static_cast<PackageDeploymentTask*>(arg);

   // Wait for parent initialization completion
   MutexLock(task->mutex);
   MutexUnlock(task->mutex);

   // Read number of upgrade threads
   int numThreads = ConfigReadInt(_T("NumberOfUpgradeThreads"), 10);
   if (numThreads > task->nodeList.size())
      numThreads = task->nodeList.size();

   // Send initial status for each node and queue them for deployment
   NXCPMessage msg;
   msg.setCode(CMD_INSTALLER_INFO);
   msg.setId(task->requestId);
   for(int i = 0; i < task->nodeList.size(); i++)
   {
      Node *node = task->nodeList.get(i);
      task->queue.put(node);
      msg.setField(VID_OBJECT_ID, node->getId());
      msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_PENDING);
      task->session->sendMessage(&msg);
      msg.deleteAllFields();
   }

   // Start worker threads
   THREAD *threadList = MemAllocArray<THREAD>(numThreads);
   for(int i = 0; i < numThreads; i++)
      threadList[i] = ThreadCreateEx(DeploymentThread, 0, task);

   // Wait for all worker threads termination
   for(int i = 0; i < numThreads; i++)
      ThreadJoin(threadList[i]);

   // Send final notification to client
   msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_FINISHED);
   task->session->sendMessage(&msg);

   // Cleanup
   delete task;
   MemFree(threadList);

   return THREAD_OK;
}
