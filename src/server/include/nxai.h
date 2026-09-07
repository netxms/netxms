/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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

#include <unordered_map>
#include <unordered_set>

#define AI_ASSISTANT_COMPONENT   L"AI-ASSISTANT"

/**
 * AI assistant function handler
 */
typedef std::function<std::string(json_t*, uint32_t)> AssistantFunctionHandler;

/**
 * Assistant function parameter descriptor. Type names follow JSON schema: "string", "integer", "number", "boolean", "array", "object".
 * For "array" parameters itemType gives the type of array elements.
 */
struct AssistantFunctionParameter
{
   std::string name;
   std::string description;
   std::string type;
   std::string itemType;

   AssistantFunctionParameter(const char *name, const char *description, const char *type = "string", const char *itemType = "string")
      : name(name), description(description), type(type), itemType(itemType)
   {
   }
};

/**
 * Assistant function descriptor
 */
struct AssistantFunction
{
   std::string name;
   std::string description;
   std::vector<AssistantFunctionParameter> parameters;
   AssistantFunctionHandler handler;
   bool chatOnly;  // Function only makes sense within interactive chat context and is not exposed to MCP clients

   AssistantFunction(const std::string& name, const std::string& description,
      const std::vector<AssistantFunctionParameter>& parameters, AssistantFunctionHandler handler, bool chatOnly = false)
      : name(name), description(description), parameters(parameters), handler(handler), chatOnly(chatOnly)
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
 * Confirmation type for binary questions
 */
enum class ConfirmationType
{
   APPROVE_REJECT = 0,
   YES_NO = 1,
   CONFIRM_CANCEL = 2
};

/**
 * Pending question for user interaction
 */
struct PendingQuestion
{
   uint64_t id;
   bool isMultipleChoice;
   ConfirmationType confirmationType;
   std::string text;
   std::string context;
   StringList options;
   time_t expiresAt;
   Condition responseReceived;
   bool responded;
   bool positiveResponse;
   int selectedOption;

   PendingQuestion() : responseReceived(false)
   {
      id = 0;
      isMultipleChoice = false;
      confirmationType = ConfirmationType::APPROVE_REJECT;
      expiresAt = 0;
      responded = false;
      positiveResponse = false;
      selectedOption = -1;
   }
};

/**
 * AI task
 */
class AITask
{
private:
   uint32_t m_id;
   uint32_t m_userId;
   mutable Mutex m_mutex;
   std::string m_prompt;
   time_t m_lastExecutionTime;
   time_t m_nextExecutionTime;
   std::string m_memento;
   StringBuffer m_explanation;
   String m_description;
   AITaskState m_state;
   uint32_t m_iteration;

   // Runtime state (not persisted), protected by AI task list lock
   bool m_executing;

   void logExecution();   // must be called from executing thread with task lock released
   void clearExecutingState();

public:
   AITask(const wchar_t *descripion, uint32_t userId, const wchar_t *prompt);
   AITask(DB_RESULT hResult, int row);

   void execute();

   uint32_t getId() const { return m_id; }
   uint32_t getUserId() const { return m_userId; }
   const wchar_t *getDescription() const { return m_description.cstr(); }
   std::string getPrompt() const { return GetAttributeWithLock(m_prompt, m_mutex); }
   AITaskState getState() const { return m_state; }
   time_t getLastExecutionTime() const { return m_lastExecutionTime; }
   time_t getNextExecutionTime() const { return m_nextExecutionTime; }
   uint32_t getIteration() const { return m_iteration; }
   String getExplanation() const { return GetAttributeWithLock<String>(m_explanation, m_mutex); }
   bool isExecuting() const { return m_executing; }
   void setExecuting() { m_executing = true; }

   void saveToDatabase() const;
   void deleteFromDatabase();

   json_t *toJson() const;

   void setNextExecutionTime(time_t t);
};

#define AI_OPERATOR_COMPONENT   L"AI-OPERATOR"

/**
 * AI operator observation state
 */
enum class AIObservationState
{
   NEW = 0,
   ACKNOWLEDGED = 1,
   DISMISSED = 2
};

/**
 * Action applied when a standing check fires
 */
enum class AICheckAction
{
   WAKE = 0,      // Queue an out-of-schedule iteration of the owning instance
   OBSERVE = 1    // Record an observation directly, without LLM involvement
};

/**
 * Standing check verdict
 */
enum class AICheckVerdict
{
   NONE = 0,      // Check was never run
   QUIET = 1,
   FIRED = 2,
   FAILED = 3     // Script returned an unexpected value or failed at runtime
};

/**
 * Result of a single standing check run. Text fields are UTF-8.
 */
struct AICheckResult
{
   AICheckVerdict verdict;
   std::string title;
   int severity;
   std::string details;
   std::string error;   // Set when verdict is FAILED

