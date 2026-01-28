/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: evproc.cpp
**
**/

#include "aitools.h"
#include <nms_users.h>

/**
 * Get event template
 */
std::string F_GetEventTemplate(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_VIEW_EVENT_DB) == 0)
      return std::string("User does not have rights to view event configuration");

   shared_ptr<EventTemplate> eventTemplate;

   uint32_t eventCode = json_object_get_uint32(arguments, "code", 0);
   if (eventCode != 0)
   {
      eventTemplate = FindEventTemplateByCode(eventCode);
      if (eventTemplate == nullptr)
      {
         char buffer[256];
         snprintf(buffer, 256, "Event template with code %u not found", eventCode);
         return std::string(buffer);
      }
   }
   else
   {
      const char *eventName = json_object_get_string_utf8(arguments, "name", nullptr);
      if ((eventName == nullptr) || (eventName[0] == 0))
         return std::string("Either event template code or name must be provided");

      wchar_t eventNameW[64];
      utf8_to_wchar(eventName, -1, eventNameW, 64);
      eventTemplate = FindEventTemplateByName(eventNameW);
      if (eventTemplate == nullptr)
      {
         char buffer[256];
         snprintf(buffer, 256, "Event template with name \"%s\" not found", eventName);
         return std::string(buffer);
      }
   }

   return JsonToString(eventTemplate->toJson());
}

/**
 * Get event processing policy
 */
std::string F_GetEventProcessingPolicy(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EPP) == 0)
      return std::string("User does not have rights to manage event processing policy");
   return JsonToString(GetEventProcessingPolicy()->toJson());
}

/**
 * Get list server side actions for event processing
 */
std::string F_GetEventProcessingActions(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & (SYSTEM_ACCESS_MANAGE_ACTIONS | SYSTEM_ACCESS_EPP)) == 0)
      return std::string("User does not have rights to access server actions");

   return JsonToString(GetActions());
}

/**
 * Get specific server side action for event processing
 */
std::string F_GetEventProcessingAction(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & (SYSTEM_ACCESS_MANAGE_ACTIONS | SYSTEM_ACCESS_EPP)) == 0)
      return std::string("User does not have rights to access server actions");

   uint32_t actionId = json_object_get_uint32(arguments, "id", 0);
   if (actionId == 0)
      return std::string("Action ID not provided");

   json_t *actionJson = GetActionById(actionId);
   if (actionJson == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Action with ID %u not found", actionId);
      return std::string(buffer);
   }

   return JsonToString(actionJson);
}
