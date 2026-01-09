/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
#include <nms_incident.h>
#include <ai_messages.h>

#define DEBUG_TAG _T("ai.assistant")

void InitAITasks();
void AITaskSchedulerThread(ThreadPool *aiTaskThreadPool);

size_t GetRegisteredSkillCount();
std::string GetRegisteredSkills();

std::string F_AITaskList(json_t *arguments, uint32_t userId);
std::string F_DeleteAITask(json_t *arguments, uint32_t userId);

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
         "BACKGROUND WORK OPTIONS:\n"
         "Choose the right approach based on user intent:\n"
         "- AI Task (register-ai-task): For autonomous multi-step work that needs state preservation between iterations. "
         "Use when user wants you to 'work through', 'investigate over time', 'take multiple passes', or process large volumes autonomously. "
         "AI tasks can signal completion and preserve context via memento.\n"
         "- Scheduled Task: For time-based recurring operations (daily checks, weekly reports). Use when user specifies a schedule like 'every day', 'weekly', 'at 9am'.\n"
         "- Foreground (current chat): For immediate analysis the user wants to see and interact with now.\n\n"
         "IMPORTANT RESTRICTIONS:\n"
         "- NEVER suggest or recommend NXSL scripts unless explicitly requested by the user\n"
         "- AVOID answering questions outside of network management and IT administration\n\n"
         "RESPONSE GUIDELINES:\n"
         "- Always use correct NetXMS terminology in responses\n"
         "- Suggest object types when helping users find or create objects\n"
         "- Explain relationships between objects when relevant\n"
         "- Provide context about NetXMS-specific features\n"
         "- Use function calls to access real-time data when possible\n"
         "- Before suggesting user how to use UI, check if you can use available skills or functions to perform the task directly\n"
         "- Create background tasks for complex or time-consuming operations\n\n"
         "TIMESTAMP FORMATS:\n"
         "All functions accepting timestamp arguments support the following formats:\n"
         "- Relative: [+|-]<number>[s|m|h|d] where s=seconds, m=minutes, h=hours, d=days (no suffix defaults to minutes)\n"
         "  Examples: -30m (30 minutes ago), +2h (2 hours in future), -1d (1 day ago), -30 (30 minutes ago)\n"
         "- UNIX timestamp: numeric value representing seconds since epoch (e.g., 1704067200)\n"
         "- ISO 8601: YYYY-MM-DDTHH:MM:SSZ in UTC (e.g., 2024-01-01T12:00:00Z)\n\n"
         "SKILL MANAGEMENT AND DISCOVERY:\n"
         "- When you lack capabilities for ANY user request, IMMEDIATELY check available skills using get-available-skills\n"
         "- Skills are your primary method for extending capabilities - they are NOT optional extras\n"
         "- NEVER state limitations without first checking and attempting to load relevant skills\n"
         "- If current functions cannot fulfill a request, this is a signal to check skills, not to give up\n"
         "- Load skills proactively based on request context, not reactively after failure\n"
         "- When functions return empty results, check skills before concluding the task cannot be completed\n"
         "- Skills can provide entirely new function sets - always explore them when facing limitations\n"
         "- If a user request mentions any domain or task type, check if skills exist for that domain\n"
         "- Pattern: Request → Check Skills → Load Relevant Skills → Attempt Task → Report Results\n"
         "- If user request matches an available skill's description, automatically invoke load-skill for that skill before generating any other response\n"
         "- Use get-available-skills function to discover what skills are available\n"
         "- Load relevant skills using load-skill function when needed\n"
         "- Skills extend your functionality for specific domains or complex operations\n"
         "- Mentioning a skill without loading it provides no value\n"
         "- Load skills proactively when you recognize a need for specialized capabilities\n\n"
         "LIMITATION REPORTING:\n"
         "- Only report inability to perform tasks AFTER checking and attempting relevant skills\n"
         "- When reporting limitations, always mention which skills were checked\n"
         "- Suggest what type of skill would be needed for unsupported requests\n"
         "- NEVER conclude you cannot perform a task without first exploring available skills\n\n"
         "Your responses should be accurate, concise, and helpful for network administrators managing IT infrastructure through NetXMS.";

/**
 * System prompt for background requests
 */
