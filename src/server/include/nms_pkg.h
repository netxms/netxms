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
** File: nms_pkg.h
**/

#ifndef _nms_pkg_h_
#define _nms_pkg_h_

/**
 * Package deployment status
 */
enum PackageDeploymentStatus
{
   PKG_JOB_SCHEDULED = 0,
   PKG_JOB_PENDING = 1,
   PKG_JOB_INITIALIZING = 2,
   PKG_JOB_FILE_TRANSFER_RUNNING = 3,
   PKG_JOB_INSTALLATION_RUNNING = 4,
   PKG_JOB_WAITING_FOR_AGENT = 5,
   PKG_JOB_COMPLETED = 6,
   PKG_JOB_FAILED = 7,
   PKG_JOB_CANCELLED = 8
};

/**
 * Package details
 */
struct PackageDetails
{
   uint32_t id;
   wchar_t type[16];
   wchar_t name[MAX_OBJECT_NAME];
   wchar_t version[32];
   wchar_t platform[MAX_PLATFORM_NAME_LEN];
   MutableString packageFile;
   MutableString command;
   MutableString description;
};

/**
 * Package deployment job
 */
class NXCORE_EXPORTABLE PackageDeploymentJob
{
private:
   uint32_t m_id;
   uint32_t m_nodeId;
   uint32_t m_userId;
   PackageDeploymentStatus m_status;
   time_t m_creationTime;
   time_t m_executionTime;
   time_t m_completionTime;
   uint32_t m_packageId;
   wchar_t m_packageType[16];
   wchar_t m_packageName[MAX_OBJECT_NAME];
   wchar_t m_platform[MAX_PLATFORM_NAME_LEN];
   wchar_t m_version[32];
   String m_packageFile;
   String m_command;
   String m_description;
   wchar_t m_errorMessage[256];

   void setCompletedStatus(PackageDeploymentStatus status, const wchar_t *errorMessage = nullptr);
   void markAsFailed(const wchar_t *errorMessage, bool reschedule);
   void notifyClients() const;

public:
   PackageDeploymentJob(uint32_t nodeId, uint32_t userId, time_t executionTime, const PackageDetails& package) : m_packageFile(package.packageFile), m_command(package.command), m_description(package.description)
   {
      m_id = CreateUniqueId(IDG_PACKAGE_DEPLOYMENT_JOB);
      m_nodeId = nodeId;
      m_userId = userId;
      m_status = PKG_JOB_SCHEDULED;
      m_creationTime = time(nullptr);
      m_executionTime = executionTime;
      m_completionTime = 0;
      m_packageId = package.id;
      wcslcpy(m_packageType, package.type, 16);
      wcslcpy(m_packageName, package.name, MAX_OBJECT_NAME);
      wcslcpy(m_platform, package.platform, MAX_PLATFORM_NAME_LEN);
      wcslcpy(m_version, package.version, 32);
      m_errorMessage[0] = 0;
   }
   PackageDeploymentJob(const PackageDeploymentJob *src, time_t executionTime) : m_packageFile(src->m_packageFile), m_command(src->m_command), m_description(src->m_description)
   {
      m_id = CreateUniqueId(IDG_PACKAGE_DEPLOYMENT_JOB);
      m_nodeId = src->m_nodeId;
      m_userId = src->m_userId;
      m_status = PKG_JOB_SCHEDULED;
      m_creationTime = time(nullptr);
      m_executionTime = executionTime;
      m_completionTime = 0;
      m_packageId = src->m_packageId;
      wcslcpy(m_packageType, src->m_packageType, 16);
      wcslcpy(m_packageName, src->m_packageName, MAX_OBJECT_NAME);
      wcslcpy(m_platform, src->m_platform, MAX_PLATFORM_NAME_LEN);
      wcslcpy(m_version, src->m_version, 32);
      _sntprintf(m_errorMessage, 256, L"Retry after failure (%s)", src->m_errorMessage);
   }
   PackageDeploymentJob(DB_RESULT hResult, int row);

   uint32_t getId() const { return m_id; }
   uint32_t getNodeId() const { return m_nodeId; }
   uint32_t getUserId() const { return m_userId; }
   time_t getExecutionTime() const { return m_executionTime; }
   time_t getCompletionTime() const { return m_completionTime; }
   PackageDeploymentStatus getStatus() const { return m_status; }
   uint32_t getPackageId() const { return m_packageId; }
   const wchar_t *getPackageFile() const { return m_packageFile.cstr(); }
   const wchar_t *getVersion() const { return m_version; }

   void setStatus(PackageDeploymentStatus status);
   void execute();

   bool createDatabaseRecord();

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
};

/**
 * Job management functions
 */
void NXCORE_EXPORTABLE RegisterPackageDeploymentJob(const shared_ptr<PackageDeploymentJob>& job);
void GetPackageDeploymentJobs(NXCPMessage *msg, uint32_t userId);
uint32_t CancelPackageDeploymentJob(uint32_t jobId, uint32_t userId, uint32_t *nodeId);

/**
 * Package functions
 */
bool IsPackageInstalled(const wchar_t *name, const wchar_t *version, const wchar_t *platform);
bool IsDuplicatePackageFileName(const wchar_t *fileName);
bool IsValidPackageId(uint32_t packageId);
uint32_t GetPackageDetails(uint32_t packageId, PackageDetails *details);
uint32_t UninstallPackage(uint32_t packageId);

#endif   /* _nms_pkg_h_ */
