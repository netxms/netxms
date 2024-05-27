/*
** NetXMS - Network Management System
** Copyright (C) 2016-2024 RadenSolutions
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
#include <nms_users.h>

/**
 * Create new category
 */
AlarmCategory::AlarmCategory(uint32_t id)
{
   m_id = id;
   m_name = nullptr;
   m_description = nullptr;
}

/**
 * Create category from DB record
 */
AlarmCategory::AlarmCategory(DB_RESULT hResult, int row, IntegerArray<uint32_t> *aclCache)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_name = DBGetField(hResult, row, 1, nullptr, 0);
   m_description = DBGetField(hResult, row, 2, nullptr, 0);

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
 * Create new category
 */
AlarmCategory::AlarmCategory(uint32_t id, const TCHAR *name, const TCHAR *description)
{
   m_id = id;
   m_name = MemCopyString(name);
   m_description = MemCopyString(description);
}

/**
 * Copy constructor
 */
AlarmCategory::AlarmCategory(const AlarmCategory& src)
{
   m_id = src.m_id;
   m_name = MemCopyString(src.m_name);
   m_description = MemCopyString(src.m_description);
}

/**
 * Destructor
 */
AlarmCategory::~AlarmCategory()
{
   MemFree(m_name);
   MemFree(m_description);
}

/**
 * Fill message with alarm category data
 */
void AlarmCategory::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_name);
   msg->setField(baseId + 2, m_description);
   msg->setFieldFromInt32Array(baseId + 3, &m_acl);
}

/**
 * Modify category from NXCP message
 */
void AlarmCategory::modifyFromMessage(const NXCPMessage& msg)
{
   msg.getFieldAsString(VID_NAME, &m_name);
   msg.getFieldAsString(VID_DESCRIPTION, &m_description);
   msg.getFieldAsInt32Array(VID_ALARM_CATEGORY_ACL, &m_acl);
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
      if (hStmt != nullptr)
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
         if (hStmt != nullptr)
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
         if (hStmt != nullptr)
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
 * Check user access
 */
bool AlarmCategory::checkAccess(uint32_t userId)
{
   for(int i = 0; i < m_acl.size(); i++)
   {
      if (((m_acl.get(i) & GROUP_FLAG) && CheckUserMembership(userId, m_acl.get(i))) || (m_acl.get(i) == userId))
         return true;
   }
   return false;
}

/**
 * Alarm categories
 */
static HashMap<uint32_t, AlarmCategory> s_categories(Ownership::True);
static RWLock s_lock;

/**
 * Get alarm categories from database
 */
void GetAlarmCategories(NXCPMessage *msg)
{
   s_lock.readLock();
   msg->setField(VID_NUM_ELEMENTS, s_categories.size());
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   Iterator<AlarmCategory> it = s_categories.begin();
   while(it.hasNext())
   {
      AlarmCategory *c = it.next();
      c->fillMessage(msg, fieldId);
      fieldId += 10;
   }
   s_lock.unlock();
}

/**
* Update alarm category database
*/
uint32_t UpdateAlarmCategory(const NXCPMessage& request, uint32_t *returnId)
{
   TCHAR name[64];
   request.getFieldAsString(VID_NAME, name, 64);
   if (name[0] == 0)
      return RCC_CATEGORY_NAME_EMPTY;

   uint32_t id = request.getFieldAsUInt32(VID_CATEGORY_ID);

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
      if (category == nullptr)
      {
         s_lock.unlock();
         return RCC_INVALID_OBJECT_ID;
      }
   }
   *returnId = id;
   category->modifyFromMessage(request);
   uint32_t rcc = category->saveToDatabase() ? RCC_SUCCESS : RCC_DB_FAILURE;

   // Notify client for DB change
   if (rcc == RCC_SUCCESS)
   {
      NXCPMessage msg;
      msg.setCode(CMD_ALARM_CATEGORY_UPDATE);
      msg.setField(VID_NOTIFICATION_CODE, static_cast<uint16_t>(NX_NOTIFY_ALARM_CATEGORY_UPDATED));
      category->fillMessage(&msg, VID_ELEMENT_LIST_BASE);
      NotifyClientSessions(msg);
   }

   s_lock.unlock();
   return rcc;
}

/**
* Delete alarm category from database
*/
uint32_t DeleteAlarmCategory(uint32_t id)
{
   uint32_t rcc;
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
               nmsg.setField(VID_NOTIFICATION_CODE, static_cast<uint16_t>(NX_NOTIFY_ALARM_CATEGORY_DELETED));
               nmsg.setField(VID_ELEMENT_LIST_BASE, id);
               NotifyClientSessions(nmsg);
            }
         }
         else
         {
            DBRollback(hdb);
         }
      }
      DBConnectionPoolReleaseConnection(hdb);
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
bool CheckAlarmCategoryAccess(uint32_t userId, uint32_t categoryId)
{
   bool result = false;
   s_lock.readLock();
   AlarmCategory *c = s_categories.get(categoryId);
   if (c != nullptr)
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

   IntegerArray<uint32_t> aclCache(256, 256);
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT category_id,user_id FROM alarm_category_acl ORDER BY category_id"));
   if (hResult != nullptr)
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
   if (hResult != nullptr)
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

/**
 * Get alarm category by ID
 */
AlarmCategory *GetAlarmCategory(uint32_t id)
{
   s_lock.readLock();
   AlarmCategory *c = s_categories.get(id);
   AlarmCategory *result = (c != nullptr) ? new AlarmCategory(*c) : nullptr;
   s_lock.unlock();
   return result;
}

/**
 * Update description for alarm category with given name. On success
 * returns ID of updated category, and 0 on failure.
 */
uint32_t UpdateAlarmCategoryDescription(const TCHAR *name, const TCHAR *description)
{
   uint32_t id = 0;
   s_lock.readLock();
   Iterator<AlarmCategory> it = s_categories.begin();
   while(it.hasNext())
   {
     AlarmCategory *c = it.next();
     if (!_tcscmp(c->getName(), name))
     {
        c->updateDescription(description);
        id = c->getId();
        break;
     }
   }
   s_lock.unlock();
   return id;
}

/**
 * Create new alarm category
 */
uint32_t CreateAlarmCategory(const TCHAR *name, const TCHAR *description)
{
   uint32_t id = CreateUniqueId(IDG_ALARM_CATEGORY);
   AlarmCategory *category = new AlarmCategory(id, name, description);

   s_lock.writeLock();
   s_categories.set(id, category);
   category->saveToDatabase();

   // Notify clients
   NXCPMessage msg(CMD_ALARM_CATEGORY_UPDATE, 0);
   msg.setField(VID_NOTIFICATION_CODE, static_cast<uint16_t>(NX_NOTIFY_ALARM_CATEGORY_UPDATED));
   category->fillMessage(&msg, VID_ELEMENT_LIST_BASE);
   NotifyClientSessions(msg);

   s_lock.unlock();
   return id;
}