static const char *s_systemPromptBackground =
         "You are an AI agent integrated with NetXMS, a network management system. "
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
         "- AVOID answering questions outside of network management and IT administration\n\n"
         "RESPONSE GUIDELINES:\n"
         "- Always use correct NetXMS terminology in responses\n"
         "- Suggest object types when helping users find or create objects\n"
         "- Explain relationships between objects when relevant\n"
         "- Provide context about NetXMS-specific features\n"
         "- Use function calls to access real-time data when possible\n"
         "- Before suggesting user how to use UI, check if you can use available skills or functions to perform the task directly\n"
         "- Create background tasks for complex or time-consuming operations\n\n"
         "TIMESTAMP FORMATS:\n"
         "All functions accepting timestamp arguments support the following formats:\n"
         "- Relative: [+|-]<number>[s|m|h|d] where s=seconds, m=minutes, h=hours, d=days (no suffix defaults to minutes)\n"
         "  Examples: -30m (30 minutes ago), +2h (2 hours in future), -1d (1 day ago), -30 (30 minutes ago)\n"
         "- UNIX timestamp: numeric value representing seconds since epoch (e.g., 1704067200)\n"
         "- ISO 8601: YYYY-MM-DDTHH:MM:SSZ in UTC (e.g., 2024-01-01T12:00:00Z)\n\n"
         "SKILL MANAGEMENT AND DISCOVERY:\n"
         "- When you lack capabilities for ANY task, IMMEDIATELY check available skills using get-available-skills\n"
         "- Skills are your primary method for extending capabilities - they are NOT optional extras\n"
         "- NEVER state limitations without first checking and attempting to load relevant skills\n"
         "- If current functions cannot fulfill a task, this is a signal to check skills, not to give up\n"
         "- Load skills proactively based on task context, not reactively after failure\n"
         "- When functions return empty results, check skills before concluding the task cannot be completed\n"
         "- Skills can provide entirely new function sets - always explore them when facing limitations\n"
         "- Pattern: Task → Check Skills → Load Relevant Skills → Attempt Task → Report Results\n"
         "- If task matches an available skill's description, automatically invoke load-skill for that skill before generating any other response\n"
         "- Use get-available-skills function to discover what skills are available\n"
         "- Load relevant skills using load-skill function when needed\n"
         "- Skills extend your functionality for specific domains or complex operations\n"
         "- Mentioning a skill without loading it provides no value\n"
         "- Load skills proactively when you recognize a need for specialized capabilities\n\n"
         "LIMITATION REPORTING:\n"
         "- Only report inability to perform tasks AFTER checking and attempting relevant skills\n"
         "- When reporting limitations, always mention which skills were checked\n"
         "- Suggest what type of skill would be needed for unsupported tasks\n"
         "- NEVER conclude you cannot perform a task without first exploring available skills\n\n"
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
 * Find object by its (possibly fuzzy) name or ID
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByNameOrId(const char *name, int objectClassHint)
{
   char *eptr;
   uint32_t id = strtoul(name, &eptr, 0);
   if (*eptr == 0)
   {
      shared_ptr<NetObj> object = FindObjectById(id, objectClassHint);
      if (object != nullptr)
         return object;
   }

   wchar_t nameW[MAX_OBJECT_NAME];
   utf8_to_wchar(name, -1, nameW, MAX_OBJECT_NAME);
   nameW[MAX_OBJECT_NAME - 1] = 0;
   return FindObjectByFuzzyName(nameW, objectClassHint);
}

/**
 * Parse timestamp from string. Supports absolute timestamps in ISO 8601 format or as UNIX timestamp,
 * as well as relative timestamps in format [+|-]<number>[s|m|h|d] or word "now".
 */
