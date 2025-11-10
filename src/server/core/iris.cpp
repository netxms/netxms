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
#include <unordered_map>

#define DEBUG_TAG _T("llm.iris")

void InitAITasks();
void AITaskSchedulerThread(ThreadPool *aiTaskThreadPool);

struct AssistantFunction
{
   std::string name;
   std::string description;
   std::vector<std::pair<std::string, std::string>> parameters;
   AssistantFunctionHandler handler;

   AssistantFunction(const std::string& name, const std::string& description,
      const std::vector<std::pair<std::string, std::string>>& parameters, AssistantFunctionHandler handler)
      : name(name), description(description), parameters(parameters), handler(handler)
   {
   }
};

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
static char s_llmAuthToken[256] = "";

/**
 * Prepared functions object for API requests
 */
static json_t *s_functionDeclarations = nullptr;

/**
 * Functions
 */
static std::unordered_map<std::string, shared_ptr<AssistantFunction>> s_functions;

/**
 * Mutex for functions
 */
static Mutex s_functionsMutex(MutexType::FAST);

/**
 * System prompt
 */
static const char *s_systemPrompt =
         "You are a helpful assistant named Iris. "
         "You have knowledge about NetXMS and its components, including network management, monitoring, and administration tasks. "
         "You can assist users with questions related to these topics. "
         "Your responses should be concise, accurate, and helpful. "
         "You can access live information from the NetXMS server to provide real-time assistance. "
         "If you are unable to answer a question, you should politely inform the user that you do not have the information available. "
         "Avoid answering questions not related to monitoring and IT administration tasks, and do not provide personal opinions or advice. "
         "Your goal is to assist users in managing their IT infrastructure effectively.";

/**
 * System prompt for background requests
 */
static const char *s_systemPromptBackground =
         "You are a helpful assistant named Iris. "
         "You have knowledge about NetXMS and its components, including network management, monitoring, and administration tasks. "
         "You can assist users with questions related to these topics. "
         "Your responses should be concise, accurate, and helpful. "
         "You can access live information from the NetXMS server to provide real-time assistance. "
         "If you are unable to answer a question, you should politely inform the user that you do not have the information available. "
         "Avoid answering questions not related to monitoring and IT administration tasks, and do not provide personal opinions or advice. "
         "You are operating autonomously without user interaction. "
         "Never stop to ask for user input or confirmation. "
         "You can create background tasks that may require multiple executions to complete if necessary. ";

/**
 * Additional prompts
 */
static std::vector<std::string> s_prompts;

/**
 * Thread pool for AI tasks
 */
static ThreadPool *s_aiTaskThreadPool = nullptr;

/**
 * Find object by its name or ID
 */
static shared_ptr<NetObj> FindObjectByNameOrId(const char *name)
{
   char *eptr;
   uint32_t id = strtoul(name, &eptr, 0);
   if (*eptr == 0)
   {
      shared_ptr<NetObj> object = FindObjectById(id);
      if (object != nullptr)
         return object;
   }

   wchar_t nameW[MAX_OBJECT_NAME];
   utf8_to_wchar(name, -1, nameW, MAX_OBJECT_NAME);
   nameW[MAX_OBJECT_NAME - 1] = 0;
   return FindObjectByName(nameW);
}

/**
 * Rebuild function declarations JSON object. Functions mutex must be locked before calling this function.
 */
static void RebuildFunctionDeclarations()
{
   if (s_functionDeclarations != nullptr)
      json_decref(s_functionDeclarations);

   s_functionDeclarations = json_array();
   for(const auto& pair : s_functions)
   {
      const AssistantFunction& function = *pair.second;
      json_t *tool = json_object();
      json_object_set_new(tool, "type", json_string("function"));
      json_t *functionObject = json_object();
      json_object_set_new(functionObject, "name", json_string(function.name.c_str()));
      json_object_set_new(functionObject, "description", json_string(function.description.c_str()));
      json_t *parametersObject = json_object();
      json_object_set_new(parametersObject, "type", json_string("object"));
      json_t *propObject = json_object();
      for(std::pair<std::string, std::string> p : function.parameters)
      {
         json_t *propData = json_object();
         json_object_set_new(propData, "description", json_string(p.second.c_str()));
         json_object_set_new(propData, "type", json_string("string"));
         json_object_set_new(propObject, p.first.c_str(), propData);
      }
      json_object_set_new(parametersObject, "properties", propObject);
      json_object_set_new(parametersObject, "required", json_array());
      json_object_set_new(functionObject, "parameters", parametersObject);
      json_object_set_new(tool, "function", functionObject);
      json_array_append_new(s_functionDeclarations, tool);
   }
}

