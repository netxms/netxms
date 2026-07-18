/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
#include <queue>

#define DEBUG_TAG _T("packages")

/**
 * Retry backoff parameters for failed deployments
 */
#define PKG_RETRY_BASE_INTERVAL   300    // 5 minutes
#define PKG_RETRY_MAX_INTERVAL    3600   // 1 hour

/**
 * Check if package with specific parameters already installed
 */
bool NXCORE_EXPORTABLE IsPackageInstalled(const wchar_t *name, const wchar_t *version, const wchar_t *platform)
{
   bool result = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT pkg_id FROM agent_pkg WHERE pkg_name=? AND version=? AND platform=?");
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
bool NXCORE_EXPORTABLE IsValidPackageId(uint32_t packageId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool result = IsDatabaseRecordExist(hdb, L"agent_pkg", L"pkg_id", packageId);
   DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Check if package file with given name already used by another package
 */
bool NXCORE_EXPORTABLE IsDuplicatePackageFileName(const wchar_t *fileName)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool result = IsDatabaseRecordExist(hdb, L"agent_pkg", L"pkg_file", fileName);
   DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Get package details from database
 */
uint32_t NXCORE_EXPORTABLE GetPackageDetails(uint32_t packageId, PackageDetails *details)
{
   uint32_t rcc;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT platform,pkg_file,pkg_type,pkg_name,version,command,description FROM agent_pkg WHERE pkg_id=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, packageId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            details->id = packageId;
            DBGetField(hResult, 0, 0, details->platform, MAX_PLATFORM_NAME_LEN);
            details->packageFile = DBGetFieldAsString(hResult, 0, 1);
            DBGetField(hResult, 0, 2, details->type, 16);
            DBGetField(hResult, 0, 3, details->name, MAX_OBJECT_NAME);
            DBGetField(hResult, 0, 4, details->version, 32);
            details->command = DBGetFieldAsString(hResult, 0, 5);
            details->description = DBGetFieldAsString(hResult, 0, 6);
            rcc = RCC_SUCCESS;
         }
         else
         {
            rcc = RCC_INVALID_PACKAGE_ID;
            nxlog_debug_tag(DEBUG_TAG, 5, L"GetPackageDetails: invalid package id %u", packageId);
         }
         DBFreeResult(hResult);
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Get all deployment packages (for NXSL)
 */
ObjectArray<PackageDetails> NXCORE_EXPORTABLE *GetAllPackages(const TCHAR *platformFilter, const TCHAR *typeFilter)
{
   ObjectArray<PackageDetails> *packages = new ObjectArray<PackageDetails>(16, 16, Ownership::True);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult;

   if ((platformFilter != nullptr) && (typeFilter != nullptr))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT pkg_id,pkg_type,pkg_name,version,platform,pkg_file,command,description FROM agent_pkg WHERE platform=? AND pkg_type=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, platformFilter, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, typeFilter, DB_BIND_STATIC);
         hResult = DBSelectPrepared(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         hResult = nullptr;
      }
   }
   else if (platformFilter != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT pkg_id,pkg_type,pkg_name,version,platform,pkg_file,command,description FROM agent_pkg WHERE platform=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, platformFilter, DB_BIND_STATIC);
         hResult = DBSelectPrepared(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         hResult = nullptr;
      }
   }
   else if (typeFilter != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT pkg_id,pkg_type,pkg_name,version,platform,pkg_file,command,description FROM agent_pkg WHERE pkg_type=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, typeFilter, DB_BIND_STATIC);
         hResult = DBSelectPrepared(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         hResult = nullptr;
      }
   }
   else
   {
      hResult = DBSelect(hdb, _T("SELECT pkg_id,pkg_type,pkg_name,version,platform,pkg_file,command,description FROM agent_pkg"));
   }

   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         PackageDetails *p = new PackageDetails();
         p->id = DBGetFieldUInt32(hResult, i, 0);
         DBGetField(hResult, i, 1, p->type, 16);
         DBGetField(hResult, i, 2, p->name, MAX_OBJECT_NAME);
         DBGetField(hResult, i, 3, p->version, 32);
         DBGetField(hResult, i, 4, p->platform, MAX_PLATFORM_NAME_LEN);
         p->packageFile = DBGetFieldAsString(hResult, i, 5);
         p->command = DBGetFieldAsString(hResult, i, 6);
         p->description = DBGetFieldAsString(hResult, i, 7);
         packages->add(p);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return packages;
}

/**
 * Deployment job comparator (used for task sorting)
 */
struct DeploymentJobComparator
{
   bool operator() (const shared_ptr<PackageDeploymentJob>& lhs, const shared_ptr<PackageDeploymentJob>& rhs) const
   {
      return lhs->getExecutionTime() > rhs->getExecutionTime();
   }
};

/**
 * Deployment jobs
 */
static Mutex s_jobLock(MutexType::FAST);
static SharedPointerIndex<PackageDeploymentJob> s_jobs;
static std::priority_queue<shared_ptr<PackageDeploymentJob>, std::vector<shared_ptr<PackageDeploymentJob>>, DeploymentJobComparator> s_jobQueue;
static Condition s_wakeupCondition(true);

