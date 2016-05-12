/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Raden Solutions
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
** File: market.cpp
**
**/

#include "nxcore.h"

/**
 * Get free repository ID
 */
static INT32 NewRepositoryId()
{
   INT32 id = -1;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(id) FROM config_repositories"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         id = DBGetFieldLong(hResult, 0, 0) + 1;
      else
         id = 1;
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return id;
}

/**
 * Check if given repository ID is valid
 */
static int CheckRepositoryId(INT32 id)
{
   int result = -1;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT count(*) FROM config_repositories WHERE id=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         result = DBGetFieldLong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Get configured repositories
 */
void ClientSession::getRepositories(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_REPOSITORIES) || checkSysAccessRights(SYSTEM_ACCESS_VIEW_REPOSITORIES))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,url,auth_token,description FROM config_repositories"));
      if (hResult != NULL)
      {
         TCHAR buffer[1024];
         int count = DBGetNumRows(hResult);
         msg.setField(VID_NUM_ELEMENTS, (INT32)count);
         UINT32 fieldId = VID_ELEMENT_LIST_BASE;
         for(int i = 0; i < count; i++)
         {
            msg.setField(fieldId++, DBGetFieldLong(hResult, i, 0));
            msg.setField(fieldId++, DBGetField(hResult, i, 1, buffer, 1024));
            msg.setField(fieldId++, DBGetField(hResult, i, 2, buffer, 1024));
            msg.setField(fieldId++, DBGetField(hResult, i, 3, buffer, 1024));
            fieldId += 6;
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Read list of configured repositories"));
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied accessing list of configured repositories"));
   }

   sendMessage(&msg);
}

/**
 * Add repository
 */
void ClientSession::addRepository(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_REPOSITORIES))
   {
      INT32 id = NewRepositoryId();
      if (id > 0)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO config_repositories (id,url,auth_token,description) VALUES (?,?,?,?)"));
         if (hStmt != NULL)
         {
            TCHAR *url = request->getFieldAsString(VID_URL);
            TCHAR *authToken = request->getFieldAsString(VID_AUTH_TOKEN);
            TCHAR *description = request->getFieldAsString(VID_DESCRIPTION);

            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, url, DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, authToken, DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, description, DB_BIND_STATIC);

            if (DBExecute(hStmt))
            {
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("New repository added (id=%d url=%s)"), id, CHECK_NULL(url));
               msg.setField(VID_RCC, RCC_SUCCESS);
               msg.setField(VID_REPOSITORY_ID, id);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }

            DBFreeStatement(hStmt);
            free(url);
            free(authToken);
            free(description);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on adding repository"));
   }

   sendMessage(&msg);
}

/**
 * Add repository
 */
void ClientSession::modifyRepository(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   INT32 id = request->getFieldAsInt32(VID_REPOSITORY_ID);
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_REPOSITORIES))
   {
      int check = CheckRepositoryId(id);
      if (check > 0)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE config_repositories SET url=?,auth_token=?,description=? WHERE id=?"));
         if (hStmt != NULL)
         {
            TCHAR *url = request->getFieldAsString(VID_URL);
            TCHAR *authToken = request->getFieldAsString(VID_AUTH_TOKEN);
            TCHAR *description = request->getFieldAsString(VID_DESCRIPTION);

            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, url, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, authToken, DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, description, DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, id);

            if (DBExecute(hStmt))
            {
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Repository updated (id=%d url=%s)"), id, CHECK_NULL(url));
               msg.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }

            DBFreeStatement(hStmt);
            free(url);
            free(authToken);
            free(description);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else if (check == 0)
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on changing repository [%d]"), id);
   }

   sendMessage(&msg);
}

/**
 * Delete repository
 */
void ClientSession::deleteRepository(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   INT32 id = request->getFieldAsInt32(VID_REPOSITORY_ID);
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_REPOSITORIES))
   {
      int check = CheckRepositoryId(id);
      if (check > 0)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM config_repositories WHERE id=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
            if (DBExecute(hStmt))
            {
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Repository [%d] deleted"), id);
               msg.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }

            DBFreeStatement(hStmt);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else if (check == 0)
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting repository [%d]"), id);
   }

   sendMessage(&msg);
}