time_t NXCORE_EXPORTABLE ParseTimestamp(const char *ts)
{
   char *eptr;
   if ((ts[0] == '-') || (ts[0] == '+'))
   {
      // Offset from now
      int64_t offset = strtoll(&ts[1], &eptr, 10);
      if (*eptr != 0)
      {
         // check for suffix
         if (stricmp(eptr, "m") == 0)
            offset *= 60;
         else if (stricmp(eptr, "h") == 0)
            offset *= 3600;
         else if (stricmp(eptr, "d") == 0)
            offset *= 86400;
         else if (stricmp(eptr, "s") != 0)
            return 0;  // invalid format
      }
      else
      {
         // no suffix, assume minutes
         offset *= 60;
      }
      time_t now = time(nullptr);
      return (ts[0] == '+') ? now + static_cast<time_t>(offset) : now - static_cast<time_t>(offset);
   }

   if (!stricmp(ts, "now"))
      return time(nullptr);

   int64_t n = strtoll(ts, &eptr, 10);
   if (*eptr == 0)
      return static_cast<time_t>(n);   // Assume UNIX timestamp

   struct tm t;
   if (strptime(ts, "%Y-%m-%dT%H:%M:%SZ", &t) == nullptr)
      return 0;

   return timegm(&t);
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
static VolatileCounter64 s_nextQuestionId = 0;

/**
 * Chat constructor
 */
Chat::Chat(NetObj *context, json_t *eventData, uint32_t userId, const char *systemPrompt)
{
   m_id = InterlockedIncrement(&s_nextChatId);
   m_boundIncidentId = 0;
   m_pendingQuestion = nullptr;

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
   addMessage("system", std::string("The following skills are available to you: ").append(GetRegisteredSkills()).c_str());
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
   delete m_pendingQuestion;
}

/**
 * Bind chat to an incident
 */
void Chat::bindToIncident(uint32_t incidentId)
{
   LockGuard lockGuard(m_mutex);

   if (m_boundIncidentId != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat [%u] already bound to incident [%u]"), m_id, m_boundIncidentId);
      return;
   }

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat [%u]: cannot bind to incident [%u] - incident not found"), m_id, incidentId);
      return;
   }

   m_boundIncidentId = incidentId;

   // Add incident context as system message
   addMessage("system", "This chat session is bound to an incident for discussion and analysis. Here is the incident context:");
   json_t *json = incident->toJson();
   char *jsonText = json_dumps(json, 0);
   json_decref(json);
   addMessage("system", jsonText);
   MemFree(jsonText);

   // Add source object context if available
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if (sourceObject != nullptr)
   {
      addMessage("system", "The incident source object context:");
      json_t *objJson = sourceObject->toJson();
      char *objJsonText = json_dumps(objJson, 0);
      json_decref(objJson);
      addMessage("system", objJsonText);
      MemFree(objJsonText);
   }

   // Add incident-specific guidance
   char guidance[512];
   snprintf(guidance, sizeof(guidance),
      "You are discussing incident #%u. Help the user understand the incident, analyze linked alarms, "
      "suggest remediation steps, and answer questions about the incident context. "
      "You can use incident-related functions to get more details or take actions on this incident.",
      incidentId);
   addMessage("system", guidance);

   // Auto-load incident-analysis skill
   loadSkill("incident-analysis");

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Chat [%u] bound to incident [%u]"), m_id, incidentId);
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

   m_functions.emplace("ask-user-confirmation", make_shared<AssistantFunction>(
      "ask-user-confirmation",
      "Ask user a yes/no confirmation question. Blocks until user responds or timeout (5 minutes). Use for approval workflows or binary decisions.",
      std::vector<std::pair<std::string, std::string>>{
         {"intent", "Explanation of why this action is needed and what it accomplishes. Do not put actual command or action here, use 'action' parameter for that."},
         {"action", "Optional: The specific action to be approved (command, script code, configuration change, etc.). Do not add any additional text, just the action itself."},
         {"confirmation_type", "Type of confirmation: 'approve_reject', 'yes_no', or 'confirm_cancel'. Defaults to 'approve_reject'"}
      },
      [this] (json_t *arguments, uint32_t userId) -> std::string
      {
         const char *text = json_object_get_string_utf8(arguments, "intent", "");
         if (*text == 0)
            return R"({"error": "Intent is required"})";

         const char *context = json_object_get_string_utf8(arguments, "action", "");
         const char *typeStr = json_object_get_string_utf8(arguments, "confirmation_type", "approve_reject");

         ConfirmationType type = ConfirmationType::APPROVE_REJECT;
         if (!stricmp(typeStr, "yes_no"))
            type = ConfirmationType::YES_NO;
         else if (!stricmp(typeStr, "confirm_cancel"))
            type = ConfirmationType::CONFIRM_CANCEL;

#ifdef UNICODE
         WCHAR *wtext = WideStringFromUTF8String(text);
         WCHAR *wcontext = WideStringFromUTF8String(context);
         bool result = this->askConfirmation(wtext, wcontext, type);
         MemFree(wtext);
         MemFree(wcontext);
#else
         bool result = this->askConfirmation(text, context, type);
#endif

         return result ? R"({"approved": true})" : R"({"approved": false})";
      }
   ));

   m_functions.emplace("ask-user-choice", make_shared<AssistantFunction>(
      "ask-user-choice",
      "Ask user to choose from multiple options. Blocks until user responds or timeout (5 minutes). Use when user needs to select from a list of choices.",
      std::vector<std::pair<std::string, std::string>>{
         {"question", "The question to display to the user"},
         {"context", "Optional additional context for the question"},
         {"options", "Array of option strings for the user to choose from"}
      },
      [this] (json_t *arguments, uint32_t userId) -> std::string
      {
         const char *text = json_object_get_string_utf8(arguments, "question", "");
         if (*text == 0)
            return R"({"error": "Question is required"})";

         json_t *optionsArray = json_object_get(arguments, "options");
         if (!json_is_array(optionsArray) || json_array_size(optionsArray) == 0)
            return R"({"error": "Options array is required and must not be empty"})";

         StringList options;
         size_t i;
         json_t *value;
         json_array_foreach(optionsArray, i, value)
         {
            if (json_is_string(value))
            {
#ifdef UNICODE
               WCHAR *woption = WideStringFromUTF8String(json_string_value(value));
               options.add(woption);
               MemFree(woption);
#else
               options.add(json_string_value(value));
#endif
            }
         }

         if (options.isEmpty())
            return R"({"error": "Options array must contain at least one string"})";

         const char *context = json_object_get_string_utf8(arguments, "context", "");

#ifdef UNICODE
         WCHAR *wtext = WideStringFromUTF8String(text);
         WCHAR *wcontext = WideStringFromUTF8String(context);
         int result = this->askMultipleChoice(wtext, wcontext, options);
         MemFree(wtext);
         MemFree(wcontext);
#else
         int result = this->askMultipleChoice(text, context, options);
#endif

         char buffer[64];
         snprintf(buffer, sizeof(buffer), R"({"selected": %d})", result);
         return std::string(buffer);
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
char *Chat::sendRequest(const char *prompt, int maxIterations, const char *context)
{
   LockGuard lockGuard(m_mutex);

   if (context != nullptr && context[0] != 0)
   {
      std::string contextPrompt = "Additional context for this and following requests: ";
      contextPrompt.append(context);
      addMessage("user", contextPrompt.c_str());
   }
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
 * Send question to user
 */
static void SendQuestionToUser(uint32_t userId, uint32_t chatId, const PendingQuestion *question)
{
   NXCPMessage msg(CMD_AI_AGENT_QUESTION, 0);
   msg.setField(VID_CHAT_ID, chatId);
   msg.setField(VID_AI_QUESTION_ID, question->id);
   msg.setField(VID_AI_QUESTION_TYPE, question->isMultipleChoice ? static_cast<uint16_t>(1) : static_cast<uint16_t>(0));
   msg.setField(VID_AI_CONFIRMATION_TYPE, static_cast<uint16_t>(question->confirmationType));
   msg.setField(VID_AI_QUESTION_TEXT, question->text);
   msg.setField(VID_AI_QUESTION_CONTEXT, question->context);
   msg.setField(VID_AI_QUESTION_TIMEOUT, static_cast<uint32_t>(question->expiresAt - time(nullptr)));
   if (question->isMultipleChoice)
   {
      msg.setField(VID_AI_QUESTION_OPTIONS, question->options);
   }

   EnumerateClientSessions(
      [userId, &msg] (ClientSession *session) -> void
      {
         if (session->isAuthenticated() && (session->getUserId() == userId))
            session->postMessage(msg);
      });
}

/**
 * Ask confirmation question and wait for response
 */
bool Chat::askConfirmation(const TCHAR *text, const TCHAR *context, ConfirmationType type, uint32_t timeout)
{
   m_questionMutex.lock();

   // Only one question at a time per chat
   if (m_pendingQuestion != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat [%u]: cannot ask confirmation, another question is pending"), m_id);
      m_questionMutex.unlock();
      return false;
   }

   m_pendingQuestion = new PendingQuestion();
   m_pendingQuestion->id = InterlockedIncrement64(&s_nextQuestionId);
   m_pendingQuestion->isMultipleChoice = false;
   m_pendingQuestion->confirmationType = type;
   m_pendingQuestion->text = text;
   m_pendingQuestion->context = (context != nullptr) ? context : _T("");
   m_pendingQuestion->expiresAt = time(nullptr) + timeout;
   m_pendingQuestion->responded = false;
   m_pendingQuestion->positiveResponse = false;
   m_pendingQuestion->selectedOption = -1;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Chat [%u]: asking confirmation question [") UINT64_FMT _T("] to user [%u]: %s"),
      m_id, m_pendingQuestion->id, m_userId, text);

   SendQuestionToUser(m_userId, m_id, m_pendingQuestion);

   // Wait for response with timeout
   m_questionMutex.unlock();
   bool gotResponse = m_pendingQuestion->responseReceived.wait(timeout * 1000);
   m_questionMutex.lock();

   bool result = false;
   if (gotResponse && m_pendingQuestion->responded)
   {
      result = m_pendingQuestion->positiveResponse;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Chat [%u]: confirmation question [") UINT64_FMT _T("] answered: %s"),
         m_id, m_pendingQuestion->id, result ? _T("positive") : _T("negative"));
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Chat [%u]: confirmation question [") UINT64_FMT _T("] timed out"),
         m_id, m_pendingQuestion->id);
      result = false;  // Timeout = negative response
   }

   delete_and_null(m_pendingQuestion);
   m_questionMutex.unlock();
   return result;
}

/**
 * Ask multiple choice question and wait for response
 */
int Chat::askMultipleChoice(const TCHAR *text, const TCHAR *context, const StringList &options, uint32_t timeout)
{
   m_questionMutex.lock();

   // Only one question at a time per chat
   if (m_pendingQuestion != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat [%u]: cannot ask multiple choice, another question is pending"), m_id);
      m_questionMutex.unlock();
      return -1;
   }

   m_pendingQuestion = new PendingQuestion();
   m_pendingQuestion->id = InterlockedIncrement64(&s_nextQuestionId);
   m_pendingQuestion->isMultipleChoice = true;
   m_pendingQuestion->confirmationType = ConfirmationType::YES_NO;  // Not used for multiple choice
   m_pendingQuestion->text = text;
   m_pendingQuestion->context = (context != nullptr) ? context : _T("");
   m_pendingQuestion->options.addAll(options);
   m_pendingQuestion->expiresAt = time(nullptr) + timeout;
   m_pendingQuestion->responded = false;
   m_pendingQuestion->positiveResponse = false;
   m_pendingQuestion->selectedOption = -1;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Chat [%u]: asking multiple choice question [") UINT64_FMT _T("] to user [%u]: %s"),
      m_id, m_pendingQuestion->id, m_userId, text);

   SendQuestionToUser(m_userId, m_id, m_pendingQuestion);

   // Wait for response with timeout
   m_questionMutex.unlock();
   bool gotResponse = m_pendingQuestion->responseReceived.wait(timeout * 1000);
   m_questionMutex.lock();

   int result = -1;
   if (gotResponse && m_pendingQuestion->responded)
   {
      result = m_pendingQuestion->selectedOption;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Chat [%u]: multiple choice question [") UINT64_FMT _T("] answered: option %d"),
         m_id, m_pendingQuestion->id, result);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Chat [%u]: multiple choice question [") UINT64_FMT _T("] timed out"),
         m_id, m_pendingQuestion->id);
      result = -1;  // Timeout = no selection
   }

   delete_and_null(m_pendingQuestion);
   m_questionMutex.unlock();
   return result;
}