   AICheckResult() : verdict(AICheckVerdict::QUIET), severity(SEVERITY_WARNING)
   {
   }
};

/**
 * State transition decided after a standing check run
 */
enum class AICheckTransition
{
   NONE = 0,            // Nothing to do (verdict recorded)
   SUPPRESSED = 1,      // Fired on the edge but inside cooldown - action deferred, previous verdict kept
   CLEAR = 2,           // Fired -> quiet
   FIRE_EDGE = 3,       // Quiet -> fired
   FIRE_RENOTIFY = 4,   // Still fired and renotify interval elapsed
   FAILURE = 5          // Script error
};

class NXSL_Value;

/**
 * Evaluate value returned by a standing check script: null, false, 0, and empty string are quiet;
 * a string or a hash { title, severity, details } fires; anything else is a check error.
 */
AICheckResult NXCORE_EXPORTABLE EvaluateAICheckResult(NXSL_Value *value, const wchar_t *checkName);

/**
 * Decide state transition for a standing check given its previous verdict and the current run result
 */
AICheckTransition NXCORE_EXPORTABLE EvaluateAICheckTransition(AICheckVerdict previous, time_t lastFire, time_t now,
   uint32_t cooldown, uint32_t renotifyInterval, AICheckVerdict current);

/**
 * AI operator standing check - NXSL script run by the scheduler without LLM involvement.
 * All fields are protected by the owning instance's lock; the script itself runs outside the lock.
 */
class NXCORE_EXPORTABLE AIOperatorCheck
{
private:
   uint32_t m_id;
   uint32_t m_instanceId;
   wchar_t m_name[64];
   MutableString m_description;
   bool m_enabled;
   bool m_locked;             // Set by a human; the model cannot modify or delete a locked check
   bool m_createdByModel;
   std::string m_source;      // NXSL source (UTF-8)
   uint32_t m_interval;       // Seconds between runs
   uint32_t m_objectId;       // Object bound as $object (0 = none)
   AICheckAction m_action;
   uint32_t m_cooldown;       // Minimum seconds between two action applications
   uint32_t m_renotifyInterval;  // Re-apply action while still fired (0 = only on the quiet->fired edge)
   time_t m_lastRun;
   AICheckVerdict m_lastVerdict;
   time_t m_lastFire;
   std::string m_lastPayload; // JSON (UTF-8) of the last fired result
   uint32_t m_consecutiveErrors;
   uint32_t m_runCount;
   time_t m_creationTime;
   time_t m_modificationTime;

   // Runtime state (not persisted)
   shared_ptr<NXSL_Program> m_program;  // Compiled at create/modify/load; null if compilation failed
   MutableString m_compileError;
   bool m_running;


public:
   AIOperatorCheck(uint32_t instanceId, bool createdByModel);
   AIOperatorCheck(DB_RESULT hResult, int row);

   uint32_t getId() const { return m_id; }
   uint32_t getInstanceId() const { return m_instanceId; }
   const wchar_t *getName() const { return m_name; }
   bool isEnabled() const { return m_enabled; }
   bool isLocked() const { return m_locked; }
   bool isCreatedByModel() const { return m_createdByModel; }
   uint32_t getInterval() const { return m_interval; }
   uint32_t getObjectId() const { return m_objectId; }
   AICheckAction getAction() const { return m_action; }
   uint32_t getCooldown() const { return m_cooldown; }
   uint32_t getRenotifyInterval() const { return m_renotifyInterval; }
   time_t getLastRun() const { return m_lastRun; }
   AICheckVerdict getLastVerdict() const { return m_lastVerdict; }
   time_t getLastFire() const { return m_lastFire; }
   uint32_t getConsecutiveErrors() const { return m_consecutiveErrors; }
   const shared_ptr<NXSL_Program>& getProgram() const { return m_program; }
   const String& getCompileError() const { return m_compileError; }
   bool isRunning() const { return m_running; }

