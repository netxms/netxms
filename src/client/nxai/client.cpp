/*
** NetXMS - Network Management System
** Command line AI assistant client
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
** File: client.cpp
**
**/

#include "nxai.h"
#include <nxlibcurl.h>

/**
 * Default request timeout in milliseconds
 */
#define DEFAULT_TIMEOUT             30000

/**
 * Default time to wait for assistant response in milliseconds
 */
#define DEFAULT_RESPONSE_TIMEOUT    120000

/**
 * Interval between status polls in milliseconds
 */
#define POLL_INTERVAL               500

/**
 * Load question from JSON document. Returns false if document does not contain valid question.
 */
bool Question::loadFromJson(json_t *json)
{
   clear();

   if (!json_is_object(json))
      return false;

   id = json_object_get_int64(json, "id", 0);
   if (id == 0)
      return false;

   const char *type = json_object_get_string_utf8(json, "type", "confirmation");
   multipleChoice = !strcmp(type, "multipleChoice");
   confirmationType = static_cast<ConfirmationType>(json_object_get_int32(json, "confirmationType", 0));
   text = json_object_get_string_utf8(json, "text", "");
   context = json_object_get_string_utf8(json, "context", "");
   expiresAt = static_cast<time_t>(json_object_get_int64(json, "expiresAt", 0));

   json_t *optionList = json_object_get(json, "options");
   if (json_is_array(optionList))
   {
      size_t i;
      json_t *option;
      json_array_foreach(optionList, i, option)
      {
         const char *value = json_string_value(option);
         if (value != nullptr)
            options.push_back(value);
      }
   }

   return true;
}

/**
 * Client constructor. Server can be given as host name, host name with port, or complete URL.
 */
WebApiClient::WebApiClient(const char *server, bool verifyPeer)
{
   m_curl = curl_easy_init();

   if (strncmp(server, "http://", 7) && strncmp(server, "https://", 8))
      m_baseUrl = "https://";
   m_baseUrl.append(server);
   while(!m_baseUrl.empty() && (m_baseUrl.back() == '/'))
      m_baseUrl.pop_back();

   m_httpStatus = 0;
   m_timeout = DEFAULT_TIMEOUT;
   m_responseTimeout = DEFAULT_RESPONSE_TIMEOUT;
   m_verifyPeer = verifyPeer;
   m_cancellationFlag = 0;
}

/**
 * Client destructor
 */
WebApiClient::~WebApiClient()
{
   if (m_curl != nullptr)
      curl_easy_cleanup(m_curl);
}

/**
 * Set error text from error response received from server
 */
void WebApiClient::setErrorFromResponse(json_t *response, const char *rawResponse)
{
   const char *reason = (response != nullptr) ? json_object_get_string_utf8(response, "reason", nullptr) : nullptr;
   if (reason == nullptr)
      reason = (response != nullptr) ? json_object_get_string_utf8(response, "error", nullptr) : nullptr;
   if (reason != nullptr)
   {
      m_errorText = reason;
      return;
   }

   // Login response with list of available methods indicates that two-factor authentication is required
   if ((response != nullptr) && json_is_array(json_object_get(response, "methods")))
   {
      m_errorText = "Two-factor authentication is required but not supported by this tool";
      return;
   }

   if ((response == nullptr) && (*rawResponse != 0))
      m_errorText = rawResponse;   // Response is not a JSON document, could be an error page from proxy
   else
      m_errorText = "HTTP error " + std::to_string(m_httpStatus);
}

/**
 * Execute API call. If response document is requested, it will be returned in *response and caller
 * is responsible for releasing it with json_decref().
 */
