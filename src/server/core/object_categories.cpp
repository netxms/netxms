/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
** File: object_categories.cpp
**
**/

#include "nxcore.h"

/**
 * Index of object categories
 */
static SharedPointerIndex<ObjectCategory> s_objectCategories;

/**
 * Load object categories from database
 */
void LoadObjectCategories()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,name,icon,map_image FROM object_categories"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         auto category = make_shared<ObjectCategory>(hResult, i);
         s_objectCategories.put(category->getId(), category);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get object category by ID
 */
shared_ptr<ObjectCategory> NXCORE_EXPORTABLE GetObjectCategory(uint32_t id)
{
   return s_objectCategories.get(id);
}

/**
 * Compare category names
 */
static bool CompareObjectCategoryName(ObjectCategory *category, const TCHAR *name)
{
   return _tcsicmp(category->getName(), name) == 0;
}

/**
 * Find object category by name
 */
shared_ptr<ObjectCategory> NXCORE_EXPORTABLE FindObjectCategoryByName(const TCHAR *name)
{
   return s_objectCategories.find(CompareObjectCategoryName, name);
}

/**
 * Reset category for objects
 */
static EnumerationCallbackResult ResetObjectCategory(NetObj *object, uint32_t *categoryId)
{
   if (object->getCategoryId() == *categoryId)
      object->setCategoryId(0);
   return _CONTINUE;
}

/**
 * Compare category for objects
 */
static bool CompareObjectCategoryId(NetObj *object, uint32_t *categoryId)
{
   return object->getCategoryId() == *categoryId;
}

/**
 * Delete object category
 */
uint32_t DeleteObjectCategory(uint32_t id, bool forceDelete)
{
   if (forceDelete)
   {
      g_idxObjectById.forEach(ResetObjectCategory, &id);
   }
   else
   {
      shared_ptr<NetObj> object = g_idxObjectById.find(CompareObjectCategoryId, &id);
      if (object != nullptr)
         return RCC_CATEGORY_IN_USE;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = ExecuteQueryOnObject(hdb, id, _T("DELETE FROM object_categories WHERE id=?"));
   DBConnectionPoolReleaseConnection(hdb);
   if (!success)
      return RCC_DB_FAILURE;

   s_objectCategories.remove(id);

   NotifyClientSessions(NX_NOTIFY_OBJECT_CATEGORY_DELETED, id, NXC_CHANNEL_OBJECTS);
   return RCC_SUCCESS;
}

/**
 * Create or modify object category from NXCP message
 */
uint32_t ModifyObjectCategory(const NXCPMessage& msg, uint32_t *categoryId)
{
   auto category = make_shared<ObjectCategory>(msg);
   if (*category->getName() == 0)
      return RCC_CATEGORY_NAME_EMPTY;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   static const TCHAR *columns[] = {
      _T("name") ,_T("icon"), _T("map_image"), nullptr
   };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("object_categories"), _T("id"), category->getId(), columns);
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, category->getName(), DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, category->getIcon());
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, category->getMapImage());
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, category->getId());
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
      return RCC_DB_FAILURE;

   s_objectCategories.put(category->getId(), category);
   *categoryId = category->getId();

   NXCPMessage notificationMessage(CMD_OBJECT_CATEGORY_UPDATE, 0);
   category->fillMessage(&notificationMessage, VID_ELEMENT_LIST_BASE);
   NotifyClientSessions(notificationMessage, NXC_CHANNEL_OBJECTS);

   return RCC_SUCCESS;
}

/**
 * Get list of all object categories into NXCP message
 */
void ObjectCategoriesToMessage(NXCPMessage *msg)
{
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   s_objectCategories.forEach(
      [msg, &fieldId] (ObjectCategory *category) -> EnumerationCallbackResult
      {
         category->fillMessage(msg, fieldId);
         fieldId += 10;
         return _CONTINUE;
      });
   msg->setField(VID_NUM_ELEMENTS, (fieldId - VID_ELEMENT_LIST_BASE) / 10);
}

/**
 * Object category constructor - create from NXCP message
 */
ObjectCategory::ObjectCategory(const NXCPMessage& msg)
{
   m_id = msg.getFieldAsUInt32(VID_CATEGORY_ID);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_OBJECT_CATEGORY);
   msg.getFieldAsString(VID_NAME, m_name, MAX_OBJECT_NAME);
   m_icon = msg.getFieldAsGUID(VID_ICON);
   m_mapImage = msg.getFieldAsGUID(VID_IMAGE);
}

/**
 * Object category constructor - create from database query
 */
ObjectCategory::ObjectCategory(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   DBGetField(hResult, row, 1, m_name, MAX_OBJECT_NAME);
   m_icon = DBGetFieldGUID(hResult, row, 2);
   m_mapImage = DBGetFieldGUID(hResult, row, 3);
}

/**
 * Fill NXCP message with category data
 */
void ObjectCategory::fillMessage(NXCPMessage *msg, uint32_t baseId)
{
   msg->setField(baseId++, m_id);
   msg->setField(baseId++, m_name);
   msg->setField(baseId++, m_icon);
   msg->setField(baseId++, m_mapImage);
}