   bool isDue(time_t now) const { return m_enabled && !m_running && (m_lastRun + static_cast<time_t>(m_interval) <= now); }
   void setRunning(bool running) { m_running = running; }
   void setEnabled(bool enabled) { m_enabled = enabled; m_modificationTime = time(nullptr); }

   /**
    * Record outcome of a run. Returns true if persistent state changed and the check should be saved.
    */
   bool recordRun(time_t now, const AICheckResult& result, AICheckTransition transition);

   uint32_t modifyFromJSON(json_t *config, bool byModel, MutableString *errorText);

   void saveToDatabase() const;
   void deleteFromDatabase();

   json_t *toJson() const;
   json_t *toPromptJson() const;
   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
};

/**
 * Pending interrupt raised by a fired standing check with "wake" action
 */
struct AIOperatorInterrupt
{
   uint32_t checkId;
   std::string checkName;   // UTF-8
   std::string payload;     // JSON (UTF-8)
   time_t timestamp;
};

/**
 * AI operator instance - perpetual adaptive monitoring loop
 */
class NXCORE_EXPORTABLE AIOperatorInstance
{
private:
   uint32_t m_id;
   mutable Mutex m_mutex;
   wchar_t m_name[64];
   MutableString m_description;
   uint32_t m_ownerUserId;
   bool m_enabled;
   std::string m_scopeFilter;
   char m_modelSlot[64];
   uint32_t m_minInterval;    // Minimum interval between executions in seconds
   uint32_t m_maxInterval;    // Maximum interval between executions in seconds
   uint32_t m_dailyTokenBudget;  // Daily LLM token budget (0 = unlimited)
   int64_t m_tokensUsed;         // Tokens used within current usage day
   uint32_t m_usageDay;          // Day number (UTC) the usage counter applies to
   std::string m_personaPrompt;
   wchar_t m_currentFocus[256];
   std::string m_watchList;
   std::string m_memento;
   uint32_t m_observationRetentionDays;  // 0 = server default
   uint32_t m_observationMaxRecords;     // 0 = server default
   std::string m_instructions;   // Standing instructions authored by the model (UTF-8)
   bool m_instructionsLocked;    // Set by a human; model updates are ignored while set
   time_t m_lastExecutionTime;
   time_t m_nextExecutionTime;
   uint32_t m_iteration;
   time_t m_creationTime;
   time_t m_modificationTime;

   // Runtime state (not persisted)
   bool m_executing;    // protected by instance list lock
   int m_consecutiveFailures;
   StringBuffer m_lastExplanation;
   SharedObjectArray<AIOperatorCheck> m_checks;
   std::vector<AIOperatorInterrupt> m_pendingInterrupts;
   bool m_interruptPending;   // mirrors !m_pendingInterrupts.empty() for lock-free scheduler checks

   bool processResponse(const char *response, time_t now);
   void handleFailure(const char *error, time_t now);
   void logExecution(wchar_t status, uint32_t durationMs, int64_t inputTokens, int64_t outputTokens, const wchar_t *explanation);
   void clearExecutingState();
   void saveToDatabase() const;   // must be called with instance lock held
   size_t setInstructions(const char *text, time_t now);  // must be called with instance lock held; returns number of characters kept if text was truncated, 0 otherwise
   int findCheckIndex(uint32_t checkId) const;   // must be called with instance lock held
   void appendChecksToPrompt(std::string& prompt) const;   // must be called with instance lock held

public:
   AIOperatorInstance(const wchar_t *name, uint32_t ownerUserId);
   AIOperatorInstance(DB_RESULT hResult, int row);

   void execute();

