/*
** NetXMS - Network Management System
** Copyright (C) 2025-2026 Raden Solutions
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
** File: ai.cpp
**
**/

#include "webapi.h"
#include <iris.h>

#define DEBUG_TAG  _T("webapi.ai")

/**
 * Handler for POST /v1/ai/chat - create new chat session
 */
int H_AiChatCreate(Context *context)
{
   json_t *request = context->getRequestDocument();
   uint32_t incidentId = (request != nullptr) ? json_object_get_uint32(request, "incidentId", 0) : 0;

   uint32_t rcc;
   shared_ptr<Chat> chat = CreateAIAssistantChat(context->getUserId(), incidentId, &rcc);
   if (chat == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_CreateChat: failed to create chat (rcc=%u)"), rcc);
      if (rcc == RCC_INVALID_INCIDENT_ID)
      {
         context->setErrorResponse("Invalid incident ID");
         return 400;
      }
      context->setErrorResponse("Failed to create chat");
      return 500;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("H_CreateChat: created chat %u for user %u"), chat->getId(), context->getUserId());

   json_t *response = json_object();
   json_object_set_new(response, "chatId", json_integer(chat->getId()));
   json_object_set_new(response, "created", json_time_string(time(nullptr)));
   context->setResponseData(response);
   json_decref(response);
   return 201;
}

/**
 * Handler for POST /v1/ai/chat/:chat-id/message - send message to AI
 * Returns 202 Accepted and processes asynchronously. Use GET /status to poll for result.
 */
