/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2019 Raden Solutions
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
 * Register policy in persistent storage
 */
static void RegisterPolicy(CommSession *session, const TCHAR *type, const uuid& guid, UINT32 version)
{
   bool isNew = true;
   TCHAR buffer[64];
   DB_HANDLE hdb = GetLocalDatabaseHandle();
	if(hdb != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT * FROM agent_policy WHERE guid=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            isNew = DBGetNumRows(hResult) <= 0;
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }

      if(isNew)
      {
         hStmt = DBPrepare(hdb,
                       _T("INSERT INTO agent_policy (type,server_info,server_id,version,guid)")
                       _T(" VALUES (?,?,?,?,?)"));
      }
      else
      {
         hStmt = DBPrepare(hdb,
                       _T("UPDATE agent_policy SET type=?,server_info=?,server_id=?,version=?")
                       _T(" WHERE guid=?"));
      }

      if (hStmt == NULL)
         return;

      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, type, DB_BIND_STATIC);
      session->getServerAddress().toString(buffer);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, buffer, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, session->getServerId());
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, version);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, guid);

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
	if(hdb != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb,
                    _T("DELETE FROM agent_policy WHERE guid=?"));

      if (hStmt == NULL)
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
   String type(_T(""));
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if(hdb != NULL)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT type FROM agent_policy WHERE guid=?"));
	   if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            if(DBGetNumRows(hResult) > 0)
            {
               type.appendPreallocated(DBGetField(hResult, 0, 0, NULL, 0));
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
static UINT32 DeployPolicy(AbstractCommSession *session, const uuid& guid, NXCPMessage *msg, TCHAR *policyPath)
{
   TCHAR path[MAX_PATH], name[64];
   int fh;
   UINT32 rcc;

   _sntprintf(path, MAX_PATH, _T("%s%s.xml"), policyPath, guid.toString(name));

   fh = _topen(path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
   if (fh != -1)
   {
      size_t size = msg->getFieldAsBinary(VID_CONFIG_FILE_DATA, NULL, 0);
      BYTE *data = (BYTE *)malloc(size);
      if (data != NULL)
      {
         msg->getFieldAsBinary(VID_CONFIG_FILE_DATA, data, size);
         if (_write(fh, data, static_cast<unsigned int>(size)) == static_cast<int>(size))
         {
            session->debugPrintf(3, _T("Policy file %s saved successfully"), path);
            rcc = ERR_SUCCESS;
         }
         else
         {
            rcc = ERR_IO_FAILURE;
         }
         free(data);
      }
      else
      {
         rcc = ERR_MEM_ALLOC_FAILED;
      }
      _close(fh);
   }
   else
   {
      session->debugPrintf(2, _T("DeployPolicy(): Error opening file %s for writing (%s)"), path, _tcserror(errno));
      rcc = ERR_FILE_OPEN_ERROR;
   }

   return rcc;
}

/**
 * Deploy policy on agent
 */
UINT32 DeployPolicy(CommSession *session, NXCPMessage *request)
{
	UINT32 rcc;

   TCHAR type[32];
   request->getFieldAsString(VID_POLICY_TYPE, type, 32);
	uuid guid = request->getFieldAsGUID(VID_GUID);

   if (!_tcscmp(type, _T("AgentConfig")))
   {
      rcc = DeployPolicy(session, guid, request, g_szConfigPolicyDir);
   }
   else if (!_tcscmp(type, _T("LogParserConfig")))
   {
      rcc = DeployPolicy(session, guid, request, g_szLogParserDirectory);
   }
   else if (!_tcscmp(type, _T("SupportApplicationConfig")))
   {
      rcc = DeployPolicy(session, guid, request, g_userAgentPolicyDirectory);
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
	{
	   UINT32 version = request->getFieldAsUInt32(VID_VERSION);
		RegisterPolicy(session, type, guid, version);
		NotifySubAgents(AGENT_NOTIFY_POLICY_INSTALLED, &guid);
	}
	session->debugPrintf(3, _T("Policy deployment: TYPE=%s RCC=%d"), type, rcc);
	return rcc;
}

/**
 * Remove configuration file
 */
static UINT32 RemovePolicy(UINT32 session, const uuid& guid, TCHAR *dir)
{
   TCHAR path[MAX_PATH], name[64];
   UINT32 rcc;

   _sntprintf(path, MAX_PATH, _T("%s%s.xml"), dir, guid.toString(name));

   if (_tremove(path) != 0)
   {
      rcc = (errno == ENOENT) ? ERR_SUCCESS : ERR_IO_FAILURE;
   }
   else
   {
      rcc = ERR_SUCCESS;
   }
   return rcc;
}

/**
 * Uninstall policy from agent
 */
UINT32 UninstallPolicy(CommSession *session, NXCPMessage *request)
{
	UINT32 rcc;
	TCHAR buffer[64];

	uuid guid = request->getFieldAsGUID(VID_GUID);
	const String type = GetPolicyType(guid);
	if(!_tcscmp(type, _T("")))
      return RCC_SUCCESS;

   if (!_tcscmp(type, _T("AgentConfig")))
   {
      rcc = RemovePolicy(session->getIndex(), guid, g_szConfigPolicyDir);
   }
   else if (!_tcscmp(type, _T("LogParserConfig")))
   {
      rcc = RemovePolicy(session->getIndex(), guid, g_szLogParserDirectory);
   }
   else if (!_tcscmp(type, _T("SupportApplicationConfig")))
   {
      rcc = RemovePolicy(session->getIndex(), guid, g_userAgentPolicyDirectory);
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
UINT32 GetPolicyInventory(CommSession *session, NXCPMessage *msg)
{
   UINT32 success = RCC_DB_FAILURE;
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if (hdb != NULL)
   {
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,type,server_info,server_id,version FROM agent_policy"));
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0 )
         {
            msg->setField(VID_NUM_ELEMENTS, (UINT32)count);

            UINT32 varId = VID_ELEMENT_LIST_BASE;
            for (int row = 0; row < count; row++, varId += 5)
            {
               msg->setField(varId++, DBGetFieldGUID(hResult, row, 0));
               TCHAR type[32];
               msg->setField(varId++, DBGetField(hResult, row, 1, type, 32));
               TCHAR *text = DBGetField(hResult, row, 2, NULL, 0);
               msg->setField(varId++, CHECK_NULL_EX(text));
               free(text);
               msg->setField(varId++, DBGetFieldInt64(hResult, row, 3));
               msg->setField(varId++, DBGetFieldULong(hResult, row, 4));
            }
         }
         else
         {
            msg->setField(VID_NUM_ELEMENTS, (UINT32)0);
         }
         DBFreeResult(hResult);
         msg->setField(VID_NEW_POLICY_TYPE, true);
         success = RCC_SUCCESS;
      }
   }

	return success;
}

void UpdatePolicyInventory()
{
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if(hdb != NULL)
   {
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,type FROM agent_policy"));
      if (hResult != NULL)
      {
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
   }
}