   uint32_t getId() const { return m_id; }
   const wchar_t *getName() const { return m_name; }
   uint32_t getOwnerUserId() const { return m_ownerUserId; }
   bool isEnabled() const { return m_enabled; }
   bool isExecuting() const { return m_executing; }
   void setExecuting() { m_executing = true; }
   time_t getNextExecutionTime() const { return m_nextExecutionTime; }
   time_t getLastExecutionTime() const { return m_lastExecutionTime; }
   uint32_t getIteration() const { return m_iteration; }
   int64_t getTokensUsedToday() const { return m_tokensUsed; }
   uint32_t getObservationRetentionDays() const { return m_observationRetentionDays; }
   uint32_t getObservationMaxRecords() const { return m_observationMaxRecords; }
   uint32_t getMinInterval() const { return m_minInterval; }
   uint32_t getMaxInterval() const { return m_maxInterval; }
   uint32_t getDailyTokenBudget() const { return m_dailyTokenBudget; }
   int getConsecutiveFailures() const { return m_consecutiveFailures; }
   time_t getCreationTime() const { return m_creationTime; }
   time_t getModificationTime() const { return m_modificationTime; }
   String getDescription() const { return GetAttributeWithLock<String>(m_description, m_mutex); }
   std::string getScopeFilter() const { return GetAttributeWithLock(m_scopeFilter, m_mutex); }
   std::string getModelSlot() const { LockGuard lockGuard(m_mutex); return std::string(m_modelSlot); }
   std::string getPersonaPrompt() const { return GetAttributeWithLock(m_personaPrompt, m_mutex); }
   String getCurrentFocus() const { LockGuard lockGuard(m_mutex); return String(m_currentFocus); }
   std::string getWatchList() const { return GetAttributeWithLock(m_watchList, m_mutex); }
   std::string getMemento() const { return GetAttributeWithLock(m_memento, m_mutex); }
   String getLastExplanation() const { return GetAttributeWithLock<String>(m_lastExplanation, m_mutex); }
   std::string getInstructions() const { return GetAttributeWithLock(m_instructions, m_mutex); }
   bool isInstructionsLocked() const { return m_instructionsLocked; }
   bool hasPendingInterrupts() const { return m_interruptPending; }

   uint32_t modifyFromJSON(json_t *config);
   void setEnabled(bool enabled);
   void resetMemento();

   // Standing checks
   uint32_t createCheck(json_t *config, bool byModel, uint32_t *checkId, MutableString *errorText);
   uint32_t modifyCheck(uint32_t checkId, json_t *config, bool byModel, MutableString *errorText);
   uint32_t deleteCheck(uint32_t checkId, bool byModel);
   shared_ptr<AIOperatorCheck> getCheck(uint32_t checkId) const;
   void getChecks(SharedObjectArray<AIOperatorCheck> *checks) const;
   int getCheckCount(int *enabledCount) const;
   void loadCheck(const shared_ptr<AIOperatorCheck>& check) { m_checks.add(check); }  // startup only, no locking
   void collectDueChecks(time_t now, std::vector<shared_ptr<AIOperatorCheck>> *checks);
   void dropPendingInterrupts();
   void runCheck(shared_ptr<AIOperatorCheck> check);

   void deleteFromDatabase();

