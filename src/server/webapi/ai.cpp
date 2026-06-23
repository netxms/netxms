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
#include <nxai.h>

#define DEBUG_TAG  _T("webapi.ai")

/**
 * Handler for POST /v1/ai/chat - create new chat session
 */
int H_AiChatCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

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

   // Check client capabilities
   if (request != nullptr)
   {
      json_t *capabilities = json_object_get(request, "capabilities");
      if (json_is_array(capabilities))
      {
         size_t index;
         json_t *value;
         json_array_foreach(capabilities, index, value)
         {
            const char *cap = json_string_value(value);
            if ((cap != nullptr) && !strcmp(cap, "visualizations"))
            {
               chat->enableVisualizationOutput();
               nxlog_debug_tag(DEBUG_TAG, 6, _T("H_CreateChat: visualization output enabled for chat %u"), chat->getId());
            }
         }
      }
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
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

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
   if (!chat->startAsyncRequest(message, contextStr))
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
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

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
         // Include currently executing function name if any
         {
            const char *currentFunction = chat->getCurrentFunction();
            json_object_set_new(response, "currentFunction", currentFunction != nullptr ? json_string(currentFunction) : json_null());
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

      case AsyncRequestState::ERROR:
         {
            json_object_set_new(response, "status", json_string("error"));
            char *errorMessage = chat->takeAsyncErrorMessage();
            if (errorMessage != nullptr && *errorMessage != '\0')
               json_object_set_new(response, "errorMessage", json_string(errorMessage));
            MemFree(errorMessage);
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
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

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
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

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
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

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
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

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

/**
 * Handler for GET /v1/ai/skills-and-functions - get all registered skills, functions, and unmatched stop list entries
 */
int H_AiSkillsAndFunctions(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_SKILLS) &&
       !context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

   json_t *response = json_object();

   json_t *skills = GetAISkillsAsJson();
   json_object_set_new(response, "skills", skills);

   json_t *functions = GetAIFunctionsAsJson();
   json_object_set_new(response, "functions", functions);

   json_t *extras = GetAIDisabledExtrasAsJson();
   json_object_set_new(response, "disabledExtras", extras);

   context->setResponseData(response);
   json_decref(response);
   return 200;
}

/**
 * Handler for POST /v1/ai/disabled-items - add item to stop list
 */
int H_AiDisabledItemCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_SKILLS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   const char *type = json_object_get_string_utf8(request, "type", nullptr);
   if (type == nullptr || (type[0] != 'S' && type[0] != 'F') || type[1] != '\0')
   {
      context->setErrorResponse("Invalid or missing \"type\" field (must be \"S\" or \"F\")");
      return 400;
   }

   const char *name = json_object_get_string_utf8(request, "name", nullptr);
   if (name == nullptr || *name == '\0')
   {
      context->setErrorResponse("Invalid or missing \"name\" field");
      return 400;
   }

   if (!AddAIDisabledItem(type[0], name))
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"AI %s \"%hs\" added to disabled list", (type[0] == 'S') ? L"skill" : L"function", name);
   return 204;
}

/**
 * Handler for DELETE /v1/ai/disabled-items/:item-type/:item-name - remove item from stop list
 */
int H_AiDisabledItemDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_AI_SKILLS))
      return 403;

   const wchar_t *itemType = context->getPlaceholderValue(L"item-type");
   if (itemType == nullptr || (itemType[0] != 'S' && itemType[0] != 'F') || itemType[1] != 0)
   {
      context->setErrorResponse("Invalid item type (must be \"S\" or \"F\")");
      return 400;
   }

   const wchar_t *itemName = context->getPlaceholderValue(L"item-name");
   if (itemName == nullptr || *itemName == 0)
   {
      context->setErrorResponse("Invalid item name");
      return 400;
   }

   char name[128];
   wchar_to_utf8(itemName, -1, name, sizeof(name));

   if (!RemoveAIDisabledItem(static_cast<char>(itemType[0]), name))
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"AI %s \"%hs\" removed from disabled list", (itemType[0] == 'S') ? L"skill" : L"function", name);
   return 204;
}

/**
 * Build JSON object from a saved prompt database row.
 * Column order: id, name, description, prompt_text
 */
