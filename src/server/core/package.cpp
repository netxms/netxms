/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
   return (_taccess(StringBuffer(g_netxmsdDataDir).append(DDIR_PACKAGES).append(FS_PATH_SEPARATOR).append(fileName), F_OK) == 0);
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
         StringBuffer fileName(g_netxmsdDataDir);
         fileName.append(DDIR_PACKAGES);
         fileName.append(FS_PATH_SEPARATOR);
         fileName.appendPreallocated(DBGetField(hResult, 0, 0, nullptr, 0));
         if ((_taccess(fileName, F_OK) == -1) || (_tunlink(fileName) == 0))
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
 * Upgrade agent
 */
static bool UpgradeAgent(PackageDeploymentTask *task, Node *node, AgentConnectionEx *upgradeConnection, NXCPMessage *msg, const TCHAR **errorMessage)
{
   bool success = false;
   if (upgradeConnection->startUpgrade(task->packageFile) == ERR_SUCCESS)
   {
      bool connected = false;

      upgradeConnection->disconnect();

      // Change deployment status to "Package installation"
      msg->setField(VID_DEPLOYMENT_STATUS, static_cast<uint16_t>(DEPLOYMENT_STATUS_INSTALLATION));
      task->session->sendMessage(msg);

      // Wait for agent's restart
      // Read configuration
      uint32_t maxWaitTime = ConfigReadULong(_T("Agent.Upgrade.WaitTime"), 600);
      if (maxWaitTime % 20 != 0)
         maxWaitTime += 20 - (maxWaitTime % 20);

      shared_ptr<AgentConnectionEx> testConnection;
      ThreadSleep(20);
      for(uint32_t i = 20; i < maxWaitTime; i += 20)
      {
         ThreadSleep(20);
         testConnection = node->createAgentConnection();
         if (testConnection != nullptr)
         {
            connected = true;
            break;   // Connected successfully
         }
      }

      // Last attempt to reconnect
      if (!connected)
      {
         testConnection = node->createAgentConnection();
         if (testConnection != nullptr)
            connected = true;
      }

      if (connected)
      {
         // Check version
         TCHAR version[MAX_AGENT_VERSION_LEN];
         if (testConnection->getParameter(_T("Agent.Version"), version, MAX_AGENT_VERSION_LEN) == ERR_SUCCESS)
         {
            if (!_tcsicmp(version, task->version))
            {
               success = true;
            }
            else
            {
               *errorMessage = _T("Agent's version doesn't match package version after upgrade");
            }
         }
         else
         {
            *errorMessage = _T("Unable to get agent's version after upgrade");
         }
      }
      else
      {
         *errorMessage = _T("Unable to contact agent after upgrade");
      }
   }
   else
   {
      *errorMessage = _T("Unable to start upgrade process");
   }
   return success;
}

/**
 * Package deployment worker thread
 */