   json_t *toJson() const;
   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
};

/**
 * Initialize AI operator subsystem
 */
void InitAIOperators();

/**
 * Create AI operator instance from JSON configuration. Instance ID is returned via instanceId.
 */
uint32_t NXCORE_EXPORTABLE CreateAIOperatorInstance(json_t *config, uint32_t ownerUserId, uint32_t *instanceId);

/**
 * Modify AI operator instance from JSON configuration
 */
uint32_t NXCORE_EXPORTABLE ModifyAIOperatorInstance(uint32_t instanceId, json_t *config);

/**
 * Delete AI operator instance (also deletes its observations, standing checks, and instructions history)
 */
uint32_t NXCORE_EXPORTABLE DeleteAIOperatorInstance(uint32_t instanceId);

/**
 * Enable or disable AI operator instance
 */
uint32_t NXCORE_EXPORTABLE SetAIOperatorInstanceEnabled(uint32_t instanceId, bool enabled);

/**
 * Reset AI operator instance accumulated state (memento, focus, watch list, iteration counter)
 */
uint32_t NXCORE_EXPORTABLE ResetAIOperatorInstanceMemento(uint32_t instanceId);

/**
 * Get AI operator instance by ID
 */
shared_ptr<AIOperatorInstance> NXCORE_EXPORTABLE GetAIOperatorInstance(uint32_t instanceId);

/**
 * Get all AI operator instances
 */
unique_ptr<SharedObjectArray<AIOperatorInstance>> NXCORE_EXPORTABLE GetAIOperatorInstances();

/**
 * Get all AI operator instances as JSON array (caller must call json_decref on result)
 */
json_t NXCORE_EXPORTABLE *GetAIOperatorInstancesAsJson();

/**
 * Fill NXCP message with all AI operator instances
 */
void FillAIOperatorListMessage(NXCPMessage *msg);

/**
 * Create standing check for AI operator instance. Check ID is returned via checkId; on compilation failure
 * or validation error a diagnostic message is returned via errorText (when provided).
 */
uint32_t NXCORE_EXPORTABLE CreateAIOperatorCheck(uint32_t instanceId, json_t *config, bool byModel, uint32_t *checkId, MutableString *errorText);

/**
 * Modify standing check of AI operator instance
 */
uint32_t NXCORE_EXPORTABLE ModifyAIOperatorCheck(uint32_t instanceId, uint32_t checkId, json_t *config, bool byModel, MutableString *errorText);

/**
 * Delete standing check of AI operator instance
 */
uint32_t NXCORE_EXPORTABLE DeleteAIOperatorCheck(uint32_t instanceId, uint32_t checkId, bool byModel);

/**
 * Get standing check of AI operator instance
 */
shared_ptr<AIOperatorCheck> NXCORE_EXPORTABLE GetAIOperatorCheck(uint32_t instanceId, uint32_t checkId);

/**
 * Get standing checks of AI operator instance as JSON array (caller must call json_decref on result).
 * Returns nullptr if instance does not exist.
 */
json_t NXCORE_EXPORTABLE *GetAIOperatorChecksAsJson(uint32_t instanceId);

/**
 * Fill NXCP message with standing checks of AI operator instance
 */
uint32_t FillAIOperatorCheckListMessage(uint32_t instanceId, NXCPMessage *msg);

/**
 * Get standing instructions history of AI operator instance as JSON array (caller must call json_decref on result).
 * Returns nullptr if instance does not exist.
 */
json_t NXCORE_EXPORTABLE *GetAIOperatorInstructionsHistoryAsJson(uint32_t instanceId);

/**
 * Fill NXCP message with standing instructions history of AI operator instance
 */
uint32_t FillAIOperatorInstructionsHistoryMessage(uint32_t instanceId, NXCPMessage *msg);

/**
 * Update AI operator observation state (acknowledge/dismiss)
 */
uint32_t NXCORE_EXPORTABLE UpdateAIOperatorObservationState(int64_t observationId, AIObservationState state, uint32_t userId, uint32_t *sourceObjectId = nullptr);

/**
 * Housekeeping for AI operator observations (retention time and per-instance record cap)
 */
void CleanAIOperatorObservations(DB_HANDLE hdb, time_t cycleStartTime);

/**
 * Print AI operator instances to server console
 */
void ShowAIOperators(ServerConsole *console);

#undef ERROR

/**
 * Async request state for WebAPI
 */
enum class AsyncRequestState
{
   IDLE = 0,
   PROCESSING = 1,
   COMPLETED = 2,
   ERROR = 3
};

/**
 * Chat with AI assistant
 */
class NXCORE_EXPORTABLE Chat
{
private:
   uint32_t m_id;
   uint32_t m_userId;
   uint32_t m_boundIncidentId;  // ID of bound incident (0 if not bound)
   Mutex m_mutex;
   json_t *m_messages;
   std::string m_systemPrompt;
   AssistantFunctionSet m_functions;
   json_t *m_functionDeclarations;
   time_t m_creationTime;
   time_t m_lastUpdateTime;
   PendingQuestion *m_pendingQuestion;
   Mutex m_questionMutex;
   std::function<void (const PendingQuestion&)> m_questionListener;
   AsyncRequestState m_asyncState;
   char *m_asyncResult;
   char *m_asyncErrorMessage;
   const char *m_currentFunction;
   Mutex m_asyncMutex;
   std::string m_lastError;
   bool m_isInteractive;
   char m_slot[32];  // Provider slot (e.g., "interactive", "background", "fast", "analytical")

   void addMessage(const char *role, const char *content)
   {
      if (!strcmp(role, "system"))
      {
         if (!m_systemPrompt.empty())
            m_systemPrompt.append("\n\n");
         m_systemPrompt.append(content);
         nxlog_debug_tag(L"ai.chat", 8, L"Appended system prompt to chat (length=%d)", static_cast<int>(m_systemPrompt.length()));
      }
      else
      {
         json_t *message = json_object();
         json_object_set_new(message, "role", json_string(role));
         json_object_set_new(message, "content", json_string(content));
         json_array_append_new(m_messages, message);
         nxlog_debug_tag(L"ai.chat", 8, L"Added message to chat: role=\"%hs\", content=\"%hs\"", role, content);
      }
   }

   void initializeFunctions();
   std::string callFunction(const char *name, json_t *arguments);
   std::string loadSkill(const char *skillName);
   std::string delegateToSkill(const char *skillName, const char *task);

public:
   Chat(NetObj *context = nullptr, json_t *eventData = nullptr, uint32_t userId = 0, const char *systemPrompt = nullptr, bool isInteractive = true, bool enableTools = true);
   ~Chat();