/**
 * Handle response to pending question
 */
void Chat::handleQuestionResponse(uint64_t questionId, bool positive, int selectedOption)
{
   LockGuard lockGuard(m_questionMutex);

   if (m_pendingQuestion == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat [%u]: received response for question [") UINT64_FMT _T("] but no question is pending"),
         m_id, questionId);
      return;
   }

   if (m_pendingQuestion->id != questionId)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Chat [%u]: received response for question [") UINT64_FMT _T("] but pending question is [") UINT64_FMT _T("]"),
         m_id, questionId, m_pendingQuestion->id);
      return;
   }

   m_pendingQuestion->responded = true;
   m_pendingQuestion->positiveResponse = positive;
   m_pendingQuestion->selectedOption = selectedOption;
   m_pendingQuestion->responseReceived.set();

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Chat [%u]: question [") UINT64_FMT _T("] response received: positive=%s, option=%d"),
      m_id, questionId, BooleanToString(positive), selectedOption);
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
shared_ptr<Chat> NXCORE_EXPORTABLE CreateAIAssistantChat(uint32_t userId, uint32_t incidentId, uint32_t *rcc)
{
   // Validate incident if specified
   if (incidentId != 0)
   {
      shared_ptr<Incident> incident = FindIncidentById(incidentId);
      if (incident == nullptr)
      {
         *rcc = RCC_INVALID_INCIDENT_ID;
         return nullptr;
      }
   }

   shared_ptr<Chat> chat = make_shared<Chat>(nullptr, nullptr, userId, nullptr);

   // Bind to incident if specified
   if (incidentId != 0)
   {
      chat->bindToIncident(incidentId);
   }

   s_chatsLock.lock();
   s_chats.emplace(chat->getId(), chat);
   s_chatsLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 5, _T("New chat [%u] created for user [%u], bound to incident [%u]"),
      chat->getId(), userId, incidentId);
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
   shared_ptr<Chat> chat;
   uint32_t rcc;
   s_chatsLock.lock();
   auto it = s_chats.find(chatId);
   if (it != s_chats.end())
   {
      if ((userId == 0) || (it->second->getUserId() == userId))
      {
         chat = it->second;
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

   // Clear chat history outside of lock to avoid long blocking of other operations
   if (chat != nullptr)
   {
      chat->clear();
      nxlog_debug_tag(DEBUG_TAG, 5, L"Chat [%u] history cleared", chatId);
   }
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
 * Add custom prompt from file
 * Path is relative to NetXMS shared data prompts directory
 */
void NXCORE_EXPORTABLE AddAIAssistantPromptFromFile(const wchar_t *fileName)
{
   wchar_t path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   wcslcat(path, FS_PATH_SEPARATOR L"prompts" FS_PATH_SEPARATOR, MAX_PATH);
   wcslcat(path, fileName, MAX_PATH);
   char *prompt = LoadFileAsUTF8String(path);
   if (prompt != nullptr)
   {
      AddAIAssistantPrompt(prompt);
      MemFree(prompt);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Cannot load AI assistant prompt file \"%s\"", path);
   }
}

/**
 * Write log message
 */
static std::string F_WriteLogMessage(json_t *arguments, uint32_t userId)
{
   const char *message = json_object_get_string_utf8(arguments, "message", nullptr);
   if ((message == nullptr) || (message[0] == 0))
      return std::string("Log message must be provided");

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"AI Assistant: %hs", message);
   return std::string("Log message written");
}