/**
 * Uninstall (remove) package from server
 */
uint32_t NXCORE_EXPORTABLE UninstallPackage(uint32_t packageId)
{
   uint32_t rcc;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[256];
   _sntprintf(query, 256, L"SELECT pkg_file FROM agent_pkg WHERE pkg_id=%u", packageId);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         TCHAR fileName[256];
         DBGetField(hResult, 0, 0, fileName, 256);

         // Delete file from directory
         StringBuffer filePath(g_netxmsdDataDir);
         filePath.append(DDIR_PACKAGES);
         filePath.append(FS_PATH_SEPARATOR);
         filePath.append(fileName);
         if ((_taccess(filePath, F_OK) == -1) || (_tremove(filePath) == 0))
         {
            // Delete record from database
            ExecuteQueryOnObject(hdb, packageId, L"DELETE FROM agent_pkg WHERE pkg_id=?");
            nxlog_debug_tag(DEBUG_TAG, 4, L"Package [%u] \"%s\" deleted from server", packageId, fileName);
            rcc = RCC_SUCCESS;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"Cannot delete package file \"%s\" (package ID = %u)", fileName, packageId);
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

   if (rcc == RCC_SUCCESS)
   {
      LockGuard lockGuard(s_jobLock);
      int cancelledCount = 0;
      s_jobs.forEach(
         [packageId, &cancelledCount] (PackageDeploymentJob *job) -> EnumerationCallbackResult
         {
            if ((job->getPackageId() == packageId) &&
                ((job->getStatus() == PKG_JOB_SCHEDULED) || (job->getStatus() == PKG_JOB_PENDING)))
            {
               job->cancel(L"Package removed from server");
               cancelledCount++;
            }
            return _CONTINUE;
         });
      if (cancelledCount > 0)
         nxlog_debug_tag(DEBUG_TAG, 4, L"Cancelled %d pending deployment job(s) for removed package [%u]", cancelledCount, packageId);
   }

   return rcc;
}

/**
 * Create job object from database record.
 * Expected column order: id,pkg_id,node_id,user_id,creation_time,execution_time,completion_time,
 * status,failure_reason,pkg_type,pkg_name,platform,version,pkg_file,command,description,retry_count
 */
PackageDeploymentJob::PackageDeploymentJob(DB_RESULT hResult, int row) : m_packageFile(DBGetFieldAsString(hResult, row, 13)),
         m_command(DBGetFieldAsString(hResult, row, 14)), m_description(DBGetFieldAsString(hResult, row, 15))
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   m_packageId = DBGetFieldUInt32(hResult, row, 1);
   m_nodeId = DBGetFieldUInt32(hResult, row, 2);
   m_userId = DBGetFieldUInt32(hResult, row, 3);
   m_creationTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 4));
   m_executionTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 5));
   m_completionTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 6));
   m_status = static_cast<PackageDeploymentStatus>(DBGetFieldInt32(hResult, row, 7));
   DBGetField(hResult, row, 8, m_errorMessage, 256);
   DBGetField(hResult, row, 9, m_packageType, 16);
   DBGetField(hResult, row, 10, m_packageName, MAX_OBJECT_NAME);
   DBGetField(hResult, row, 11, m_platform, MAX_PLATFORM_NAME_LEN);
   DBGetField(hResult, row, 12, m_version, 32);
   m_retryCount = DBGetFieldInt32(hResult, row, 16);
}

/**
 * Create job database record
 */
bool PackageDeploymentJob::createDatabaseRecord()
{
   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO package_deployment_jobs (id,pkg_id,node_id,user_id,creation_time,execution_time,completion_time,status,retry_count) VALUES (?,?,?,?,?,?,0,?,?)");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_packageId);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_nodeId);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_userId);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_executionTime));
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_status);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_retryCount);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Set job status
 */
void PackageDeploymentJob::setStatus(PackageDeploymentStatus status)
{
   if ((status == PKG_JOB_COMPLETED) || (status == PKG_JOB_CANCELLED) || (status == PKG_JOB_FAILED))
   {
      setCompletedStatus(status);
      return;
   }

   m_status = status;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"UPDATE package_deployment_jobs SET status=? WHERE id=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, status);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   notifyClients();
}

/**
 * Set job as completed/failed
 */
