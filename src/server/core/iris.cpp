/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: iris.cpp
**
**/

#include "nxcore.h"
#include <nxlibcurl.h>
#include <netxms-version.h>
#include <iris.h>

#define DEBUG_TAG _T("llm.iris")

/**
 * Loaded server config
 */
extern Config g_serverConfig;

/**
 * LLM service URL
 */
static char s_llmServiceURL[256] = "http://127.0.0.1:11434/api/chat";

/**
 * Model
 */
static char s_llmModel[64] = "llama3.2";

/**
 * Authentication token
 */
static char s_llmAuthToken[64] = "";

/**
 * Prepared functions object for API requests
 */
static json_t *s_functionDeclarations = nullptr;

/**
 * Function handlers
 */
static std::unordered_map<std::string, AssistantFunctionHandler> s_functionHandlers;

/**
 * System prompt
 */
static const char *s_systemPrompt = "You are a helpful assistant named Iris. You have knowledge about NetXMS and its components, including network management, monitoring, and administration tasks. You can assist users with questions related to these topics. Your responses should be concise, accurate, and helpful. You can access live information from the NetXMS server to provide real-time assistance. If you are unable to answer a question, you should politely inform the user that you do not have the information available. Avoid answering questions not related to monitoring and IT administration tasks, and do not provide personal opinions or advice. Your goal is to assist users in managing their IT infrastructure effectively.";

/**
 * Additional prompts
 */
static std::vector<std::string> s_prompts;

/**
 * Initialize AI assistant
 */
bool InitAIAssistant()
{
   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("Model"), CT_MB_STRING, 0, 0, sizeof(s_llmModel), 0, s_llmModel },
      { _T("Token"), CT_MB_STRING, 0, 0, sizeof(s_llmAuthToken), 0, s_llmAuthToken },
      { _T("URL"), CT_MB_STRING, 0, 0, sizeof(s_llmServiceURL), 0, s_llmServiceURL },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };
   if (!g_serverConfig.parseTemplate(L"IRIS", configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("AI assistant initialization failed (cannot parse module configuration)"));
      return false;
   }

   if (s_functionDeclarations == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("AI assistant disabled (no functions registered)"));
      return true;  // No functions registered
   }

   RegisterComponent(AI_ASSISTANT_COMPONENT);

   nxlog_debug_tag(DEBUG_TAG, 2, L"LLM service URL = \"%hs\", model = \"%hs\"", s_llmServiceURL, s_llmModel);
   nxlog_debug_tag(DEBUG_TAG, 2, L"%d functions registered", static_cast<int>(s_functionHandlers.size()));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("AI assistant initialized"));
   return true;
}

/**
 * Add custom prompt
 */
void NXCORE_EXPORTABLE AddAIAssistantPrompt(const char *text)
{
   if ((text != nullptr) && (text[0] != 0))
      s_prompts.emplace_back(text);
}

/**
 * Call registered function
 */
std::string NXCORE_EXPORTABLE CallAIAssistantFunction(const char *name, json_t *arguments, uint32_t userId)
{
   auto it = s_functionHandlers.find(name);
   if (it != s_functionHandlers.end())
   {
      return it->second(arguments, userId);
   }
   return std::string("Error: function not found");
}

/**
 * Register AI assistant function
 */
void NXCORE_EXPORTABLE RegisterAIAssistantFunction(const char *name, const char *description, const std::vector<std::pair<const char*, const char*>>& properties, AssistantFunctionHandler handler)
{
   if (s_functionHandlers.find(name) != s_functionHandlers.end())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI assistant function \"%hs\" already registered", name);
      return;
   }

   json_t *tool = json_object();
   json_object_set_new(tool, "type", json_string("function"));
   json_t *function = json_object();
   json_object_set_new(function, "name", json_string(name));
   json_object_set_new(function, "description", json_string(description));
   json_t *parameters = json_object();
   json_object_set_new(parameters, "type", json_string("object"));
   json_t *propObject = json_object();
   for(std::pair<const char*, const char*> p : properties)
   {
      json_t *propData = json_object();
      json_object_set_new(propData, "description", json_string(p.second));
      json_object_set_new(propData, "type", json_string("string"));
      json_object_set_new(propObject, p.first, propData);
   }
   json_object_set_new(parameters, "properties", propObject);
   json_object_set_new(parameters, "required", json_array());
   json_object_set_new(function, "parameters", parameters);
   json_object_set_new(tool, "function", function);

   if (s_functionDeclarations == nullptr)
      s_functionDeclarations = json_array();
   json_array_append_new(s_functionDeclarations, tool);
   s_functionHandlers[name] = handler;
}