/**
 * Register AI assistant function. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantFunction(const char *name, const char *description, const std::vector<std::pair<std::string, std::string>>& parameters, AssistantFunctionHandler handler)
{
   if (s_functions.find(name) != s_functions.end())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI assistant function \"%hs\" already registered", name);
      return;
   }
   s_functions.emplace(name, make_shared<AssistantFunction>(name, description, parameters, handler));
}

/**
 * Handler for AI assistant function provided by NXSL script
 */
static std::string ScriptFunctionHandler(const wchar_t *scriptName, bool objectContext, json_t *arguments, uint32_t userId)
{
   shared_ptr<NetObj> object;
   const char *objectIdStr = json_object_get_string_utf8(arguments, "object", "");
   if (objectIdStr[0] != 0)
      object = FindObjectByNameOrId(objectIdStr);

   if (objectContext && (object == nullptr))
      return std::string("Error: invalid or missing object context; valid object must be provided for this tool");

   ScriptVMHandle vm = CreateServerScriptVM(scriptName, nullptr, shared_ptr<DCObjectInfo>());
   if (!vm.isValid())
      return std::string("Error: cannot create script VM");

   ObjectRefArray<NXSL_Value> args(0, 16);
   if (arguments != nullptr)
   {
      const char *argListStr = json_object_get_string_utf8(arguments, "args", "");
      if (argListStr[0] != 0)
      {
         wchar_t *argListWStr = WideStringFromUTF8String(argListStr);
         wchar_t *p = argListWStr;
         if (!ParseValueList(vm, &p, args, false))
         {
            MemFree(argListWStr);
            return std::string("Error: cannot parse argument list");
         }
         MemFree(argListWStr);
      }
   }

   std::string output;
   if (vm->run(args))
   {
      NXSL_Value *result = vm->getResult();
      if ((result != nullptr) && !result->isNull())
      {
         char *text = UTF8StringFromWideString(result->getValueAsCString());
         output = text;
         MemFree(text);
      }
      else
      {
         output = "Script executed successfully but did not provide any output";
      }
   }
   else
   {
      output = "Script execution failed: ";
      char text[256];
      wchar_to_utf8(vm->getErrorText(), -1, text, 256);
      output.append(text);
   }
   vm.destroy();
   return output;
}

/**
 * Register AI assistant function handler provided by NXSL script
 */
void RegisterAIFunctionScriptHandler(const NXSL_LibraryScript *script, bool runtimeChange)
{
   bool objectContext = script->getMetadataEntryAsBoolean(L"object_context");

   char *toolName = StringBuffer(L"nxsl-").append(script->getName()).getUTF8String();
   for(int i = 0; toolName[i] != 0; i++)
   {
      if (toolName[i] == ':')
         toolName[i] = '_';
   }
   char *d = UTF8StringFromWideString(script->getMetadataEntry(L"description"));
   std::string description = (d != nullptr) ? d : "No description provided.";
   MemFree(d);
   std::vector<std::pair<std::string, std::string>> args;
   args.emplace_back("args", "optional argument list as a single string (description should define expected arguments)");
   if (objectContext)
   {
      args.emplace_back("object", "ID or name of NetXMS object providing context for this function");
      description.append(" This function requires object context; provide object ID or name using 'object' parameter.");
   }

   if (runtimeChange)
      s_functionsMutex.lock();

   String scriptName(script->getName());
   RegisterAIAssistantFunction(toolName, description.c_str(), args,
      [scriptName, objectContext] (json_t *arguments, uint32_t userId) -> std::string
      {
         return ScriptFunctionHandler(scriptName, objectContext, arguments, userId);
      });

   if (runtimeChange)
   {
      RebuildFunctionDeclarations();
      s_functionsMutex.unlock();
   }

   MemFree(toolName);

   nxlog_debug_tag(DEBUG_TAG, 4, L"Registered AI function handler provided by script \"%s\"", script->getName());
}

/**
 * Unregister AI assistant function handler provided by NXSL script
 */
