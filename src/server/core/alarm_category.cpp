/*
** NetXMS - Network Management System
** Copyright (C) 2016 RadenSolutions
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
** File: alarm_category.cpp
**
**/

#include "nxcore.h"

/**
* Callback for sending alarm category configuration change notifications
*/
static void SendAlarmCategoryDBChangeNotification(ClientSession *session, void *arg)
{
   if (session->isAuthenticated())
      session->postMessage((NXCPMessage *)arg);
}
/**
 * Get alarm categories from database
*/
void GetCategories(NXCPMessage *msg)
{
   if (!(g_flags & AF_DB_CONNECTION_LOST))
      {
         // Prepare data message
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,name,descr FROM alarm_categories"));
         DB_RESULT hResultAcl = DBSelect(hdb, _T("SELECT category_id,user_id FROM alarm_category_acl"));

         if (hResult != NULL && hResultAcl != NULL)
         {
            int base = VID_LOC_LIST_BASE;
            int i;
            UINT32 numRows = (UINT32)DBGetNumRows(hResult);
            UINT32 numRowsAcl = (UINT32)DBGetNumRows(hResultAcl);
            IntegerArray<UINT32> *dwUserId = new IntegerArray<UINT32>(16, 16);
            for(i = 0; i < numRowsAcl; i++)
            {
               dwUserId->add(DBGetFieldULong(hResultAcl, i, 1));
            }

            TCHAR szBuffer[256];
            UINT32 counter = 5+numRowsAcl;

            msg->setField(VID_NUM_RECORDS, numRows);
            msg->setField(VID_NUM_FIELDS, counter);

            for (i = 0; i < numRows; i++, base+=counter)
            {
               msg->setField(base, DBGetFieldULong(hResult, i, 0));
               msg->setField(base+1, DBGetField(hResult, i, 1, szBuffer, 64));
               msg->setField(base+2, DBGetField(hResult, i, 2, szBuffer, 256));
               msg->setFieldFromInt32Array(base+3, dwUserId);
            }

            DBFreeResult(hResult);
            DBFreeResult(hResultAcl);
            delete dwUserId;
         }
         else
         {
            msg->setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         msg->setField(VID_RCC, RCC_DB_CONNECTION_LOST);
      }
}

/**
* Update alarm category database
*/
UINT32 UpdateAlarmCategory(NXCPMessage *pRequest)
{
   UINT32 result = 0;
   UINT32 newId = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   UINT32 dwCategoryId = pRequest->getFieldAsUInt32(VID_CATEGORY_ID);

   // Check if category with specified name exists
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT name FROM alarm_categories WHERE id!=? AND name=?"));
   if (hStmt != NULL)
   {
      TCHAR name[64];
      pRequest->getFieldAsString(VID_CATEGORY_NAME, name, 64);

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwCategoryId);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      UINT32 rowCount = (UINT32)DBGetNumRows(hResult);
      DBFreeStatement(hStmt);
      DBFreeResult(hResult);

      // Prepare and execute SQL query
      if (IsValidObjectName(name, FALSE))
      {
         if (rowCount == 0)
         {
            // If new category is created, the passed id == -1
            if (dwCategoryId != -1)
            {
               hStmt = DBPrepare(hdb, _T("UPDATE alarm_categories SET name=?,descr=? WHERE id=?"));
            }
            else
            {
               hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_categories (name,descr,id) VALUES (?,?,?)"));
            }

            if(hStmt != NULL)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, pRequest->getFieldAsString(VID_CATEGORY_DESCRIPTION), DB_BIND_DYNAMIC, 255);

               if (dwCategoryId != -1)
               {
                  DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, dwCategoryId);
               }
               else
               {
                  newId = CreateUniqueId(IDG_ALARM_CATEGORY);
                  DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, newId);
               }

               if (DBExecute(hStmt))
               {
                  // Notify client for DB change
                  NXCPMessage nmsg(pRequest);
                  nmsg.setCode(CMD_ALARM_CATEGORY_UPDATE);
                  nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ALARM_CATEGORY_UPDATE);

                  if (newId != 0)
                  {
                     nmsg.setField(VID_CATEGORY_ID, newId);
                  }

                  EnumerateClientSessions(SendAlarmCategoryDBChangeNotification, &nmsg);
                  DBFreeStatement(hStmt);

                  result = RCC_SUCCESS;
               }
               else
               {
                  result = RCC_DB_FAILURE;
               }
            }
            else
            {
               result = RCC_DB_FAILURE;
            }
         }
         else
         {
            result = RCC_NAME_ALEARDY_EXISTS;
         }
      }
      else
      {
         result = RCC_INVALID_OBJECT_NAME;
      }
   }
   else
   {
      result = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);

   return result;
}