/**
 * Send request to Ollama server
 */
static json_t *OllamaApiRequest(json_t *requestData)
{
   if (IsShutdownInProgress())
      return nullptr;

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Call to curl_easy_init() failed"));
      return nullptr;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, (long)1);
#endif

   curl_easy_setopt(curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(curl, CURLOPT_TIMEOUT, 180);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Server/" NETXMS_VERSION_STRING_A);

   char *postData = (requestData != nullptr) ? json_dumps(requestData, 0) : strdup("{}");
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
   curl_easy_setopt(curl, CURLOPT_POST, (long)1);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   char errorBuffer[CURL_ERROR_SIZE];
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

   struct curl_slist *headers = curl_slist_append(nullptr, "Content-Type: application/json");
   if (s_llmAuthToken[0] != 0)
   {
      char authHeader[256];
      snprintf(authHeader, sizeof(authHeader), "Authorization: Bearer %s", s_llmAuthToken);
      curl_slist_append(headers, authHeader);
   }
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   bool success = true;

   if (curl_easy_setopt(curl, CURLOPT_URL, s_llmServiceURL) != CURLE_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Call to curl_easy_setopt(CURLOPT_URL) failed");
      success = false;
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, L"Sending request: %hs", postData);
      CURLcode rc = curl_easy_perform(curl);
      if (rc != CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"Call to curl_easy_perform(%hs) failed (%d: %hs)", s_llmServiceURL, rc, errorBuffer);
         success = false;
      }
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Got %d bytes from %hs"), static_cast<int>(responseData.size()), s_llmServiceURL);
      long httpStatusCode = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);
      if ((httpStatusCode < 200) || (httpStatusCode > 299))
      {
         char errorMessage[256] = "Error message not provided";
         if (responseData.size() > 0)
         {
            responseData.write('\0');
            const char *data = reinterpret_cast<const char*>(responseData.buffer());
            json_error_t error;
            json_t *json = json_loads(data, 0, &error);
            if (json != nullptr)
            {
               const char *msg = json_object_get_string_utf8(json, "message", nullptr);
               if ((msg != nullptr) && (*msg != 0))
               {
                  strlcpy(errorMessage, msg, sizeof(errorMessage));
               }
               else
               {
                  json_t *errorObj = json_object_get(json, "error");
                  if (json_is_object(errorObj))
                  {
                     msg = json_object_get_string_utf8(errorObj, "message", nullptr);
                     if ((msg != nullptr) && (*msg != 0))
                     {
                        strlcpy(errorMessage, msg, sizeof(errorMessage));
                     }
                  }
               }
               json_decref(json);
            }
            nxlog_debug_tag(DEBUG_TAG, 7, L"Error response document: %hs", data);
         }
         nxlog_debug_tag(DEBUG_TAG, 5, L"Error response from Ollama server: %d (%hs)", static_cast<int>(httpStatusCode), errorMessage);
         success = false;
      }
   }

   if (success && responseData.size() <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Empty response from Ollama server");
      success = false;
   }

   json_t *response = nullptr;
   if (success)
   {
      responseData.write('\0');
      const char *data = reinterpret_cast<const char*>(responseData.buffer());
      json_error_t error;
      response = json_loads(data, 0, &error);
      if (response != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Successful response from Ollama server: %hs"), data);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON on line %d: %hs"), error.line, error.text);
         success = false;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   free(postData);
   return response;
}

/**
 * Add message to messages array
 */
static inline void AddMessage(json_t *messages, const char *role, const char *content)
{
   json_t *message = json_object();
   json_object_set_new(message, "role", json_string(role));
   json_object_set_new(message, "content", json_string(content));
   json_array_append_new(messages, message);
}

/**
 * Chat destructor
 */
static void DeleteChat(void *chat, HashMapBase *map)
{
   json_decref(static_cast<json_t*>(chat));
}

