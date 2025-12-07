/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: iris.h
**
**/

#ifndef _iris_h_
#define _iris_h_

#define AI_ASSISTANT_COMPONENT   L"AI-ASSISTANT"

/**
 * AI assistant function handler
 */
typedef std::function<std::string(json_t*, uint32_t)> AssistantFunctionHandler;

/**
 * Assistant function descriptor
 */
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
 * Set of AI assistant functions
 */
typedef std::unordered_map<std::string, shared_ptr<AssistantFunction>> AssistantFunctionSet;

/**
 * AI task state
 */
enum class AITaskState
{
   SCHEDULED = 0,
   RUNNING = 1,
   COMPLETED = 2,
   FAILED = 3
};

/**
 * AI task
 */
class AITask
{
private:
   uint32_t m_id;
   uint32_t m_userId;
   std::string m_prompt;
   time_t m_lastExecutionTime;
   time_t m_nextExecutionTime;
   std::string m_memento;
   StringBuffer m_explanation;
   String m_description;
   AITaskState m_state;
   uint32_t m_iteration;

   void logExecution();

public:
   AITask(const wchar_t *descripion, uint32_t userId, const wchar_t *prompt);
   AITask(DB_RESULT hResult, int row);

   void execute();

   uint32_t getId() const { return m_id; }
   uint32_t getUserId() const { return m_userId; }
   const wchar_t *getDescription() const { return m_description.cstr(); }
   const std::string& getPrompt() const { return m_prompt; }
   AITaskState getState() const { return m_state; }
   time_t getLastExecutionTime() const { return m_lastExecutionTime; }
   time_t getNextExecutionTime() const { return m_nextExecutionTime; }
   uint32_t getIteration() const { return m_iteration; }
   const wchar_t *getExplanation() const { return m_explanation.cstr(); }

   void saveToDatabase() const;
   void deleteFromDatabase();

   json_t *toJson() const;

   void setNextExecutionTime(time_t t);
};

/**
 * Chat with AI assistant
 */
class Chat
{
private:
   uint32_t m_id;
   uint32_t m_userId;
   Mutex m_mutex;
   json_t *m_messages;
   size_t m_initialMessageCount;
   AssistantFunctionSet m_functions;
   json_t *m_functionDeclarations;
   time_t m_creationTime;
   time_t m_lastUpdateTime;

   void addMessage(const char *role, const char *content)
   {
      json_t *message = json_object();
      json_object_set_new(message, "role", json_string(role));
      json_object_set_new(message, "content", json_string(content));
      json_array_append_new(m_messages, message);
      nxlog_debug_tag(L"llm.chat", 8, L"Added message to chat: role=\"%hs\", content=\"%hs\"", role, content);
   }

   void initializeFunctions();
   std::string callFunction(const char *name, json_t *arguments);
   std::string loadSkill(const char *skillName);

public:
   Chat(NetObj *context = nullptr, json_t *eventData = nullptr, uint32_t userId = 0, const char *systemPrompt = nullptr);
   ~Chat();

   uint32_t getId() const { return m_id; }
   uint32_t getUserId() const { return m_userId; }

   char *sendRequest(const char *prompt, int maxIterations);

   void clear();
};

/**
 * AI assistant skill
 */
struct AssistantSkill
{
   std::string name;
   std::string description;
   std::string prompt;
   std::vector<AssistantFunction> functions;

   AssistantSkill(const std::string& _name, const std::string& _description, const std::string& _prompt) :
      name(_name), description(_description), prompt(_prompt)
   {
   }

   AssistantSkill(const std::string& _name, const std::string& _description, const std::string& _prompt, const std::vector<AssistantFunction>& _functions) :
      name(_name), description(_description), prompt(_prompt), functions(_functions)
   {
   }
};

/**
 * Register assistant function. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantFunction(const char *name, const char *description, const std::vector<std::pair<std::string, std::string>>& parameters, AssistantFunctionHandler handler);

/**
 * Call globally registered AI assistant function (intended for MCP bridge)
 */
std::string NXCORE_EXPORTABLE CallGlobalAIAssistantFunction(const char *name, json_t *arguments, uint32_t userId);

/**
 * Fill message with registered function list
 */
void FillAIAssistantFunctionListMessage(NXCPMessage *msg);

/**
 * Register AI assistant skill. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt);

/**
 * Register AI assistant skill with functions. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt, const std::vector<AssistantFunction>& functions);

/**
 * Add custom prompt
 */
void NXCORE_EXPORTABLE AddAIAssistantPrompt(const char *text);

/**
 * Create new chat
 */
shared_ptr<Chat> NXCORE_EXPORTABLE CreateAIAssistantChat(uint32_t userId, uint32_t *rcc);

/**
 * Get chat with given ID
 */
shared_ptr<Chat> NXCORE_EXPORTABLE GetAIAssistantChat(uint32_t chatId, uint32_t userId, uint32_t *rcc);

/**
 * Clear history for given chat session
 */
uint32_t NXCORE_EXPORTABLE ClearAIAssistantChat(uint32_t chatId, uint32_t userId);

/**
 * Delete chat
 */
uint32_t NXCORE_EXPORTABLE DeleteAIAssistantChat(uint32_t chatId, uint32_t userId);

/**
 * Send single idependent query to AI assistant
 */
char NXCORE_EXPORTABLE *QueryAIAssistant(const char *prompt, NetObj *context, int maxIterations = 32);

/**
 * Register AI task
 */
uint32_t NXCORE_EXPORTABLE RegisterAITask(const wchar_t *description, uint32_t userId, const wchar_t *prompt, time_t nextExecutionTime = 0);

/**
 * Delete AI task
 */
uint32_t NXCORE_EXPORTABLE DeleteAITask(uint32_t taskId, uint32_t userId);

/**
 * Convert JSON object to std::string and consume JSON object
 */
static inline std::string JsonToString(json_t *json)
{
   char *jsonText = json_dumps(json, 0);
   json_decref(json);
   std::string result(jsonText);
   MemFree(jsonText);
   return result;
}

#endif
