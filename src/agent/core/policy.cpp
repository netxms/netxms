/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: policy.cpp
**
**/

#include "nxagentd.h"
#include <nxstat.h>

#define DEBUG_TAG _T("policy")

void UpdateUserAgentsConfiguration();
void SyncPoliciesWithExtSubagents();
void NotifyExtSubagentsOnPolicyInstall(uuid *guid);

/**
 * Get policy deployment directory from policy type
 */
static const TCHAR *GetPolicyDeploymentDirectory(const TCHAR *type)
{
   if (!_tcscmp(type, _T("AgentConfig")))
      return g_szConfigPolicyDir;
   if (!_tcscmp(type, _T("LogParserConfig")))
      return g_szLogParserDirectory;
   if (!_tcscmp(type, _T("SupportApplicationConfig")))
      return g_userAgentPolicyDirectory;
   return g_szDataDirectory;
}

/**
 * Update agent environment from config
 */
static void UpdateEnvironment()
{
   shared_ptr<Config> oldConfig = g_config;
   StringList currEnvList;
   unique_ptr<ObjectArray<ConfigEntry>> entrySet = oldConfig->getSubEntries(_T("/ENV"), _T("*"));
   if (entrySet != nullptr)
   {
      for(int i = 0; i < entrySet->size(); i++)
         currEnvList.add(entrySet->get(i)->getName());
   }

   if (LoadConfig(oldConfig->getAlias(_T("agent")), StringBuffer(), false, false))
   {
      StringList newEnvList;
      shared_ptr<Config> newConfig = g_config;
      unique_ptr<ObjectArray<ConfigEntry>> newEntrySet = newConfig->getSubEntries(_T("/ENV"), _T("*"));
      if (newEntrySet != nullptr)
      {
         for(int i = 0; i < newEntrySet->size(); i++)
         {
            ConfigEntry *e = newEntrySet->get(i);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("UpdateEnvironment(): set environment variable %s=%s"), e->getName(), e->getValue());
            SetEnvironmentVariable(e->getName(), e->getValue());
            newEnvList.add(e->getName());
         }
      }

      for(int i = 0; i < currEnvList.size(); i++)
      {
         const TCHAR *var = currEnvList.get(i);
         int j = 0;
         for(;j < newEnvList.size(); j++)
         {
            if (!_tcscmp(var, newEnvList.get(j)))
               break;
         }
         if (j == newEnvList.size())
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("UpdateEnvironment(): unset environment variable %s"), var);
            SetEnvironmentVariable(var, nullptr);
         }
      }
   }
}

/**
 * Check is if new config contains ENV section
 */
static void CheckEnvSectionAndReload(const char *content, size_t contentLength)
{
   Config config;
   config.setTopLevelTag(_T("config"));
   bool validConfig = config.loadConfigFromMemory(content, contentLength, DEFAULT_CONFIG_SECTION, nullptr, true, false);
   if (validConfig)
   {
      unique_ptr<ObjectArray<ConfigEntry>> entrySet = config.getSubEntries(_T("/ENV"), _T("*"));
      if (entrySet != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("CheckEnvSectionAndReload(): ENV section exists"));
         UpdateEnvironment();
      }
   }
}

/**
 * Register policy in persistent storage
 */
static void RegisterPolicy(const TCHAR *type, const uuid& guid, uint32_t version, uint64_t serverId, const TCHAR *serverInfo, const BYTE *hash)
{
   bool isNew = true;
   DB_HANDLE hdb = GetLocalDatabaseHandle();
	if (hdb != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT type FROM agent_policy WHERE guid=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            isNew = DBGetNumRows(hResult) <= 0;
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }

      if (isNew)
      {
         hStmt = DBPrepare(hdb,
                       _T("INSERT INTO agent_policy (type,server_info,server_id,version,content_hash,guid)")
                       _T(" VALUES (?,?,?,?,?,?)"));
      }
      else
      {
         hStmt = DBPrepare(hdb,
                       _T("UPDATE agent_policy SET type=?,server_info=?,server_id=?,version=?,content_hash=?")
                       _T(" WHERE guid=?"));
      }

      if (hStmt == nullptr)
         return;

      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, type, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, serverInfo, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, serverId);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, version);
      TCHAR hashAsText[33];
      BinToStr(hash, MD5_DIGEST_SIZE, hashAsText);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, hashAsText, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, guid);

      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
}