bool WebApiClient::call(const char *method, const char *path, json_t *request, json_t **response)
{
   if (response != nullptr)
      *response = nullptr;

   m_errorText.clear();
   m_httpStatus = 0;

   if (m_curl == nullptr)
   {
      m_errorText = "cURL initialization failed";
      return false;
   }

   curl_easy_reset(m_curl);

   std::string url = m_baseUrl;
   url.append(path);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);

   curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
   curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, method);
   curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, static_cast<long>(m_timeout));
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);
   curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
   curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "NetXMS AI Assistant/" NETXMS_VERSION_STRING_A);
   EnableLibCURLUnexpectedEOFWorkaround(m_curl);

   if (!m_verifyPeer)
   {
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
   }

   struct curl_slist *headers = curl_slist_append(nullptr, "Accept: application/json");
   if (!m_token.empty())
   {
      std::string authHeader("Authorization: Bearer ");
      authHeader.append(m_token);
      headers = curl_slist_append(headers, authHeader.c_str());
   }

   char *requestData = (request != nullptr) ? json_dumps(request, 0) : nullptr;
   if (requestData != nullptr)
   {
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, requestData);
      curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(strlen(requestData)));
   }

   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

   CURLcode rc = curl_easy_perform(m_curl);

   curl_slist_free_all(headers);
   MemFree(requestData);

   if (rc != CURLE_OK)
   {
      m_errorText = curl_easy_strerror(rc);
      return false;
   }

   long httpStatus = 0;
   curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpStatus);
   m_httpStatus = static_cast<int>(httpStatus);

   responseData.write(static_cast<char>(0));
   const char *rawResponse = reinterpret_cast<const char*>(responseData.buffer());

   json_t *document = nullptr;
   if (*rawResponse != 0)
   {
      json_error_t error;
      document = json_loads(rawResponse, 0, &error);
   }

   if ((m_httpStatus < 200) || (m_httpStatus > 299))
   {
      setErrorFromResponse(document, rawResponse);
      json_decref(document);
      return false;
   }

   if (response == nullptr)
   {
      json_decref(document);
      return true;
   }

   if (document == nullptr)
   {
      m_errorText = "Empty or malformed response received from server";
      return false;
   }

   *response = document;
   return true;
}

/**
 * Authenticate on server and obtain access token
 */
bool WebApiClient::login(const char *username, const char *password)
{
   json_t *request = json_object();
   json_object_set_new(request, "username", json_string(username));
   json_object_set_new(request, "password", json_string(password));

   json_t *response;
   bool success = call("POST", "/v1/login", request, &response);
   json_decref(request);

   if (!success)
      return false;

   const char *token = json_object_get_string_utf8(response, "token", nullptr);
   if (token != nullptr)
      m_token = token;
   else
      m_errorText = "Server did not provide access token";
   json_decref(response);

   return token != nullptr;
}

/**
 * Create new chat session
 */
bool WebApiClient::createChat(uint32_t incidentId, uint32_t objectId, uint32_t *chatId)
{
   json_t *request = json_object();
   if (incidentId != 0)
      json_object_set_new(request, "incidentId", json_integer(incidentId));
   if (objectId != 0)
      json_object_set_new(request, "objectId", json_integer(objectId));

   json_t *response;
   bool success = call("POST", "/v1/ai/chat", request, &response);
   json_decref(request);

   if (!success)
      return false;

   *chatId = json_object_get_uint32(response, "chatId", 0);
   if (*chatId == 0)
      m_errorText = "Server did not provide chat ID";
   json_decref(response);

   return *chatId != 0;
}

/**
 * Clear chat history
 */
bool WebApiClient::clearChat(uint32_t chatId)
{
   char path[64];
   snprintf(path, sizeof(path), "/v1/ai/chat/%u/clear", chatId);
   return call("POST", path, nullptr, nullptr);
}

/**
 * Delete chat session
 */
bool WebApiClient::deleteChat(uint32_t chatId)
{
   char path[64];
   snprintf(path, sizeof(path), "/v1/ai/chat/%u", chatId);
   return call("DELETE", path, nullptr, nullptr);
}

/**
 * Send message to assistant and wait for response. Optional context document is sent along with
 * the message. Progress callback, if provided, is called on every status poll with the name of
 * function being executed by assistant (or nullptr if assistant is not executing any function).
 */
bool WebApiClient::sendMessage(uint32_t chatId, const char *message, json_t *context, ChatResponse *response,
      const std::function<void (const char*)>& progressCallback)
{
   response->clear();

   json_t *request = json_object();
   json_object_set_new(request, "message", json_string(message));
   if (context != nullptr)
      json_object_set(request, "context", context);

   char path[64];
   snprintf(path, sizeof(path), "/v1/ai/chat/%u/message", chatId);

   json_t *document;
   bool success = call("POST", path, request, &document);
   json_decref(request);

   if (!success)
      return false;

   // Server accepted request for asynchronous processing, poll for result
   if (m_httpStatus == 202)
   {
      json_decref(document);
      return waitForResponse(chatId, response, progressCallback);
   }

   response->text = json_object_get_string_utf8(document, "response", "");
   response->question.loadFromJson(json_object_get(document, "pendingQuestion"));
   json_decref(document);
   return true;
}

/**
 * Get chat processing status
 */
