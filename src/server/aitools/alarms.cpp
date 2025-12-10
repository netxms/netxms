/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: alarms.cpp
**
**/

#include "aitools.h"
#include <nms_users.h>

/**
 * Get alarm list
 */
std::string F_AlarmList(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccessRights = GetEffectiveSystemRights(userId);

   uint32_t rootId = 0;
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName != nullptr) && (objectName[0] != 0))
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
      if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         char buffer[256];
         snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
         return std::string(buffer);
      }
      rootId = object->getId();
   }

   json_t *output = json_array();
   ObjectArray<Alarm> *alarms = GetAlarms();
   for(int i = 0; i < alarms->size(); i++)
   {
      Alarm *alarm = alarms->get(i);
      shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
      if ((object != nullptr) &&
          ((rootId == 0) || (rootId == object->getId()) || object->isParent(rootId)) &&
          object->checkAccessRights(userId, OBJECT_ACCESS_READ_ALARMS) &&
          alarm->checkCategoryAccess(userId, systemAccessRights))
      {
         json_t *json = json_object();
         json_object_set_new(json, "severity", json_string(AlarmSeverityTextFromCode(alarm->getCurrentSeverity())));
         json_object_set_new(json, "source_name", json_string_w(GetObjectName(object->getId(), L"unknown")));
         json_object_set_new(json, "message", json_string_t(alarm->getMessage()));
         json_object_set_new(json, "last_change_time", json_time_string(alarm->getLastChangeTime()));
         json_array_append_new(output, json);
      }
   }
   delete alarms;

   return JsonToString(output);
}