/**
 * Register policy in persistent storage
 */
static void UnregisterPolicy(const uuid& guid)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM agent_policy WHERE guid=?"));
      if (hStmt == nullptr)
         return;

      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
}

/**
 * Get policy type by GUID
 */
String GetPolicyType(const uuid& guid)
{
   StringBuffer type;
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if(hdb != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT type FROM agent_policy WHERE guid=?"));
	   if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               type.appendPreallocated(DBGetField(hResult, 0, 0, nullptr, 0));
            }
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
	   }
   }
	return type;
}

/**
 * Deploy policy file
 */
static uint32_t DeployPolicy(const uuid& guid, const BYTE *content, size_t size, const TCHAR *policyPath, bool *sameFileContent)
{
   *sameFileContent = false;

   TCHAR policyFileName[MAX_PATH], name[64];
   _sntprintf(policyFileName, MAX_PATH, _T("%s%s.xml"), policyPath, guid.toString(name));

   uint32_t rcc;
   if (content != nullptr)
   {
      BYTE hashA[MD5_DIGEST_SIZE];
      if (CalculateFileMD5Hash(policyFileName, hashA))
      {
         BYTE hashB[MD5_DIGEST_SIZE];
         CalculateMD5Hash(content, size, hashB);
         *sameFileContent = (memcmp(hashA, hashB, MD5_DIGEST_SIZE) == 0);
      }

      if (*sameFileContent)
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Policy file %s was not changed"), policyFileName);
         rcc = ERR_SUCCESS;
      }
      else
      {
         SaveFileStatus status = SaveFile(policyFileName, content, size);
         if (status == SaveFileStatus::SUCCESS)
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Policy file %s saved successfully"), policyFileName);
            rcc = ERR_SUCCESS;
         }
         else if ((status == SaveFileStatus::OPEN_ERROR) || (status == SaveFileStatus::RENAME_ERROR))
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("DeployPolicy(): Error opening file %s for writing (%s)"), policyFileName, _tcserror(errno));
            rcc = ERR_FILE_OPEN_ERROR;
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("DeployPolicy(): Error writing policy file (%s)"), _tcserror(errno));
            rcc = ERR_IO_FAILURE;
         }
      }
   }
   else
   {
      rcc = ERR_MALFORMED_COMMAND;
   }
   return rcc;
}

/**
 * Deploy policy on agent
 */