   uint32_t getId() const { return m_id; }
   uint32_t getUserId() const { return m_userId; }
   uint32_t getBoundIncidentId() const { return m_boundIncidentId; }
   bool isInteractive() const { return m_isInteractive; }
   const char *getSlot() const { return m_slot; }
   void setSlot(const char *slot) { strlcpy(m_slot, slot, sizeof(m_slot)); }

   void bindToIncident(uint32_t incidentId);
   void enableVisualizationOutput();

   char *sendRequest(const char *prompt, const char *context = nullptr);

   bool startAsyncRequest(const char *prompt, const char *context = nullptr);
   AsyncRequestState getAsyncState() const { return m_asyncState; }
   const char *getCurrentFunction() const { return m_currentFunction; }
   char *takeAsyncResult();
   char *takeAsyncErrorMessage();

   void clear();

   bool askConfirmation(const char *text, const char *context, ConfirmationType type, uint32_t timeout = 300);
   int askMultipleChoice(const char *text, const char *context, const StringList &options, uint32_t timeout = 300);
   void handleQuestionResponse(uint64_t questionId, bool positive, int selectedOption);
   bool hasPendingQuestion() const { return m_pendingQuestion != nullptr; }
   json_t *getPendingQuestion();

   /**
    * Set question listener called each time a new pending question is posted (in addition to
    * client session delivery). Called from the thread executing the chat request with question
    * mutex held - the listener must not block and should dispatch actual delivery asynchronously.
    */
   void setQuestionListener(std::function<void (const PendingQuestion&)> listener) { m_questionListener = listener; }
};

/**
 * Skill execution mode
 */
enum class SkillExecutionMode
{
   LOADED = 0,      // Load into caller's context
   DELEGATED = 1    // Run in separate context
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
   bool supportsDelegation;
   SkillExecutionMode defaultMode;

   AssistantSkill(const std::string& _name, const std::string& _description, const std::string& _prompt,
      bool _supportsDelegation = true, SkillExecutionMode _defaultMode = SkillExecutionMode::LOADED) :
      name(_name), description(_description), prompt(_prompt), supportsDelegation(_supportsDelegation), defaultMode(_defaultMode)
   {
   }

