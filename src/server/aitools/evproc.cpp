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
      eventTemplate = FindEventTemplate(eventNameW);
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
      eventTemplate = FindEventTemplate(eventNameW);
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
      eventTemplate = FindEventTemplate(eventNameW);
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

// =============================================================================
// EPP / Server-Action write-side: shared helpers
// =============================================================================

/**
 * Build a JSON-encoded filter/validation error for direct return from F_*.
 */
static std::string EppFilterError(const char *errorText, const char *fieldPath, const char *value)
{
   json_t *err = json_object();
   json_object_set_new(err, "error", json_string(errorText));
   if (fieldPath != nullptr)
      json_object_set_new(err, "field", json_string(fieldPath));
   if (value != nullptr)
      json_object_set_new(err, "value", json_string(value));
   return JsonToString(err);
}

/**
 * Resolve an event identifier (numeric code or string name) into a code.
 * Returns true on success. Returns false with *error populated when the value
 * is the wrong type or names an unknown event.
 */
static bool EppResolveEventCode(json_t *value, uint32_t *out, const char *fieldPath, std::string *error)
{
   if (json_is_integer(value))
   {
      *out = static_cast<uint32_t>(json_integer_value(value));
      return true;
   }
   if (json_is_string(value))
   {
      const char *s = json_string_value(value);
      String name(s, "utf-8");
      uint32_t code = EventCodeFromName(name.cstr(), 0);
      if (code == 0)
      {
         *error = EppFilterError("unknown event", fieldPath, s);
         return false;
      }
      *out = code;
      return true;
   }
   *error = EppFilterError("event must be numeric code or name string", fieldPath, nullptr);
   return false;
}

/**
 * Resolve a NetXMS object reference (numeric ID or string name) into an object ID.
 */
static bool EppResolveObjectId(json_t *value, uint32_t *out, const char *fieldPath, std::string *error)
{
   if (json_is_integer(value))
   {
      *out = static_cast<uint32_t>(json_integer_value(value));
      return true;
   }
   if (json_is_string(value))
   {
      const char *s = json_string_value(value);
      String name(s, "utf-8");
      shared_ptr<NetObj> object = FindObjectByName(name.cstr(), -1);
      if (object == nullptr)
      {
         *error = EppFilterError("unknown object", fieldPath, s);
         return false;
      }
      *out = object->getId();
      return true;
   }
   *error = EppFilterError("object must be numeric ID or name string", fieldPath, nullptr);
   return false;
}

/**
 * Resolve a server-action reference (numeric ID or string name) into an action ID.
 */
static bool EppResolveActionId(json_t *value, uint32_t *out, const char *fieldPath, std::string *error)
{
   if (json_is_integer(value))
   {
      *out = static_cast<uint32_t>(json_integer_value(value));
      return true;
   }
   if (json_is_string(value))
   {
      const char *s = json_string_value(value);
      // Resolve by name: iterate the full action list (small N in practice).
      json_t *actions = GetActions();
      uint32_t found = 0;
      size_t i;
      json_t *a;
      json_array_foreach(actions, i, a)
      {
         const char *name = json_object_get_string_utf8(a, "name", "");
         if (!stricmp(name, s))
         {
            found = json_object_get_uint32(a, "id", 0);
            break;
         }
      }
      json_decref(actions);
      if (found == 0)
      {
         *error = EppFilterError("unknown server action", fieldPath, s);
         return false;
      }
      *out = found;
      return true;
   }
   *error = EppFilterError("action must be numeric ID or name string", fieldPath, nullptr);
   return false;
}

/**
 * Resolve an alarm category reference (numeric ID or string name) into a category ID.
 */
static bool EppResolveAlarmCategoryId(json_t *value, uint32_t *out, const char *fieldPath, std::string *error)
{
   if (json_is_integer(value))
   {
      *out = static_cast<uint32_t>(json_integer_value(value));
      return true;
   }
   if (json_is_string(value))
   {
      const char *s = json_string_value(value);
      json_t *categories = GetAlarmCategoriesAsJson();
      uint32_t found = 0;
      size_t i;
      json_t *c;
      json_array_foreach(categories, i, c)
      {
         const char *name = json_object_get_string_utf8(c, "name", "");
         if (!stricmp(name, s))
         {
            found = json_object_get_uint32(c, "id", 0);
            break;
         }
      }
      json_decref(categories);
      if (found == 0)
      {
         *error = EppFilterError("unknown alarm category", fieldPath, s);
         return false;
      }
      *out = found;
      return true;
   }
   *error = EppFilterError("alarm category must be numeric ID or name string", fieldPath, nullptr);
   return false;
}

/**
 * Map an alarm-severity string (info/warning/minor/major/critical/normal/
 * 'same as event'/resolve/terminate) to a SEVERITY_* constant.
 */
static bool EppParseAlarmSeverity(const char *s, int *out)
{
   if (s == nullptr)
      return false;
   if (!stricmp(s, "normal")) { *out = SEVERITY_NORMAL; return true; }
   if (!stricmp(s, "info") || !stricmp(s, "informational")) { *out = SEVERITY_NORMAL; return true; }
   if (!stricmp(s, "warning")) { *out = SEVERITY_WARNING; return true; }
   if (!stricmp(s, "minor")) { *out = SEVERITY_MINOR; return true; }
   if (!stricmp(s, "major")) { *out = SEVERITY_MAJOR; return true; }
   if (!stricmp(s, "critical")) { *out = SEVERITY_CRITICAL; return true; }
   if (!stricmp(s, "same as event") || !stricmp(s, "from_event")) { *out = SEVERITY_FROM_EVENT; return true; }
   if (!stricmp(s, "resolve")) { *out = SEVERITY_RESOLVE; return true; }
   if (!stricmp(s, "terminate")) { *out = SEVERITY_TERMINATE; return true; }
   return false;
}

/**
 * Convert a match_severities array to the RF_SEVERITY_* bitmask.
 * Empty array maps to all five bits set (matches every severity).
 */
static bool EppSeveritiesToFlags(json_t *array, uint32_t *flags, std::string *error)
{
   *flags = 0;
   if (json_array_size(array) == 0)
   {
      *flags = RF_SEVERITY_INFO | RF_SEVERITY_WARNING | RF_SEVERITY_MINOR | RF_SEVERITY_MAJOR | RF_SEVERITY_CRITICAL;
      return true;
   }
   size_t i;
   json_t *v;
   char fieldPath[32];
   json_array_foreach(array, i, v)
   {
      snprintf(fieldPath, sizeof(fieldPath), "match_severities[%zu]", i);
      if (!json_is_string(v))
      {
         *error = EppFilterError("severity must be string", fieldPath, nullptr);
         return false;
      }
      const char *s = json_string_value(v);
      if (!stricmp(s, "info") || !stricmp(s, "informational") || !stricmp(s, "normal"))
         *flags |= RF_SEVERITY_INFO;
      else if (!stricmp(s, "warning"))
         *flags |= RF_SEVERITY_WARNING;
      else if (!stricmp(s, "minor"))
         *flags |= RF_SEVERITY_MINOR;
      else if (!stricmp(s, "major"))
         *flags |= RF_SEVERITY_MAJOR;
      else if (!stricmp(s, "critical"))
         *flags |= RF_SEVERITY_CRITICAL;
      else
      {
         *error = EppFilterError("unknown severity value", fieldPath, s);
         return false;
      }
   }
   return true;
}