uint32_t DeployPolicy(NXCPMessage *request, uint64_t serverId, const TCHAR *serverInfo)
{
   if (g_dwFlags & AF_DISABLE_LOCAL_DATABASE)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Policy deployment: local database is disabled, reject policy deployment request"));
      return ERR_AGENT_DB_FAILURE;
   }

   if (!request->isFieldExist(VID_POLICY_TYPE))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Policy deployment: missing VID_POLICY_TYPE in request message"));
      return ERR_MALFORMED_COMMAND;
   }

   TCHAR type[32] = _T("");
   request->getFieldAsString(VID_POLICY_TYPE, type, 32);
   if (type[0] == 0)
   {
      uint32_t tmp = request->getFieldAsUInt16(VID_POLICY_TYPE);
      if (tmp == AGENT_POLICY_LOG_PARSER)
         _tcscpy(type, _T("LogParserConfig"));
      else if (tmp == AGENT_POLICY_CONFIG)
         _tcscpy(type, _T("AgentConfig"));
   }

	uuid guid = request->getFieldAsGUID(VID_GUID);

   size_t size = 0;
   const BYTE *content = request->getBinaryFieldPtr(VID_CONFIG_FILE_DATA, &size);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("DeployPolicy(): content size %d"), size);

   uint32_t rcc;
   bool sameFileContent = false;
   if (!_tcscmp(type, _T("AgentConfig")))
   {
      rcc = DeployPolicy(guid, content, size, g_szConfigPolicyDir, &sameFileContent);
      if ((size != 0) && (rcc == ERR_SUCCESS))
      {
         CheckEnvSectionAndReload(reinterpret_cast<const char*>(content), size);
         if (!sameFileContent)
            g_restartPending = true;
      }
   }
   else if (!_tcscmp(type, _T("LogParserConfig")))
   {
      rcc = DeployPolicy(guid, content, size, g_szLogParserDirectory, &sameFileContent);
   }
   else if (!_tcscmp(type, _T("SupportApplicationConfig")))
   {
      rcc = DeployPolicy(guid, content, size, g_userAgentPolicyDirectory, &sameFileContent);
      if (rcc == ERR_SUCCESS)
      {
         UpdateUserAgentsConfiguration();
      }
   }
   else
   {
      rcc = ERR_BAD_ARGUMENTS;
   }

	if (rcc == ERR_SUCCESS)
	{
	   uint32_t version = request->getFieldAsUInt32(VID_VERSION);
      BYTE newHash[MD5_DIGEST_SIZE];
      if (content != nullptr)
      {
         CalculateMD5Hash(reinterpret_cast<const BYTE*>(content), size, newHash);
      }
      else
      {
         memset(newHash, 0, MD5_DIGEST_SIZE);
      }
		RegisterPolicy(type, guid, version, serverId, serverInfo, newHash);

      PolicyChangeNotification n;
      n.guid = guid;
      n.type = type;
      n.sameContent = sameFileContent;
		NotifySubAgents(AGENT_NOTIFY_POLICY_INSTALLED, &n);

      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Policy %s of type %s successfully deployed"), guid.toString().cstr(), type);
      ThreadPoolExecuteSerialized(g_commThreadPool, _T("SyncPolicies"), SyncPoliciesWithExtSubagents);
      ThreadPoolExecuteSerialized(g_commThreadPool, _T("SyncPolicies"), NotifyExtSubagentsOnPolicyInstall, new uuid(guid));
	}
	else
	{
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Deployment of policy %s of type %s failed (error %u)"), guid.toString().cstr(), type, rcc);
	}
   return rcc;
}

/**
 * Remove configuration file
 */
static uint32_t RemovePolicy(const uuid& guid, const TCHAR *dir)
{
   TCHAR path[MAX_PATH], name[64];
   _sntprintf(path, MAX_PATH, _T("%s%s.xml"), dir, guid.toString(name));

   uint32_t rcc;
   if (_tremove(path) == 0)
   {
      rcc = ERR_SUCCESS;
   }
   else
   {
      rcc = (errno == ENOENT) ? ERR_SUCCESS : ERR_IO_FAILURE;
   }
   return rcc;
}

/**
 * Uninstall policy from agent
 */
uint32_t UninstallPolicy(NXCPMessage *request)
{
   uint32_t rcc;

	uuid guid = request->getFieldAsGUID(VID_GUID);
	const String type = GetPolicyType(guid);
	if(!_tcscmp(type, _T("")))
      return ERR_SUCCESS;

   if (!_tcscmp(type, _T("AgentConfig")))
   {
      rcc = RemovePolicy(guid, g_szConfigPolicyDir);
      if (rcc == ERR_SUCCESS)
         g_restartPending = true;
   }
   else if (!_tcscmp(type, _T("LogParserConfig")))
   {
      rcc = RemovePolicy(guid, g_szLogParserDirectory);
   }
   else if (!_tcscmp(type, _T("SupportApplicationConfig")))
   {
      rcc = RemovePolicy(guid, g_userAgentPolicyDirectory);
      if (rcc == ERR_SUCCESS)
      {
         UpdateUserAgentsConfiguration();
      }
   }
   else
   {
      rcc = ERR_BAD_ARGUMENTS;
   }

   TCHAR buffer[64];
	if (rcc == ERR_SUCCESS)
	{
		UnregisterPolicy(guid);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Policy %s of type %s uninstalled successfully"), guid.toString(buffer), type.cstr());
      ThreadPoolExecuteSerialized(g_commThreadPool, _T("SyncPolicies"), SyncPoliciesWithExtSubagents);
	}
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Policy %s of type %s uninstall failed (error %u)"), guid.toString(buffer), type.cstr(), rcc);
   }
	return rcc;
}