void PackageDeploymentJob::setCompletedStatus(PackageDeploymentStatus status, const wchar_t *errorMessage)
{
   m_status = status;
   m_completionTime = time(nullptr);
   if (errorMessage != nullptr)
      wcslcpy(m_errorMessage, errorMessage, 256);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, L"UPDATE package_deployment_jobs SET status=?,completion_time=?,failure_reason=? WHERE id=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_status);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_completionTime));
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_errorMessage, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   // Create log record
   hStmt = DBPrepare(hdb,
      (g_dbSyntax == DB_SYNTAX_TSDB) ?
         L"INSERT INTO package_deployment_log (job_id,execution_time,completion_time,node_id,user_id,status,failure_reason,pkg_id,pkg_type,pkg_name,version,platform,pkg_file,command,description) VALUES (?,to_timestamp(?),to_timestamp(?),?,?,?,?,?,?,?,?,?,?,?,?)" :
         L"INSERT INTO package_deployment_log (job_id,execution_time,completion_time,node_id,user_id,status,failure_reason,pkg_id,pkg_type,pkg_name,version,platform,pkg_file,command,description) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_executionTime));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_completionTime));
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_nodeId);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_userId);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_status);
      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_errorMessage, DB_BIND_STATIC);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_packageId);
      DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_packageType, DB_BIND_STATIC);
      DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_packageName, DB_BIND_STATIC);
      DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_version, DB_BIND_STATIC);
      DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_platform, DB_BIND_STATIC);
      DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_packageFile, DB_BIND_STATIC, 255);
      DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, m_command, DB_BIND_STATIC, 255);
      DBBind(hStmt, 15, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, 255);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);

   notifyClients();
}

/**
 * Mark job as failed
 */
void PackageDeploymentJob::markAsFailed(const wchar_t *errorMessage, bool reschedule)
{
   if (reschedule)
   {
      int maxRetryCount = ConfigReadInt(L"PackageDeployment.MaxRetryCount", 16);
      if (m_retryCount < maxRetryCount)
      {
         setCompletedStatus(PKG_JOB_FAILED, errorMessage);

         // Reschedule with exponential backoff (5 minutes base, doubling on each retry, capped at 1 hour)
         uint32_t delay = (m_retryCount >= 20) ? PKG_RETRY_MAX_INTERVAL : std::min(static_cast<uint32_t>(PKG_RETRY_BASE_INTERVAL) << m_retryCount, static_cast<uint32_t>(PKG_RETRY_MAX_INTERVAL));
         auto job = make_shared<PackageDeploymentJob>(this, time(nullptr) + delay, m_retryCount + 1);
         if (job->createDatabaseRecord())
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"PackageDeploymentJob::markAsFailed: scheduled retry %d of %d in %u seconds for deployment of package [%u] on node \"%s\" [%u]",
               m_retryCount + 1, maxRetryCount, delay, m_packageId, GetObjectName(m_nodeId, nullptr), m_nodeId);
            RegisterPackageDeploymentJob(job);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"PackageDeploymentJob::markAsFailed: cannot create database record for deployment job [%u] on node \"%s\" [%u]", job->getId(), GetObjectName(m_nodeId, nullptr), m_nodeId);
         }
      }
      else
      {
         // Retry limit reached - fail permanently
         wchar_t reason[256];
         nx_swprintf(reason, 256, L"%s (retry limit of %d reached)", errorMessage, maxRetryCount);
         setCompletedStatus(PKG_JOB_FAILED, reason);
         nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::markAsFailed: retry limit of %d reached for deployment of package [%u] on node \"%s\" [%u], giving up",
            maxRetryCount, m_packageId, GetObjectName(m_nodeId, nullptr), m_nodeId);
      }
   }
   else
   {
      setCompletedStatus(PKG_JOB_FAILED, errorMessage);
   }
}

/**
 * Fill NXCP message
 */
/**
 * Convert package deployment status to text
 */
static const char *PackageJobStatusToText(PackageDeploymentStatus status)
{
   switch(status)
   {
      case PKG_JOB_SCHEDULED:
         return "scheduled";
      case PKG_JOB_PENDING:
         return "pending";
      case PKG_JOB_INITIALIZING:
         return "initializing";
      case PKG_JOB_FILE_TRANSFER_RUNNING:
         return "file-transfer";
      case PKG_JOB_INSTALLATION_RUNNING:
         return "installing";
      case PKG_JOB_WAITING_FOR_AGENT:
         return "waiting-for-agent";
      case PKG_JOB_COMPLETED:
         return "completed";
      case PKG_JOB_FAILED:
         return "failed";
      case PKG_JOB_CANCELLED:
         return "cancelled";
      default:
         return "unknown";
   }
}

/**
 * Serialize deployment job to JSON
 */
json_t *PackageDeploymentJob::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "packageId", json_integer(m_packageId));
   json_object_set_new(root, "packageName", json_string_t(m_packageName));
   json_object_set_new(root, "packageType", json_string_t(m_packageType));
   json_object_set_new(root, "version", json_string_t(m_version));
   json_object_set_new(root, "platform", json_string_t(m_platform));
   json_object_set_new(root, "packageFile", json_string_t(m_packageFile.cstr()));
   json_object_set_new(root, "nodeId", json_integer(m_nodeId));
   shared_ptr<NetObj> node = FindObjectById(m_nodeId);
   if (node != nullptr)
      json_object_set_new(root, "nodeName", json_string_t(node->getName()));
   json_object_set_new(root, "userId", json_integer(m_userId));
   json_object_set_new(root, "status", json_integer(m_status));
   json_object_set_new(root, "statusText", json_string(PackageJobStatusToText(m_status)));
   json_object_set_new(root, "retryCount", json_integer(m_retryCount));
   json_object_set_new(root, "creationTime", json_integer(m_creationTime));
   json_object_set_new(root, "executionTime", json_integer(m_executionTime));
   if (m_completionTime != 0)
      json_object_set_new(root, "completionTime", json_integer(m_completionTime));
   if (m_errorMessage[0] != 0)
      json_object_set_new(root, "errorMessage", json_string_t(m_errorMessage));
   return root;
}

