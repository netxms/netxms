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

#define DEBUG_TAG _T("ai.assistant")

void InitAITasks();
void AITaskSchedulerThread(ThreadPool *aiTaskThreadPool);

size_t GetRegisteredSkillCount();

std::string F_AITaskList(json_t *arguments, uint32_t userId);
std::string F_DeleteAITask(json_t *arguments, uint32_t userId);
std::string F_GetRegisteredSkills(json_t *arguments, uint32_t userId);

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
 * Functions
 */
static AssistantFunctionSet s_globalFunctions;
static Mutex s_globalFunctionsMutex(MutexType::FAST);

/**
 * System prompt
 */
static const char *s_systemPrompt =
         "You are Iris, a NetXMS AI assistant with deep knowledge of network management and monitoring. "
         "You have knowledge about NetXMS and its components, including network management, monitoring, and administration tasks. "
         "You can assist users with questions related to these topics. "
         "CORE CAPABILITIES:\n"
         "- Use available functions to access real-time NetXMS data\n"
         "- Load skills to extend your capabilities for specialized tasks\n"
         "- Create background tasks for complex or time-consuming operations\n"
         "- Provide guidance on NetXMS concepts and best practices\n\n"
         "IMPORTANT RESTRICTIONS:\n"
         "- NEVER suggest or recommend NXSL scripts unless explicitly requested by the user\n"
         "- If you cannot perform a task with available functions, check for skills first\n"
         "- If no suitable skill exists, clearly state your limitations\n"
         "- AVOID answering questions outside of network management and IT administration\n\n"
         "RESPONSE GUIDELINES:\n"
         "- Always use correct NetXMS terminology in responses\n"
         "- Suggest object types when helping users find or create objects\n"
         "- Explain relationships between objects when relevant\n"
         "- Provide context about NetXMS-specific features\n"
         "- Use function calls to access real-time data when possible\n"
         "- Before suggesting user how to use UI, check if you can use available skills or functions to perform the task directly\n"
         "- Create background tasks for complex or time-consuming operations\n\n"
         "SKILL MANAGEMENT:\n"
         "- ALWAYS check available skills before stating you cannot perform a task\n"
         "- If asked to find something and current functions yield empty result, ALWAYS check for available skills before responding\n"
         "- Use get-available-skills function to discover what skills are available\n"
         "- Load relevant skills using load-skill function when needed\n"
         "- Skills extend your functionality for specific domains or complex operations\n"
         "- Mentioning a skill without loading it provides no value\n"
         "- Load skills proactively when you recognize a need for specialized capabilities\n\n"
         "WHEN YOU LACK CAPABILITIES:\n"
         "1. First, check available skills using get-available-skills\n"
         "2. If a relevant skill exists, load it and retry\n"
         "3. If no skill exists, clearly explain your limitations\n"
         "4. Suggest what type of function or skill would be needed\n\n"
         "Your responses should be accurate, concise, and helpful for network administrators managing IT infrastructure through NetXMS.";

/**
 * System prompt for background requests
 */