static void DeploymentThread(PackageDeploymentTask *task)
{
   // Notification message
   NXCPMessage msg(CMD_INSTALLER_INFO, task->requestId);

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

      // Check if user has rights to deploy packages on that specific node
      if (node->checkAccessRights(task->session->getUserId(), OBJECT_ACCESS_MODIFY | OBJECT_ACCESS_CONTROL | OBJECT_ACCESS_UPLOAD))
      {
         // Check if node is a management server itself
         if (!(node->getCapabilities() & NC_IS_LOCAL_MGMT) || _tcscmp(task->packageType, _T("agent-installer")))
         {
            // Change deployment status to "Initializing"
            msg.setField(VID_DEPLOYMENT_STATUS, static_cast<uint16_t>(DEPLOYMENT_STATUS_INITIALIZE));
            task->session->sendMessage(msg);

            // Create agent connection
            shared_ptr<AgentConnectionEx> agentConn = node->createAgentConnection();
            if (agentConn != nullptr)
            {
               bool targetCheckOK = false;

               // Check if package can be deployed on target node
               if (!_tcsicmp(task->platform, _T("src")) && !_tcscmp(task->packageType, _T("agent-installer")))
               {
                  // Source package, check if target node
                  // supports source packages
                  TCHAR value[32];
                  if (agentConn->getParameter(_T("Agent.SourcePackageSupport"), value, 32) == ERR_SUCCESS)
                  {
                     targetCheckOK = (_tcstol(value, nullptr, 0) != 0);
                  }
               }
               else
               {
                  // Binary package, check target platform
                  TCHAR platform[MAX_RESULT_LENGTH];
                  if (agentConn->getParameter(_T("System.PlatformName"), platform, MAX_RESULT_LENGTH) == ERR_SUCCESS)
                  {
                     targetCheckOK = MatchString(task->platform, platform, false);
                  }
               }

               if (targetCheckOK)
               {
                  // Change deployment status to "File Transfer"
                  msg.setField(VID_DEPLOYMENT_STATUS, static_cast<uint16_t>(DEPLOYMENT_STATUS_TRANSFER));
                  task->session->sendMessage(msg);

                  // Upload package file to agent
                  StringBuffer packageFile(g_netxmsdDataDir);
                  packageFile.append(DDIR_PACKAGES);
                  packageFile.append(FS_PATH_SEPARATOR);
                  packageFile.append(task->packageFile);
                  if (agentConn->uploadFile(packageFile) == ERR_SUCCESS)
                  {
                     if (!_tcscmp(task->packageType, _T("agent-installer")))
                     {
                        success = UpgradeAgent(task, node, agentConn.get(), &msg, &errorMessage);
                     }
                     else
                     {
                        uint32_t maxWaitTime = ConfigReadULong(_T("Agent.Upgrade.WaitTime"), 600);
                        if (maxWaitTime < 30)
                           maxWaitTime = 30;
                        agentConn->setCommandTimeout(maxWaitTime * 1000);
                        uint32_t rcc = agentConn->installPackage(task->packageFile, task->packageType,
                              (task->command[0] == '@') ? node->expandText(&task->command[1]) : task->command);
                        if (rcc == ERR_SUCCESS)
                        {
                           success = true;
                        }
                        else
                        {
                           errorMessage = AgentErrorCodeToText(rcc);
                        }
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
      }
      else
      {
         errorMessage = _T("Access denied");
      }

      // Finish node processing
      msg.setField(VID_DEPLOYMENT_STATUS, 
         success ? static_cast<uint16_t>(DEPLOYMENT_STATUS_COMPLETED) : static_cast<uint16_t>(DEPLOYMENT_STATUS_FAILED));
      msg.setField(VID_ERROR_MESSAGE, errorMessage);
      task->session->sendMessage(msg);

      if (success)
         node->forceConfigurationPoll();
   }
}

/**
 * Package deployment thread
 */
void DeploymentManager(PackageDeploymentTask *task)
{
   // Read number of upgrade threads
   int numThreads = ConfigReadInt(_T("Agent.Upgrade.NumberOfThreads"), 10);
   if (numThreads > task->nodeList.size())
      numThreads = task->nodeList.size();

   // Send initial status for each node and queue them for deployment
   NXCPMessage msg(CMD_INSTALLER_INFO, task->requestId);
   for(int i = 0; i < task->nodeList.size(); i++)
   {
      Node *node = task->nodeList.get(i);
      task->queue.put(node);
      msg.setField(VID_OBJECT_ID, node->getId());
      msg.setField(VID_DEPLOYMENT_STATUS, static_cast<uint16_t>(DEPLOYMENT_STATUS_PENDING));
      task->session->sendMessage(msg);
      msg.deleteAllFields();
   }

   // Start worker threads
   THREAD *threadList = MemAllocArray<THREAD>(numThreads);
   for(int i = 0; i < numThreads; i++)
      threadList[i] = ThreadCreateEx(DeploymentThread, task);

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