void PackageDeploymentJob::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   uint32_t fieldId = baseId;
   msg->setField(fieldId++, m_id);
   msg->setField(fieldId++, m_nodeId);
   msg->setField(fieldId++, m_userId);
   msg->setField(fieldId++, static_cast<int16_t>(m_status));
   msg->setFieldFromTime(fieldId++, m_creationTime);
   msg->setFieldFromTime(fieldId++, m_executionTime);
   msg->setFieldFromTime(fieldId++, m_completionTime);
   msg->setField(fieldId++, m_packageId);
   msg->setField(fieldId++, m_packageType);
   msg->setField(fieldId++, m_packageName);
   msg->setField(fieldId++, m_platform);
   msg->setField(fieldId++, m_version);
   msg->setField(fieldId++, m_packageFile);
   msg->setField(fieldId++, m_command);
   msg->setField(fieldId++, m_description);
   msg->setField(fieldId++, m_errorMessage);
}

/**
 * Notify clients about job change
 */
void PackageDeploymentJob::notifyClients() const
{
   shared_ptr<NetObj> node = FindObjectById(m_nodeId);
   if (node == nullptr)
      return;

   NXCPMessage msg(CMD_PACKAGE_DEPLOYMENT_JOB_UPDATE, 0);
   fillMessage(&msg, VID_ELEMENT_LIST_BASE);
   NotifyClientSessions(msg,
      [node] (ClientSession *session) -> bool
      {
         return session->isSubscribedTo(NXC_CHANNEL_PACKAGE_JOBS) && node->checkAccessRights(session->getUserId(), OBJECT_ACCESS_READ);
      });
}

/**
 * Upgrade agent
 */
static bool UpgradeAgent(PackageDeploymentJob *job, Node *node, AgentConnectionEx *upgradeConnection, uint32_t transferId, const wchar_t **errorMessage)
{
   uint32_t rcc = upgradeConnection->startUpgrade(job->getPackageFile(), transferId);
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"UpgradeAgent(): cannot start agent upgrade on node %s [%u] in job [%u] (%u: %s)",
         node->getName(), node->getId(), job->getId(), rcc, AgentErrorCodeToText(rcc));
      switch(rcc)
      {
         case ERR_ACCESS_DENIED:
            *errorMessage = L"Access denied by agent";
            break;
         case ERR_BAD_SIGNATURE:
            *errorMessage = L"Installer signature verification failed";
            break;
         case ERR_UNTRUSTED_PACKAGE:
            *errorMessage = L"Installer package rejected as untrusted";
            break;
         case ERR_DOWNGRADE_NOT_ALLOWED:
            *errorMessage = L"Agent downgrade not allowed";
            break;
         case ERR_CONNECTION_BROKEN:
            *errorMessage = L"Agent disconnected";
            break;
         default:
            *errorMessage = L"Unable to start upgrade process";
            break;
      }
      return false;
   }

   bool connected = false;

   upgradeConnection->disconnect();

   // Wait for agent's restart
   // Read configuration
   job->setStatus(PKG_JOB_WAITING_FOR_AGENT);
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

   if (!connected)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"UpgradeAgent(): cannot connect to agent after upgrade on node %s [%u] in job [%u]", node->getName(), node->getId(), job->getId());
      *errorMessage = L"Unable to contact agent after upgrade";
      return false;
   }

   // Check version
   bool success = false;
   wchar_t version[MAX_AGENT_VERSION_LEN];
   if (testConnection->getParameter(_T("Agent.Version"), version, MAX_AGENT_VERSION_LEN) == ERR_SUCCESS)
   {
      if (!wcsicmp(version, job->getVersion()))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"UpgradeAgent(): agent upgrade completed on node %s [%u] in job [%u]", node->getName(), node->getId(), job->getId());
         success = true;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"UpgradeAgent(): agent version mismatch after upgrade on node %s [%u] in job [%u] (expected: %s, reported: %s)",
            node->getName(), node->getId(), job->getId(), job->getVersion(), version);
         *errorMessage = L"Agent's version doesn't match package version after upgrade";
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"UpgradeAgent(): cannot get agent version after upgrade on node %s [%u] in job [%u]", node->getName(), node->getId(), job->getId());
      *errorMessage = L"Unable to get agent's version after upgrade";
   }
   return success;
}

/**
 * Package deployment worker
 */