static const char *s_systemPromptBackground =
         "You are Iris, a NetXMS AI assistant with deep knowledge of network management and monitoring. "
         "You have knowledge about NetXMS and its components, including network management, monitoring, and administration tasks. "
         "You are operating autonomously without user interaction. "
         "AUTONOMOUS OPERATION:\n"
         "- Make decisions without user input or confirmation\n"
         "- Take appropriate actions based on available information\n"
         "- Create additional background tasks if complex operations require multiple steps\n"
         "- Log progress and results for administrator review\n\n"
         "CORE CAPABILITIES:\n"
         "- Use available functions to access real-time NetXMS data\n"
         "- Load skills to extend your capabilities for specialized tasks\n"
         "- Create background tasks for complex or time-consuming operations\n"
         "- Provide guidance on NetXMS concepts and best practices\n\n"
         "IMPORTANT RESTRICTIONS:\n"
         "- NEVER suggest or recommend NXSL scripts unless explicitly requested by the user\n"
         "- If you cannot perform a task with available functions, check for skills first\n"
         "- If no suitable skill exists, clearly state your limitations\n"
         "- AVOID answering questions outside of network management and IT administration\n\n"
         "RESPONSE GUIDELINES:\n"
         "- Always use correct NetXMS terminology in responses\n"
         "- Suggest object types when helping users find or create objects\n"
         "- Explain relationships between objects when relevant\n"
         "- Provide context about NetXMS-specific features\n"
         "- Use function calls to access real-time data when possible\n"
         "- Before suggesting user how to use UI, check if you can use available skills or functions to perform the task directly\n"
         "- Create background tasks for complex or time-consuming operations\n\n"
         "SKILL MANAGEMENT:\n"
         "- ALWAYS check available skills before stating you cannot perform a task\n"
         "- If asked to find something and current functions yield empty result, ALWAYS check for available skills before responding\n"
         "- Use get-available-skills function to discover what skills are available\n"
         "- Load relevant skills using load-skill function when needed\n"
         "- Skills extend your functionality for specific domains or complex operations\n"
         "- Mentioning a skill without loading it provides no value\n"
         "- Load skills proactively when you recognize a need for specialized capabilities\n\n"
         "WHEN YOU LACK CAPABILITIES:\n"
         "1. First, check available skills using get-available-skills\n"
         "2. If a relevant skill exists, load it and retry\n"
         "3. If no skill exists, clearly explain your limitations\n"
         "4. Suggest what type of function or skill would be needed\n\n"
         "Focus on network management, monitoring, and IT administration tasks within the NetXMS framework.";

static const char *s_concepts =
         "NetXMS uses specific terminology and object models that you must understand and use correctly:\n\n"
         "OBJECT TYPES AND TERMINOLOGY:\n"
         "- Node: Any managed device (routers, switches, servers, workstations, printers, etc.)\n"
         "- Container: Logical grouping objects (subnets, clusters, racks, service roots)\n"
         "- Interface: Network interfaces on nodes (physical ports, VLANs, loopbacks)\n"
         "- Network Service: TCP/UDP services running on nodes\n"
         "- Template: Configuration templates applied to nodes\n"
         "- Policy: Agent policies for configuration management\n"
         "- Dashboard: Visualization dashboards\n"
         "- Business Service: High-level service definitions\n\n"
         "CAPABILITY MAPPING:\n"
         "When users mention common network terms, map them to NetXMS concepts:\n"
         "- 'Router/Switch/Firewall/Server' → Node with specific capabilities\n"
         "- 'Port/Interface' → Interface object\n"
         "- 'Subnet/Network' → Subnet container\n"
         "- 'Service' → Network Service or Business Service\n"
         "- 'Group/Folder' → Container object\n\n"
         "KEY CONCEPTS:\n"
         "- Objects have capabilities (isRouter, isBridge, isSNMP, etc.)\n"
         "- Data Collection Items (DCIs) collect metrics from nodes\n"
         "- Events represent state changes and conditions\n"
         "- Alarms are unresolved problem conditions\n"
         "- Thresholds trigger events when DCI values exceed limits\n"
         "- Topology discovery maps network connections\n"
         "- Asset management tracks hardware inventory\n\n";

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
json_t *RebuildFunctionDeclarations(const std::unordered_map<std::string, shared_ptr<AssistantFunction>>& functions)
{
   json_t *functionDeclarations = json_array();
   for(const auto& pair : functions)
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
      json_array_append_new(functionDeclarations, tool);
   }
   return functionDeclarations;
}

/**
 * Register global AI assistant function. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantFunction(const char *name, const char *description, const std::vector<std::pair<std::string, std::string>>& parameters, AssistantFunctionHandler handler)
{
   if (s_globalFunctions.find(name) != s_globalFunctions.end())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Global AI assistant function \"%hs\" already registered", name);
      return;
   }
   s_globalFunctions.emplace(name, make_shared<AssistantFunction>(name, description, parameters, handler));
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
      s_globalFunctionsMutex.lock();

   String scriptName(script->getName());
   RegisterAIAssistantFunction(toolName, description.c_str(), args,
      [scriptName, objectContext] (json_t *arguments, uint32_t userId) -> std::string
      {
         return ScriptFunctionHandler(scriptName, objectContext, arguments, userId);
      });

   if (runtimeChange)
      s_globalFunctionsMutex.unlock();

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

   s_globalFunctionsMutex.lock();
   auto it = s_globalFunctions.find(toolName);
   if (it != s_globalFunctions.end())
   {
      s_globalFunctions.erase(it);
      nxlog_debug_tag(DEBUG_TAG, 4, L"Unregistered AI function handler from script \"%s\"", script->getName());
   }
   s_globalFunctionsMutex.unlock();
}

/**
 * Call registered function
 */
