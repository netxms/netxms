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
#include <nms_events.h>

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

/**
 * Helper function to parse severity string to numeric value
 */
static int ParseSeverityString(const char *severity)
{
   if (severity == nullptr)
      return -1;
   if (!stricmp(severity, "normal"))
      return SEVERITY_NORMAL;
   if (!stricmp(severity, "warning"))
      return SEVERITY_WARNING;
   if (!stricmp(severity, "minor"))
      return SEVERITY_MINOR;
   if (!stricmp(severity, "major"))
      return SEVERITY_MAJOR;
   if (!stricmp(severity, "critical"))
      return SEVERITY_CRITICAL;
   return -1;
}

/**
 * Create event template
 */
std::string F_CreateEventTemplate(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EDIT_EVENT_DB) == 0)
      return std::string("User does not have rights to edit event configuration");

   const char *name = json_object_get_string_utf8(arguments, "name", nullptr);
   if ((name == nullptr) || (name[0] == 0))
      return std::string("Event template name is required");

   const char *severityStr = json_object_get_string_utf8(arguments, "severity", nullptr);
   if ((severityStr == nullptr) || (severityStr[0] == 0))
      return std::string("Event severity is required");

   int severity = ParseSeverityString(severityStr);
   if (severity == -1)
      return std::string("Invalid severity value. Must be one of: normal, warning, minor, major, critical");

   const char *message = json_object_get_string_utf8(arguments, "message", nullptr);
   if ((message == nullptr) || (message[0] == 0))
      return std::string("Event message template is required");

   json_t *templateJson = json_object();
   json_object_set_new(templateJson, "name", json_string(name));
   json_object_set_new(templateJson, "severity", json_integer(severity));
   json_object_set_new(templateJson, "flags", json_integer(EF_LOG));
   json_object_set_new(templateJson, "message", json_string(message));

   const char *description = json_object_get_string_utf8(arguments, "description", nullptr);
   if (description != nullptr)
      json_object_set_new(templateJson, "description", json_string(description));

   const char *tags = json_object_get_string_utf8(arguments, "tags", nullptr);
   if (tags != nullptr)
      json_object_set_new(templateJson, "tags", json_string(tags));

   json_t *newValue = nullptr;
   uint32_t rcc = CreateEventTemplateFromJson(templateJson, &newValue);
   json_decref(templateJson);

   if (rcc == RCC_SUCCESS)
   {
      uint32_t eventCode = static_cast<uint32_t>(json_integer_value(json_object_get(newValue, "code")));
      json_decref(newValue);
      char buffer[256];
      snprintf(buffer, 256, "Event template \"%s\" created successfully with code %u", name, eventCode);
      return std::string(buffer);
   }

   if (newValue != nullptr)
      json_decref(newValue);

   switch (rcc)
   {
      case RCC_INVALID_OBJECT_NAME:
         return std::string("Invalid event template name. Name must be alphanumeric with underscores.");
      case RCC_NAME_ALEARDY_EXISTS:
         return std::string("Event template with this name already exists");
      case RCC_DB_FAILURE:
         return std::string("Database error occurred while creating event template");
      default:
         return std::string("Failed to create event template");
   }
}

/**
 * Modify event template
 */
std::string F_ModifyEventTemplate(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EDIT_EVENT_DB) == 0)
      return std::string("User does not have rights to edit event configuration");

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

      wchar_t eventNameW[MAX_EVENT_NAME];
      utf8_to_wchar(eventName, -1, eventNameW, MAX_EVENT_NAME);
      eventTemplate = FindEventTemplateByName(eventNameW);
      if (eventTemplate == nullptr)
      {
         char buffer[256];
         snprintf(buffer, 256, "Event template with name \"%s\" not found", eventName);
         return std::string(buffer);
      }
      eventCode = eventTemplate->getCode();
   }

   if (eventCode < 100000)
      return std::string("Cannot modify system event templates. Only user-defined templates (code >= 100000) can be modified.");

   json_t *modifyJson = json_object();

   const char *newName = json_object_get_string_utf8(arguments, "newName", nullptr);
   if (newName != nullptr)
      json_object_set_new(modifyJson, "name", json_string(newName));

   const char *severityStr = json_object_get_string_utf8(arguments, "severity", nullptr);
   if (severityStr != nullptr)
   {
      int severity = ParseSeverityString(severityStr);
      if (severity == -1)
      {
         json_decref(modifyJson);
         return std::string("Invalid severity value. Must be one of: normal, warning, minor, major, critical");
      }
      json_object_set_new(modifyJson, "severity", json_integer(severity));
   }

   const char *message = json_object_get_string_utf8(arguments, "message", nullptr);
   if (message != nullptr)
      json_object_set_new(modifyJson, "message", json_string(message));

   const char *description = json_object_get_string_utf8(arguments, "description", nullptr);
   if (description != nullptr)
      json_object_set_new(modifyJson, "description", json_string(description));

   const char *tags = json_object_get_string_utf8(arguments, "tags", nullptr);
   if (tags != nullptr)
      json_object_set_new(modifyJson, "tags", json_string(tags));

   if (json_object_size(modifyJson) == 0)
   {
      json_decref(modifyJson);
      return std::string("No modifications specified. Provide at least one of: newName, severity, message, description, tags");
   }

   json_t *oldValue = nullptr;
   json_t *newValue = nullptr;
   uint32_t rcc = ModifyEventTemplateFromJson(eventCode, modifyJson, &oldValue, &newValue);
   json_decref(modifyJson);

   if (oldValue != nullptr)
      json_decref(oldValue);

   if (rcc == RCC_SUCCESS)
   {
      json_decref(newValue);
      char buffer[256];
      snprintf(buffer, 256, "Event template with code %u modified successfully", eventCode);
      return std::string(buffer);
   }

   if (newValue != nullptr)
      json_decref(newValue);

   switch (rcc)
   {
      case RCC_INVALID_OBJECT_NAME:
         return std::string("Invalid event template name. Name must be alphanumeric with underscores.");
      case RCC_NAME_ALEARDY_EXISTS:
         return std::string("Event template with this name already exists");
      case RCC_INVALID_EVENT_CODE:
         return std::string("Event template not found");
      case RCC_DB_FAILURE:
         return std::string("Database error occurred while modifying event template");
      default:
         return std::string("Failed to modify event template");
   }
}

/**
 * Delete event template
 */
std::string F_DeleteEventTemplate(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EDIT_EVENT_DB) == 0)
      return std::string("User does not have rights to edit event configuration");

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

      wchar_t eventNameW[MAX_EVENT_NAME];
      utf8_to_wchar(eventName, -1, eventNameW, MAX_EVENT_NAME);
      eventTemplate = FindEventTemplateByName(eventNameW);
      if (eventTemplate == nullptr)
      {
         char buffer[256];
         snprintf(buffer, 256, "Event template with name \"%s\" not found", eventName);
         return std::string(buffer);
      }
      eventCode = eventTemplate->getCode();
   }

   if (eventCode < 100000)
      return std::string("Cannot delete system event templates. Only user-defined templates (code >= 100000) can be deleted.");

   uint32_t rcc = DeleteEventTemplate(eventCode);

   if (rcc == RCC_SUCCESS)
   {
      char buffer[256];
      snprintf(buffer, 256, "Event template with code %u deleted successfully", eventCode);
      return std::string(buffer);
   }

   switch (rcc)
   {
      case RCC_INVALID_EVENT_CODE:
         return std::string("Event template not found");
      case RCC_DB_FAILURE:
         return std::string("Database error occurred while deleting event template");
      default:
         return std::string("Failed to delete event template");
   }
}