/**
 * Saved chat sessions
 */
static HashMap<session_id_t, json_t> s_chats(Ownership::True, DeleteChat);
static Mutex s_chatsLock;

/**
 * Process request to assistant
 */
char NXCORE_EXPORTABLE *ProcessRequestToAIAssistant(const char *prompt, NetObj *context, GenericClientSession *session)
{
   json_t *messages;
   s_chatsLock.lock();
   messages = (session != nullptr) ? s_chats.get(session->getId()) : nullptr;
   if (messages == nullptr)
   {
      messages = json_array();
      if (session != nullptr)
         s_chats.set(session->getId(), messages);
      AddMessage(messages, "system", s_systemPrompt);
      if (context != nullptr)
      {
         AddMessage(messages, "system", "This request is made in the context of the following object:");
         json_t *json = context->toJson();
         char *jsonText = json_dumps(json, 0);
         json_decref(json);
         AddMessage(messages, "system", jsonText);
         MemFree(jsonText);
      }
      for(const std::string& p : s_prompts)
      {
         AddMessage(messages, "system", p.c_str());
      }
   }
   s_chatsLock.unlock();
   AddMessage(messages, "user", prompt);

   int iterations = 3;
   char *answer = nullptr;
   while(iterations-- > 0)
   {
      json_t *request = json_object();
      json_object_set_new(request, "model", json_string(s_llmModel));
      json_object_set_new(request, "stream", json_boolean(false));
      json_object_set(request, "tools", s_functionDeclarations);
      json_object_set(request, "messages", messages);

      json_t *response = OllamaApiRequest(request);
      json_decref(request);
      if (response == nullptr)
         return nullptr;

      json_t *message = json_object_get(response, "message");
      if (!json_is_object(message))
      {
         json_t *choices = json_object_get(response, "choices");
         if (json_is_array(choices) && (json_array_size(choices) > 0))
         {
            json_t *choice = json_array_get(choices, 0);
            if (json_is_object(choice))
            {
               message = json_object_get(choice, "message");
            }
         }
      }

      if (json_is_object(message))
      {
         json_array_append(messages, message);
         json_t *toolCalls = json_object_get(message, "tool_calls");
         if (toolCalls != nullptr)
         {
            size_t i;
            json_t *tool;
            json_array_foreach(toolCalls, i, tool)
            {
               json_t *function = json_object_get(tool, "function");
               if (json_is_object(function))
               {
                  json_t *toolCallId = json_object_get(tool, "id");
                  const char *name = json_object_get_string_utf8(function, "name", "");
                  nxlog_debug_tag(DEBUG_TAG, 5, L"LLM function call requested: %hs", name);
                  std::string functionResult = CallAIAssistantFunction(name, json_object_get(function, "arguments"), (session != nullptr) ? session->getUserId() : 0);
                  if (!functionResult.empty())
                  {
                     json_t *functionMessage = json_object();
                     json_object_set_new(functionMessage, "role", json_string("tool"));
                     json_object_set_new(functionMessage, "name", json_string(name));
                     json_object_set_new(functionMessage, "content", json_string(functionResult.c_str()));
                     if (json_is_string(toolCallId))
                     {
                        json_object_set(functionMessage, "tool_call_id", toolCallId);
                     }
                     json_array_append_new(messages, functionMessage);
                  }
               }
            }
         }
         else
         {
            json_t *content = json_object_get(message, "content");
            if (json_is_string(content))
            {
               answer = MemCopyStringA(json_string_value(content));
               iterations = 0;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, L"Property \"content\" is missing or empty");
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, L"Property \"message\" is missing in response");
         iterations = 0;
      }

      json_decref(response);
   }
   nxlog_debug_tag(DEBUG_TAG, 5, L"Final LLM response %s", (answer != nullptr) ? L"received" : L"not received");
   if (session == nullptr)
      json_decref(messages);  // if session is null, chat is not saved - delete it now
   return answer;
}

/**
 * Clear chat history for given session
 */
uint32_t NXCORE_EXPORTABLE ClearAIAssistantChat(GenericClientSession *session)
{
   s_chatsLock.lock();
   s_chats.remove(session->getId());
   s_chatsLock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 5, L"Chat history for session %d cleared", session->getId());
   return RCC_SUCCESS;
}
