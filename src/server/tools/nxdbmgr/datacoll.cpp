/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2024 Victor Kirhenshtein
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
** File: datacoll.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Data collection object info
 */
struct DCObjectInfo
{
   uint32_t id;
   uint32_t ownerId;
   uint32_t templateId;
   bool isTable;
   int retentionTime;
};

/**
 * Data collection objects
 */
static HashMap<uint32_t, DCObjectInfo> s_dcObjects(Ownership::True);

/**
 * Load data collection objects
 */
bool LoadDataCollectionObjects()
{
   s_dcObjects.clear();

   DB_RESULT hResult = SQLSelect(L"SELECT item_id,node_id,template_id,retention_time FROM items");
   if (hResult == nullptr)
      return false;

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      DCObjectInfo *object = new DCObjectInfo;
      object->isTable = false;
      object->id = DBGetFieldULong(hResult, i, 0);
      object->ownerId = DBGetFieldULong(hResult, i, 1);
      object->templateId = DBGetFieldULong(hResult, i, 2);
      object->retentionTime = DBGetFieldULong(hResult, i, 3);
      s_dcObjects.set(object->id, object);
   }
   DBFreeResult(hResult);

   hResult = SQLSelect(L"SELECT item_id,node_id,template_id,retention_time FROM dc_tables");
   if (hResult == nullptr)
      return false;

   count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      DCObjectInfo *object = new DCObjectInfo;
      object->isTable = true;
      object->id = DBGetFieldULong(hResult, i, 0);
      object->ownerId = DBGetFieldULong(hResult, i, 1);
      object->templateId = DBGetFieldULong(hResult, i, 2);
      object->retentionTime = DBGetFieldULong(hResult, i, 3);
      s_dcObjects.set(object->id, object);
   }
   DBFreeResult(hResult);

   return true;
}

/**
 * Get storage class for DCI
 */
const wchar_t *GetDCObjectStorageClass(uint32_t id)
{
   DCObjectInfo *object = s_dcObjects.get(id);
   if ((object == nullptr) || (object->retentionTime == 0))
      return L"default";
   if (object->retentionTime <= 7)
      return L"7";
   if (object->retentionTime <= 30)
      return L"30";
   if (object->retentionTime <= 90)
      return L"90";
   if (object->retentionTime <= 180)
      return L"180";
   return L"other";
}