void PackageDeploymentJob::execute()
{
   if (m_status == PKG_JOB_CANCELLED)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::execute(): job [%u] was cancelled before execution started", m_id);
      return;
   }

   setStatus(PKG_JOB_INITIALIZING);

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   if (node == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::execute(): invalid node ID [%u] in job [%u]", m_nodeId, m_id);
      markAsFailed(L"Target node no longer available", false);
      return;
   }

   // Check if user has rights to deploy packages on that specific node
   if (!node->checkAccessRights(m_userId, OBJECT_ACCESS_MODIFY | OBJECT_ACCESS_CONTROL | OBJECT_ACCESS_UPLOAD))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::execute(): user [%u] has insufficient rights on node %s [%u] in job [%u]", m_userId, node->getName(), m_nodeId, m_id);
      markAsFailed(L"Access denied", false);
      return;
   }

   if (node->isLocalManagement() && !wcscmp(m_packageType, L"agent-installer"))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::execute(): attempt to deploy agent on management node in job [%u]", m_id);
      markAsFailed(L"Management server cannot deploy agent to itself", false);
      return;
   }

   // Create agent connection
   shared_ptr<AgentConnectionEx> agentConn = node->createAgentConnection();
   if (agentConn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::execute(): cannot connect to agent on node %s [%u] in job [%u]", node->getName(), m_nodeId, m_id);
      markAsFailed(L"Unable to connect to agent", true);
      return;
   }

   bool targetCheckOK = false;

   // Check if package can be deployed on target node
   if (!wcsicmp(m_platform, L"src") && !wcscmp(m_packageType, L"agent-installer"))
   {
      // Source package, check if target node
      // supports source packages
      wchar_t value[32];
      if (agentConn->getParameter(L"Agent.SourcePackageSupport", value, 32) == ERR_SUCCESS)
      {
         targetCheckOK = (wcstol(value, nullptr, 0) != 0);
      }
   }
   else
   {
      // Binary package, check target platform.
      // Agent installer must match agent binary architecture; everything else
      // must match the operating system architecture (which can differ from
      // the agent's own architecture, e.g. 32-bit agent on 64-bit Windows).
      const wchar_t *paramName = !wcscmp(m_packageType, L"agent-installer")
         ? L"System.PlatformName"
         : L"System.OSPlatformName";
      TCHAR platform[MAX_RESULT_LENGTH];
      uint32_t rcc = agentConn->getParameter(paramName, platform, MAX_RESULT_LENGTH);
      if ((rcc != ERR_SUCCESS) && wcscmp(paramName, L"System.PlatformName"))
      {
         // Fall back to legacy parameter for older agents that do not expose System.OSPlatformName
         rcc = agentConn->getParameter(L"System.PlatformName", platform, MAX_RESULT_LENGTH);
      }
      if (rcc == ERR_SUCCESS)
      {
         targetCheckOK = MatchString(m_platform, platform, false);
      }
   }

   if (!targetCheckOK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::execute(): package \"%s\" [%u] is not compatible with node %s [%u] in job [%u]",
         m_packageFile.cstr(), m_packageId, node->getName(), m_nodeId, m_id);
      markAsFailed(L"Package is not compatible with target platform", false);
      return;
   }

   setStatus(PKG_JOB_FILE_TRANSFER_RUNNING);

   // Upload package file to agent
   StringBuffer packageFile(g_netxmsdDataDir);
   packageFile.append(DDIR_PACKAGES);
   packageFile.append(FS_PATH_SEPARATOR);
   packageFile.append(m_packageFile);
   uint32_t bwLimit = node->getCustomAttributeAsUInt32(L"SysConfig:Agent.UploadBandwidthLimit", g_agentUploadBandwidthLimit);
   uint32_t bandwidthLimit = (bwLimit > 0) ? bwLimit * 1024 : 0;

   bool isAgentUpgrade = !wcscmp(m_packageType, L"agent-installer");
   uint32_t upgradeTransferId = 0;
   uint32_t rcc;

   if (isAgentUpgrade)
   {
      // Prefer the isolated upload flow so agents configured with UpgradeServers
      // (but not MasterServers) can still receive upgrades. Falls back to legacy
      // CMD_TRANSFER_FILE for older agents that don't implement the new command.
      rcc = agentConn->uploadAgentUpgradePackage(packageFile, &upgradeTransferId, nullptr, bandwidthLimit);
      if ((rcc == ERR_NOT_IMPLEMENTED) || (rcc == ERR_UNKNOWN_COMMAND))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"PackageDeploymentJob::execute(): agent on node %s [%u] does not support isolated upgrade flow, falling back to legacy upload", node->getName(), m_nodeId);
         upgradeTransferId = 0;
         rcc = agentConn->uploadFile(packageFile, nullptr, false, nullptr, NXCP_STREAM_COMPRESSION_NONE, bandwidthLimit);
      }
   }
   else
   {
      rcc = agentConn->uploadFile(packageFile, nullptr, false, nullptr, NXCP_STREAM_COMPRESSION_NONE, bandwidthLimit);
   }

   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"PackageDeploymentJob::execute(): file transfer failed to node %s [%u] in job [%u] (%u: %s)",
         node->getName(), m_nodeId, m_id, rcc, AgentErrorCodeToText(rcc));
      bool transient;
      switch(rcc)
      {
         case ERR_CONNECTION_BROKEN:
         case ERR_NOT_CONNECTED:
         case ERR_REQUEST_TIMEOUT:
         case ERR_IO_FAILURE:
         case ERR_SOCKET_ERROR:
         case ERR_CONNECT_FAILED:
         case ERR_PROXY_CONNECT_FAILED:
         case ERR_RESOURCE_BUSY:
         case ERR_OUT_OF_RESOURCES:
            transient = true;
            break;
         default:
            transient = false;
            break;
      }
      StringBuffer errorMessage(L"File transfer failed (");
      errorMessage.append(AgentErrorCodeToText(rcc));
      errorMessage.append(L')');
      markAsFailed(errorMessage, transient);
      return;
   }

   setStatus(PKG_JOB_INSTALLATION_RUNNING);

   bool success;
   if (isAgentUpgrade)
   {
      const wchar_t *errorMessage;
      success = UpgradeAgent(this, node.get(), agentConn.get(), upgradeTransferId, &errorMessage);
      if (!success)
         markAsFailed(errorMessage, false);
   }
   else
   {
      uint32_t maxWaitTime = ConfigReadULong(L"Agent.Upgrade.WaitTime", 600);
      if (maxWaitTime < 30)
         maxWaitTime = 30;
      agentConn->setCommandTimeout(maxWaitTime * 1000);
      uint32_t rcc = agentConn->installPackage(m_packageFile, m_packageType, (m_command.charAt(0) == '@') ? node->expandText(m_command.cstr() + 1) : m_command);
      if (rcc == ERR_SUCCESS)
      {
         success = true;
      }
      else
      {
         const wchar_t *errorMessage = AgentErrorCodeToText(rcc);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("PackageDeploymentJob::execute(): installation of package \"%s\" [%u] failed on node \"%s\" [%u] (%s)"),
            m_packageFile.cstr(), m_packageId, node->getName(), node->getId(), errorMessage);
         markAsFailed(errorMessage, false);
      }
   }

   // Finish node processing
   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Package \"%s\" [%u] successfully deployed to node \"%s\" [%u]", m_packageFile.cstr(), m_packageId, node->getName(), node->getId());
      node->forceConfigurationPoll();
      setStatus(PKG_JOB_COMPLETED);
   }
}