/**
 * Map an incident AI analysis depth string (quick/standard/thorough) to integer.
 */
static bool EppParseIncidentDepth(const char *s, int *out)
{
   if (s == nullptr)
      return false;
   if (!stricmp(s, "quick"))    { *out = 0; return true; }
   if (!stricmp(s, "standard")) { *out = 1; return true; }
   if (!stricmp(s, "thorough")) { *out = 2; return true; }
   return false;
}

/**
 * Parse a server-action type name into the ServerActionType integer code.
 */
static bool EppParseServerActionType(const char *s, int *out)
{
   if (s == nullptr)
      return false;
   if (!stricmp(s, "local_command"))   { *out = static_cast<int>(ServerActionType::LOCAL_COMMAND); return true; }
   if (!stricmp(s, "agent_command"))   { *out = static_cast<int>(ServerActionType::AGENT_COMMAND); return true; }
   if (!stricmp(s, "ssh_command"))     { *out = static_cast<int>(ServerActionType::SSH_COMMAND); return true; }
   if (!stricmp(s, "notification"))    { *out = static_cast<int>(ServerActionType::NOTIFICATION); return true; }
   if (!stricmp(s, "forward_event"))   { *out = static_cast<int>(ServerActionType::FORWARD_EVENT); return true; }
   if (!stricmp(s, "nxsl_script"))     { *out = static_cast<int>(ServerActionType::NXSL_SCRIPT); return true; }
   return false;
}

/**
 * Reverse of EppParseServerActionType.
 */
static const char *EppServerActionTypeName(int t)
{
   switch(static_cast<ServerActionType>(t))
   {
      case ServerActionType::LOCAL_COMMAND:  return "local_command";
      case ServerActionType::AGENT_COMMAND:  return "agent_command";
      case ServerActionType::SSH_COMMAND:    return "ssh_command";
      case ServerActionType::NOTIFICATION:   return "notification";
      case ServerActionType::FORWARD_EVENT:  return "forward_event";
      case ServerActionType::NXSL_SCRIPT:    return "nxsl_script";
      default:                               return "unknown";
   }
}

/**
 * Reads a snapshot of the current event processing policy. *version receives
 * the policy version. The returned JSON's "rules" array carries the rules in
 * export format suitable for round-tripping through EPRule(json_t*, ImportContext*).
 * Caller owns the returned json_t.
 */
static json_t *EppReadPolicySnapshot(uint32_t *version)
{
   json_t *snapshot = GetEventProcessingPolicy()->toJson();
   if (snapshot != nullptr)
      *version = json_object_get_uint32(snapshot, "version", 0);
   return snapshot;
}

/**
 * Parse a JSON array of rules (export format) into a SharedObjectArray<EPRule>.
 * Returns nullptr with *error populated on parse failure.
 */
static SharedObjectArray<EPRule> *EppParseRulesArray(json_t *rulesArray, std::string *error)
{
   if (!json_is_array(rulesArray))
   {
      *error = EppFilterError("rules payload is not a JSON array", "rules", nullptr);
      return nullptr;
   }
   auto *result = new SharedObjectArray<EPRule>();
   ImportContext importContext;
   size_t i;
   json_t *r;
   json_array_foreach(rulesArray, i, r)
   {
      if (!json_is_object(r))
      {
         delete result;
         *error = EppFilterError("rule entry is not a JSON object", "rules[]", nullptr);
         return nullptr;
      }
      result->add(make_shared<EPRule>(r, &importContext));
   }
   return result;
}

/**
 * Find a rule in a rules JSON array by GUID. Returns the array index or -1.
 */
static int EppFindRuleIndexByGuid(json_t *rulesArray, const uuid& guid)
{
   size_t i;
   json_t *r;
   json_array_foreach(rulesArray, i, r)
   {
      const char *g = json_object_get_string_utf8(r, "guid", "");
      if (uuid::parseA(g).equals(guid))
         return static_cast<int>(i);
   }
   return -1;
}

/**
 * Save policy with optimistic concurrency check. Returns RCC code; on success,
 * *newVersion receives the new policy version.
 */
static uint32_t EppSavePolicy(uint32_t userId, const SharedObjectArray<EPRule>& rules, uint32_t expectedVersion, uint32_t *newVersion)
{
   uuid userGuid = GetUserGuidById(userId);
   wchar_t userName[MAX_USER_NAME];
   ResolveUserId(userId, userName, true);
   return GetEventProcessingPolicy()->replaceAllRules(rules, true, expectedVersion, userGuid, userName, newVersion);
}

/**
 * Build a structured "concurrent modification" error JSON, used when two retry
 * attempts both lost the race to a concurrent policy update.
 */
static std::string EppConflictError(uint32_t currentVersion)
{
   json_t *err = json_object();
   json_object_set_new(err, "error", json_string("concurrent_modification"));
   json_object_set_new(err, "current_version", json_integer(currentVersion));
   json_object_set_new(err, "retry_advice", json_string("Re-read the policy with get-event-processing-policy and try again."));
   return JsonToString(err);
}

/**
 * Orchestrate the read-modify-write cycle for EPP rule mutations with single
 * retry-on-conflict and audit-log emission.
 *
 * `mutate` is invoked with the current rules JSON array (export shape) and
 * the policy version. It must update the array in place to express the desired
 * new state and return RCC_SUCCESS to proceed with the save. To abort (e.g.
 * rule not found, validation failure), it must set *errorOut and return any
 * other RCC. On RCC_EPP_CONFLICT from the save, the wrapper re-reads the
 * policy and re-invokes `mutate` once before giving up.
 *
 * `auditDetail` is written by the mutator (e.g. "rule {abc} - fields: x, y") and
 * appended to the audit log line by the wrapper.
 *
 * Returns the AI tool's response string (success JSON or error JSON).
 */