void UnregisterAIFunctionScriptHandler(const NXSL_LibraryScript *script)
{
   char toolName[256];
   snprintf(toolName, 256, "nxsl-%ls", script->getName());
   for(int i = 0; toolName[i] != 0; i++)
   {
      if (toolName[i] == ':')
         toolName[i] = '_';
   }

   s_functionsMutex.lock();
   auto it = s_functions.find(toolName);
   if (it != s_functions.end())
   {
      s_functions.erase(it);
      nxlog_debug_tag(DEBUG_TAG, 4, L"Unregistered AI function handler from script \"%s\"", script->getName());
      RebuildFunctionDeclarations();
   }
   s_functionsMutex.unlock();
}

/**
 * Call registered function
 */
std::string NXCORE_EXPORTABLE CallAIAssistantFunction(const char *name, json_t *arguments, uint32_t userId)
{
   s_functionsMutex.lock();
   auto it = s_functions.find(name);
   if (it != s_functions.end())
   {
      shared_ptr<AssistantFunction> function = it->second;
      s_functionsMutex.unlock();
      if (json_is_string(arguments))
      {
         arguments = json_loads(json_string_value(arguments), 0, nullptr);
         std::string response = function->handler(arguments, userId);
         json_decref(arguments);
         return response;
      }
      return function->handler(arguments, userId);
   }
   s_functionsMutex.unlock();
   return std::string("Error: function not found");
}

/**
 * Fill message with registered function list
 */
void FillAIAssistantFunctionListMessage(NXCPMessage *msg)
{
   LockGuard lockGuard(s_functionsMutex);

   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(s_functions.size()));

   uint32_t baseFieldId = VID_ELEMENT_LIST_BASE;
   for(auto f : s_functions)
   {
      const AssistantFunction& function = *f.second;
      uint32_t fieldId = baseFieldId;
      msg->setFieldFromUtf8String(fieldId++, function.name.c_str());
      msg->setFieldFromUtf8String(fieldId++, function.description.c_str());
      fieldId += 7;
      msg->setField(fieldId++, static_cast<uint32_t>(function.parameters.size()));
      for(const auto& p : function.parameters)
      {
         msg->setFieldFromUtf8String(fieldId++, p.first.c_str());
         msg->setFieldFromUtf8String(fieldId++, p.second.c_str());
      }
      baseFieldId += 0x100;
   }
}

/**
 * Send request to Ollama or OpenAI server
 */
static json_t *DoApiRequest(json_t *requestData)
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

   char *postData = (requestData != nullptr) ? json_dumps(requestData, 0) : MemCopyStringA("{}");
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
      char authHeader[384];
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
         nxlog_debug_tag(DEBUG_TAG, 5, L"Error response from LLM: %d (%hs)", static_cast<int>(httpStatusCode), errorMessage);
         success = false;
      }
   }

   if (success && responseData.size() <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Empty response from LLM");
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
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Successful response from LLM: %hs"), data);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON on line %d: %hs"), error.line, error.text);
         success = false;
      }
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   MemFree(postData);
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
   nxlog_debug_tag(DEBUG_TAG, 8, L"Added message to chat: role=\"%hs\", content=\"%hs\"", role, content);
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
 * Process request to assistant (actual internal implementation)
 */