/**
 * Get policy inventory
 */
uint32_t GetPolicyInventory(NXCPMessage *msg, uint64_t serverId)
{
   uint32_t success = ERR_AGENT_DB_FAILURE;
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if (hdb != nullptr)
   {
      TCHAR query[256];
      _sntprintf(query, 256,
            _T("SELECT guid,type,server_info,server_id,version,content_hash FROM agent_policy WHERE server_id=") UINT64_FMT, serverId);
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0 )
         {
            msg->setField(VID_NUM_ELEMENTS, (UINT32)count);

            uint32_t fieldId = VID_ELEMENT_LIST_BASE;
            for (int row = 0; row < count; row++, fieldId += 4)
            {
               msg->setField(fieldId++, DBGetFieldGUID(hResult, row, 0));
               TCHAR type[32];
               msg->setField(fieldId++, DBGetField(hResult, row, 1, type, 32));
               TCHAR *text = DBGetField(hResult, row, 2, nullptr, 0);
               msg->setField(fieldId++, CHECK_NULL_EX(text));
               MemFree(text);
               msg->setField(fieldId++, DBGetFieldInt64(hResult, row, 3));
               msg->setField(fieldId++, DBGetFieldULong(hResult, row, 4));
               TCHAR hashAsText[33];
               if (DBGetField(hResult, row, 5, hashAsText, 33) != nullptr)
               {
                  BYTE hash[MD5_DIGEST_SIZE];
                  StrToBin(hashAsText, hash, MD5_DIGEST_SIZE);
                  msg->setField(fieldId++, hash, MD5_DIGEST_SIZE);
               }
            }
         }
         else
         {
            msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(0));
         }
         DBFreeResult(hResult);
         msg->setField(VID_NEW_POLICY_TYPE, true);
         success = ERR_SUCCESS;
      }
   }
	return success;
}

/**
 * Update policy inventory in database from actual policy files
 */
static void UpdatePolicyInventory()
{
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if (hdb == nullptr)
	   return;

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,type,content_hash FROM agent_policy"));
   if (hResult == nullptr)
      return;

   for(int row = 0; row < DBGetNumRows(hResult); row++)
   {
      uuid guid = DBGetFieldGUID(hResult, row, 0);
      TCHAR type[32];
      DBGetField(hResult, row, 1, type, 32);

      TCHAR filePath[MAX_PATH], name[64];
      _sntprintf(filePath, MAX_PATH, _T("%s%s.xml"), GetPolicyDeploymentDirectory(type), guid.toString(name));
      if (_taccess(filePath, R_OK) != 0)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Unregistering policy %s (policy file is missing)"), guid.toString().cstr());
         UnregisterPolicy(guid);
      }
      else
      {
         TCHAR hashAsText[33];
         if (DBGetField(hResult, row, 2, hashAsText, 33) != nullptr)
         {
            BYTE hashB[MD5_DIGEST_SIZE];
            StrToBin(hashAsText, hashB, MD5_DIGEST_SIZE);

            BYTE hashA[MD5_DIGEST_SIZE];
            if (CalculateFileMD5Hash(filePath, hashA))
            {
               if (memcmp(hashA, hashB, MD5_DIGEST_SIZE) != 0)
               {
                  nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Delete and unregistering policy %s (policy hash mismatch)"), guid.toString().cstr());
                  _tremove(filePath);
                  UnregisterPolicy(guid);
               }
            }
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Unregistering policy %s (policy hash missing)"), guid.toString().cstr());
            UnregisterPolicy(guid);
         }
      }
   }
   DBFreeResult(hResult);
}