/**
 * Register new deployment job
 */
void NXCORE_EXPORTABLE RegisterPackageDeploymentJob(const shared_ptr<PackageDeploymentJob>& job)
{
   s_jobLock.lock();
   s_jobs.put(job->getId(), job);
   s_jobQueue.push(job);
   s_wakeupCondition.pulse();
   s_jobLock.unlock();
   job->notifyClients();
}

/**
 * Deployment thread pool
 */
static ThreadPool *s_deploymentThreadPool = nullptr;

/**
 * Package deployment manager
 */
static void PackageDeploymentManager()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"Package deployment manager started");

   int maxThreads = ConfigReadInt(L"PackageDeployment.MaxThreads", 25);
   s_deploymentThreadPool = ThreadPoolCreate(L"PACKAGE-MANAGER", 2, std::max(2, maxThreads));

   uint32_t sleepTime = 10;
   while(!(g_flags & AF_SHUTDOWN))
   {
      s_wakeupCondition.wait(sleepTime);
      if (g_flags & AF_SHUTDOWN)
         break;

      sleepTime = 600;
      time_t now = time(nullptr);
      s_jobLock.lock();
      while(s_jobQueue.size() > 0)
      {
         shared_ptr<PackageDeploymentJob> job = s_jobQueue.top();
         if (job->getExecutionTime() > now)
         {
            uint32_t delay = static_cast<uint32_t>(job->getExecutionTime() - now);
            if (delay < sleepTime)
               sleepTime = delay;
            break;
         }
         s_jobQueue.pop();
         if (job->getStatus() == PKG_JOB_CANCELLED)
            continue;
         job->setStatus(PKG_JOB_PENDING);
         ThreadPoolExecute(s_deploymentThreadPool, job, &PackageDeploymentJob::execute);
      }
      s_jobLock.unlock();
   }

   ThreadPoolDestroy(s_deploymentThreadPool);
   nxlog_debug_tag(DEBUG_TAG, 1, L"Package deployment manager stopped");
}

/**
 * Manager thread
 */
static THREAD s_managerThread = INVALID_THREAD_HANDLE;

/**
 * Start package deployment manager
 */