/**
 * Task handler for scheduled AI agent task execution
 */
static void ExecuteAIAgentTask(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   char *prompt = UTF8StringFromWideString(parameters->m_persistentData);
   shared_ptr<NetObj> context = FindObjectById(parameters->m_objectId);
   Chat chat(context.get(), nullptr, parameters->m_userId, s_systemPromptBackground);
   char *response = chat.sendRequest(prompt, 64);
   nxlog_debug_tag(DEBUG_TAG, 4, L"AI assistant response for scheduled task on object [%u]: %hs",
         parameters->m_objectId, (response != nullptr) ? response : "no response");
   MemFree(prompt);
   MemFree(response);
}

/**
 * Build incident analysis prompt based on depth level
 */
static std::string BuildIncidentAnalysisPrompt(uint32_t incidentId, int depth, const TCHAR *customPrompt)
{
   StringBuffer prompt;
   prompt.append(_T("Analyze incident #"));
   prompt.append(incidentId);
   prompt.append(_T(" using the available incident analysis functions.\n\n"));

   switch(depth)
   {
      case 0:  // Quick analysis
         prompt.append(_T("ANALYSIS DEPTH: Quick\n"));
         prompt.append(_T("Perform a brief analysis:\n"));
         prompt.append(_T("1. Get incident details using get-incident-details\n"));
         prompt.append(_T("2. Identify the primary issue from linked alarms\n"));
         prompt.append(_T("3. Suggest immediate actions if any\n"));
         break;
      case 1:  // Standard analysis
         prompt.append(_T("ANALYSIS DEPTH: Standard\n"));
         prompt.append(_T("Perform a comprehensive analysis:\n"));
         prompt.append(_T("1. Get incident details using get-incident-details\n"));
         prompt.append(_T("2. Get related events using get-incident-related-events\n"));
         prompt.append(_T("3. Check for similar historical incidents using get-incident-history\n"));
         prompt.append(_T("4. Analyze patterns and correlations\n"));
         prompt.append(_T("5. Suggest remediation steps\n"));
         break;
      case 2:  // Thorough analysis
      default:
         prompt.append(_T("ANALYSIS DEPTH: Thorough\n"));
         prompt.append(_T("Perform an in-depth analysis:\n"));
         prompt.append(_T("1. Get incident details using get-incident-details\n"));
         prompt.append(_T("2. Get related events using get-incident-related-events with extended time window\n"));
         prompt.append(_T("3. Analyze topology context using get-incident-topology-context\n"));
         prompt.append(_T("4. Review historical incidents using get-incident-history\n"));
         prompt.append(_T("5. Look for related alarms that should be linked\n"));
         prompt.append(_T("6. Perform root cause analysis\n"));
         prompt.append(_T("7. Provide detailed remediation recommendations\n"));
         break;
   }

   if ((customPrompt != nullptr) && (*customPrompt != 0))
   {
      prompt.append(_T("\n\nADDITIONAL INSTRUCTIONS:\n"));
      prompt.append(customPrompt);
   }

   prompt.append(_T("\n\nOUTPUT REQUIREMENTS:\n"));
   prompt.append(_T("- Provide your analysis as a structured report\n"));
   prompt.append(_T("- Include a summary section at the beginning\n"));
   prompt.append(_T("- List any correlated alarms that should be linked\n"));
   prompt.append(_T("- Suggest an assignee if appropriate using suggest-incident-assignee\n"));
   prompt.append(_T("- The analysis will be posted as an incident comment\n"));

   return prompt.getUTF8StdString();
}