static json_t *BuildPromptJson(DB_RESULT hResult, int row)
{
   json_t *obj = json_object();
   json_object_set_new(obj, "id", json_integer(DBGetFieldULong(hResult, row, 0)));

   TCHAR name[256];
   DBGetField(hResult, row, 1, name, 256);
   json_object_set_new(obj, "name", json_string_t(name));

   char *description = DBGetFieldUTF8(hResult, row, 2, nullptr, 0);
   if (description != nullptr && *description != 0)
      json_object_set_new(obj, "description", json_string(description));
   else
      json_object_set_new(obj, "description", json_null());
   MemFree(description);

   char *promptText = DBGetFieldUTF8(hResult, row, 3, nullptr, 0);
   json_object_set_new(obj, "promptText", (promptText != nullptr) ? json_string(promptText) : json_null());
   MemFree(promptText);

   return obj;
}

/**
 * Handler for GET /v1/ai/saved-prompts - list all saved prompts for current user
 */
int H_AiSavedPrompts(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("SELECT id,name,description,prompt_text FROM ai_saved_prompts WHERE user_id=? ORDER BY name"));
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, context->getUserId());
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   DBFreeStatement(hStmt);

   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   json_t *output = json_array();
   int count = DBGetNumRows(hResult);
   for (int i = 0; i < count; i++)
      json_array_append_new(output, BuildPromptJson(hResult, i));

   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/ai/saved-prompts/:prompt-id - get a single saved prompt
 */
int H_AiSavedPromptDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

   uint32_t promptId = context->getPlaceholderValueAsUInt32(L"prompt-id");
   if (promptId == 0)
      return 400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("SELECT id,name,description,prompt_text FROM ai_saved_prompts WHERE id=? AND user_id=?"));
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, promptId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, context->getUserId());
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   DBFreeStatement(hStmt);

   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      DBConnectionPoolReleaseConnection(hdb);
      return 404;
   }

   json_t *output = BuildPromptJson(hResult, 0);
   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/ai/saved-prompts - create a new saved prompt
 */
int H_AiSavedPromptCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   const char *name = json_object_get_string_utf8(request, "name", nullptr);
   if (name == nullptr || *name == '\0')
   {
      context->setErrorResponse("Missing or empty \"name\" field");
      return 400;
   }

   const char *promptText = json_object_get_string_utf8(request, "promptText", nullptr);
   if (promptText == nullptr || *promptText == '\0')
   {
      context->setErrorResponse("Missing or empty \"promptText\" field");
      return 400;
   }

   const char *description = json_object_get_string_utf8(request, "description", nullptr);

   uint32_t promptId = CreateUniqueId(IDG_AI_SAVED_PROMPT);
   if (promptId == 0)
   {
      context->setErrorResponse("Unable to allocate unique ID");
      return 500;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("INSERT INTO ai_saved_prompts (id,user_id,name,description,prompt_text) VALUES (?,?,?,?,?)"));
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, promptId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, context->getUserId());
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, name, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, (description != nullptr) ? description : "", DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, promptText, DB_BIND_STATIC);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(promptId));
   json_object_set_new(output, "name", json_string(name));
   json_object_set_new(output, "description", (description != nullptr) ? json_string(description) : json_null());
   json_object_set_new(output, "promptText", json_string(promptText));
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Handler for PUT /v1/ai/saved-prompts/:prompt-id - update a saved prompt
 */
int H_AiSavedPromptUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

   uint32_t promptId = context->getPlaceholderValueAsUInt32(L"prompt-id");
   if (promptId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   const char *name = json_object_get_string_utf8(request, "name", nullptr);
   if (name == nullptr || *name == '\0')
   {
      context->setErrorResponse("Missing or empty \"name\" field");
      return 400;
   }

   const char *promptText = json_object_get_string_utf8(request, "promptText", nullptr);
   if (promptText == nullptr || *promptText == '\0')
   {
      context->setErrorResponse("Missing or empty \"promptText\" field");
      return 400;
   }

   const char *description = json_object_get_string_utf8(request, "description", nullptr);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("UPDATE ai_saved_prompts SET name=?,description=?,prompt_text=? WHERE id=? AND user_id=?"));
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, (description != nullptr) ? description : "", DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, promptText, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, promptId);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, context->getUserId());

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(promptId));
   json_object_set_new(output, "name", json_string(name));
   json_object_set_new(output, "description", (description != nullptr) ? json_string(description) : json_null());
   json_object_set_new(output, "promptText", json_string(promptText));
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for DELETE /v1/ai/saved-prompts/:prompt-id - delete a saved prompt
 */
int H_AiSavedPromptDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_USE_AI_ASSISTANT))
      return 403;

   uint32_t promptId = context->getPlaceholderValueAsUInt32(L"prompt-id");
   if (promptId == 0)
      return 400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("DELETE FROM ai_saved_prompts WHERE id=? AND user_id=?"));
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return 500;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, promptId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, context->getUserId());

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   return 204;
}