void StartPackageDeploymentManager()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load only active jobs (scheduled or pending). Terminal jobs (completed/failed/cancelled) are not re-queued and are
   // served to clients directly from the database on demand, so there is no need to keep them resident for the server's lifetime.
   wchar_t query[1024];
   nx_swprintf(query, 1024,
      L"SELECT id,j.pkg_id,node_id,user_id,creation_time,execution_time,completion_time,status,failure_reason,pkg_type,pkg_name,platform,version,pkg_file,command,description,retry_count FROM package_deployment_jobs j "
      L"LEFT OUTER JOIN agent_pkg p ON p.pkg_id=j.pkg_id WHERE j.status IN (%d,%d)", PKG_JOB_SCHEDULED, PKG_JOB_PENDING);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         auto job = make_shared<PackageDeploymentJob>(hResult, i);
         s_jobs.put(job->getId(), job);
         if (job->getStatus() == PKG_JOB_PENDING)
            job->setStatus(PKG_JOB_SCHEDULED);
         s_jobQueue.push(job);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);

   s_managerThread = ThreadCreateEx(PackageDeploymentManager);
}

/**
 * Stop package deployment manager
 */
void StopPackageDeploymentManager()
{
   s_wakeupCondition.set();
   nxlog_debug_tag(DEBUG_TAG, 2, L"Waiting for package deployment manager to stop");
   ThreadJoin(s_managerThread);
}

/**
 * Check if user has read access to given deployment job
 */
static bool CheckJobReadAccess(const PackageDeploymentJob& job, uint32_t userId)
{
   if (userId == 0)
      return true;
   shared_ptr<NetObj> node = FindObjectById(job.getNodeId());
   return (node != nullptr) && node->checkAccessRights(userId, OBJECT_ACCESS_READ);
}

/**
 * Build query for retrieving most recent terminal (completed/failed/cancelled) jobs from database, limited to given number of rows
 */
static StringBuffer BuildJobHistoryQuery(int limit)
{
   StringBuffer query;
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query.appendFormattedString(L"SELECT TOP %d ", limit);
         break;
      case DB_SYNTAX_INFORMIX:
         query.appendFormattedString(L"SELECT FIRST %d ", limit);
         break;
      case DB_SYNTAX_ORACLE:
         query.append(L"SELECT * FROM (SELECT ");
         break;
      default:
         query.append(L"SELECT ");
         break;
   }

   query.appendFormattedString(
      L"id,j.pkg_id,node_id,user_id,creation_time,execution_time,completion_time,status,failure_reason,pkg_type,pkg_name,platform,version,pkg_file,command,description,retry_count "
      L"FROM package_deployment_jobs j LEFT OUTER JOIN agent_pkg p ON p.pkg_id=j.pkg_id WHERE j.status IN (%d,%d,%d) ORDER BY j.completion_time DESC",
      PKG_JOB_COMPLETED, PKG_JOB_FAILED, PKG_JOB_CANCELLED);

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         query.appendFormattedString(L" LIMIT %d", limit);
         break;
      case DB_SYNTAX_ORACLE:
         query.appendFormattedString(L") WHERE ROWNUM<=%d", limit);
         break;
      case DB_SYNTAX_DB2:
         query.appendFormattedString(L" FETCH FIRST %d ROWS ONLY", limit);
         break;
   }
   return query;
}

/**
 * Enumerate package deployment jobs readable by given user, invoking callback for each. Active jobs (and jobs completed during
 * current server session) are served from the in-memory index; older terminal jobs are read from the database on demand,
 * limited by PackageDeployment.JobHistorySize.
 */
