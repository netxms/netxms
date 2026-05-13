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
** File: alarms.cpp
**
**/

#include "aitools.h"
#include <nms_users.h>
#include <nms_alarm.h>

/**
 * Get text representation of alarm state
 */
static const char *AlarmStateText(BYTE state)
{
   switch(state & ALARM_STATE_MASK)
   {
      case ALARM_STATE_OUTSTANDING:
         return "outstanding";
      case ALARM_STATE_ACKNOWLEDGED:
         return "acknowledged";
      case ALARM_STATE_RESOLVED:
         return "resolved";
      case ALARM_STATE_TERMINATED:
         return "terminated";
      default:
         return "unknown";
   }
}

/**
 * Get human readable text for result code returned by alarm operations
 */
static const char *AlarmOperationErrorText(uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_INVALID_ALARM_ID:
         return "invalid alarm id";
      case RCC_ACCESS_DENIED:
         return "access denied";
      case RCC_ALARM_NOT_OUTSTANDING:
         return "alarm is not in outstanding state and cannot be acknowledged";
      case RCC_ALARM_OPEN_IN_HELPDESK:
         return "alarm is open in an external helpdesk system and cannot be terminated";
      case RCC_OUT_OF_STATE_REQUEST:
         return "operation is not valid for current alarm state";
      case RCC_DB_FAILURE:
         return "database error";
      default:
         return "operation failed";
   }
}

/**
 * Parse alarm ID list from an argument that can be a single numeric ID,
 * a numeric string, or an array of either. Returns true if at least one
 * valid (non-zero) ID was parsed.
 */
static bool ParseAlarmIdList(json_t *arguments, const char *key, IntegerArray<uint32_t>& ids)
{
   json_t *value = json_object_get(arguments, key);
   if (value == nullptr)
      return false;

   if (json_is_array(value))
   {
      size_t index;
      json_t *element;
      json_array_foreach(value, index, element)
      {
         uint32_t id = 0;
         if (json_is_integer(element))
            id = static_cast<uint32_t>(json_integer_value(element));
         else if (json_is_string(element))
            id = strtoul(json_string_value(element), nullptr, 10);
         if (id != 0)
            ids.add(id);
      }
   }
   else if (json_is_integer(value))
   {
      uint32_t id = static_cast<uint32_t>(json_integer_value(value));
      if (id != 0)
         ids.add(id);
   }
   else if (json_is_string(value))
   {
      uint32_t id = strtoul(json_string_value(value), nullptr, 10);
      if (id != 0)
         ids.add(id);
   }

   return !ids.isEmpty();
}

/**
 * Result of an alarm access check
 */
enum class AlarmAccessResult
{
   ALLOWED,
   DENIED,
   NOT_FOUND
};

/**
 * Check whether given user is allowed to perform an operation on alarm with given required object access rights.
 */
static AlarmAccessResult CheckAlarmAccess(uint32_t alarmId, uint32_t userId, uint64_t systemAccessRights, uint32_t requiredObjectAccess)
{
   Alarm *alarm = FindAlarmById(alarmId);   // returns a copy that must be deleted by caller
   if (alarm == nullptr)
      return AlarmAccessResult::NOT_FOUND;
   shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
   bool allowed = (object != nullptr) &&
            object->checkAccessRights(userId, requiredObjectAccess) &&
            alarm->checkCategoryAccess(userId, systemAccessRights);
   delete alarm;
   return allowed ? AlarmAccessResult::ALLOWED : AlarmAccessResult::DENIED;
}

/**
 * Alarm action type
 */
enum class AlarmAction
{
   ACKNOWLEDGE,
   RESOLVE,
   TERMINATE
};

/**
 * Perform an alarm action (acknowledge / resolve / terminate) on one or more alarms.
 * Operation is recorded as performed by the system (NetXMS server), same as alarm
 * actions initiated from NXSL scripts or event processing policy.
 */