   AssistantSkill(const std::string& _name, const std::string& _description, const std::string& _prompt, const std::vector<AssistantFunction>& _functions,
      bool _supportsDelegation = true, SkillExecutionMode _defaultMode = SkillExecutionMode::LOADED) :
      name(_name), description(_description), prompt(_prompt), functions(_functions), supportsDelegation(_supportsDelegation), defaultMode(_defaultMode)
   {
   }
};

/**
 * Register assistant function. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantFunction(const char *name, const char *description, const std::vector<AssistantFunctionParameter>& parameters, AssistantFunctionHandler handler);

/**
 * Get unified tool list for MCP clients: global functions plus functions from all skills flattened into one namespace,
 * excluding disabled and chat-only items. Each element is a JSON object with name, description, and MCP input schema.
 * Caller must call json_decref on result.
 */
json_t NXCORE_EXPORTABLE *GetMCPToolsAsJson();

/**
 * Call AI assistant function by name on behalf of MCP client. Function name is resolved in global functions first,
 * then across all skills, honoring disabled lists and chat-only flags. If function cannot be resolved, *found is
 * set to false (when provided) and error message is returned.
 */
std::string NXCORE_EXPORTABLE CallMCPTool(const char *name, json_t *arguments, uint32_t userId, bool *found = nullptr);

/**
 * Get skill prompt list for MCP clients (excluding disabled skills) as JSON array of objects with name and description.
 * Caller must call json_decref on result.
 */
json_t NXCORE_EXPORTABLE *GetMCPPromptsAsJson();

/**
 * Get single skill prompt for MCP clients as JSON object with name, description, and prompt text.
 * Returns nullptr if skill does not exist or is disabled. Caller must call json_decref on result.
 */
json_t NXCORE_EXPORTABLE *GetMCPPromptAsJson(const char *name);

/**
 * Fill message with registered skills and functions list (including disabled status)
 */
void FillAISkillsAndFunctionsMessage(NXCPMessage *msg);

/**
 * Load AI disabled items from database
 */
void LoadAIDisabledLists();

/**
 * Get snapshot of disabled skills set (for batch checking without repeated locking)
 */
std::unordered_set<std::string> NXCORE_EXPORTABLE GetAIDisabledSkills();

/**
 * Get snapshot of disabled functions set (for batch checking without repeated locking)
 */
std::unordered_set<std::string> NXCORE_EXPORTABLE GetAIDisabledFunctions();

/**
 * Check if a specific AI skill is disabled (convenience wrapper for single-item checks)
 */
bool NXCORE_EXPORTABLE IsAISkillDisabled(const std::string& name);

/**
 * Check if a specific AI function is disabled (convenience wrapper for single-item checks)
 */
bool NXCORE_EXPORTABLE IsAIFunctionDisabled(const std::string& name);

/**
 * Add item to AI disabled list (writes to DB and updates cache)
 */
bool NXCORE_EXPORTABLE AddAIDisabledItem(char type, const char *name);

/**
 * Remove item from AI disabled list (writes to DB and updates cache)
 */
bool NXCORE_EXPORTABLE RemoveAIDisabledItem(char type, const char *name);

/**
 * Get registered skills as JSON array (caller must call json_decref on result)
 */
json_t NXCORE_EXPORTABLE *GetAISkillsAsJson();

/**
 * Get registered functions as JSON array (caller must call json_decref on result)
 */
json_t NXCORE_EXPORTABLE *GetAIFunctionsAsJson();

/**
 * Get disabled items not matching any registered entity as JSON array (caller must call json_decref on result)
 */
json_t NXCORE_EXPORTABLE *GetAIDisabledExtrasAsJson();

/**
 * Register AI assistant skill. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt);

/**
 * Register AI assistant skill with functions. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt, const std::vector<AssistantFunction>& functions);

/**
 * Register AI assistant skill with delegation support. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt,
   bool supportsDelegation, SkillExecutionMode defaultMode);

/**
 * Register AI assistant skill with functions and delegation support. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt,
   const std::vector<AssistantFunction>& functions, bool supportsDelegation, SkillExecutionMode defaultMode);

/**
 * Add custom prompt
 */
void NXCORE_EXPORTABLE AddAIAssistantPrompt(const char *text);

/**
 * Add custom prompt from file
 */
void NXCORE_EXPORTABLE AddAIAssistantPromptFromFile(const wchar_t *fileName);

/**
 * Get current chat context (thread-local).
 * Returns the Chat object that is currently processing a request on this thread, or nullptr if none.
 * This allows AI function handlers to access the Chat for user interaction (e.g., askConfirmation).
 */
Chat NXCORE_EXPORTABLE *GetCurrentAIChat();

/**
 * Create new chat
 * @param userId ID of the user creating the chat
 * @param incidentId ID of incident to bind to (0 for no binding)
 * @param rcc Pointer to store result code
 */
shared_ptr<Chat> NXCORE_EXPORTABLE CreateAIAssistantChat(uint32_t userId, uint32_t incidentId, uint32_t *rcc);

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
 * Send single independent query to AI assistant
 * @param prompt Query prompt
 * @param context Optional object context
 * @param slot Provider slot to use (e.g., "fast", "analytical"); falls back to "default" slot if requested slot is not configured
 * @param enableTools If true, register functions and skills; if false, no tools and single iteration
 */
char NXCORE_EXPORTABLE *QueryAIAssistant(const char *prompt, NetObj *context = nullptr, const char *slot = nullptr, bool enableTools = true);

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

/**
 * Find object by its name or ID
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByNameOrId(const char *name, int objectClassHint = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByNameOrId(json_t *parent, const char *tag, int objectClassHint = -1);

/**
 * Spawn background AI analysis for an incident
 * @param incidentId ID of the incident to analyze
 * @param depth Analysis depth: 0=quick, 1=standard, 2=thorough
 * @param autoAssign If true, automatically assign incident based on AI suggestion
 * @param customPrompt Optional custom instructions for analysis (can be nullptr)
 */
void NXCORE_EXPORTABLE SpawnIncidentAIAnalysis(uint32_t incidentId, int depth, bool autoAssign, const TCHAR *customPrompt);

/**
 * Generate anomaly detection profile for a DCI asynchronously
 * @param dciId ID of the DCI
 * @param nodeId ID of the node owning the DCI
 */
void NXCORE_EXPORTABLE GenerateAnomalyProfileAsync(uint32_t dciId, uint32_t nodeId);

#endif