static void EnumeratePackageDeploymentJobs(uint32_t userId, const std::function<void (const PackageDeploymentJob&)>& callback)
{
   HashSet<uint32_t> residentJobs;

   // Jobs currently held in memory (active jobs and jobs completed during current server session)
   {
      LockGuard lockGuard(s_jobLock);
      s_jobs.forEach(
         [&residentJobs, &callback, userId] (PackageDeploymentJob *job) -> EnumerationCallbackResult
         {
            residentJobs.put(job->getId());
            if (CheckJobReadAccess(*job, userId))
               callback(*job);
            return _CONTINUE;
         });
   }

   // Historical terminal jobs from database (most recent first), excluding those already served from memory
   int historySize = ConfigReadInt(L"PackageDeployment.JobHistorySize", 1000);
   if (historySize > 0)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      StringBuffer query = BuildJobHistoryQuery(historySize);
      DB_RESULT hResult = DBSelect(hdb, query.cstr());
      if (hResult != nullptr)
      {
         int rows = DBGetNumRows(hResult);
         for(int i = 0; i < rows; i++)
         {
            if (residentJobs.contains(DBGetFieldUInt32(hResult, i, 0)))
               continue;
            PackageDeploymentJob job(hResult, i);
            if (CheckJobReadAccess(job, userId))
               callback(job);
         }
         DBFreeResult(hResult);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
}

/**
 * Get package deployment jobs into NXCP message
 */
void GetPackageDeploymentJobs(NXCPMessage *msg, uint32_t userId)
{
   uint32_t count = 0;
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   EnumeratePackageDeploymentJobs(userId,
      [msg, &count, &fieldId] (const PackageDeploymentJob& job)
      {
         job.fillMessage(msg, fieldId);
         count++;
         fieldId += 50;
      });
   msg->setField(VID_NUM_ELEMENTS, count);
}

/**
 * Get package deployment jobs as JSON array
 */
json_t NXCORE_EXPORTABLE *GetPackageDeploymentJobsAsJson(uint32_t userId)
{
   json_t *array = json_array();
   EnumeratePackageDeploymentJobs(userId,
      [array] (const PackageDeploymentJob& job)
      {
         json_array_append_new(array, job.toJson());
      });
   return array;
}

/**
 * Cancel package deployment job. Returns client RCC.
 */
uint32_t NXCORE_EXPORTABLE CancelPackageDeploymentJob(uint32_t jobId, uint32_t userId, uint32_t *nodeId)
{
   LockGuard lockGuard(s_jobLock);

   shared_ptr<PackageDeploymentJob> job = s_jobs.get(jobId);
   if (job == nullptr)
      return RCC_INVALID_JOB_ID;

   if ((userId != 0) && (job->getUserId() != userId))
   {
      shared_ptr<NetObj> node = FindObjectById(job->getNodeId());
      if (!node->checkAccessRights(userId, OBJECT_ACCESS_CONTROL))
         return RCC_ACCESS_DENIED;
   }

   if (job->getStatus() != PKG_JOB_SCHEDULED)
      return RCC_OUT_OF_STATE_REQUEST;

   *nodeId = job->getNodeId();
   job->cancel();
   return RCC_SUCCESS;
}

/**
 * Remove expired jobs
 */
void RemoveExpiredPackageDeploymentJobs(DB_HANDLE hdb)
{
   time_t retentionTime = ConfigReadInt(L"PackageDeployment.JobRetentionTime", 7) * 86400;
   int64_t cutoff = static_cast<int64_t>(time(nullptr) - retentionTime);

   // Delete expired terminal jobs from database (covers historical jobs that are not kept in memory)
   wchar_t query[256];
   nx_swprintf(query, 256, L"DELETE FROM package_deployment_jobs WHERE status IN (%d,%d,%d) AND completion_time<" INT64_FMT,
      PKG_JOB_COMPLETED, PKG_JOB_FAILED, PKG_JOB_CANCELLED, cutoff);
   if (DBQuery(hdb, query))
      nxlog_debug_tag(DEBUG_TAG, 4, L"Expired package deployment jobs removed from database");

   // Drop expired terminal jobs from the in-memory index
   LockGuard lockGuard(s_jobLock);
   std::vector<uint32_t> deleteList;
   s_jobs.forEach(
      [&deleteList, cutoff] (PackageDeploymentJob *job) -> EnumerationCallbackResult
      {
         if (((job->getStatus() == PKG_JOB_COMPLETED) || (job->getStatus() == PKG_JOB_FAILED) || (job->getStatus() == PKG_JOB_CANCELLED)) && (static_cast<int64_t>(job->getCompletionTime()) < cutoff))
            deleteList.push_back(job->getId());
         return _CONTINUE;
      });
   for(uint32_t id : deleteList)
      s_jobs.remove(id);
}

/**
 * Task handler for scheduled action execution
 */
void ExecuteScheduledPackageDeployment(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   shared_ptr<NetObj> object = FindObjectById(parameters->m_objectId);
   if (object == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteScheduledPackageDeployment: invalid object ID [%u]"), parameters->m_objectId);
      return;
   }

   if (!object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_READ))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteScheduledPackageDeployment: user [%u] has no rights to object %s [%u]"), parameters->m_userId, object->getName(), object->getId());
      return;
   }

   SharedObjectArray<Node> nodeList;
   if (object->getObjectClass() == OBJECT_NODE)
   {
      if (!object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_MODIFY | OBJECT_ACCESS_CONTROL | OBJECT_ACCESS_UPLOAD))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteScheduledPackageDeployment: user [%u] has no rights to object %s [%u]"), parameters->m_userId, object->getName(), object->getId());
         return;
      }
      nodeList.add(static_pointer_cast<Node>(object));
   }
   else
   {
      object->addChildNodesToList(&nodeList, parameters->m_userId);
   }

   uint32_t packageId = ExtractNamedOptionValueAsUInt(parameters->m_persistentData, _T("package"), 0);

   if (nodeList.isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteScheduledPackageDeployment: no accessible nodes under object %s [%u]"), parameters->m_userId, object->getName(), object->getId());
      return;
   }

   PackageDetails package;
   uint32_t rcc = GetPackageDetails(packageId, &package);
   if (rcc != RCC_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteScheduledPackageDeployment: cannot read package details (RCC = %u)"), rcc);
      return;
   }

   time_t now = time(nullptr);
   for(int i = 0; i < nodeList.size(); i++)
   {
      Node *node = nodeList.get(i);
      auto job = make_shared<PackageDeploymentJob>(node->getId(), parameters->m_userId, now, package);
      if (job->createDatabaseRecord())
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteScheduledPackageDeployment: scheduled deployment of package [%u] on node \"%s\" [%u]"), packageId, node->getName(), node->getId());
         RegisterPackageDeploymentJob(job);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteScheduledPackageDeployment: cannot create database record for deployment job [%u] on node \"%s\" [%u]"), job->getId(), node->getName(), node->getId());
      }
   }
}
