/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
   if (hResult != NULL)
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
   if (hResult != NULL)
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
   if (hResult != NULL)
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


//
// Package deployment worker thread
//

static THREAD_RESULT THREAD_CALL DeploymentThread(void *pArg)
{
   DT_STARTUP_INFO *pStartup = (DT_STARTUP_INFO *)pArg;
   Node *pNode;
   NXCPMessage msg;
   BOOL bSuccess;
   AgentConnection *pAgentConn;
   const TCHAR *pszErrorMsg;
   UINT32 dwMaxWait;

   // Read configuration
   dwMaxWait = ConfigReadULong(_T("AgentUpgradeWaitTime"), 600);
   if (dwMaxWait % 20 != 0)
      dwMaxWait += 20 - (dwMaxWait % 20);

   // Prepare notification message
   msg.setCode(CMD_INSTALLER_INFO);
   msg.setId(pStartup->dwRqId);

   while(1)
   {
      // Get node object for upgrade
      pNode = (Node *)pStartup->pQueue->get();
      if (pNode == NULL)
         break;   // Queue is empty, exit

      bSuccess = FALSE;
		pszErrorMsg = _T("");

      // Preset node id in notification message
      msg.setField(VID_OBJECT_ID, pNode->getId());

      // Check if node is a management server itself
      if (!(pNode->getFlags() & NF_IS_LOCAL_MGMT))
      {
         // Change deployment status to "Initializing"
         msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_INITIALIZE);
         pStartup->pSession->sendMessage(&msg);

         // Create agent connection
         pAgentConn = pNode->createAgentConnection();
         if (pAgentConn != NULL)
         {
            BOOL bCheckOK = FALSE;
            TCHAR szBuffer[256];

            // Check if package can be deployed on target node
            if (!_tcsicmp(pStartup->szPlatform, _T("src")))
            {
               // Source package, check if target node
               // supports source packages
               if (pAgentConn->getParameter(_T("Agent.SourcePackageSupport"), 32, szBuffer) == ERR_SUCCESS)
               {
                  bCheckOK = (_tcstol(szBuffer, NULL, 0) != 0);
               }
            }
            else
            {
               // Binary package, check target platform
               if (pAgentConn->getParameter(_T("System.PlatformName"), 256, szBuffer) == ERR_SUCCESS)
               {
                  bCheckOK = !_tcsicmp(szBuffer, pStartup->szPlatform);
               }
            }

            if (bCheckOK)
            {
               // Change deployment status to "File Transfer"
               msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_TRANSFER);
               pStartup->pSession->sendMessage(&msg);

               // Upload package file to agent
               _tcscpy(szBuffer, g_netxmsdDataDir);
               _tcscat(szBuffer, DDIR_PACKAGES);
               _tcscat(szBuffer, FS_PATH_SEPARATOR);
               _tcscat(szBuffer, pStartup->szPkgFile);
               if (pAgentConn->uploadFile(szBuffer) == ERR_SUCCESS)
               {
                  if (pAgentConn->startUpgrade(pStartup->szPkgFile) == ERR_SUCCESS)
                  {
                     BOOL bConnected = FALSE;
                     UINT32 i;

                     // Delete current connection
                     pAgentConn->decRefCount();

                     // Change deployment status to "Package installation"
                     msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_INSTALLATION);
                     pStartup->pSession->sendMessage(&msg);

                     // Wait for agent's restart
                     ThreadSleep(20);
                     for(i = 20; i < dwMaxWait; i += 20)
                     {
                        ThreadSleep(20);
                        pAgentConn = pNode->createAgentConnection();
                        if (pAgentConn != NULL)
                        {
                           bConnected = TRUE;
                           break;   // Connected successfully
                        }
                     }

                     // Last attempt to reconnect
                     if (!bConnected)
                     {
                        pAgentConn = pNode->createAgentConnection();
                        if (pAgentConn != NULL)
                           bConnected = TRUE;
                     }

                     if (bConnected)
                     {
                        // Check version
                        if (pAgentConn->getParameter(_T("Agent.Version"), MAX_AGENT_VERSION_LEN, szBuffer) == ERR_SUCCESS)
                        {
                           if (!_tcsicmp(szBuffer, pStartup->szVersion))
                           {
                              bSuccess = TRUE;
                           }
                           else
                           {
                              pszErrorMsg = _T("Agent's version doesn't match package version after upgrade");
                           }
                        }
                        else
                        {
                           pszErrorMsg = _T("Unable to get agent's version after upgrade");
                        }
                     }
                     else
                     {
                        pszErrorMsg = _T("Unable to contact agent after upgrade");
                     }
                  }
                  else
                  {
                     pszErrorMsg = _T("Unable to start upgrade process");
                  }
               }
               else
               {
                  pszErrorMsg = _T("File transfer failed");
               }
            }
            else
            {
               pszErrorMsg = _T("Package is not compatible with target machine");
            }
            if (pAgentConn != NULL)
               pAgentConn->decRefCount();
         }
         else
         {
            pszErrorMsg = _T("Unable to connect to agent");
         }
      }
      else
      {
         pszErrorMsg = _T("Management server cannot deploy agent to itself");
      }

      // Finish node processing
      msg.setField(VID_DEPLOYMENT_STATUS, 
         bSuccess ? (WORD)DEPLOYMENT_STATUS_COMPLETED : (WORD)DEPLOYMENT_STATUS_FAILED);
      msg.setField(VID_ERROR_MESSAGE, pszErrorMsg);
      pStartup->pSession->sendMessage(&msg);
      pNode->decRefCount();
   }
   return THREAD_OK;
}