static std::string PerformAlarmAction(json_t *arguments, uint32_t userId, AlarmAction action)
{
   IntegerArray<uint32_t> ids;
   if (!ParseAlarmIdList(arguments, "alarms", ids))
      return std::string("Error: one or more alarm IDs must be provided in \"alarms\" argument");

   bool sticky = json_object_get_boolean(arguments, "sticky", false);
   uint32_t acknowledgmentActionTime = 0;
   if (action == AlarmAction::ACKNOWLEDGE)
   {
      int timeoutMinutes = json_object_get_int32(arguments, "timeout_minutes", 0);
      if (timeoutMinutes > 0)
         acknowledgmentActionTime = static_cast<uint32_t>(timeoutMinutes) * 60;
   }
   bool includeSubordinates = json_object_get_boolean(arguments, "include_subordinates", true);

   uint32_t requiredObjectAccess = (action == AlarmAction::TERMINATE) ? OBJECT_ACCESS_TERM_ALARMS : OBJECT_ACCESS_UPDATE_ALARMS;
   uint64_t systemAccessRights = GetEffectiveSystemRights(userId);

   json_t *results = json_array();
   int succeeded = 0, failed = 0;
   for(int i = 0; i < ids.size(); i++)
   {
      uint32_t alarmId = ids.get(i);
      json_t *entry = json_object();
      json_object_set_new(entry, "alarm_id", json_integer(alarmId));

      AlarmAccessResult access = CheckAlarmAccess(alarmId, userId, systemAccessRights, requiredObjectAccess);
      if (access != AlarmAccessResult::ALLOWED)
      {
         json_object_set_new(entry, "status", json_string("error"));
         json_object_set_new(entry, "reason", json_string((access == AlarmAccessResult::NOT_FOUND) ?
            "invalid alarm id (alarm is not currently active)" : "access denied"));
         json_array_append_new(results, entry);
         failed++;
         continue;
      }

      uint32_t rcc;
      switch(action)
      {
         case AlarmAction::ACKNOWLEDGE:
            rcc = AckAlarmById(alarmId, nullptr, sticky, acknowledgmentActionTime, includeSubordinates);
            break;
         case AlarmAction::RESOLVE:
            rcc = ResolveAlarmById(alarmId, nullptr, false, includeSubordinates);
            break;
         case AlarmAction::TERMINATE:
            rcc = ResolveAlarmById(alarmId, nullptr, true, includeSubordinates);
            break;
         default:
            rcc = RCC_INVALID_ARGUMENT;
            break;
      }

      if (rcc == RCC_SUCCESS)
      {
         json_object_set_new(entry, "status", json_string("ok"));
         succeeded++;
      }
      else
      {
         json_object_set_new(entry, "status", json_string("error"));
         json_object_set_new(entry, "reason", json_string(AlarmOperationErrorText(rcc)));
         failed++;
      }
      json_array_append_new(results, entry);
   }

   json_t *output = json_object();
   json_object_set_new(output, "requested", json_integer(ids.size()));
   json_object_set_new(output, "succeeded", json_integer(succeeded));
   json_object_set_new(output, "failed", json_integer(failed));
   json_object_set_new(output, "results", results);
   return JsonToString(output);
}

/**
 * Get alarm list
 */
std::string F_AlarmList(json_t *arguments, uint32_t userId)
{
   uint64_t systemAccessRights = GetEffectiveSystemRights(userId);

   uint32_t rootId = 0;
   shared_ptr<NetObj> rootObject = FindObjectByNameOrId(arguments, "object");
   if (rootObject != nullptr)
   {
      if (!rootObject->checkAccessRights(userId, OBJECT_ACCESS_READ))
         return std::string("Access denied");
      rootId = rootObject->getId();
   }

   // Optional state filter: "active" (outstanding + acknowledged, the default), "outstanding", "acknowledged", "resolved", or "all"
   const char *stateFilter = json_object_get_string_utf8(arguments, "state", "active");
   bool wantOutstanding = !stricmp(stateFilter, "all") || !stricmp(stateFilter, "active") || !stricmp(stateFilter, "outstanding");
   bool wantAcknowledged = !stricmp(stateFilter, "all") || !stricmp(stateFilter, "active") || !stricmp(stateFilter, "acknowledged");
   bool wantResolved = !stricmp(stateFilter, "all") || !stricmp(stateFilter, "resolved");

   wchar_t userNameBuffer[MAX_USER_NAME];

   json_t *output = json_array();
   ObjectArray<Alarm> *alarms = GetAlarms();
   for(int i = 0; i < alarms->size(); i++)
   {
      Alarm *alarm = alarms->get(i);
      switch(alarm->getState() & ALARM_STATE_MASK)
      {
         case ALARM_STATE_OUTSTANDING:
            if (!wantOutstanding)
               continue;
            break;
         case ALARM_STATE_ACKNOWLEDGED:
            if (!wantAcknowledged)
               continue;
            break;
         case ALARM_STATE_RESOLVED:
            if (!wantResolved)
               continue;
            break;
         default:
            break;
      }

      shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
      if ((object != nullptr) &&
          ((rootId == 0) || (rootId == object->getId()) || object->isParent(rootId)) &&
          object->checkAccessRights(userId, OBJECT_ACCESS_READ_ALARMS) &&
          alarm->checkCategoryAccess(userId, systemAccessRights))
      {
         json_t *json = json_object();
         json_object_set_new(json, "id", json_integer(alarm->getAlarmId()));
         json_object_set_new(json, "state", json_string(AlarmStateText(alarm->getState())));
         json_object_set_new(json, "severity", json_string(AlarmSeverityTextFromCode(alarm->getCurrentSeverity())));
         json_object_set_new(json, "source_object_id", json_integer(object->getId()));
         json_object_set_new(json, "source_name", json_string_w(GetObjectName(object->getId(), L"unknown")));
         json_object_set_new(json, "message", json_string_t(alarm->getMessage()));
         json_object_set_new(json, "repeat_count", json_integer(alarm->getRepeatCount()));
         json_object_set_new(json, "creation_time", json_time_string(alarm->getCreationTime()));
         json_object_set_new(json, "last_change_time", json_time_string(alarm->getLastChangeTime()));
         if ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_ACKNOWLEDGED)
            json_object_set_new(json, "acknowledged_by", json_string_t(ResolveUserId(alarm->getAckByUser(), userNameBuffer, true)));
         else if ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_RESOLVED)
            json_object_set_new(json, "resolved_by", json_string_t(ResolveUserId(alarm->getResolvedByUser(), userNameBuffer, true)));
         json_array_append_new(output, json);
      }
   }
   delete alarms;

   return JsonToString(output);
}