char *ProcessRequestToAIAssistantEx(const char *prompt, const char *systemPrompt, NetObj *context, json_t *eventData, GenericClientSession *session, int maxIterations)
{
   json_t *messages;
   s_chatsLock.lock();
   messages = (session != nullptr) ? s_chats.get(session->getId()) : nullptr;
   if (messages == nullptr)
   {
      messages = json_array();
      if (session != nullptr)
         s_chats.set(session->getId(), messages);
      AddMessage(messages, "system", (systemPrompt != nullptr) ? systemPrompt : s_systemPrompt);
      if (context != nullptr)
      {
         AddMessage(messages, "system", "This request is made in the context of the following object:");
         json_t *json = context->toJson();
         char *jsonText = json_dumps(json, 0);
         json_decref(json);
         AddMessage(messages, "system", jsonText);
         MemFree(jsonText);
      }
      if (eventData != nullptr)
      {
         AddMessage(messages, "system", "The following is the event data associated with this request:");
         char *jsonText = json_dumps(eventData, 0);
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

   int iterations = maxIterations;
   char *answer = nullptr;
   while(iterations-- > 0)
   {
      json_t *request = json_object();
      json_object_set_new(request, "model", json_string(s_llmModel));
      json_object_set_new(request, "stream", json_boolean(false));
      s_functionsMutex.lock();
      json_object_set(request, "tools", s_functionDeclarations);
      s_functionsMutex.unlock();
      json_object_set(request, "messages", messages);

      json_t *response = DoApiRequest(request);

      // Request should be cleared with mutex held because functions (tools) object is part of it
      s_functionsMutex.lock();
      json_decref(request);
      s_functionsMutex.unlock();

      if (response == nullptr)
      {
         if (session == nullptr)
            json_decref(messages);  // if session is null, chat is not saved - delete it now
         return nullptr;
      }

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
                     nxlog_debug_tag(DEBUG_TAG, 5, L"LLM function \"%hs\" executed, result length=%d", name, static_cast<int>(functionResult.length()));
                     nxlog_debug_tag(DEBUG_TAG, 7, L"Function result: %hs", functionResult.c_str());

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
 * Process request to assistant
 */
char NXCORE_EXPORTABLE *ProcessRequestToAIAssistant(const char *prompt, NetObj *context, GenericClientSession *session, int maxIterations)
{
   return ProcessRequestToAIAssistantEx(prompt, nullptr, context, nullptr, session, maxIterations);
}

/**
 * Process event with AI assistant
 */
void ProcessEventWithAIAssistant(Event *event, const shared_ptr<NetObj>& object, const wchar_t *instructions)
{
   char *prompt = event->expandText(instructions).getUTF8String();
   json_t *eventData = event->toJson();
   uint64_t eventId = event->getId();
   ThreadPoolExecute(s_aiTaskThreadPool,
      [prompt, object, eventData, eventId] () -> void
      {
         char *response = ProcessRequestToAIAssistantEx(prompt, s_systemPromptBackground, object.get(), eventData, nullptr, 10);
         if (response != nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"AI assistant response for event " UINT64_FMT L": %hs", eventId, response);
            MemFree(response);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"AI assistant did not return response for event " UINT64_FMT, eventId);
         }
         MemFree(prompt);
         json_decref(eventData);
      });
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

/**
 * Add custom prompt
 */
void NXCORE_EXPORTABLE AddAIAssistantPrompt(const char *text)
{
   if ((text != nullptr) && (text[0] != 0))
      s_prompts.emplace_back(text);
}

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

   NXSL_Library *scriptLibrary = GetServerScriptLibrary();
   scriptLibrary->lock();
   scriptLibrary->forEach(
      [] (const NXSL_LibraryScript *script) -> void
      {
         if (script->getMetadataEntryAsBoolean(L"ai_tool"))
            RegisterAIFunctionScriptHandler(script, false);
      });
   scriptLibrary->unlock();

   if (s_functions.empty())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("AI assistant disabled (no functions registered)"));
      return true;  // No functions registered
   }

   RegisterComponent(AI_ASSISTANT_COMPONENT);

   RegisterAIAssistantFunction(
      "register-ai-task",
      "Register new AI task for background execution. Use this function to create long running tasks that may require multiple executions to complete.",
      {
          { "description", "task description" },
          { "prompt", "initial prompt for the task" }
      },
      [] (json_t *arguments, uint32_t userId) -> std::string
      {
         const char *description = json_object_get_string_utf8(arguments, "description", nullptr);
         const char *prompt = json_object_get_string_utf8(arguments, "prompt", nullptr);
         if ((description == nullptr) || (description[0] == 0))
            return std::string("Error: task description must be provided");
         if ((prompt == nullptr) || (prompt[0] == 0))
            return std::string("Error: task prompt must be provided");

         uint32_t taskId = RegisterAITask(String(description, "utf-8"), userId, String(prompt, "utf-8"));
         return std::to_string(taskId);
      });

   RebuildFunctionDeclarations();

   InitAITasks();
   s_aiTaskThreadPool = ThreadPoolCreate(_T("AI-TASKS"),
         ConfigReadInt(_T("ThreadPool.AITasks.BaseSize"), 4),
         ConfigReadInt(_T("ThreadPool.AITasks.MaxSize"), 16));
   ThreadCreate(AITaskSchedulerThread, s_aiTaskThreadPool);

   nxlog_debug_tag(DEBUG_TAG, 2, L"LLM service URL = \"%hs\", model = \"%hs\"", s_llmServiceURL, s_llmModel);
   nxlog_debug_tag(DEBUG_TAG, 2, L"%d functions registered", static_cast<int>(s_functions.size()));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("AI assistant initialized"));
   return true;
}