/**
 * Package deployment thread
 */
THREAD_RESULT THREAD_CALL DeploymentManager(void *pArg)
{
   DT_STARTUP_INFO *pStartup = (DT_STARTUP_INFO *)pArg;
   int i, numThreads;
   NXCPMessage msg;
   Queue *pQueue;
   THREAD *pThreadList;

   // Wait for parent initialization completion
   MutexLock(pStartup->mutex);
   MutexUnlock(pStartup->mutex);

   // Sanity check
   if ((pStartup->nodeList == NULL) || (pStartup->nodeList->size() == 0))
   {
      pStartup->pSession->decRefCount();
      return THREAD_OK;
   }

   // Read number of upgrade threads
   numThreads = ConfigReadInt(_T("NumberOfUpgradeThreads"), 10);
   if (numThreads > pStartup->nodeList->size())
      numThreads = pStartup->nodeList->size();

   // Create processing queue
   pQueue = new Queue;
   pStartup->pQueue = pQueue;

   // Send initial status for each node and queue them for deployment
   msg.setCode(CMD_INSTALLER_INFO);
   msg.setId(pStartup->dwRqId);
   for(i = 0; i < pStartup->nodeList->size(); i++)
   {
      pQueue->put(pStartup->nodeList->get(i));
      msg.setField(VID_OBJECT_ID, pStartup->nodeList->get(i)->getId());
      msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_PENDING);
      pStartup->pSession->sendMessage(&msg);
      msg.deleteAllFields();
   }

   // Start worker threads
   pThreadList = (THREAD *)malloc(sizeof(THREAD) * numThreads);
   for(i = 0; i < numThreads; i++)
      pThreadList[i] = ThreadCreateEx(DeploymentThread, 0, pStartup);

   // Wait for all worker threads termination
   for(i = 0; i < numThreads; i++)
      ThreadJoin(pThreadList[i]);

   // Send final notification to client
   msg.setField(VID_DEPLOYMENT_STATUS, (WORD)DEPLOYMENT_STATUS_FINISHED);
   pStartup->pSession->sendMessage(&msg);
   pStartup->pSession->decRefCount();

   // Cleanup
   MutexDestroy(pStartup->mutex);
   delete pStartup->nodeList;
   free(pStartup);
   free(pThreadList);
   delete pQueue;

   return THREAD_OK;
}
