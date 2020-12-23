/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2020 Raden Solutions
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

void UpdateUserAgentsConfiguration();

/**
 * Update agent environment from config
 */
static void UpdateEnvironment(CommSession *session)
{
   shared_ptr<Config> oldConfig = g_config;
   StringList currEnvList;
   ObjectArray<ConfigEntry> *entrySet = oldConfig->getSubEntries(_T("/ENV"), _T("*"));
   if (entrySet != nullptr)
   {
      for(int i = 0; i < entrySet->size(); i++)
      {
         ConfigEntry *e = entrySet->get(i);
         currEnvList.add(e->getName());
      }
      delete entrySet;
   }

   if (LoadConfig(oldConfig->getAlias(_T("agent")), false))
   {
      StringList newEnvList;
      shared_ptr<Config> newConfig = g_config;
      ObjectArray<ConfigEntry> *newEntrySet = newConfig->getSubEntries(_T("/ENV"), _T("*"));
      if (newEntrySet != nullptr)
      {
         for(int i = 0; i < newEntrySet->size(); i++)
         {
            ConfigEntry *e = newEntrySet->get(i);
            session->debugPrintf(6, _T("UpdateEnvironment(): set environment variable %s=%s"), e->getName(), e->getValue());
            SetEnvironmentVariable(e->getName(), e->getValue());
            newEnvList.add(e->getName());
         }
         delete newEntrySet;
      }

      for(int i = 0; i < currEnvList.size(); i++)
      {
         int j = 0;
         for(;j < newEnvList.size(); j++)
         {
            if(_tcscmp(currEnvList.get(i), newEnvList.get(j)))
               break;
         }
         if (j == newEnvList.size())
         {
            session->debugPrintf(6, _T("UpdateEnvironment(): unset environment variable %s"), currEnvList.get(i));
            SetEnvironmentVariable(currEnvList.get(i), nullptr);
         }
      }
   }
}

/**
 * Check is if new config contains ENV section
 */
static void CheckEnvSectionAndReload(CommSession *session, const char *content, size_t contentLength)
{
   Config config;
   config.setTopLevelTag(_T("config"));
   bool validConfig = config.loadConfigFromMemory(content, contentLength, DEFAULT_CONFIG_SECTION, nullptr, true, false);
   if (validConfig)
   {
      ObjectArray<ConfigEntry> *entrySet = config.getSubEntries(_T("/ENV"), _T("*"));
      if (entrySet != nullptr)
      {
         session->debugPrintf(7, _T("CheckEnvSectionAndReload(): ENV section exists"));
         UpdateEnvironment(session);
         delete entrySet;
      }
   }
}

/**
 * Register policy in persistent storage
 */