/**
 * Update policy inventory in database from actual policy files
 */
void StartPolicyHousekeeper()
{
   UpdatePolicyInventory();
   ThreadPoolScheduleRelative(g_webSvcThreadPool, 24 * 60 * 60 * 1000, StartPolicyHousekeeper); // Schedule next update in 24 hours
}

/**
 * Synchronize agent policies with master agent
 */
void SyncAgentPolicies(const NXCPMessage& msg)
{
   TCHAR masterDataDirictory[MAX_PATH];
   msg.getFieldAsString(VID_DATA_DIRECTORY, masterDataDirictory, MAX_PATH);
#ifdef _WIN32
   bool sameDataDirectory = (_tcsicmp(g_szDataDirectory, masterDataDirictory) == 0);
#else
   bool sameDataDirectory = (_tcscmp(g_szDataDirectory, masterDataDirictory) == 0);
#endif
   if (sameDataDirectory)
      nxlog_debug_tag(DEBUG_TAG, 2, _T("This external subagent loader uses same data directory as master agent, policy file deployment is not needed"));

   StringBuffer query(_T("SELECT guid,type FROM agent_policy"));
   uint32_t count = msg.getFieldAsUInt32(VID_NUM_ELEMENTS);
   if (count > 0)
   {
      query.append(_T(" WHERE guid NOT IN ("));
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for(uint32_t i = 0; i < count; i++)
      {
         TCHAR type[32];
         msg.getFieldAsString(fieldId++, type, 32);

         uuid guid = msg.getFieldAsGUID(fieldId++);
         query.append(_T("'"));
         query.append(guid);
         query.append(_T("',"));

         size_t size = 0;
         const BYTE *content = msg.getBinaryFieldPtr(fieldId++, &size);

         nxlog_debug_tag(DEBUG_TAG, 3, _T("Deploying policy %s of type %s from master agent"), guid.toString().cstr(), type);

         bool sameFileContent;
         uint32_t rcc = sameDataDirectory ? ERR_SUCCESS : DeployPolicy(guid, content, size, GetPolicyDeploymentDirectory(type), &sameFileContent);
         if (rcc == ERR_SUCCESS)
         {
            TCHAR serverInfo[256];
            msg.getFieldAsString(fieldId++, serverInfo, 256);
            uint64_t serverId = msg.getFieldAsUInt64(fieldId++);
            uint32_t version = msg.getFieldAsUInt32(fieldId++);

            const BYTE *hash = msg.getBinaryFieldPtr(fieldId++, &size);
            if (hash != nullptr)
               RegisterPolicy(type, guid, version, serverId, serverInfo, hash);

            nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Policy %s of type %s successfully %s"), guid.toString().cstr(), type, sameDataDirectory ? _T("registered") : _T("deployed"));
            fieldId += 93;
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Deployment of policy %s of type %s failed (error %u)"), guid.toString().cstr(), type, rcc);
            fieldId += 97;
         }
      }
      query.shrink(1);
      query.append(_T(")"));
   }

   // Delete policies that are no longer installed in master agent
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Query for expired policies: %s"), query.cstr());

   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return;

   for(int row = 0; row < DBGetNumRows(hResult); row++)
   {
      uuid guid = DBGetFieldGUID(hResult, row, 0);
      TCHAR type[32];
      DBGetField(hResult, row, 1, type, 32);

      uint32_t rcc = sameDataDirectory ? ERR_SUCCESS : RemovePolicy(guid, GetPolicyDeploymentDirectory(type));
      if (rcc == ERR_SUCCESS)
      {
         UnregisterPolicy(guid);
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Policy %s of type %s %s successfully"), guid.toString().cstr(), type, sameDataDirectory ? _T("unregistered") : _T("uninstalled"));
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Policy %s of type %s uninstall failed (error %u)"), guid.toString().cstr(), type, rcc);
      }
   }
   DBFreeResult(hResult);
}