static std::string EppReadModifyWrite(
   uint32_t userId, const char *auditAction,
   std::function<uint32_t(json_t *rulesArray, uint32_t version, std::string *errorOut, wchar_t *auditDetail, size_t auditDetailSize)> mutate)
{
   wchar_t auditDetail[512] = L"";
   json_t *snapshot = nullptr;
   for (int attempt = 0; attempt < 2; attempt++)
   {
      uint32_t version = 0;
      snapshot = EppReadPolicySnapshot(&version);
      if (snapshot == nullptr)
         return std::string("Failed to read event processing policy");
      json_t *rulesArray = json_object_get(snapshot, "rules");

      std::string err;
      uint32_t mutateRcc = mutate(rulesArray, version, &err, auditDetail, sizeof(auditDetail) / sizeof(auditDetail[0]));
      if (mutateRcc != RCC_SUCCESS)
      {
         json_decref(snapshot);
         return err;
      }

      std::string parseError;
      SharedObjectArray<EPRule> *rules = EppParseRulesArray(rulesArray, &parseError);
      if (rules == nullptr)
      {
         json_decref(snapshot);
         return parseError;
      }

      uint32_t newVersion = 0;
      uint32_t saveRcc = EppSavePolicy(userId, *rules, version, &newVersion);
      delete rules;

      if (saveRcc == RCC_SUCCESS)
      {
         WriteAuditLog(AUDIT_SYSCFG, true, userId, nullptr, 0, 0, L"AI agent %hs: %s (policy version %u)",
            auditAction, auditDetail, newVersion);
         json_t *response = json_object();
         json_object_set_new(response, "status", json_string("ok"));
         json_object_set_new(response, "operation", json_string(auditAction));
         json_object_set_new(response, "policy_version", json_integer(newVersion));
         // Caller may want to attach the affected rule's new JSON; we leave that
         // to per-tool functions which can re-call this helper and merge in.
         std::string result = JsonToString(response);
         json_decref(snapshot);
         return result;
      }

      if (saveRcc == RCC_EPP_CONFLICT && attempt == 0)
      {
         // Lost the race; refresh and retry once.
         json_decref(snapshot);
         continue;
      }

      json_decref(snapshot);
      if (saveRcc == RCC_EPP_CONFLICT)
         return EppConflictError(newVersion);
      return std::string("Database error during event processing policy save");
   }
   return std::string("Unexpected control flow in EPP retry loop");
}

/**
 * Helper: set or clear a flag bit on the rule's flags field based on a boolean
 * argument. The argument is left untouched if absent from `arguments`. Uses
 * the lenient json_object_get_boolean parser so LLM-stringified booleans
 * ("true"/"false" as JSON strings, 0/1 as integers) are accepted.
 */
static void EppPatchFlagBit(json_t *rule, json_t *arguments, const char *argName, uint32_t bit)
{
   if (json_object_get(arguments, argName) == nullptr)
      return;
   uint32_t flags = json_object_get_uint32(rule, "flags", 0);
   if (json_object_get_boolean(arguments, argName, false))
      flags |= bit;
   else
      flags &= ~bit;
   json_object_set_new(rule, "flags", json_integer(flags));
}

/**
 * Helper: resolve an array of id-or-name entries to an integer array.
 * Used for events/sources/source_exclusions/alarm_categories. Tolerates
 * stringified-JSON and bare-scalar inputs via json_object_get_array_ex.
 */
static bool EppPatchResolvedArray(json_t *rule, json_t *arguments, const char *argName,
   const char *outFieldName,
   bool (*resolver)(json_t *value, uint32_t *out, const char *fieldPath, std::string *error),
   std::string *error)
{
   if (json_object_get(arguments, argName) == nullptr)
      return true; // not provided - leave rule field unchanged
   json_t *src = json_object_get_array_ex(arguments, argName);
   if (src == nullptr)
   {
      *error = EppFilterError("expected array (or array literal as JSON string)", argName, nullptr);
      return false;
   }
   json_t *dest = json_array();
   size_t i;
   json_t *v;
   char fieldPath[64];
   json_array_foreach(src, i, v)
   {
      snprintf(fieldPath, sizeof(fieldPath), "%s[%zu]", argName, i);
      uint32_t resolved = 0;
      if (!resolver(v, &resolved, fieldPath, error))
      {
         json_decref(dest);
         json_decref(src);
         return false;
      }
      json_array_append_new(dest, json_integer(resolved));
   }
   json_decref(src);
   json_object_set_new(rule, outFieldName, dest);
   return true;
}

/**
 * Helper: apply the `actions` array patch. Each element is an object whose
 * `id` may be an integer or an action name; other fields (timerDelay,
 * timerKey, etc.) pass through as strings. Tolerates stringified-JSON forms
 * via json_object_get_array_ex.
 */
static bool EppPatchActionsArray(json_t *rule, json_t *arguments, std::string *error)
{
   if (json_object_get(arguments, "actions") == nullptr)
      return true;
   json_t *src = json_object_get_array_ex(arguments, "actions");
   if (src == nullptr)
   {
      *error = EppFilterError("expected array (or array literal as JSON string)", "actions", nullptr);
      return false;
   }
   json_t *dest = json_array();
   size_t i;
   json_t *a;
   char fieldPath[64];
   json_array_foreach(src, i, a)
   {
      if (!json_is_object(a))
      {
         json_decref(dest);
         json_decref(src);
         *error = EppFilterError("action entry must be an object", "actions[]", nullptr);
         return false;
      }
      snprintf(fieldPath, sizeof(fieldPath), "actions[%zu].id", i);
      uint32_t actionId = 0;
      json_t *idVal = json_object_get(a, "id");
      if (idVal == nullptr)
      {
         json_decref(dest);
         json_decref(src);
         *error = EppFilterError("missing action id", fieldPath, nullptr);
         return false;
      }
      if (!EppResolveActionId(idVal, &actionId, fieldPath, error))
      {
         json_decref(dest);
         json_decref(src);
         return false;
      }
      json_t *out = json_object();
      json_object_set_new(out, "id", json_integer(actionId));
      json_t *v;
      v = json_object_get(a, "timer_delay");
      if (v == nullptr) v = json_object_get(a, "timerDelay");
      if (json_is_string(v)) json_object_set(out, "timerDelay", v);
      v = json_object_get(a, "timer_key");
      if (v == nullptr) v = json_object_get(a, "timerKey");
      if (json_is_string(v)) json_object_set(out, "timerKey", v);
      v = json_object_get(a, "blocking_timer_key");
      if (v == nullptr) v = json_object_get(a, "blockingTimerKey");
      if (json_is_string(v)) json_object_set(out, "blockingTimerKey", v);
      v = json_object_get(a, "snooze_time");
      if (v == nullptr) v = json_object_get(a, "snoozeTime");
      if (json_is_string(v)) json_object_set(out, "snoozeTime", v);
      v = json_object_get(a, "active");
      if (json_is_boolean(v)) json_object_set(out, "active", v);
      json_array_append_new(dest, out);
   }
   json_decref(src);
   json_object_set_new(rule, "actions", dest);
   return true;
}

