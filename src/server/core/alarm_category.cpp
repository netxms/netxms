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
 * Alarm category
 */
class AlarmCategory
{
private:
   UINT32 m_id;
   TCHAR *m_name;
   TCHAR *m_description;
   IntegerArray<UINT32> m_acl;

public:
   AlarmCategory(UINT32 id);
   AlarmCategory(DB_RESULT hResult, int row, IntegerArray<UINT32> *aclCache);
   ~AlarmCategory();

   UINT32 getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDescription() const { return m_description; }

   bool checkAccess(UINT32 userId) const { return m_acl.contains(userId); }

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;
   void modifyFromMessage(const NXCPMessage *msg);
   bool saveToDatabase() const;
};

/**
 * Create new category
 */
AlarmCategory::AlarmCategory(UINT32 id)
{
   m_id = id;
   m_name = NULL;
   m_description = NULL;
}

/**
 * Create category from DB record
 */
AlarmCategory::AlarmCategory(DB_RESULT hResult, int row, IntegerArray<UINT32> *aclCache)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_name = DBGetField(hResult, row, 1, NULL, 0);
   m_description = DBGetField(hResult, row, 2, NULL, 0);

   int i = 0;
   while((i < aclCache->size()) && (aclCache->get(i) != m_id))
      i += 2;
   while((i < aclCache->size()) && (aclCache->get(i) == m_id))
   {
      m_acl.add(aclCache->get(i + 1));
      i += 2;
   }
}

/**
 * Destructor
 */
AlarmCategory::~AlarmCategory()
{
   free(m_name);
   free(m_description);
}

/**
 * Fill message with alarm category data
 */
void AlarmCategory::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_name);
   msg->setField(baseId + 2, m_description);
   msg->setFieldFromInt32Array(baseId + 3, &m_acl);
}

/**
 * Modify category from NXCP message
 */
void AlarmCategory::modifyFromMessage(const NXCPMessage *msg)
{
   UINT32 fields = msg->getFieldAsUInt32(VID_FIELDS);

   if (fields & ALARM_MODIFY_CATEGORY)
   {
      free(m_name);
      m_name = msg->getFieldAsString(VID_NAME);

      free(m_description);
      m_description = msg->getFieldAsString(VID_DESCRIPTION);
   }

   if (fields & ALARM_MODIFY_ACCESS_LIST)
   {
      msg->getFieldAsInt32Array(VID_ALARM_CATEGORY_ACL, &m_acl);
   }
}

/**
 * Save category to database
 */
bool AlarmCategory::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   bool success = DBBegin(hdb);
   if (success)
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("alarm_categories"), _T("id"), m_id))
         hStmt = DBPrepare(hdb, _T("UPDATE alarm_categories SET name=?,descr=? WHERE id=?"));
      else
         hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_categories (name,descr,id) VALUES (?,?,?)"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_category_acl WHERE category_id=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_category_acl (category_id,user_id) VALUES (?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_acl.size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_acl.get(i));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
      }

      if (success)
         success = DBCommit(hdb);
      else
         DBRollback(hdb);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Alarm categories
 */
static HashMap<UINT32, AlarmCategory> s_categories(true);
static RWLock s_lock;

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
void GetAlarmCategories(NXCPMessage *msg)
{
   s_lock.readLock();
   msg->setField(VID_NUM_ELEMENTS, s_categories.size());
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
   Iterator<AlarmCategory> *it = s_categories.iterator();
   while(it->hasNext())
   {
      AlarmCategory *c = it->next();
      c->fillMessage(msg, fieldId);
      fieldId += 10;
   }
   delete it;
   s_lock.unlock();
}

