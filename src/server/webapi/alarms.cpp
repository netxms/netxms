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
#include <nms_users.h>

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
         json_object_set_new(json, "sourceName", json_string_t(object->getName()));
         json_object_set_new(json, "zoneUIN", json_integer(alarm->getZoneUIN()));
         json_object_set_new(json, "message", json_string_t(alarm->getMessage()));
         json_object_set_new(json, "repeatCount", json_integer(alarm->getRepeatCount()));
         json_object_set_new(json, "commentCount", json_integer(alarm->getCommentCount()));
         json_object_set_new(json, "helpdeskReference", json_string_t(alarm->getHelpDeskRef()));
         json_object_set_new(json, "creationTime", json_time_string(alarm->getCreationTime()));
         json_object_set_new(json, "lastChangeTime", json_time_string(alarm->getLastChangeTime()));

         wchar_t userName[MAX_USER_NAME];
         json_object_set_new(json, "ackByUserName", json_string_t(
            (alarm->getAckByUser() != INVALID_UID) ? ResolveUserId(alarm->getAckByUser(), userName, true) : L""));
         json_object_set_new(json, "resolvedByUserName", json_string_t(
            (alarm->getResolvedByUser() != INVALID_UID) ? ResolveUserId(alarm->getResolvedByUser(), userName, true) : L""));

         json_object_set_new(json, "categories", alarm->categoryListToJson());
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
static Alarm *AlarmFromRequest(Context *context, uint64_t objectAccess, int *responseCode)
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
 * Convert alarm comment to JSON
 */
static json_t *AlarmCommentToJson(uint32_t alarmId, uint32_t commentId, time_t changeTime, uint32_t userId, const wchar_t *text)
{
   wchar_t userName[MAX_USER_NAME];

   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(commentId));
   json_object_set_new(json, "alarmId", json_integer(alarmId));
   json_object_set_new(json, "userId", json_integer(userId));
   json_object_set_new(json, "userName", json_string_t(ResolveUserId(userId, userName, true)));
   json_object_set_new(json, "lastChangeTime", json_time_string(changeTime));
   json_object_set_new(json, "text", json_string_t(CHECK_NULL_EX(text)));
   return json;
}

/**
 * Get text of given alarm comment. Returns nullptr if comment does not exist or has no text.
 */
static wchar_t *AlarmCommentText(uint32_t alarmId, uint32_t commentId)
{
   wchar_t *text = nullptr;
   ObjectArray<AlarmComment> *comments = GetAlarmComments(alarmId);
   for(int i = 0; i < comments->size(); i++)
   {
      AlarmComment *comment = comments->get(i);
      if (comment->getId() == commentId)
         text = MemCopyString(comment->getText());
      delete comment;   // array is created with Ownership::False
   }
   delete comments;
   return text;
}

/**
 * Convert return code of alarm comment operation to HTTP response code
 */
static int AlarmCommentResponseCode(Context *context, uint32_t rcc, int successCode)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return successCode;
      case RCC_INVALID_ALARM_ID:
      case RCC_INVALID_ALARM_NOTE_ID:
         return 404;
      default:
         context->setErrorResponse("Database failure");
         return 500;
   }
}

/**
 * Get comment text from request document. Returns nullptr and sets response code if text is missing or empty.
 */
static wchar_t *CommentTextFromRequest(Context *context, int *responseCode)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      *responseCode = 400;
      return nullptr;
   }

   wchar_t *text = json_object_get_string_w(request, "text", nullptr);
   if ((text == nullptr) || (*text == 0))
   {
      MemFree(text);
      context->setErrorResponse("Comment text cannot be empty");
      *responseCode = 400;
      return nullptr;
   }

   return text;
}

/**
 * Handler for GET /v1/alarms/:alarm-id/comments
 */