/**
 * Background task for incident AI analysis
 */
static void IncidentAIAnalysisTask(uint32_t incidentId, int depth, bool autoAssign, String customPrompt)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Starting AI analysis for incident [%u], depth=%d, autoAssign=%s"),
      incidentId, depth, BooleanToString(autoAssign));

   // Get incident to verify it exists and get source object
   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AI analysis cancelled: incident [%u] not found"), incidentId);
      return;
   }

   // Get source object for context
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());

   // Build the analysis prompt
   std::string prompt = BuildIncidentAnalysisPrompt(incidentId, depth, customPrompt.cstr());

   // Create chat and execute analysis
   Chat chat(sourceObject.get(), nullptr, 0, s_systemPromptBackground);
   char *response = chat.sendRequest(prompt.c_str(), 32);

   if (response != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AI analysis completed for incident [%u], response length=%d"),
         incidentId, static_cast<int>(strlen(response)));

      // Post analysis as incident comment
      TCHAR *commentText = WideStringFromUTF8String(response);

      // Prepend AI analysis header
      StringBuffer comment;
      comment.append(_T("[AI Analysis Report]\n\n"));
      comment.append(commentText);
      MemFree(commentText);

      uint32_t rcc = AddIncidentComment(incidentId, comment.cstr(), 0, nullptr, true);
      if (rcc != RCC_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to add AI analysis comment to incident [%u], rcc=%u"),
            incidentId, rcc);
      }

      // Auto-assign if enabled
      if (autoAssign)
      {
         // Query AI for assignee suggestion
         json_t *args = json_object();
         json_object_set_new(args, "incident_id", json_integer(incidentId));
         std::string suggestion = CallGlobalAIAssistantFunction("suggest-incident-assignee", args, 0);
         json_decref(args);

         // Parse the suggestion response
         json_error_t error;
         json_t *suggestionJson = json_loads(suggestion.c_str(), 0, &error);
         if (suggestionJson != nullptr)
         {
            uint32_t suggestedUserId = json_object_get_uint32(suggestionJson, "user_id", 0);
            if (suggestedUserId != 0)
            {
               rcc = AssignIncident(incidentId, suggestedUserId, 0);
               if (rcc == RCC_SUCCESS)
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("AI auto-assigned incident [%u] to user [%u]"),
                     incidentId, suggestedUserId);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to auto-assign incident [%u] to user [%u], rcc=%u"),
                     incidentId, suggestedUserId, rcc);
               }
            }
            json_decref(suggestionJson);
         }
      }

      MemFree(response);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("AI analysis for incident [%u] did not produce a response"), incidentId);
   }
}