int H_AiChatSendMessage(Context *context)
{
   uint32_t chatId = context->getPlaceholderValueAsUInt32(_T("chat-id"));
   if (chatId == 0)
   {
      context->setErrorResponse("Invalid chat ID");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   const char *message = json_object_get_string_utf8(request, "message", nullptr);
   if (message == nullptr || *message == '\0')
   {
      context->setErrorResponse("Message is required");
      return 400;
   }

   uint32_t rcc;
   shared_ptr<Chat> chat = GetAIAssistantChat(chatId, context->getUserId(), &rcc);
   if (chat == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_SendMessage: chat %u not found or access denied (rcc=%u)"), chatId, rcc);
      if (rcc == RCC_ACCESS_DENIED)
      {
         context->setErrorResponse("Access denied");
         return 403;
      }
      context->setErrorResponse("Chat not found");
      return 404;
   }

   // Build context JSON string if context object provided
   json_t *contextObj = json_object_get(request, "context");
   char *contextStr = (contextObj != nullptr) ? json_dumps(contextObj, 0) : nullptr;

   nxlog_debug_tag(DEBUG_TAG, 6, _T("H_SendMessage: starting async message processing for chat %u"), chatId);

   // Start async processing
   if (!chat->startAsyncRequest(message, 10, contextStr))
   {
      MemFree(contextStr);
      context->setErrorResponse("Request already in progress");
      return 409;  // Conflict
   }
   MemFree(contextStr);

   // Return 202 Accepted - client should poll /status for result
   json_t *response = json_object();
   json_object_set_new(response, "status", json_string("processing"));
   context->setResponseData(response);
   json_decref(response);
   return 202;
}

/**
 * Handler for GET /v1/ai/chat/:chat-id/status - get processing status
 * Returns current status, pending question (if any), and response (if complete)
 */
int H_AiChatGetStatus(Context *context)
{
   uint32_t chatId = context->getPlaceholderValueAsUInt32(_T("chat-id"));
   if (chatId == 0)
   {
      context->setErrorResponse("Invalid chat ID");
      return 400;
   }

   uint32_t rcc;
   shared_ptr<Chat> chat = GetAIAssistantChat(chatId, context->getUserId(), &rcc);
   if (chat == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_GetStatus: chat %u not found or access denied (rcc=%u)"), chatId, rcc);
      if (rcc == RCC_ACCESS_DENIED)
      {
         context->setErrorResponse("Access denied");
         return 403;
      }
      context->setErrorResponse("Chat not found");
      return 404;
   }

   json_t *response = json_object();

   AsyncRequestState state = chat->getAsyncState();
   switch(state)
   {
      case AsyncRequestState::PROCESSING:
         json_object_set_new(response, "status", json_string("processing"));
         // Include pending question if any (allows client to see and answer questions while processing)
         {
            json_t *question = chat->getPendingQuestion();
            json_object_set_new(response, "pendingQuestion", question != nullptr ? question : json_null());
         }
         break;

      case AsyncRequestState::COMPLETED:
         {
            json_object_set_new(response, "status", json_string("completed"));
            char *result = chat->takeAsyncResult();
            json_object_set_new(response, "response", result != nullptr ? json_string(result) : json_null());
            MemFree(result);
         }
         break;

      case AsyncRequestState::IDLE:
      default:
         json_object_set_new(response, "status", json_string("idle"));
         break;
   }

   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for GET /v1/ai/chat/:chat-id/question - poll for pending question
 */
int H_AiCharPollQuestion(Context *context)
{
   uint32_t chatId = context->getPlaceholderValueAsUInt32(_T("chat-id"));
   if (chatId == 0)
   {
      context->setErrorResponse("Invalid chat ID");
      return 400;
   }

   uint32_t rcc;
   shared_ptr<Chat> chat = GetAIAssistantChat(chatId, context->getUserId(), &rcc);
   if (chat == nullptr)
   {
      if (rcc == RCC_ACCESS_DENIED)
      {
         context->setErrorResponse("Access denied");
         return 403;
      }
      context->setErrorResponse("Chat not found");
      return 404;
   }

   json_t *response = json_object();
   json_t *question = chat->getPendingQuestion();
   json_object_set_new(response, "question", question != nullptr ? question : json_null());

   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for POST /v1/ai/chat/:chat-id/answer - answer a pending question
 */
int H_AiChatAnswerQuestion(Context *context)
{
   uint32_t chatId = context->getPlaceholderValueAsUInt32(_T("chat-id"));
   if (chatId == 0)
   {
      context->setErrorResponse("Invalid chat ID");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is required");
      return 400;
   }

   uint64_t questionId = json_object_get_int64(request, "questionId", 0);
   if (questionId == 0)
   {
      context->setErrorResponse("Question ID is required");
      return 400;
   }

   bool positive = json_object_get_boolean(request, "positive", false);
   int selectedOption = json_object_get_int32(request, "selectedOption", -1);

   uint32_t rcc;
   shared_ptr<Chat> chat = GetAIAssistantChat(chatId, context->getUserId(), &rcc);
   if (chat == nullptr)
   {
      if (rcc == RCC_ACCESS_DENIED)
      {
         context->setErrorResponse("Access denied");
         return 403;
      }
      context->setErrorResponse("Chat not found");
      return 404;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("H_AnswerQuestion: answering question ") UINT64_FMT _T(" in chat %u (positive=%s, option=%d)"),
      questionId, chatId, BooleanToString(positive), selectedOption);

   chat->handleQuestionResponse(questionId, positive, selectedOption);

   json_t *response = json_object();
   json_object_set_new(response, "success", json_boolean(true));
   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for POST /v1/ai/chat/:chat-id/clear - clear chat history
 */
int H_AiChatClear(Context *context)
{
   uint32_t chatId = context->getPlaceholderValueAsUInt32(_T("chat-id"));
   if (chatId == 0)
   {
      context->setErrorResponse("Invalid chat ID");
      return 400;
   }

   uint32_t rcc = ClearAIAssistantChat(chatId, context->getUserId());
   if (rcc != RCC_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ClearChat: failed to clear chat %u (rcc=%u)"), chatId, rcc);
      if (rcc == RCC_ACCESS_DENIED)
      {
         context->setErrorResponse("Access denied");
         return 403;
      }
      context->setErrorResponse("Chat not found");
      return 404;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("H_ClearChat: cleared chat %u"), chatId);
   return 204;
}

/**
 * Handler for DELETE /v1/ai/chat/:chat-id - delete chat session
 */
int H_AiChatDelete(Context *context)
{
   uint32_t chatId = context->getPlaceholderValueAsUInt32(_T("chat-id"));
   if (chatId == 0)
   {
      context->setErrorResponse("Invalid chat ID");
      return 400;
   }

   uint32_t rcc = DeleteAIAssistantChat(chatId, context->getUserId());
   if (rcc != RCC_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_DeleteChat: failed to delete chat %u (rcc=%u)"), chatId, rcc);
      if (rcc == RCC_ACCESS_DENIED)
      {
         context->setErrorResponse("Access denied");
         return 403;
      }
      context->setErrorResponse("Chat not found");
      return 404;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("H_DeleteChat: deleted chat %u"), chatId);
   return 204;
}