/**
 * Get detailed information about a single alarm, including comment history and related events.
 */
std::string F_GetAlarmDetails(json_t *arguments, uint32_t userId)
{
   uint32_t alarmId = json_object_get_uint32(arguments, "alarm", 0);
   if (alarmId == 0)
      return std::string("Error: alarm ID must be provided");

   Alarm *alarm = FindAlarmById(alarmId);
   if (alarm == nullptr)
      return std::string("Error: invalid alarm id (alarm is not currently active; terminated alarms are not retained in the active alarm list)");

   shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
   if ((object == nullptr) ||
       !object->checkAccessRights(userId, OBJECT_ACCESS_READ_ALARMS) ||
       !alarm->checkCategoryAccess(userId, GetEffectiveSystemRights(userId)))
   {
      delete alarm;
      return std::string("Access denied");
   }

   wchar_t userNameBuffer[MAX_USER_NAME];

   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(alarm->getAlarmId()));
   json_object_set_new(json, "state", json_string(AlarmStateText(alarm->getState())));
   json_object_set_new(json, "sticky", json_boolean((alarm->getState() & ALARM_STATE_STICKY) != 0));
   json_object_set_new(json, "current_severity", json_string(AlarmSeverityTextFromCode(alarm->getCurrentSeverity())));
   json_object_set_new(json, "original_severity", json_string(AlarmSeverityTextFromCode(alarm->getOriginalSeverity())));
   json_object_set_new(json, "message", json_string_t(alarm->getMessage()));
   if (alarm->getKey()[0] != 0)
      json_object_set_new(json, "key", json_string_t(alarm->getKey()));
   json_object_set_new(json, "source_object_id", json_integer(alarm->getSourceObject()));
   json_object_set_new(json, "source_object_name", json_string_t(object->getName()));
   json_object_set_new(json, "source_object_class", json_string_t(object->getObjectClassName()));
   json_object_set_new(json, "source_object_status", json_string_t(GetStatusAsText(object->getStatus(), false)));
   json_object_set_new(json, "creation_time", json_time_string(alarm->getCreationTime()));
   json_object_set_new(json, "last_change_time", json_time_string(alarm->getLastChangeTime()));
   json_object_set_new(json, "repeat_count", json_integer(alarm->getRepeatCount()));
   if (alarm->getDciId() != 0)
      json_object_set_new(json, "dci_id", json_integer(alarm->getDciId()));
   if (alarm->getRuleDescription()[0] != 0)
      json_object_set_new(json, "epp_rule_description", json_string_t(alarm->getRuleDescription()));
   if ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_ACKNOWLEDGED)
      json_object_set_new(json, "acknowledged_by", json_string_t(ResolveUserId(alarm->getAckByUser(), userNameBuffer, true)));
   if (alarm->getAckTimeout() != 0)
      json_object_set_new(json, "sticky_acknowledgment_expires", json_time_string(alarm->getAckTimeout()));
   if ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_RESOLVED)
      json_object_set_new(json, "resolved_by", json_string_t(ResolveUserId(alarm->getResolvedByUser(), userNameBuffer, true)));
   if (alarm->getHelpDeskState() != ALARM_HELPDESK_IGNORED)
   {
      json_object_set_new(json, "helpdesk_state", json_string((alarm->getHelpDeskState() == ALARM_HELPDESK_OPEN) ? "open" : "closed"));
      if (alarm->getHelpDeskRef()[0] != 0)
         json_object_set_new(json, "helpdesk_reference", json_string_t(alarm->getHelpDeskRef()));
   }
   if (alarm->getImpact()[0] != 0)
      json_object_set_new(json, "impact", json_string_t(alarm->getImpact()));

   delete alarm;

   // Comment history
   json_t *commentsArray = json_array();
   ObjectArray<AlarmComment> *comments = GetAlarmComments(alarmId);
   for(int i = 0; i < comments->size(); i++)
   {
      AlarmComment *comment = comments->get(i);
      json_t *commentJson = json_object();
      json_object_set_new(commentJson, "id", json_integer(comment->getId()));
      json_object_set_new(commentJson, "time", json_time_string(comment->getChangeTime()));
      wchar_t commentUserBuffer[MAX_USER_NAME];
      json_object_set_new(commentJson, "user", json_string_t(ResolveUserId(comment->getUserId(), commentUserBuffer, true)));
      json_object_set_new(commentJson, "text", json_string_t(CHECK_NULL_EX(comment->getText())));
      json_array_append_new(commentsArray, commentJson);
      delete comment;
   }
   delete comments;
   json_object_set_new(json, "comments", commentsArray);

   // Related events
   json_t *eventsArray = json_array();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT event_id,event_code,event_name,severity,source_object_id,event_timestamp,message FROM alarm_events WHERE alarm_id=? ORDER BY event_timestamp DESC");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         if (count > 50)
            count = 50;
         for(int i = 0; i < count; i++)
         {
            json_t *eventJson = json_object();
            json_object_set_new(eventJson, "event_id", json_integer(DBGetFieldUInt64(hResult, i, 0)));
            json_object_set_new(eventJson, "event_code", json_integer(DBGetFieldUInt32(hResult, i, 1)));
            wchar_t eventName[MAX_DB_STRING];
            json_object_set_new(eventJson, "name", json_string_t(DBGetField(hResult, i, 2, eventName, MAX_DB_STRING)));
            json_object_set_new(eventJson, "severity", json_string(AlarmSeverityTextFromCode(DBGetFieldInt32(hResult, i, 3))));
            uint32_t sourceId = DBGetFieldUInt32(hResult, i, 4);
            json_object_set_new(eventJson, "source_object_id", json_integer(sourceId));
            json_object_set_new(eventJson, "source_object_name", json_string_w(GetObjectName(sourceId, L"unknown")));
            json_object_set_new(eventJson, "timestamp", json_time_string(DBGetFieldUInt32(hResult, i, 5)));
            wchar_t eventMessage[MAX_EVENT_MSG_LENGTH];
            json_object_set_new(eventJson, "message", json_string_t(DBGetField(hResult, i, 6, eventMessage, MAX_EVENT_MSG_LENGTH)));
            json_array_append_new(eventsArray, eventJson);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   json_object_set_new(json, "related_events", eventsArray);

   return JsonToString(json);
}