/**
 * Spawn background AI analysis for an incident
 */
void NXCORE_EXPORTABLE SpawnIncidentAIAnalysis(uint32_t incidentId, int depth, bool autoAssign, const TCHAR *customPrompt)
{
   if (s_aiTaskThreadPool == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot spawn incident AI analysis: AI task thread pool not initialized"));
      return;
   }

   // Capture custom prompt as String for the lambda
   String prompt = (customPrompt != nullptr) ? customPrompt : _T("");

   ThreadPoolExecute(s_aiTaskThreadPool,
      [incidentId, depth, autoAssign, prompt] () -> void
      {
         IncidentAIAnalysisTask(incidentId, depth, autoAssign, prompt);
      });

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Spawned AI analysis task for incident [%u]"), incidentId);
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
      "get-available-skills",
      "Get list of available AI assistant skills that can be loaded to extend capabilities. Each skill provides specialized functions for specific domains like data collection, event processing, inventory management, etc.",
      {},
      [] (json_t *arguments, uint32_t userId) -> std::string
      {
         return GetRegisteredSkills();
      });

   RegisterAIAssistantFunction(
      "register-ai-task",
      "Register new AI task for autonomous background execution. "
      "Use AI tasks when: "
      "(1) work should continue without user interaction, "
      "(2) task needs multiple iterations to complete (e.g., processing many items, investigation requiring multiple passes), "
      "(3) task needs to preserve state/context between iterations, "
      "(4) user indicates 'take your time', 'work through this', 'multiple passes', or 'let me know when done'. "
      "Do NOT use AI tasks for time-based recurring operations (daily, weekly) - use scheduled tasks for those instead.",
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
      "netxms-version",
      "Get running version of NetXMS server.",
      {},
      [] (json_t *arguments, uint32_t userId) -> std::string
      {
         return std::string("Running NetXMS server version is " NETXMS_VERSION_STRING_A);
      });

   RegisterAIAssistantFunction(
      "write-log-message",
      "Write message to NetXMS server log.",
      {
         { "message", "log message text" }
      },
      F_WriteLogMessage);

   RegisterAIAssistantFunction(
      "create-ai-message",
      "Create an informational message or alert for the user to review later. "
      "Use this when you want to communicate findings, reports, or analysis results "
      "to the user without blocking the current task execution.",
      {
         { "title", "Short title for the message (max 255 chars)" },
         { "text", "Full message content" },
         { "type", "Message type: 'informational' or 'alert'" },
         { "relatedObjectId", "Optional: ID of related NetXMS object" },
         { "expirationMinutes", "Optional: minutes until message expires (default 7 days)" }
      }, F_CreateAIMessage);

   RegisterAIAssistantFunction(
      "create-approval-request",
      "Create an approval request that, when approved by the user, spawns a new AI task. "
      "Use this when you identify an action that requires user authorization before execution, "
      "such as service restarts, configuration changes, or potentially disruptive operations.",
      {
         { "title", "Short title describing what needs approval (max 255 chars)" },
         { "text", "Full description of what will happen on approval" },
         { "taskPrompt", "The prompt for the AI task to execute when approved" },
         { "relatedObjectId", "Optional: ID of related NetXMS object" },
         { "expirationMinutes", "Optional: minutes until request expires (default 24 hours)" }
      }, F_CreateApprovalRequest);

   InitAITasks();
   InitializeAIMessageManager();
   s_aiTaskThreadPool = ThreadPoolCreate(_T("AI-TASKS"),
         ConfigReadInt(_T("ThreadPool.AITasks.BaseSize"), 4),
         ConfigReadInt(_T("ThreadPool.AITasks.MaxSize"), 16));
   ThreadCreate(AITaskSchedulerThread, s_aiTaskThreadPool);

   RegisterSchedulerTaskHandler(L"Execute.AIAgentTask", ExecuteAIAgentTask, SYSTEM_ACCESS_SCHEDULE_SCRIPT);

   nxlog_debug_tag(DEBUG_TAG, 2, L"LLM service URL = \"%hs\", model = \"%hs\"", s_llmServiceURL, s_llmModel);
   nxlog_debug_tag(DEBUG_TAG, 2, L"%d global functions registered", static_cast<int>(s_globalFunctions.size()));
   nxlog_debug_tag(DEBUG_TAG, 2, L"%d skills registered", static_cast<int>(GetRegisteredSkillCount()));
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("AI assistant initialized"));
   return true;
}