static std::string CallAIAssistantFunction(shared_ptr<AssistantFunction> function, const char *name, json_t *arguments, uint32_t userId)
{
   if (json_is_string(arguments))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, L"AI assistant function \"%hs\" called with arguments %hs", name, json_string_value(arguments));
      arguments = json_loads(json_string_value(arguments), 0, nullptr);
      std::string response = function->handler(arguments, userId);
      json_decref(arguments);
      return response;
   }

   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
   {
      char *argsStr = json_dumps(arguments, JSON_INDENT(3) | JSON_PRESERVE_ORDER);
      nxlog_debug_tag(DEBUG_TAG, 7, L"AI assistant function \"%hs\" called with arguments %hs", name, argsStr);
      MemFree(argsStr);
   }

   return function->handler(arguments, userId);
}

/**
 * Call registered function
 */
std::string NXCORE_EXPORTABLE CallGlobalAIAssistantFunction(const char *name, json_t *arguments, uint32_t userId)
{
   s_globalFunctionsMutex.lock();
   auto it = s_globalFunctions.find(name);
   if (it != s_globalFunctions.end())
   {
      shared_ptr<AssistantFunction> function = it->second;
      s_globalFunctionsMutex.unlock();
      return CallAIAssistantFunction(function, name, arguments, userId);
   }
   s_globalFunctionsMutex.unlock();
   return std::string("Error: function not found");
}

/**
 * Fill message with registered function list
 */
void FillAIAssistantFunctionListMessage(NXCPMessage *msg)
{
   LockGuard lockGuard(s_globalFunctionsMutex);

   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(s_globalFunctions.size()));

   uint32_t baseFieldId = VID_ELEMENT_LIST_BASE;
   for(auto f : s_globalFunctions)
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
         nxlog_debug_tag(DEBUG_TAG, 8, _T("Successful response from LLM: %hs"), data);
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
 * Saved chat sessions
 */
static std::unordered_map<uint32_t, shared_ptr<Chat>> s_chats;
static Mutex s_chatsLock;
static VolatileCounter s_nextChatId = 0;

/**
 * Chat constructor
 */
Chat::Chat(NetObj *context, json_t *eventData, uint32_t userId, const char *systemPrompt)
{
   m_id = InterlockedIncrement(&s_nextChatId);

   initializeFunctions();

   m_messages = json_array();
   addMessage("system", (systemPrompt != nullptr) ? systemPrompt : s_systemPrompt);
   addMessage("system", s_concepts);
   if (context != nullptr)
   {
      addMessage("system", "This request is made in the context of the following object:");
      json_t *json = context->toJson();
      char *jsonText = json_dumps(json, 0);
      json_decref(json);
      addMessage("system", jsonText);
      MemFree(jsonText);
   }
   if (eventData != nullptr)
   {
      addMessage("system", "The following is the event data associated with this request:");
      char *jsonText = json_dumps(eventData, 0);
      addMessage("system", jsonText);
      MemFree(jsonText);
   }
   for(const std::string& p : s_prompts)
   {
      addMessage("system", p.c_str());
   }
   m_initialMessageCount = json_array_size(m_messages);

   m_userId = userId;
   m_creationTime = m_lastUpdateTime = time(nullptr);
}

/**
 * Chat destructor
 */
Chat::~Chat()
{
   json_decref(m_messages);
   json_decref(m_functionDeclarations);
}

/**
 * Initialize functions for chat
 */