/**
 * Parse the rule GUID from arguments. Returns true on success, false with
 * *error populated on validation failure.
 */
static bool EppParseRuleGuidArg(json_t *arguments, const char *fieldName, uuid *out, std::string *error)
{
   const char *guidStr = json_object_get_string_utf8(arguments, fieldName, nullptr);
   if ((guidStr == nullptr) || (guidStr[0] == 0))
   {
      *error = EppFilterError("rule GUID is required", fieldName, nullptr);
      return false;
   }
   *out = uuid::parseA(guidStr);
   if (out->isNull())
   {
      *error = EppFilterError("invalid GUID format", fieldName, guidStr);
      return false;
   }
   return true;
}

/**
 * Determine the target index for an insert/move position relative to the rules
 * array, honoring after_guid / before_guid / position arguments. Sets
 * *targetIndex to the array position where the rule should be placed (i.e. the
 * index that the moved/inserted rule will occupy after the operation completes).
 *
 * Returns RCC_SUCCESS on success. If no position argument is supplied,
 * *targetIndex is set to the supplied default (e.g. end of array for append).
 */
static uint32_t EppResolvePosition(json_t *arguments, json_t *rulesArray, int defaultIndex,
   int *targetIndex, std::string *error)
{
   const char *afterGuid = json_object_get_string_utf8(arguments, "after_guid", nullptr);
   const char *beforeGuid = json_object_get_string_utf8(arguments, "before_guid", nullptr);
   const char *position = json_object_get_string_utf8(arguments, "position", nullptr);

   int anchorsProvided = ((afterGuid != nullptr) && afterGuid[0] != 0) +
                         ((beforeGuid != nullptr) && beforeGuid[0] != 0) +
                         ((position != nullptr) && position[0] != 0);
   if (anchorsProvided > 1)
   {
      *error = EppFilterError("specify only one of after_guid, before_guid, position", "position", nullptr);
      return RCC_INVALID_ARGUMENT;
   }

   if (anchorsProvided == 0)
   {
      *targetIndex = defaultIndex;
      return RCC_SUCCESS;
   }

   if ((afterGuid != nullptr) && afterGuid[0] != 0)
   {
      uuid anchor = uuid::parseA(afterGuid);
      if (anchor.isNull())
      {
         *error = EppFilterError("invalid GUID format", "after_guid", afterGuid);
         return RCC_INVALID_ARGUMENT;
      }
      int idx = EppFindRuleIndexByGuid(rulesArray, anchor);
      if (idx < 0)
      {
         *error = EppFilterError("anchor rule not found", "after_guid", afterGuid);
         return RCC_INVALID_ARGUMENT;
      }
      *targetIndex = idx + 1;
      return RCC_SUCCESS;
   }

   if ((beforeGuid != nullptr) && beforeGuid[0] != 0)
   {
      uuid anchor = uuid::parseA(beforeGuid);
      if (anchor.isNull())
      {
         *error = EppFilterError("invalid GUID format", "before_guid", beforeGuid);
         return RCC_INVALID_ARGUMENT;
      }
      int idx = EppFindRuleIndexByGuid(rulesArray, anchor);
      if (idx < 0)
      {
         *error = EppFilterError("anchor rule not found", "before_guid", beforeGuid);
         return RCC_INVALID_ARGUMENT;
      }
      *targetIndex = idx;
      return RCC_SUCCESS;
   }

   if (!stricmp(position, "first"))
   {
      *targetIndex = 0;
      return RCC_SUCCESS;
   }
   if (!stricmp(position, "last"))
   {
      *targetIndex = static_cast<int>(json_array_size(rulesArray));
      return RCC_SUCCESS;
   }
   *error = EppFilterError("position must be 'first' or 'last'", "position", position);
   return RCC_INVALID_ARGUMENT;
}

/**
 * Apply an AI-shape patch to an export-format rule JSON. The patch is the
 * arguments dictionary received by F_CreateEppRule or F_ModifyEppRule; only
 * fields actually present in `arguments` produce updates. Returns false with
 * *error populated on a validation failure.
 */
