/*
** NetXMS - Network Management System
** Copyright (C) 2023 Raden Solutions
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
** File: objects.cpp
**
**/

#include "webapi.h"

/**
 * Handler for /v1/alarms
 */
int H_Alarms(Context *context)
{
   uint32_t rootId = context->getQueryParameterAsUInt32("rootObject");
   bool includeObjectDetails = context->getQueryParameterAsBoolean("includeObjectDetails");

   json_t *output = json_array();

   ObjectArray<Alarm> *alarms = GetAlarms(rootId, true);
   for(int i = 0; i < alarms->size(); i++)
   {
      Alarm *alarm = alarms->get(i);
      shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
      if ((object != nullptr) &&
          object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_ALARMS) &&
          alarm->checkCategoryAccess(context->getUserId(), context->getSystemAccessRights()))
      {
         json_t *json = json_object();
         json_object_set_new(json, "id", json_integer(alarm->getAlarmId()));
         json_object_set_new(json, "severity", json_integer(alarm->getCurrentSeverity()));
         json_object_set_new(json, "state", json_integer(alarm->getState() & ALARM_STATE_MASK));
         json_object_set_new(json, "source", json_integer(object->getId()));
         json_object_set_new(json, "message", json_string_t(alarm->getMessage()));
         json_object_set_new(json, "lastChangeTime", json_time_string(alarm->getLastChangeTime()));
         if (includeObjectDetails)
         {
            json_object_set_new(json, "sourceObject", CreateObjectSummary(*object));
         }
         json_array_append_new(output, json);
      }
   }
   delete alarms;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Get alarm from request
 */
static Alarm *AlarmFromRequest(Context *context, uint32_t objectAccess, int *responseCode)
{
   uint32_t alarmId = context->getPlaceholderValueAsUInt32(_T("alarm-id"));
   if (alarmId == 0)
   {
      *responseCode = 400;
      return nullptr;
   }

   Alarm *alarm = FindAlarmById(alarmId);
   if (alarm == nullptr)
   {
      *responseCode = 404;
      return nullptr;
   }

   shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
   if ((object == nullptr) ||
       !object->checkAccessRights(context->getUserId(), objectAccess) ||
       !alarm->checkCategoryAccess(context->getUserId(), context->getSystemAccessRights()))
   {
      delete alarm;
      *responseCode = 403;
      return nullptr;
   }

   return alarm;
}

/**
 * Handler for /v1/alarms/:alarm-id
 */
int H_AlarmDetails(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_READ_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   json_t *json = alarm->toJson();
   context->setResponseData(json);
   json_decref(json);

   delete alarm;
   return 200;
}

/**
 * Handler for /v1/alarms/:alarm-id/acknowledge
 */
int H_AlarmAcknowledge(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_UPDATE_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   AckAlarmById(alarm->getAlarmId(), context, false, 0, true);
   delete alarm;
   return 204;
}

/**
 * Handler for /v1/alarms/:alarm-id/resolve
 */
int H_AlarmResolve(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_TERM_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   ResolveAlarmById(alarm->getAlarmId(), context, false, true);
   delete alarm;
   return 204;
}

/**
 * Handler for /v1/alarms/:alarm-id/terminate
 */
int H_AlarmTerminate(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_TERM_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   ResolveAlarmById(alarm->getAlarmId(), context, true, true);
   delete alarm;
   return 204;
}