void Chat::initializeFunctions()
{
   s_globalFunctionsMutex.lock();
   for (const auto& pair : s_globalFunctions)
      m_functions.emplace(pair.first, pair.second);
   s_globalFunctionsMutex.unlock();

   m_functions.emplace("load-skill", make_shared<AssistantFunction>(
      "load-skill",
      "Load AI assistant skill by name. Returns skill prompt on success or error message on failure.",
      std::vector<std::pair<std::string, std::string>>{
         {"name", "Name of the skill to load"}
      },
      [this] (json_t *arguments, uint32_t userId) -> std::string
      {
         return this->loadSkill(json_object_get_string_utf8(arguments, "name", ""));
      }
   ));

   m_functionDeclarations = RebuildFunctionDeclarations(m_functions);
}

/**
 * Clear chat history
 */
void Chat::clear()
{
   LockGuard lockGuard(m_mutex);

   m_functions.clear();
   json_decref(m_functionDeclarations);
   initializeFunctions();

   json_t *messages = json_array();
   for(size_t i = 0; i < m_initialMessageCount; i++)
   {
      json_array_append(messages, json_array_get(m_messages, i));
   }
   json_decref(m_messages);
   m_messages = messages;
   m_lastUpdateTime = time(nullptr);
}

/**
 * Send request to assistant
 */