static bool EppApplyAiPatchToRule(json_t *rule, json_t *arguments, std::string *error)
{
   json_t *v;

   // Flat string fields - direct passthrough
   v = json_object_get(arguments, "comments");
   if (json_is_string(v)) json_object_set(rule, "comments", v);

   v = json_object_get(arguments, "filter_script");
   if (json_is_string(v)) json_object_set(rule, "filterScript", v);

   v = json_object_get(arguments, "action_script");
   if (json_is_string(v)) json_object_set(rule, "actionScript", v);

   v = json_object_get(arguments, "alarm_message");
   if (json_is_string(v)) json_object_set(rule, "alarmMessage", v);

   v = json_object_get(arguments, "alarm_impact");
   if (json_is_string(v)) json_object_set(rule, "alarmImpact", v);

   v = json_object_get(arguments, "alarm_key");
   if (json_is_string(v)) json_object_set(rule, "alarmKey", v);

   v = json_object_get(arguments, "incident_title");
   if (json_is_string(v)) json_object_set(rule, "incidentTitle", v);

   v = json_object_get(arguments, "incident_description");
   if (json_is_string(v)) json_object_set(rule, "incidentDescription", v);

   v = json_object_get(arguments, "incident_ai_prompt");
   if (json_is_string(v)) json_object_set(rule, "incidentAIPrompt", v);

   v = json_object_get(arguments, "downtime_tag");
   if (json_is_string(v)) json_object_set(rule, "downtimeTag", v);

   v = json_object_get(arguments, "rca_script_name");
   if (json_is_string(v)) json_object_set(rule, "rootCauseAnalysisScript", v);

   v = json_object_get(arguments, "ai_agent_instructions");
   if (json_is_string(v)) json_object_set(rule, "aiAgentInstructions", v);

   // Flat integer fields
   v = json_object_get(arguments, "alarm_timeout");
   if (json_is_integer(v)) json_object_set(rule, "alarmTimeout", v);

   v = json_object_get(arguments, "incident_delay");
   if (json_is_integer(v)) json_object_set(rule, "incidentDelay", v);

   // alarm_timeout_event: event code or name
   v = json_object_get(arguments, "alarm_timeout_event");
   if (v != nullptr)
   {
      uint32_t code = 0;
      if (!EppResolveEventCode(v, &code, "alarm_timeout_event", error))
         return false;
      json_object_set_new(rule, "alarmTimeoutEvent", json_integer(code));
   }

   // alarm_severity (string name)
   v = json_object_get(arguments, "alarm_severity");
   if (json_is_string(v))
   {
      int sev;
      if (!EppParseAlarmSeverity(json_string_value(v), &sev))
      {
         *error = EppFilterError("unknown alarm severity", "alarm_severity", json_string_value(v));
         return false;
      }
      json_object_set_new(rule, "alarmSeverity", json_integer(sev));
   }

   // incident_ai_analysis_depth (string name)
   v = json_object_get(arguments, "incident_ai_analysis_depth");
   if (json_is_string(v))
   {
      int depth;
      if (!EppParseIncidentDepth(json_string_value(v), &depth))
      {
         *error = EppFilterError("unknown incident AI analysis depth", "incident_ai_analysis_depth", json_string_value(v));
         return false;
      }
      json_object_set_new(rule, "incidentAIAnalysisDepth", json_integer(depth));
   }

   // Severity match list -> RF_SEVERITY_* bits
   if (json_object_get(arguments, "match_severities") != nullptr)
   {
      json_t *sev = json_object_get_array_ex(arguments, "match_severities");
      if (sev == nullptr)
      {
         *error = EppFilterError("expected array (or array literal as JSON string)", "match_severities", nullptr);
         return false;
      }
      uint32_t sevFlags = 0;
      bool ok = EppSeveritiesToFlags(sev, &sevFlags, error);
      json_decref(sev);
      if (!ok)
         return false;
      uint32_t flags = json_object_get_uint32(rule, "flags", 0);
      flags = (flags & ~(uint32_t)(RF_SEVERITY_INFO | RF_SEVERITY_WARNING | RF_SEVERITY_MINOR | RF_SEVERITY_MAJOR | RF_SEVERITY_CRITICAL)) | sevFlags;
      json_object_set_new(rule, "flags", json_integer(flags));
   }

   // Boolean flag patches (order matters only in that they all operate on `flags`)
   EppPatchFlagBit(rule, arguments, "stop_processing",      RF_STOP_PROCESSING);
   EppPatchFlagBit(rule, arguments, "generate_alarm",       RF_GENERATE_ALARM);
   EppPatchFlagBit(rule, arguments, "create_ticket",        RF_CREATE_TICKET);
   EppPatchFlagBit(rule, arguments, "terminate_by_regexp",  RF_TERMINATE_BY_REGEXP);
   EppPatchFlagBit(rule, arguments, "start_downtime",       RF_START_DOWNTIME);
   EppPatchFlagBit(rule, arguments, "end_downtime",         RF_END_DOWNTIME);
   EppPatchFlagBit(rule, arguments, "request_ai_comment",   RF_REQUEST_AI_COMMENT);
   EppPatchFlagBit(rule, arguments, "create_incident",      RF_CREATE_INCIDENT);
   EppPatchFlagBit(rule, arguments, "ai_analyze_incident",  RF_AI_ANALYZE_INCIDENT);
   EppPatchFlagBit(rule, arguments, "negated_event_match",  RF_NEGATED_EVENTS);
   EppPatchFlagBit(rule, arguments, "negated_source_match", RF_NEGATED_SOURCE);

   // Resolved arrays
   if (!EppPatchResolvedArray(rule, arguments, "events", "events", EppResolveEventCode, error))
      return false;
   if (!EppPatchResolvedArray(rule, arguments, "sources", "sources", EppResolveObjectId, error))
      return false;
   if (!EppPatchResolvedArray(rule, arguments, "source_exclusions", "sourceExclusions", EppResolveObjectId, error))
      return false;
   if (!EppPatchResolvedArray(rule, arguments, "alarm_categories", "alarmCategories", EppResolveAlarmCategoryId, error))
      return false;

   // Array passthroughs - all tolerate stringified-JSON and bare-scalar forms.
   struct ArrayPassthrough
   {
      const char *argName;
      const char *outFieldName;
   };
   static const ArrayPassthrough arrayPassthroughs[] = {
      { "time_frames",             "timeFrames" },
      { "timer_cancellations",     "timerCancellations" },
      { "pstorage_delete",         "pstorageDeleteActions" },
      { "custom_attribute_delete", "customAttributeDeleteActions" }
   };
   for(const ArrayPassthrough &p : arrayPassthroughs)
   {
      if (json_object_get(arguments, p.argName) == nullptr)
         continue;
      json_t *arr = json_object_get_array_ex(arguments, p.argName);
      if (arr == nullptr)
      {
         *error = EppFilterError("expected array (or array literal as JSON string)", p.argName, nullptr);
         return false;
      }
      json_object_set_new(rule, p.outFieldName, arr);
   }

   // Actions list (per-entry id resolution)
   if (!EppPatchActionsArray(rule, arguments, error))
      return false;

   // Object-map passthroughs
   struct ObjectPassthrough
   {
      const char *argName;
      const char *outFieldName;
   };
   static const ObjectPassthrough objectPassthroughs[] = {
      { "pstorage_set",         "pstorageSetActions" },
      { "custom_attribute_set", "customAttributeSetActions" }
   };
   for(const ObjectPassthrough &p : objectPassthroughs)
   {
      if (json_object_get(arguments, p.argName) == nullptr)
         continue;
      json_t *obj = json_object_get_object_ex(arguments, p.argName);
      if (obj == nullptr)
      {
         *error = EppFilterError("expected object map (or object literal as JSON string)", p.argName, nullptr);
         return false;
      }
      json_object_set_new(rule, p.outFieldName, obj);
   }

   return true;
}

/**
 * Build a fresh export-format rule JSON with defaults suitable as a starting
 * point for AI-driven create. The caller layers patches on top.
 * - flags: all five severity bits set (rule matches every severity by default)
 * - alarmSeverity: SEVERITY_FROM_EVENT (mirrors UI default)
 * - guid: freshly generated
 */
static json_t *EppBuildDefaultRuleJson()
{
   json_t *rule = json_object();
   uuid g = uuid::generate();
   json_object_set_new(rule, "guid", g.toJson());
   uint32_t defaultFlags = RF_SEVERITY_INFO | RF_SEVERITY_WARNING | RF_SEVERITY_MINOR | RF_SEVERITY_MAJOR | RF_SEVERITY_CRITICAL;
   json_object_set_new(rule, "flags", json_integer(defaultFlags));
   json_object_set_new(rule, "alarmSeverity", json_integer(SEVERITY_FROM_EVENT));
   json_object_set_new(rule, "alarmTimeoutEvent", json_integer(0));
   json_object_set_new(rule, "events", json_array());
   json_object_set_new(rule, "sources", json_array());
   json_object_set_new(rule, "sourceExclusions", json_array());
   json_object_set_new(rule, "alarmCategories", json_array());
   json_object_set_new(rule, "timeFrames", json_array());
   json_object_set_new(rule, "actions", json_array());
   json_object_set_new(rule, "timerCancellations", json_array());
   json_object_set_new(rule, "pstorageSetActions", json_object());
   json_object_set_new(rule, "pstorageDeleteActions", json_array());
   json_object_set_new(rule, "customAttributeSetActions", json_object());
   json_object_set_new(rule, "customAttributeDeleteActions", json_array());
   return rule;
}