/**
 * Acknowledge one or more alarms
 */
std::string F_AcknowledgeAlarm(json_t *arguments, uint32_t userId)
{
   return PerformAlarmAction(arguments, userId, AlarmAction::ACKNOWLEDGE);
}

/**
 * Resolve one or more alarms (alarm stays in the active list and is re-armed on the next matching event)
 */
std::string F_ResolveAlarm(json_t *arguments, uint32_t userId)
{
   return PerformAlarmAction(arguments, userId, AlarmAction::RESOLVE);
}

/**
 * Terminate one or more alarms (alarm is removed from the active list)
 */
std::string F_TerminateAlarm(json_t *arguments, uint32_t userId)
{
   return PerformAlarmAction(arguments, userId, AlarmAction::TERMINATE);
}

/**
 * Add operator comment to an alarm
 */
std::string F_AddAlarmComment(json_t *arguments, uint32_t userId)
{
   uint32_t alarmId = json_object_get_uint32(arguments, "alarm", 0);
   if (alarmId == 0)
      return std::string("Error: alarm ID must be provided");

   String text(json_object_get_string_utf8(arguments, "text", ""), "utf-8");
   if (text.isEmpty())
      return std::string("Error: comment text must be provided");

   switch(CheckAlarmAccess(alarmId, userId, GetEffectiveSystemRights(userId), OBJECT_ACCESS_UPDATE_ALARMS))
   {
      case AlarmAccessResult::NOT_FOUND:
         return std::string("Error: invalid alarm id (alarm is not currently active)");
      case AlarmAccessResult::DENIED:
         return std::string("Access denied");
      default:
         break;
   }

   uint32_t commentId = 0;
   uint32_t rcc = UpdateAlarmComment(alarmId, &commentId, text.cstr(), userId);

   json_t *output = json_object();
   json_object_set_new(output, "alarm_id", json_integer(alarmId));
   if (rcc == RCC_SUCCESS)
   {
      json_object_set_new(output, "status", json_string("ok"));
      json_object_set_new(output, "comment_id", json_integer(commentId));
   }
   else
   {
      json_object_set_new(output, "status", json_string("error"));
      json_object_set_new(output, "reason", json_string(AlarmOperationErrorText(rcc)));
   }
   return JsonToString(output);
}