static void RegisterPolicy(CommSession *session, const TCHAR *type, const uuid& guid, UINT32 version, BYTE *hash)
{
   bool isNew = true;
   TCHAR buffer[64];
   DB_HANDLE hdb = GetLocalDatabaseHandle();
	if (hdb != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT * FROM agent_policy WHERE guid=?"));
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
      session->getServerAddress().toString(buffer);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, buffer, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, session->getServerId());
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
	if(hdb != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb,
                    _T("DELETE FROM agent_policy WHERE guid=?"));

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
static const String GetPolicyType(const uuid& guid)
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
static uint32_t DeployPolicy(AbstractCommSession *session, const uuid& guid, NXCPMessage *msg, const TCHAR *policyPath, bool *sameFileContent)
{
   *sameFileContent = false;

   TCHAR policyFileName[MAX_PATH], name[64];
   _sntprintf(policyFileName, MAX_PATH, _T("%s%s.xml"), policyPath, guid.toString(name));

   uint32_t rcc;
   size_t size;
   const BYTE *data = msg->getBinaryFieldPtr(VID_CONFIG_FILE_DATA, &size);
   if (data != nullptr)
   {
      BYTE hashA[MD5_DIGEST_SIZE];
      if (CalculateFileMD5Hash(policyFileName, hashA))
      {
         BYTE hashB[MD5_DIGEST_SIZE];
         CalculateMD5Hash(data, size, hashB);
         *sameFileContent = (memcmp(hashA, hashB, MD5_DIGEST_SIZE) == 0);
      }

      if (*sameFileContent)
      {
         session->debugPrintf(3, _T("Policy file %s was not changed"), policyFileName);
         rcc = ERR_SUCCESS;
      }
      else
      {
         SaveFileStatus status = SaveFile(policyFileName, data, size);
         if (status == SaveFileStatus::SUCCESS)
         {
            session->debugPrintf(3, _T("Policy file %s saved successfully"), policyFileName);
            rcc = ERR_SUCCESS;
         }
         else if ((status == SaveFileStatus::OPEN_ERROR) || (status == SaveFileStatus::RENAME_ERROR))
         {
            session->debugPrintf(2, _T("DeployPolicy(): Error opening file %s for writing (%s)"), policyFileName, _tcserror(errno));
            rcc = ERR_FILE_OPEN_ERROR;
         }
         else
         {
            session->debugPrintf(2, _T("DeployPolicy(): Error writing policy file (%s)"), _tcserror(errno));
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
uint32_t DeployPolicy(CommSession *session, NXCPMessage *request)
{
   if (!request->isFieldExist(VID_POLICY_TYPE))
   {
      session->debugPrintf(3, _T("Policy deployment: missing VID_POLICY_TYPE in request message"));
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
   const char *content = reinterpret_cast<const char*>(request->getBinaryFieldPtr(VID_CONFIG_FILE_DATA, &size));
   session->debugPrintf(2, _T("DeployPolicy(): content size %d"), size);

   uint32_t rcc;
   if (!_tcscmp(type, _T("AgentConfig")))
   {
      bool sameFileContent;
      rcc = DeployPolicy(session, guid, request, g_szConfigPolicyDir, &sameFileContent);
      if ((size != 0) && (rcc == ERR_SUCCESS))
      {
         CheckEnvSectionAndReload(session, content, size);
         if (!sameFileContent)
            g_restartPending = true;
      }
   }
   else if (!_tcscmp(type, _T("LogParserConfig")))
   {
      bool sameFileContent;
      rcc = DeployPolicy(session, guid, request, g_szLogParserDirectory, &sameFileContent);
   }
   else if (!_tcscmp(type, _T("SupportApplicationConfig")))
   {
      bool sameFileContent;
      rcc = DeployPolicy(session, guid, request, g_userAgentPolicyDirectory, &sameFileContent);
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
		RegisterPolicy(session, type, guid, version, newHash);

      PolicyChangeNotification n;
      n.guid = guid;
      n.type = type;
		NotifySubAgents(AGENT_NOTIFY_POLICY_INSTALLED, &n);
	}
	session->debugPrintf(3, _T("Policy deployment: TYPE=%s RCC=%d"), type, rcc);
   return rcc;
}

/**
 * Remove configuration file
 */
static uint32_t RemovePolicy(uint32_t session, const uuid& guid, TCHAR *dir)
{
   TCHAR path[MAX_PATH], name[64];
   uint32_t rcc;

   _sntprintf(path, MAX_PATH, _T("%s%s.xml"), dir, guid.toString(name));
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
uint32_t UninstallPolicy(CommSession *session, NXCPMessage *request)
{
   uint32_t rcc;
	TCHAR buffer[64];

	uuid guid = request->getFieldAsGUID(VID_GUID);
	const String type = GetPolicyType(guid);
	if(!_tcscmp(type, _T("")))
      return RCC_SUCCESS;

   if (!_tcscmp(type, _T("AgentConfig")))
   {
      rcc = RemovePolicy(session->getIndex(), guid, g_szConfigPolicyDir);
      if (rcc == RCC_SUCCESS)
         g_restartPending = true;
   }
   else if (!_tcscmp(type, _T("LogParserConfig")))
   {
      rcc = RemovePolicy(session->getIndex(), guid, g_szLogParserDirectory);
   }
   else if (!_tcscmp(type, _T("SupportApplicationConfig")))
   {
      rcc = RemovePolicy(session->getIndex(), guid, g_userAgentPolicyDirectory);
      if (rcc == RCC_SUCCESS)
      {
         UpdateUserAgentsConfiguration();
      }
   }
   else
   {
      rcc = ERR_BAD_ARGUMENTS;
   }

	if (rcc == RCC_SUCCESS)
		UnregisterPolicy(guid);

   session->debugPrintf(3, _T("Policy uninstall: GUID=%s TYPE=%s RCC=%d"), guid.toString(buffer), static_cast<const TCHAR*>(type), rcc);
	return rcc;
}

/**
 * Get policy inventory
 */
uint32_t GetPolicyInventory(CommSession *session, NXCPMessage *msg)
{
   uint32_t success = RCC_DB_FAILURE;
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if (hdb != nullptr)
   {
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,type,server_info,server_id,version,content_hash FROM agent_policy"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0 )
         {
            msg->setField(VID_NUM_ELEMENTS, (UINT32)count);

            UINT32 varId = VID_ELEMENT_LIST_BASE;
            for (int row = 0; row < count; row++, varId += 4)
            {
               msg->setField(varId++, DBGetFieldGUID(hResult, row, 0));
               TCHAR type[32];
               msg->setField(varId++, DBGetField(hResult, row, 1, type, 32));
               TCHAR *text = DBGetField(hResult, row, 2, nullptr, 0);
               msg->setField(varId++, CHECK_NULL_EX(text));
               MemFree(text);
               msg->setField(varId++, DBGetFieldInt64(hResult, row, 3));
               msg->setField(varId++, DBGetFieldULong(hResult, row, 4));
               TCHAR hashAsText[33];
               if (DBGetField(hResult, row, 5, hashAsText, 33) != nullptr)
               {
                  BYTE hash[MD5_DIGEST_SIZE];
                  StrToBin(hashAsText, hash, MD5_DIGEST_SIZE);
                  msg->setField(varId++, hash, MD5_DIGEST_SIZE);
               }
            }
         }
         else
         {
            msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(0));
         }
         DBFreeResult(hResult);
         msg->setField(VID_NEW_POLICY_TYPE, true);
         success = RCC_SUCCESS;
      }
   }

	return success;
}

/**
 * Update policy inventory in database from actual policy files
 */
void UpdatePolicyInventory()
{
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if (hdb == nullptr)
	   return;

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,type FROM agent_policy"));
   if (hResult == nullptr)
      return;

   for(int row = 0; row < DBGetNumRows(hResult); row++)
   {
      uuid guid = DBGetFieldGUID(hResult, row, 0);
      TCHAR type[32];
      DBGetField(hResult, row, 1, type, 32);

      TCHAR filePath[MAX_PATH], name[64];
      if (!_tcscmp(type, _T("AgentConfig")))
      {
         _sntprintf(filePath, MAX_PATH, _T("%s%s.xml"), g_szConfigPolicyDir, guid.toString(name));
      }
      else if (!_tcscmp(type, _T("LogParserConfig")))
      {
         _sntprintf(filePath, MAX_PATH, _T("%s%s.xml"), g_szLogParserDirectory, guid.toString(name));
      }
      else if (!_tcscmp(type, _T("SupportApplicationConfig")))
      {
         _sntprintf(filePath, MAX_PATH, _T("%s%s.xml"), g_userAgentPolicyDirectory, guid.toString(name));
      }

      NX_STAT_STRUCT st;
      if (CALL_STAT(filePath, &st) == 0)
      {
         continue;
      }
      UnregisterPolicy(guid);
   }
   DBFreeResult(hResult);
}