bool WebApiClient::getStatus(uint32_t chatId, ChatStatus *status)
{
   char path[64];
   snprintf(path, sizeof(path), "/v1/ai/chat/%u/status", chatId);

   json_t *document;
   if (!call("GET", path, nullptr, &document))
      return false;

   const char *state = json_object_get_string_utf8(document, "status", "");
   if (!strcmp(state, "processing"))
      status->state = ChatState::PROCESSING;
   else if (!strcmp(state, "completed"))
      status->state = ChatState::COMPLETED;
   else if (!strcmp(state, "error"))
      status->state = ChatState::FAILED;
   else if (!strcmp(state, "idle"))
      status->state = ChatState::IDLE;
   else
      status->state = ChatState::UNKNOWN;

   status->response = json_object_get_string_utf8(document, "response", "");
   status->errorMessage = json_object_get_string_utf8(document, "errorMessage", "");
   status->currentFunction = json_object_get_string_utf8(document, "currentFunction", "");
   status->question.loadFromJson(json_object_get(document, "pendingQuestion"));

   json_decref(document);
   return true;
}

/**
 * Wait for assistant to complete processing of current request. Returns as soon as response is ready
 * or assistant asks a question.
 */
bool WebApiClient::waitForResponse(uint32_t chatId, ChatResponse *response, const std::function<void (const char*)>& progressCallback)
{
   response->clear();

   uint32_t elapsed = 0;
   while(true)
   {
      if (isCancelled())
      {
         m_errorText = "Request cancelled";
         return false;
      }

      ChatStatus status;
      if (!getStatus(chatId, &status))
         return false;

      switch(status.state)
      {
         case ChatState::COMPLETED:
            response->text = status.response;
            return true;
         case ChatState::PROCESSING:
            // Assistant can ask question while processing request
            if (status.question.id != 0)
            {
               response->question = status.question;
               return true;
            }
            break;
         case ChatState::FAILED:
            m_errorText = !status.errorMessage.empty() ? status.errorMessage : "Request processing failed";
            return false;
         case ChatState::IDLE:
            m_errorText = "Server is not processing any request";
            return false;
         default:
            m_errorText = "Unexpected request status received from server";
            return false;
      }

      if (progressCallback)
         progressCallback(!status.currentFunction.empty() ? status.currentFunction.c_str() : nullptr);

      if (elapsed >= m_responseTimeout)
      {
         m_errorText = "Timeout waiting for assistant response";
         return false;
      }

      ThreadSleepMs(POLL_INTERVAL);
      elapsed += POLL_INTERVAL;
   }
}

/**
 * Poll for pending question. On success question ID is set to 0 if assistant has no pending question.
 */
bool WebApiClient::pollQuestion(uint32_t chatId, Question *question)
{
   char path[64];
   snprintf(path, sizeof(path), "/v1/ai/chat/%u/question", chatId);

   json_t *document;
   if (!call("GET", path, nullptr, &document))
      return false;

   question->loadFromJson(json_object_get(document, "question"));
   json_decref(document);
   return true;
}

/**
 * Answer pending question. Selected option should be set to -1 for confirmation questions.
 */
bool WebApiClient::answerQuestion(uint32_t chatId, uint64_t questionId, bool positive, int selectedOption)
{
   json_t *request = json_object();
   json_object_set_new(request, "questionId", json_integer(questionId));
   json_object_set_new(request, "positive", json_boolean(positive));
   json_object_set_new(request, "selectedOption", json_integer(selectedOption));

   char path[64];
   snprintf(path, sizeof(path), "/v1/ai/chat/%u/answer", chatId);

   bool success = call("POST", path, request, nullptr);
   json_decref(request);
   return success;
}

/**
 * Find object by name. Returns false if request failed. On success object ID is set to 0
 * if no matching object was found.
 */
bool WebApiClient::findObject(const char *name, ObjectInfo *object)
{
   json_t *request = json_object();
   json_object_set_new(request, "name", json_string(name));

   json_t *document;
   bool success = call("POST", "/v1/objects/search", request, &document);
   json_decref(request);

   if (!success)
   {
      // Older server versions may return 404 instead of empty result set
      if (m_httpStatus == 404)
      {
         *object = ObjectInfo();
         return true;
      }
      return false;
   }

   json_t *element = json_is_array(document) ? json_array_get(document, 0) : nullptr;
   if (element != nullptr)
   {
      object->id = json_object_get_uint32(element, "id", 0);
      object->name = json_object_get_string_utf8(element, "name", "");
      object->className = json_object_get_string_utf8(element, "class", "");
   }
   else
   {
      *object = ObjectInfo();
   }

   json_decref(document);
   return true;
}