int H_AlarmComments(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_READ_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   json_t *output = json_array();
   ObjectArray<AlarmComment> *comments = GetAlarmComments(alarm->getAlarmId());
   for(int i = 0; i < comments->size(); i++)
   {
      AlarmComment *comment = comments->get(i);
      json_array_append_new(output, AlarmCommentToJson(alarm->getAlarmId(), comment->getId(), comment->getChangeTime(),
         comment->getUserId(), comment->getText()));
      delete comment;   // array is created with Ownership::False
   }
   delete comments;

   context->setResponseData(output);
   json_decref(output);

   delete alarm;
   return 200;
}

/**
 * Handler for POST /v1/alarms/:alarm-id/comments
 */
int H_AlarmCommentCreate(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_UPDATE_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   wchar_t *text = CommentTextFromRequest(context, &responseCode);
   if (text == nullptr)
   {
      delete alarm;
      return responseCode;
   }

   uint32_t commentId = 0;   // 0 means new comment
   uint32_t rcc = UpdateAlarmComment(alarm->getAlarmId(), &commentId, text, context->getUserId());
   responseCode = AlarmCommentResponseCode(context, rcc, 201);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_OBJECTS, true, alarm->getSourceObject(), L"Comment [%u] added to alarm %u on object %s via REST API",
         commentId, alarm->getAlarmId(), GetObjectName(alarm->getSourceObject(), L""));

      json_t *output = AlarmCommentToJson(alarm->getAlarmId(), commentId, time(nullptr), context->getUserId(), text);
      context->setResponseData(output);
      json_decref(output);
   }

   MemFree(text);
   delete alarm;
   return responseCode;
}

/**
 * Handler for PUT /v1/alarms/:alarm-id/comments/:comment-id
 */
int H_AlarmCommentUpdate(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_UPDATE_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   uint32_t commentId = context->getPlaceholderValueAsUInt32(L"comment-id");
   if (commentId == 0)
   {
      delete alarm;
      return 400;
   }

   wchar_t *text = CommentTextFromRequest(context, &responseCode);
   if (text == nullptr)
   {
      delete alarm;
      return responseCode;
   }

   wchar_t *oldText = AlarmCommentText(alarm->getAlarmId(), commentId);

   uint32_t rcc = UpdateAlarmComment(alarm->getAlarmId(), &commentId, text, context->getUserId());
   responseCode = AlarmCommentResponseCode(context, rcc, 200);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLogWithValues(AUDIT_OBJECTS, true, alarm->getSourceObject(), oldText, text, 'T',
         L"Comment [%u] of alarm %u on object %s updated via REST API",
         commentId, alarm->getAlarmId(), GetObjectName(alarm->getSourceObject(), L""));

      json_t *output = AlarmCommentToJson(alarm->getAlarmId(), commentId, time(nullptr), context->getUserId(), text);
      context->setResponseData(output);
      json_decref(output);
   }

   MemFree(oldText);
   MemFree(text);
   delete alarm;
   return responseCode;
}

/**
 * Handler for DELETE /v1/alarms/:alarm-id/comments/:comment-id
 */
int H_AlarmCommentDelete(Context *context)
{
   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_UPDATE_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   uint32_t commentId = context->getPlaceholderValueAsUInt32(L"comment-id");
   if (commentId == 0)
   {
      delete alarm;
      return 400;
   }

   uint32_t rcc = DeleteAlarmCommentByID(alarm->getAlarmId(), commentId);
   responseCode = AlarmCommentResponseCode(context, rcc, 204);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_OBJECTS, true, alarm->getSourceObject(), L"Comment [%u] deleted from alarm %u on object %s via REST API",
         commentId, alarm->getAlarmId(), GetObjectName(alarm->getSourceObject(), L""));
   }

   delete alarm;
   return responseCode;
}

/**
 * Handler for GET /v1/alarms/:alarm-id/events
 */
int H_AlarmEvents(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_VIEW_EVENT_LOG))
      return 403;

   int responseCode;
   Alarm *alarm = AlarmFromRequest(context, OBJECT_ACCESS_READ_ALARMS, &responseCode);
   if (alarm == nullptr)
      return responseCode;

   json_t *output = GetAlarmEventsAsJson(alarm->getAlarmId());
   context->setResponseData(output);
   json_decref(output);

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
