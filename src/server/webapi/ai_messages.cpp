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
** File: ai_messages.cpp
**
** AI message endpoints (messages and approval requests sent by AI tasks to users)
**
**/

#include "webapi.h"
#include <ai_messages.h>

/**
 * Map RCC from AI message functions to HTTP status code
 */
static int MapAIMessageRCC(Context *context, uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return 200;
      case RCC_ACCESS_DENIED:
         return 403;
      case RCC_INVALID_MESSAGE_ID:
         context->setErrorResponse("Message not found");
         return 404;
      case RCC_INCOMPATIBLE_OPERATION:
         context->setErrorResponse("Operation is not applicable to this message");
         return 409;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse("Invalid request");
         return 400;
      default:
         context->setErrorResponse("Internal server error");
         return 500;
   }
}

/**
 * Get message by ID from request path. Returns nullptr and sets HTTP status code if message cannot be accessed by caller.
 */
static shared_ptr<AIMessage> GetMessageFromRequest(Context *context, int *httpStatus)
{
   uint32_t messageId = context->getPlaceholderValueAsUInt32(L"message-id");
   if (messageId == 0)
   {
      context->setErrorResponse("Invalid message ID");
      *httpStatus = 400;
      return shared_ptr<AIMessage>();
   }

   shared_ptr<AIMessage> message = GetAIMessage(messageId);
   if ((message == nullptr) || message->isExpired())
   {
      context->setErrorResponse("Message not found");
      *httpStatus = 404;
      return shared_ptr<AIMessage>();
   }

   if (!message->isRecipient(context->getUserId()))
   {
      *httpStatus = 403;
      return shared_ptr<AIMessage>();
   }

   return message;
}

/**
 * Handler for GET /v1/ai/messages - list AI messages addressed to the caller.
 * Optional query parameters:
 *   type   - informational|alert|approvalRequest
 *   status - pending|read|approved|rejected (status as seen by the caller)
 */
int H_AiMessages(Context *context)
{
   const char *typeFilter = context->getQueryParameter("type");
   if ((typeFilter != nullptr) && strcmp(typeFilter, "informational") && strcmp(typeFilter, "alert") && strcmp(typeFilter, "approvalRequest"))
   {
      context->setErrorResponse("Invalid \"type\" parameter (must be \"informational\", \"alert\", or \"approvalRequest\")");
      return 400;
   }

   const char *statusFilter = context->getQueryParameter("status");
   if ((statusFilter != nullptr) && strcmp(statusFilter, "pending") && strcmp(statusFilter, "read") && strcmp(statusFilter, "approved") && strcmp(statusFilter, "rejected"))
   {
      context->setErrorResponse("Invalid \"status\" parameter (must be \"pending\", \"read\", \"approved\", or \"rejected\")");
      return 400;
   }

   uint32_t userId = context->getUserId();
   unique_ptr<SharedObjectArray<AIMessage>> messages = GetAIMessagesForUser(userId);
   json_t *output = json_array();
   for (int i = 0; i < messages->size(); i++)
   {
      json_t *json = messages->get(i)->toJson(userId);
      if (((typeFilter != nullptr) && strcmp(json_object_get_string_utf8(json, "type", ""), typeFilter)) ||
          ((statusFilter != nullptr) && strcmp(json_object_get_string_utf8(json, "status", ""), statusFilter)))
      {
         json_decref(json);
         continue;
      }
      json_array_append_new(output, json);
   }
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/ai/messages/:message-id - get AI message details
 */
int H_AiMessageDetails(Context *context)
{
   int httpStatus;
   shared_ptr<AIMessage> message = GetMessageFromRequest(context, &httpStatus);
   if (message == nullptr)
      return httpStatus;

   json_t *output = message->toJson(context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for PUT /v1/ai/messages/:message-id/status - mark message as read, approve or reject approval request.
 * Request body: { "status": "read|approved|rejected" }
 */
int H_AiMessageStatusUpdate(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   const char *status = json_object_get_string_utf8(request, "status", "");
   if (strcmp(status, "read") && strcmp(status, "approved") && strcmp(status, "rejected"))
   {
      context->setErrorResponse("Invalid or missing \"status\" field (must be \"read\", \"approved\", or \"rejected\")");
      return 400;
   }

   int httpStatus;
   shared_ptr<AIMessage> message = GetMessageFromRequest(context, &httpStatus);
   if (message == nullptr)
      return httpStatus;

   uint32_t userId = context->getUserId();
   uint32_t rcc;
   if (!strcmp(status, "read"))
   {
      rcc = MarkAIMessageAsRead(message->getId(), userId);
   }
   else
   {
      bool approve = !strcmp(status, "approved");
      rcc = approve ? ApproveAIMessage(message->getId(), userId) : RejectAIMessage(message->getId(), userId);
      if (rcc == RCC_SUCCESS)
         context->writeAuditLog(AUDIT_AI, true, message->getRelatedObjectId(), L"AI approval request [%u] \"%s\" %s", message->getId(), message->getTitle(), approve ? L"approved" : L"rejected");
   }
   if (rcc != RCC_SUCCESS)
      return MapAIMessageRCC(context, rcc);

   json_t *output = message->toJson(userId);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/ai/messages/:message-id - delete AI message (allowed for message initiator and users with AI task management rights)
 */
int H_AiMessageDelete(Context *context)
{
   uint32_t messageId = context->getPlaceholderValueAsUInt32(L"message-id");
   if (messageId == 0)
   {
      context->setErrorResponse("Invalid message ID");
      return 400;
   }

   shared_ptr<AIMessage> message = GetAIMessage(messageId);
   if ((message == nullptr) || message->isExpired())
   {
      context->setErrorResponse("Message not found");
      return 404;
   }

   uint32_t rcc = DeleteAIMessage(messageId, context->getUserId());
   if (rcc != RCC_SUCCESS)
   {
      if (rcc == RCC_ACCESS_DENIED)
         context->writeAuditLog(AUDIT_AI, false, message->getRelatedObjectId(), L"Access denied on deleting AI message [%u]", messageId);
      return MapAIMessageRCC(context, rcc);
   }

   context->writeAuditLog(AUDIT_AI, true, message->getRelatedObjectId(), L"AI message [%u] \"%s\" deleted", messageId, message->getTitle());
   return 204;
}