/**
 * Create a new event processing policy rule
 */
std::string F_CreateEppRule(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EPP) == 0)
      return std::string("User does not have rights to modify event processing policy");

   // Pre-validate by building the rule JSON eagerly against an empty policy.
   // Re-validation happens inside the mutator on each retry, but doing it once
   // here lets us reject bad inputs without burning an EPP snapshot read.
   json_t *prepRule = EppBuildDefaultRuleJson();
   std::string prepErr;
   if (!EppApplyAiPatchToRule(prepRule, arguments, &prepErr))
   {
      json_decref(prepRule);
      return prepErr;
   }
   // Extract the generated GUID for audit logging and for the mutator's lookup.
   uuid newGuid = uuid::parseA(json_object_get_string_utf8(prepRule, "guid", ""));
   json_decref(prepRule);

   return EppReadModifyWrite(userId, "create-epp-rule",
      [arguments, newGuid](json_t *rulesArray, uint32_t version, std::string *errorOut,
                           wchar_t *auditDetail, size_t auditDetailSize) -> uint32_t
      {
         // Resolve target position against the current rules array.
         int targetIdx = static_cast<int>(json_array_size(rulesArray));
         std::string posErr;
         uint32_t posRcc = EppResolvePosition(arguments, rulesArray, targetIdx, &targetIdx, &posErr);
         if (posRcc != RCC_SUCCESS)
         {
            *errorOut = posErr;
            return posRcc;
         }

         // Build the new rule. We rebuild it inside the mutator (instead of
         // capturing the pre-built one) so retry-on-conflict re-validates
         // anchor GUIDs and id-or-name lookups against the fresh snapshot.
         json_t *rule = EppBuildDefaultRuleJson();
         json_object_set_new(rule, "guid", newGuid.toJson()); // keep stable GUID across retries
         std::string err;
         if (!EppApplyAiPatchToRule(rule, arguments, &err))
         {
            json_decref(rule);
            *errorOut = err;
            return RCC_INVALID_ARGUMENT;
         }

         if (targetIdx >= static_cast<int>(json_array_size(rulesArray)))
            json_array_append_new(rulesArray, rule);
         else
            json_array_insert_new(rulesArray, targetIdx, rule);

         nx_swprintf(auditDetail, auditDetailSize, L"created rule %s at index %d",
            newGuid.toString().cstr(), targetIdx);
         return RCC_SUCCESS;
      });
}

/**
 * Modify an existing event processing policy rule
 */
std::string F_ModifyEppRule(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EPP) == 0)
      return std::string("User does not have rights to modify event processing policy");

   uuid ruleGuid;
   std::string err;
   if (!EppParseRuleGuidArg(arguments, "guid", &ruleGuid, &err))
      return err;

   return EppReadModifyWrite(userId, "modify-epp-rule",
      [ruleGuid, arguments](json_t *rulesArray, uint32_t version, std::string *errorOut,
                            wchar_t *auditDetail, size_t auditDetailSize) -> uint32_t
      {
         int idx = EppFindRuleIndexByGuid(rulesArray, ruleGuid);
         if (idx < 0)
         {
            json_t *errJson = json_object();
            json_object_set_new(errJson, "error", json_string("rule not found"));
            json_object_set_new(errJson, "guid", ruleGuid.toJson());
            *errorOut = JsonToString(errJson);
            return RCC_INVALID_ARGUMENT;
         }
         json_t *rule = json_array_get(rulesArray, idx);
         if (!EppApplyAiPatchToRule(rule, arguments, errorOut))
            return RCC_INVALID_ARGUMENT;
         nx_swprintf(auditDetail, auditDetailSize, L"modified rule %s", ruleGuid.toString().cstr());
         return RCC_SUCCESS;
      });
}

/**
 * Delete an event processing policy rule
 */
std::string F_DeleteEppRule(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EPP) == 0)
      return std::string("User does not have rights to modify event processing policy");

   uuid ruleGuid;
   std::string err;
   if (!EppParseRuleGuidArg(arguments, "guid", &ruleGuid, &err))
      return err;

   return EppReadModifyWrite(userId, "delete-epp-rule",
      [ruleGuid](json_t *rulesArray, uint32_t version, std::string *errorOut,
                 wchar_t *auditDetail, size_t auditDetailSize) -> uint32_t
      {
         int idx = EppFindRuleIndexByGuid(rulesArray, ruleGuid);
         if (idx < 0)
         {
            json_t *errJson = json_object();
            json_object_set_new(errJson, "error", json_string("rule not found"));
            json_object_set_new(errJson, "guid", ruleGuid.toJson());
            *errorOut = JsonToString(errJson);
            return RCC_INVALID_ARGUMENT;
         }
         json_array_remove(rulesArray, idx);
         nx_swprintf(auditDetail, auditDetailSize, L"deleted rule %s", ruleGuid.toString().cstr());
         return RCC_SUCCESS;
      });
}

/**
 * Set or clear RF_DISABLED on a rule by GUID. Shared core of enable/disable.
 */
static std::string EppToggleRuleDisabled(json_t *arguments, uint32_t userId, bool disable)
{
   uuid ruleGuid;
   std::string err;
   if (!EppParseRuleGuidArg(arguments, "guid", &ruleGuid, &err))
      return err;

   const char *opName = disable ? "disable-epp-rule" : "enable-epp-rule";
   return EppReadModifyWrite(userId, opName,
      [ruleGuid, disable, opName](json_t *rulesArray, uint32_t version, std::string *errorOut,
                                  wchar_t *auditDetail, size_t auditDetailSize) -> uint32_t
      {
         int idx = EppFindRuleIndexByGuid(rulesArray, ruleGuid);
         if (idx < 0)
         {
            json_t *errJson = json_object();
            json_object_set_new(errJson, "error", json_string("rule not found"));
            json_object_set_new(errJson, "guid", ruleGuid.toJson());
            *errorOut = JsonToString(errJson);
            return RCC_INVALID_ARGUMENT;
         }
         json_t *rule = json_array_get(rulesArray, idx);
         uint32_t flags = json_object_get_uint32(rule, "flags", 0);
         uint32_t newFlags = disable ? (flags | RF_DISABLED) : (flags & ~static_cast<uint32_t>(RF_DISABLED));
         if (newFlags == flags)
         {
            // Idempotent - no-op. Still proceed with save (cheap) so the audit
            // log records the request, mirroring NX UI behavior.
         }
         json_object_set_new(rule, "flags", json_integer(newFlags));
         nx_swprintf(auditDetail, auditDetailSize, L"%hs rule %s", disable ? "disabled" : "enabled",
            ruleGuid.toString().cstr());
         return RCC_SUCCESS;
      });
}