/**
* Update alarm category database
*/
UINT32 UpdateAlarmCategory(const NXCPMessage *request)
{
   UINT32 id = request->getFieldAsUInt32(VID_CATEGORY_ID);

   TCHAR name[64];
   request->getFieldAsString(VID_NAME, name, 64);
   s_lock.readLock();
   bool nameExists = false;
   Iterator<AlarmCategory> *it = s_categories.iterator();
   while(it->hasNext())
   {
      AlarmCategory *c = it->next();
      if (!_tcsicmp(c->getName(), name) && (c->getId() != id))
      {
         nameExists = true;
         break;
      }
   }
   delete it;
   s_lock.unlock();
   if (nameExists)
      return RCC_NAME_ALEARDY_EXISTS;

   AlarmCategory *category;
   s_lock.writeLock();
   if (id == 0)
   {
      id = CreateUniqueId(IDG_ALARM_CATEGORY);
      category = new AlarmCategory(id);
      s_categories.set(id, category);
   }
   else
   {
      category = s_categories.get(id);
      if (category == NULL)
      {
         s_lock.unlock();
         return RCC_INVALID_OBJECT_ID;
      }
   }
   category->modifyFromMessage(request);
   UINT32 rcc = category->saveToDatabase() ? RCC_SUCCESS : RCC_DB_FAILURE;

   // Notify client for DB change
   if (rcc == RCC_SUCCESS)
   {
      NXCPMessage msg;
      msg.setCode(CMD_ALARM_CATEGORY_UPDATE);
      msg.setField(VID_NOTIFICATION_CODE, (UINT16)NX_NOTIFY_ALARM_CATEGORY_UPDATE);
      category->fillMessage(&msg, VID_ELEMENT_LIST_BASE);
      EnumerateClientSessions(SendAlarmCategoryDBChangeNotification, &msg);
   }

   s_lock.unlock();
   return rcc;
}

/**
* Delete alarm category from database
*/
UINT32 DeleteAlarmCategory(UINT32 id)
{
   UINT32 rcc;
   s_lock.readLock();
   if (s_categories.contains(id))
   {
      s_lock.unlock();

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      bool success = DBBegin(hdb);
      if (success)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_categories WHERE id=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }

         if (success)
         {
            hStmt = DBPrepare(hdb, _T("DELETE FROM alarm_category_acl WHERE category_id=?"));
            if (hStmt != NULL)
            {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
                  success = DBExecute(hStmt);
                  DBFreeStatement(hStmt);
            }
            else
            {
               success = false;
            }
         }

         if (success)
         {
            success = DBCommit(hdb);
            if (success)
            {
               s_lock.writeLock();
               s_categories.remove(id);
               s_lock.unlock();

               NXCPMessage nmsg;
               nmsg.setCode(CMD_ALARM_CATEGORY_UPDATE);
               nmsg.setField(VID_NOTIFICATION_CODE, (UINT16)NX_NOTIFY_ALARM_CATEGORY_DELETE);
               nmsg.setField(VID_CATEGORY_ID, id);
               EnumerateClientSessions(SendAlarmCategoryDBChangeNotification, &nmsg);
            }
         }
         else
         {
            DBRollback(hdb);
         }
      }
      rcc = success ? RCC_SUCCESS : RCC_DB_FAILURE;
   }
   else
   {
      s_lock.unlock();
      rcc = RCC_INVALID_OBJECT_ID;
   }
   return rcc;
}

/**
 * Check user access to alarm category
 */
bool CheckAlarmCategoryAccess(UINT32 userId, UINT32 categoryId)
{
   bool result = false;
   s_lock.readLock();
   AlarmCategory *c = s_categories.get(categoryId);
   if (c != NULL)
   {
      result = c->checkAccess(userId);
   }
   s_lock.unlock();
   return result;
}

/**
 * Load alarm categories
 */
void LoadAlarmCategories()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   IntegerArray<UINT32> aclCache(256, 256);
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT category_id,user_id FROM alarm_category_acl ORDER BY category_id"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         aclCache.add(DBGetFieldULong(hResult, i, 0));
         aclCache.add(DBGetFieldULong(hResult, i, 1));
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, _T("SELECT id,name,descr FROM alarm_categories"));
   if (hResult != NULL)
   {
      int numRows = DBGetNumRows(hResult);
      s_lock.writeLock();
      s_categories.clear();
      for(int i = 0; i < numRows; i++)
      {
         AlarmCategory *c = new AlarmCategory(hResult, i, &aclCache);
         s_categories.set(c->getId(), c);
      }
      s_lock.unlock();
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
