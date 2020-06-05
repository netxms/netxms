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
bool IsPackageInstalled(const TCHAR *name, const TCHAR *version, const TCHAR *platform)
{
   bool result = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT pkg_id FROM agent_pkg WHERE pkg_name=? AND version=? AND platform=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, version, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, platform, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         result = (DBGetNumRows(hResult) > 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Check if given package ID is valid
 */
bool IsValidPackageId(uint32_t packageId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool result = IsDatabaseRecordExist(hdb, _T("agent_pkg"), _T("pkg_id"), packageId);
   DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Check if package file with given name exist
 */
bool IsPackageFileExist(const TCHAR *fileName)
{
   StringBuffer fullPath = g_netxmsdDataDir;
   fullPath.append(DDIR_PACKAGES);
   fullPath.append(FS_PATH_SEPARATOR);
   fullPath.append(fileName);
   return (_taccess(fullPath, 0) == 0);
}

/**
 * Uninstall (remove) package from server
 */
uint32_t UninstallPackage(uint32_t packageId)
{
   uint32_t rcc;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT pkg_file FROM agent_pkg WHERE pkg_id=%u"), packageId);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         // Delete file from directory
         StringBuffer fileName = g_netxmsdDataDir;
         fileName.append(DDIR_PACKAGES);
         fileName.append(FS_PATH_SEPARATOR);
         fileName.appendPreallocated(DBGetField(hResult, 0, 0, nullptr, 0));
         if ((_taccess(fileName, 0) == -1) || (_tunlink(fileName) == 0))
         {
            // Delete record from database
            ExecuteQueryOnObject(hdb, packageId, _T("DELETE FROM agent_pkg WHERE pkg_id=?"));
            rcc = RCC_SUCCESS;
         }
         else
         {
            rcc = RCC_IO_ERROR;
         }
      }
      else
      {
         rcc = RCC_INVALID_PACKAGE_ID;
      }
      DBFreeResult(hResult);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
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
            bool targetCheckOK = false;
            TCHAR szBuffer[MAX_PATH];

            // Check if package can be deployed on target node
            if (!_tcsicmp(task->platform, _T("src")))
            {
               // Source package, check if target node
               // supports source packages
               if (agentConn->getParameter(_T("Agent.SourcePackageSupport"), szBuffer, 32) == ERR_SUCCESS)
               {
                  targetCheckOK = (_tcstol(szBuffer, nullptr, 0) != 0);
               }
            }
            else
            {
               // Binary package, check target platform
               if (agentConn->getParameter(_T("System.PlatformName"), szBuffer, MAX_PATH) == ERR_SUCCESS)
               {
                  targetCheckOK = (_tcsicmp(szBuffer, task->platform) == 0);
               }
            }

            if (targetCheckOK)
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
                     bool connected = false;

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
                           connected = true;
                           break;   // Connected successfully
                        }
                     }

                     // Last attempt to reconnect
                     if (!connected)
                     {
                        agentConn = node->createAgentConnection();
                        if (agentConn != nullptr)
                           connected = true;
                     }

                     if (connected)
                     {
                        // Check version
                        if (agentConn->getParameter(_T("Agent.Version"), szBuffer, MAX_AGENT_VERSION_LEN) == ERR_SUCCESS)
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
         success ? static_cast<uint16_t>(DEPLOYMENT_STATUS_COMPLETED) : static_cast<uint16_t>(DEPLOYMENT_STATUS_FAILED));
      msg.setField(VID_ERROR_MESSAGE, errorMessage);
      task->session->sendMessage(&msg);
   }
   return THREAD_OK;
}

/**
 * Package deployment thread
 */
void DeploymentManager(PackageDeploymentTask *task)
{
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
      msg.setField(VID_DEPLOYMENT_STATUS, static_cast<uint16_t>(DEPLOYMENT_STATUS_PENDING));
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
   msg.setField(VID_DEPLOYMENT_STATUS, static_cast<uint16_t>(DEPLOYMENT_STATUS_FINISHED));
   task->session->sendMessage(&msg);

   // Cleanup
   delete task;
   MemFree(threadList);
}