/**
 * Modify alarm acl
 */
UINT32 ModifyAlarmAcl(NXCPMessage *pRequest)
{
   UINT32 result = 0;
   UINT32 dwCategoryId = pRequest->getFieldAsUInt32(VID_CATEGORY_ID);
   UINT32 numAlarmCategoryAcl = pRequest->getFieldAsInt32(VID_NUM_ALARM_CATEGORY_ACL);

   IntegerArray<UINT32> *dwUserId = new IntegerArray<UINT32>(16, 16);
   pRequest->getFieldAsInt32Array(VID_ALARM_CATEGORY_ACL, dwUserId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = DBBegin(hdb);
   if (success)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_category_acl WHERE category_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwCategoryId);
         success = !DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }

      hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_category_acl (category_id,user_id) VALUES (?,?)"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwCategoryId);
         for (int i = 0; i < numAlarmCategoryAcl; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, dwUserId->get(i));
            success = DBExecute(hStmt);
            if (!success)
            {
               break;
            }
         }

         DBFreeStatement(hStmt);
         delete dwUserId;
      }
      else
      {
         result = RCC_DB_FAILURE;
      }

      if (success)
      {
         DBCommit(hdb);
         result = RCC_SUCCESS;
      }
      else
      {
         DBRollback(hdb);
         result = RCC_DB_FAILURE;
      }
   }
   else
   {
      result = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);

   return result;
 }

/**
* Delete alarm category from database
*/
UINT32 DeleteAlarmCategory(NXCPMessage *pRequest)
{
   UINT32 result = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check if category with specific ID exists
   UINT32 dwCategoryId = pRequest->getFieldAsUInt32(VID_CATEGORY_ID);
   bool bCategoryExist = IsDatabaseRecordExist(hdb, _T("alarm_categories"), _T("id"), dwCategoryId);

   // Prepare and execute SQL query
   DB_STATEMENT hStmt;
   if (bCategoryExist && DBBegin(hdb))
   {
      hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_categories WHERE id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwCategoryId);
         if (DBExecute(hStmt))
         {
            result = RCC_SUCCESS;

            NXCPMessage nmsg;
            nmsg.setCode(CMD_ALARM_CATEGORY_UPDATE);
            nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ALARM_CATEGORY_DELETE);
            nmsg.setField(VID_CATEGORY_ID, dwCategoryId);
            EnumerateClientSessions(SendAlarmCategoryDBChangeNotification, &nmsg);

            DBFreeStatement(hStmt);
            // If category delete was successful, delete its acl as well
            hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_category_acl WHERE category_id=?"));
            if (hStmt != NULL)
            {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwCategoryId);
                  if (DBExecute(hStmt))
                  {
                     result = RCC_SUCCESS;
                     DBCommit(hdb);
                  }
                  else
                  {
                     result = RCC_DB_FAILURE;
                     DBRollback(hdb);
                  }
                  DBFreeStatement(hStmt);
            }
            else
            {
               result = RCC_DB_FAILURE;
            }
         }
         else
         {
            result = RCC_DB_FAILURE;
         }
      }

   }
   else
   {
      result = RCC_INVALID_OBJECT_ID;
   }
   DBConnectionPoolReleaseConnection(hdb);

   return result;
}
