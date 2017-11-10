/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Register policy in persistent storage
 */
static void RegisterPolicy(CommSession *session, UINT32 type, const uuid& guid)
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
                       _T("INSERT INTO agent_policy (type,server_info,server_id,version, guid)")
                       _T(" VALUES (?,?,?,0,?)"));
      }
      else
      {
         hStmt = DBPrepare(hdb,
                       _T("UPDATE agent_policy SET type=?,server_info=?,server_id=?,version=0")
                       _T(" WHERE guid=?"));
      }

      if (hStmt == NULL)
         return;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)type);
      session->getServerAddress().toString(buffer);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, buffer, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, session->getServerId());
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, guid);

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
static int GetPolicyType(const uuid& guid)
{
   int type = -1;
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
            type = DBGetNumRows(hResult) > 0 ? DBGetFieldULong(hResult, 0, 0) : -1;
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
	   }
   }
	return type;
}

/**
 * Deploy configuration file
 */
static UINT32 DeployConfig(AbstractCommSession *session, const uuid& guid, NXCPMessage *msg)
{
	TCHAR path[MAX_PATH], name[64], tail;
	int fh;
	UINT32 rcc;

	tail = g_szConfigPolicyDir[_tcslen(g_szConfigPolicyDir) - 1];
	_sntprintf(path, MAX_PATH, _T("%s%s%s.conf"), g_szConfigPolicyDir,
	           ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
              guid.toString(name));

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
		      session->debugPrintf(3, _T("Configuration file %s saved successfully"), path);
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
		session->debugPrintf(2, _T("DeployConfig(): Error opening file %s for writing (%s)"), path, _tcserror(errno));
		rcc = ERR_FILE_OPEN_ERROR;
	}

	return rcc;
}

/**
 * Deploy log parser policy
 */
static UINT32 DeployLogParser(AbstractCommSession *session, const uuid& guid, NXCPMessage *msg)
{
   TCHAR path[MAX_PATH], name[64];
	int fh;
	UINT32 rcc;

	_sntprintf(path, MAX_PATH, _T("%s%s.xml"), g_szLogParserDirectory
               ,guid.toString(name));

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
		      session->debugPrintf(3, _T("Log parser file %s saved successfully"), path);
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
		session->debugPrintf(2, _T("DeployLogParser(): Error opening file %s for writing (%s)"), path, _tcserror(errno));
		rcc = ERR_FILE_OPEN_ERROR;
	}

	return rcc;
}

/**
 * Deploy policy on agent
 */
UINT32 DeployPolicy(CommSession *session, NXCPMessage *request)
{
	UINT32 type, rcc;

	type = request->getFieldAsUInt16(VID_POLICY_TYPE);
	uuid guid = request->getFieldAsGUID(VID_GUID);

	switch(type)
	{
		case AGENT_POLICY_CONFIG:
			rcc = DeployConfig(session, guid, request);
			break;
		case AGENT_POLICY_LOG_PARSER:
			rcc = DeployLogParser(session, guid, request);
			break;
		default:
			rcc = ERR_BAD_ARGUMENTS;
			break;
	}

	if (rcc == RCC_SUCCESS)
		RegisterPolicy(session, type, guid);

	session->debugPrintf(3, _T("Policy deployment: TYPE=%d RCC=%d"), type, rcc);
	return rcc;
}

/**
 * Remove configuration file
 */
static UINT32 RemoveConfig(UINT32 session, const uuid& guid, NXCPMessage *msg)
{
	TCHAR path[MAX_PATH], name[64], tail;
	UINT32 rcc;

	tail = g_szConfigPolicyDir[_tcslen(g_szConfigPolicyDir) - 1];
	_sntprintf(path, MAX_PATH, _T("%s%s%s.conf"), g_szConfigPolicyDir,
	           ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
              guid.toString(name));

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
 * Remove log parser file
 */
static UINT32 RemoveLogParser(UINT32 session, const uuid& guid,  NXCPMessage *msg)
{
	TCHAR path[MAX_PATH], name[64];
	UINT32 rcc;

	_sntprintf(path, MAX_PATH, _T("%s%s.xml"), g_szLogParserDirectory
               ,guid.toString(name));

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
	int type;
	TCHAR buffer[64];

	uuid guid = request->getFieldAsGUID(VID_GUID);
	type = GetPolicyType(guid);
	if(type == -1)
      return RCC_SUCCESS;

	switch(type)
	{
		case AGENT_POLICY_CONFIG:
			rcc = RemoveConfig(session->getIndex(), guid, request);
			break;
		case AGENT_POLICY_LOG_PARSER:
			rcc = RemoveLogParser(session->getIndex(), guid, request);
			break;
		default:
			rcc = ERR_BAD_ARGUMENTS;
			break;
	}

	if (rcc == RCC_SUCCESS)
		UnregisterPolicy(guid);

   session->debugPrintf(3, _T("Policy uninstall: GUID=%s TYPE=%d RCC=%d"), guid.toString(buffer), type, rcc);
	return rcc;
}

/**
 * Get policy inventory
 */
UINT32 GetPolicyInventory(CommSession *session, NXCPMessage *msg)
{
   UINT32 success = RCC_DB_FAILURE;
	DB_HANDLE hdb = GetLocalDatabaseHandle();
	if(hdb != NULL)
   {
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,type,server_info,server_id,version FROM agent_policy"));
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         if(count > 0 )
         {
            msg->setField(VID_NUM_ELEMENTS, (UINT32)count);
         }
         else
            msg->setField(VID_NUM_ELEMENTS, (UINT32)0);

         UINT32 varId = VID_ELEMENT_LIST_BASE;
         for(int row = 0; row < count; row++, varId += 5)
         {
				msg->setField(varId++, DBGetFieldGUID(hResult, row, 0));
				msg->setField(varId++, DBGetFieldULong(hResult, row, 1));
				TCHAR *text = DBGetField(hResult, row, 2, NULL, 0);
				msg->setField(varId++, CHECK_NULL_EX(text));
				free(text);
				msg->setField(varId++, DBGetFieldInt64(hResult, row, 3));
				msg->setField(varId++, DBGetFieldULong(hResult, row, 4));

         }
         DBFreeResult(hResult);
      }
      success = RCC_SUCCESS;
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
            int type = DBGetFieldULong(hResult, row, 1);
            TCHAR filePath[MAX_PATH], name[64], tail;

            switch(type)
            {
               case AGENT_POLICY_CONFIG:
                  tail = g_szConfigPolicyDir[_tcslen(g_szConfigPolicyDir) - 1];
                  _sntprintf(filePath, MAX_PATH, _T("%s%s%s.conf"), g_szConfigPolicyDir,
                             ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
                             guid.toString(name));
                  break;
               case AGENT_POLICY_LOG_PARSER:
                  _sntprintf(filePath, MAX_PATH, _T("%s%s.xml"), g_szLogParserDirectory, guid.toString(name));
                  break;
               default:
                  continue;
                  break;
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