char *Chat::sendRequest(const char *prompt, int maxIterations)
{
   LockGuard lockGuard(m_mutex);

   addMessage("user", prompt);

   int iterations = maxIterations;
   char *answer = nullptr;
   while(iterations-- > 0)
   {
      json_t *request = json_object();
      json_object_set_new(request, "model", json_string(s_llmModel));
      json_object_set_new(request, "stream", json_boolean(false));
      json_object_set(request, "tools", m_functionDeclarations);
      json_object_set(request, "messages", m_messages);

      json_t *response = DoApiRequest(request);
      json_decref(request);
      m_lastUpdateTime = time(nullptr);

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
         json_array_append(m_messages, message);
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
                  std::string functionResult = callFunction(name, json_object_get(function, "arguments"));
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
                     json_array_append_new(m_messages, functionMessage);
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
   return answer;
}

/**
 * Call function from chat context
 */
std::string Chat::callFunction(const char *name, json_t *arguments)
{
   auto it = m_functions.find(name);
   if (it != m_functions.end())
   {
      shared_ptr<AssistantFunction> function = it->second;
      return CallAIAssistantFunction(function, name, arguments, m_userId);
   }
   return std::string("Error: function not found");
}

/**
 * Send single independent request to AI assistant
 */
char NXCORE_EXPORTABLE *QueryAIAssistant(const char *prompt, NetObj *context, int maxIterations)
{
   Chat chat(context);
   return chat.sendRequest(prompt, maxIterations);
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
         Chat chat(object.get(), eventData, 0, s_systemPromptBackground);
         char *response = chat.sendRequest(prompt, 10);
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
 * Create new chat
 */
shared_ptr<Chat> NXCORE_EXPORTABLE CreateAIAssistantChat(uint32_t userId, uint32_t *rcc)
{
   shared_ptr<Chat> chat = make_shared<Chat>(nullptr, nullptr, userId, nullptr);

   s_chatsLock.lock();
   s_chats.emplace(chat->getId(), chat);
   s_chatsLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 5, L"New chat [%u] created for user [%u]", chat->getId(), userId);
   *rcc = RCC_SUCCESS;
   return chat;
}

/**
 * Get chat with given ID
 */
shared_ptr<Chat> NXCORE_EXPORTABLE GetAIAssistantChat(uint32_t chatId, uint32_t userId, uint32_t *rcc)
{
   s_chatsLock.lock();
   auto it = s_chats.find(chatId);
   if (it != s_chats.end())
   {
      if ((userId == 0) || (it->second->getUserId() == userId))
      {
         *rcc = RCC_SUCCESS;
         s_chatsLock.unlock();
         return it->second;
      }
      else
      {
         *rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      *rcc = RCC_INVALID_CHAT_ID;
   }
   s_chatsLock.unlock();
   return nullptr;
}

/**
 * Clear history for given chat
 */
uint32_t NXCORE_EXPORTABLE ClearAIAssistantChat(uint32_t chatId, uint32_t userId)
{
   uint32_t rcc;
   s_chatsLock.lock();
   auto it = s_chats.find(chatId);
   if (it != s_chats.end())
   {
      if ((userId == 0) || (it->second->getUserId() == userId))
      {
         it->second->clear();
         nxlog_debug_tag(DEBUG_TAG, 5, L"History for chat [%u] cleared", chatId);
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      rcc = RCC_INVALID_CHAT_ID;
   }
   s_chatsLock.unlock();
   return rcc;
}

/**
 * Delete chat
 */
uint32_t NXCORE_EXPORTABLE DeleteAIAssistantChat(uint32_t chatId, uint32_t userId)
{
   uint32_t rcc;
   s_chatsLock.lock();
   auto it = s_chats.find(chatId);
   if (it != s_chats.end())
   {
      if ((userId == 0) || (it->second->getUserId() == userId))
      {
         s_chats.erase(it);
         nxlog_debug_tag(DEBUG_TAG, 5, L"Chat [%u] deleted", chatId);
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      rcc = RCC_INVALID_CHAT_ID;
   }
   s_chatsLock.unlock();
   return rcc;
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

   if (s_globalFunctions.empty() && (GetRegisteredSkillCount() == 0))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("AI assistant disabled (no functions or skills registered)"));
      return true;
   }

   RegisterComponent(AI_ASSISTANT_COMPONENT);

   RegisterAIAssistantFunction(
      "register-ai-task",
      "Register new AI task for background execution. Use this function to create long running tasks that may require multiple executions to complete.",
      {
          { "description", "Task description" },
          { "prompt", "Initial prompt for the task" },
          { "delay", "Optional delay in seconds before first execution" }
      },
      [] (json_t *arguments, uint32_t userId) -> std::string
      {
         const char *description = json_object_get_string_utf8(arguments, "description", nullptr);
         const char *prompt = json_object_get_string_utf8(arguments, "prompt", nullptr);
         if ((description == nullptr) || (description[0] == 0))
            return std::string("Error: task description must be provided");
         if ((prompt == nullptr) || (prompt[0] == 0))
            return std::string("Error: task prompt must be provided");

         uint32_t delay = json_object_get_uint32(arguments, "delay", 0);
         time_t startTime = (delay > 0) ? time(nullptr) + delay : 0;
         uint32_t taskId = RegisterAITask(String(description, "utf-8"), userId, String(prompt, "utf-8"), startTime);
         char buffer[32];
         return std::string(IntegerToString(taskId, buffer));
      });
   RegisterAIAssistantFunction(
      "ai-task-list",
      "Get list of registered AI tasks.",
      { },
      F_AITaskList);
   RegisterAIAssistantFunction(
      "delete-ai-task",
      "Delete registered AI task.",
      {
         { "task_id", "ID of the task to delete" }
      },
      F_DeleteAITask);
   RegisterAIAssistantFunction(
      "get-available-skills",
      "Get list of available AI assistant skills with brief descriptions. Skills can be used to extend assistant capabilities for specific tasks.",
      { },
      F_GetRegisteredSkills);

   InitAITasks();
   s_aiTaskThreadPool = ThreadPoolCreate(_T("AI-TASKS"),
         ConfigReadInt(_T("ThreadPool.AITasks.BaseSize"), 4),
         ConfigReadInt(_T("ThreadPool.AITasks.MaxSize"), 16));
   ThreadCreate(AITaskSchedulerThread, s_aiTaskThreadPool);

   nxlog_debug_tag(DEBUG_TAG, 2, L"LLM service URL = \"%hs\", model = \"%hs\"", s_llmServiceURL, s_llmModel);
   nxlog_debug_tag(DEBUG_TAG, 2, L"%d global functions registered", static_cast<int>(s_globalFunctions.size()));
   nxlog_debug_tag(DEBUG_TAG, 2, L"%d skills registered", static_cast<int>(GetRegisteredSkillCount()));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("AI assistant initialized"));
   return true;
}