/**
 * Enable a disabled event processing policy rule
 */
std::string F_EnableEppRule(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EPP) == 0)
      return std::string("User does not have rights to modify event processing policy");
   return EppToggleRuleDisabled(arguments, userId, false);
}

/**
 * Disable an event processing policy rule
 */
std::string F_DisableEppRule(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EPP) == 0)
      return std::string("User does not have rights to modify event processing policy");
   return EppToggleRuleDisabled(arguments, userId, true);
}

/**
 * Move (reorder) an event processing policy rule
 */
std::string F_MoveEppRule(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_EPP) == 0)
      return std::string("User does not have rights to modify event processing policy");

   uuid ruleGuid;
   std::string err;
   if (!EppParseRuleGuidArg(arguments, "guid", &ruleGuid, &err))
      return err;

   // Require an explicit position argument; "move" with no destination is a no-op
   // that almost certainly indicates the model forgot to set the destination.
   const char *afterGuid = json_object_get_string_utf8(arguments, "after_guid", nullptr);
   const char *beforeGuid = json_object_get_string_utf8(arguments, "before_guid", nullptr);
   const char *positionStr = json_object_get_string_utf8(arguments, "position", nullptr);
   if (((afterGuid == nullptr) || afterGuid[0] == 0) &&
       ((beforeGuid == nullptr) || beforeGuid[0] == 0) &&
       ((positionStr == nullptr) || positionStr[0] == 0))
      return EppFilterError("specify one of after_guid, before_guid, position", "position", nullptr);

   return EppReadModifyWrite(userId, "move-epp-rule",
      [ruleGuid, arguments](json_t *rulesArray, uint32_t version, std::string *errorOut,
                            wchar_t *auditDetail, size_t auditDetailSize) -> uint32_t
      {
         int currentIdx = EppFindRuleIndexByGuid(rulesArray, ruleGuid);
         if (currentIdx < 0)
         {
            json_t *errJson = json_object();
            json_object_set_new(errJson, "error", json_string("rule not found"));
            json_object_set_new(errJson, "guid", ruleGuid.toJson());
            *errorOut = JsonToString(errJson);
            return RCC_INVALID_ARGUMENT;
         }

         // Resolve the requested target position against the current array
         // (with the rule still in its old slot - we adjust below).
         int targetIdx = currentIdx;
         std::string posErr;
         uint32_t posRcc = EppResolvePosition(arguments, rulesArray, currentIdx, &targetIdx, &posErr);
         if (posRcc != RCC_SUCCESS)
         {
            *errorOut = posErr;
            return posRcc;
         }

         if (targetIdx == currentIdx || targetIdx == currentIdx + 1)
         {
            // No-op: target is the current position. Still proceed to save so
            // the audit log records the request - mirrors UI semantics where
            // an unchanged drag still issues a save.
            nx_swprintf(auditDetail, auditDetailSize, L"moved rule %s (no position change)",
               ruleGuid.toString().cstr());
            return RCC_SUCCESS;
         }

         // Take a ref to the existing rule object so it survives the remove,
         // then re-insert at the corrected index. When removing index < target,
         // the target index shifts left by one.
         json_t *rule = json_array_get(rulesArray, currentIdx);
         json_incref(rule);
         json_array_remove(rulesArray, currentIdx);
         int adjusted = (currentIdx < targetIdx) ? targetIdx - 1 : targetIdx;
         if (adjusted >= static_cast<int>(json_array_size(rulesArray)))
            json_array_append_new(rulesArray, rule);
         else
            json_array_insert_new(rulesArray, adjusted, rule);

         nx_swprintf(auditDetail, auditDetailSize, L"moved rule %s from index %d to %d",
            ruleGuid.toString().cstr(), currentIdx, adjusted);
         return RCC_SUCCESS;
      });
}

/**
 * Translate AI-shape arguments into the WebAPI-shape JSON consumed by
 * ModifyActionFromJson. Fields absent from `arguments` are not set, so the
 * downstream "only update provided fields" semantics are preserved on modify
 * and "use defaults" semantics on create. Returns nullptr + populated *error
 * if the action type string is invalid.
 */
static json_t *EppBuildActionWebApiJson(json_t *arguments, std::string *error)
{
   json_t *output = json_object();

   json_t *v = json_object_get(arguments, "name");
   if (json_is_string(v))
      json_object_set(output, "name", v);

   v = json_object_get(arguments, "type");
   if (json_is_string(v))
   {
      int typeCode;
      if (!EppParseServerActionType(json_string_value(v), &typeCode))
      {
         json_decref(output);
         *error = EppFilterError("unknown action type", "type", json_string_value(v));
         return nullptr;
      }
      json_object_set_new(output, "type", json_integer(typeCode));
   }

   // disabled: lenient parse (accept bool, "true"/"false", 0/1) via
   // json_object_get_boolean. We check presence first to distinguish "model
   // omitted the field" from "model set it to false".
   if (json_object_get(arguments, "disabled") != nullptr)
      json_object_set_new(output, "isDisabled", json_boolean(json_object_get_boolean(arguments, "disabled", false)));

   v = json_object_get(arguments, "recipient");
   if (json_is_string(v))
      json_object_set(output, "recipientAddress", v);

   v = json_object_get(arguments, "email_subject");
   if (json_is_string(v))
      json_object_set(output, "emailSubject", v);

   v = json_object_get(arguments, "data");
   if (json_is_string(v))
      json_object_set(output, "data", v);

   v = json_object_get(arguments, "channel");
   if (json_is_string(v))
      json_object_set(output, "notificationChannelName", v);

   return output;
}

/**
 * Translate a stored Action JSON (from GetActionById) back into the AI-shape
 * keys used by AI assistant responses, so the LLM sees the same vocabulary on
 * input and output.
 */
static json_t *EppActionToAiJson(json_t *webApiAction)
{
   json_t *output = json_object();
   json_t *v = json_object_get(webApiAction, "id");
   if (v != nullptr)
      json_object_set(output, "id", v);
   v = json_object_get(webApiAction, "guid");
   if (v != nullptr)
      json_object_set(output, "guid", v);
   v = json_object_get(webApiAction, "name");
   if (v != nullptr)
      json_object_set(output, "name", v);
   v = json_object_get(webApiAction, "type");
   if (json_is_integer(v))
      json_object_set_new(output, "type", json_string(EppServerActionTypeName(static_cast<int>(json_integer_value(v)))));
   v = json_object_get(webApiAction, "isDisabled");
   if (json_is_boolean(v))
      json_object_set(output, "disabled", v);
   v = json_object_get(webApiAction, "recipientAddress");
   if (json_is_string(v))
      json_object_set(output, "recipient", v);
   v = json_object_get(webApiAction, "emailSubject");
   if (json_is_string(v))
      json_object_set(output, "email_subject", v);
   v = json_object_get(webApiAction, "data");
   if (json_is_string(v))
      json_object_set(output, "data", v);
   v = json_object_get(webApiAction, "notificationChannelName");
   if (json_is_string(v))
      json_object_set(output, "channel", v);
   return output;
}

/**
 * Build the standard success-response envelope for action mutations.
 */
static std::string EppActionSuccessResponse(const char *operation, uint32_t actionId)
{
   json_t *response = json_object();
   json_object_set_new(response, "status", json_string("ok"));
   json_object_set_new(response, "operation", json_string(operation));
   json_object_set_new(response, "id", json_integer(actionId));
   json_t *webApiAction = GetActionById(actionId);
   if (webApiAction != nullptr)
   {
      json_object_set_new(response, "action", EppActionToAiJson(webApiAction));
      json_decref(webApiAction);
   }
   return JsonToString(response);
}

/**
 * Create a new server-side action
 */
std::string F_CreateServerAction(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS) == 0)
      return std::string("User does not have rights to manage server actions");

   const char *nameStr = json_object_get_string_utf8(arguments, "name", nullptr);
   if ((nameStr == nullptr) || (nameStr[0] == 0))
      return EppFilterError("name is required", "name", nullptr);

   std::string err;
   json_t *webApiArgs = EppBuildActionWebApiJson(arguments, &err);
   if (webApiArgs == nullptr)
      return err;

   wchar_t name[MAX_OBJECT_NAME];
   utf8_to_wchar(nameStr, -1, name, MAX_OBJECT_NAME);

   uint32_t actionId = 0;
   uint32_t rcc = CreateAction(name, &actionId);
   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      json_decref(webApiArgs);
      return EppFilterError("action with this name already exists", "name", nameStr);
   }
   if (rcc != RCC_SUCCESS)
   {
      json_decref(webApiArgs);
      return std::string("Database failure while creating server action");
   }

   // Action::Action defaults isDisabled=true. For AI-driven creation that's
   // poor UX - the model usually wants the new action live immediately. Inject
   // a default isDisabled=false unless the caller explicitly set "disabled".
   if (json_object_get(webApiArgs, "isDisabled") == nullptr)
      json_object_set_new(webApiArgs, "isDisabled", json_false());

   // Apply all fields via ModifyActionFromJson - CreateAction only sets the
   // name, so we always run a modify pass to flip isDisabled and set the rest.
   json_t *oldValue = nullptr, *newValue = nullptr;
   ModifyActionFromJson(actionId, webApiArgs, &oldValue, &newValue);
   if (oldValue != nullptr)
      json_decref(oldValue);
   if (newValue != nullptr)
      json_decref(newValue);
   json_decref(webApiArgs);

   WriteAuditLog(AUDIT_SYSCFG, true, userId, nullptr, 0, 0,
      L"AI agent: created server action %u (%ls)", actionId, name);

   return EppActionSuccessResponse("create-server-action", actionId);
}

/**
 * Modify an existing server-side action
 */
std::string F_ModifyServerAction(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS) == 0)
      return std::string("User does not have rights to manage server actions");

   uint32_t actionId = json_object_get_uint32(arguments, "id", 0);
   if (actionId == 0)
      return EppFilterError("id is required", "id", nullptr);

   std::string err;
   json_t *webApiArgs = EppBuildActionWebApiJson(arguments, &err);
   if (webApiArgs == nullptr)
      return err;

   json_t *oldValue = nullptr, *newValue = nullptr;
   uint32_t rcc = ModifyActionFromJson(actionId, webApiArgs, &oldValue, &newValue);
   json_decref(webApiArgs);

   if (rcc == RCC_INVALID_ACTION_ID)
   {
      if (oldValue != nullptr) json_decref(oldValue);
      if (newValue != nullptr) json_decref(newValue);
      json_t *errJson = json_object();
      json_object_set_new(errJson, "error", json_string("action not found"));
      json_object_set_new(errJson, "id", json_integer(actionId));
      return JsonToString(errJson);
   }
   if (rcc == RCC_OBJECT_ALREADY_EXISTS)
   {
      if (oldValue != nullptr) json_decref(oldValue);
      if (newValue != nullptr) json_decref(newValue);
      return EppFilterError("action with this name already exists", "name", nullptr);
   }
   if (rcc != RCC_SUCCESS)
   {
      if (oldValue != nullptr) json_decref(oldValue);
      if (newValue != nullptr) json_decref(newValue);
      return std::string("Failed to modify server action");
   }

   WriteAuditLogWithJsonValues(AUDIT_SYSCFG, true, userId, nullptr, 0, 0,
      oldValue, newValue, L"AI agent: modified server action %u", actionId);
   if (oldValue != nullptr) json_decref(oldValue);
   if (newValue != nullptr) json_decref(newValue);

   return EppActionSuccessResponse("modify-server-action", actionId);
}

/**
 * Delete a server-side action
 */
std::string F_DeleteServerAction(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccess = GetEffectiveSystemRights(userId);
   if ((systemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS) == 0)
      return std::string("User does not have rights to manage server actions");

   uint32_t actionId = json_object_get_uint32(arguments, "id", 0);
   if (actionId == 0)
      return EppFilterError("id is required", "id", nullptr);

   if (GetEventProcessingPolicy()->isActionInUse(actionId))
   {
      json_t *err = json_object();
      json_object_set_new(err, "error", json_string("action is in use by event processing policy"));
      json_object_set_new(err, "id", json_integer(actionId));
      json_object_set_new(err, "hint", json_string("Remove the action from referencing rules before deleting"));
      return JsonToString(err);
   }

   uint32_t rcc = DeleteAction(actionId);
   if (rcc == RCC_INVALID_ACTION_ID)
   {
      json_t *errJson = json_object();
      json_object_set_new(errJson, "error", json_string("action not found"));
      json_object_set_new(errJson, "id", json_integer(actionId));
      return JsonToString(errJson);
   }
   if (rcc != RCC_SUCCESS)
      return std::string("Failed to delete server action");

   WriteAuditLog(AUDIT_SYSCFG, true, userId, nullptr, 0, 0,
      L"AI agent: deleted server action %u", actionId);

   json_t *response = json_object();
   json_object_set_new(response, "status", json_string("ok"));
   json_object_set_new(response, "operation", json_string("delete-server-action"));
   json_object_set_new(response, "id", json_integer(actionId));
   return JsonToString(response);
}
