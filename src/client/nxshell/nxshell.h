/*
** NetXMS - Network Management System
** NetXMS shell
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
** File: nxshell.h
**
**/

#ifndef _nxshell_h_
#define _nxshell_h_

#include <nms_util.h>
#include <netxms-version.h>
#include <websocket.h>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "parser.h"

/**
 * Confirmation type for binary questions (mirrors server side definition)
 */
enum class ConfirmationType
{
   APPROVE_REJECT = 0,
   YES_NO = 1,
   CONFIRM_CANCEL = 2
};

/**
 * Question from AI assistant that requires user response
 */
struct Question
{
   uint64_t id;
   bool multipleChoice;
   ConfirmationType confirmationType;
   std::string text;
   std::string context;
   std::vector<std::string> options;
   time_t expiresAt;

   Question()
   {
      id = 0;
      multipleChoice = false;
      confirmationType = ConfirmationType::APPROVE_REJECT;
      expiresAt = 0;
   }

   /**
    * Load question from JSON document. Returns false if document does not contain valid question.
    */
   bool loadFromJson(json_t *json);

   void clear()
   {
      *this = Question();
   }
};

/**
 * State of asynchronous request processing
 */
enum class ChatState
{
   IDLE,
   PROCESSING,
   COMPLETED,
   FAILED,
   UNKNOWN
};

/**
 * Chat processing status
 */
struct ChatStatus
{
   ChatState state;
   std::string response;         // Assistant response, set when state is COMPLETED
   std::string errorMessage;     // Error message, set when state is FAILED
   std::string currentFunction;  // Name of function being executed by assistant, if any
   Question question;            // Pending question, valid if question.id is not 0

   ChatStatus()
   {
      state = ChatState::UNKNOWN;
   }
};

/**
 * Response to user message. Either response text or pending question is set.
 */
struct ChatResponse
{
   std::string text;
   Question question;

   void clear()
   {
      text.clear();
      question.clear();
   }
};

/**
 * Object information
 */
struct ObjectInfo
{
   uint32_t id;
   int status;
   std::string name;
   std::string className;
   std::vector<uint32_t> parents;   // Set only if object details were requested

   ObjectInfo()
   {
      id = 0;
      status = 0;
   }

   void loadFromJson(json_t *json);
};

/**
 * NetXMS web API client
 */
class WebApiClient
{
private:
   CURL *m_curl;
   std::string m_baseUrl;
   std::string m_token;
   std::string m_errorText;
   json_t *m_errorDocument;
   int m_httpStatus;
   uint32_t m_timeout;
   uint32_t m_responseTimeout;
   bool m_verifyPeer;
   bool m_connectionFailed;
   bool m_authenticating;
   std::function<bool ()> m_authenticator;
   VolatileCounter m_cancellationFlag;

   bool execute(const char *method, const char *path, json_t *request, json_t **response);
   void setErrorFromResponse(json_t *response, const char *rawResponse);

public:
   WebApiClient(const char *server, bool verifyPeer);
   ~WebApiClient();

   const char *getServerUrl() const { return m_baseUrl.c_str(); }
   const char *getToken() const { return m_token.c_str(); }
   void setToken(const char *token) { m_token = CHECK_NULL_EX_A(token); }

   /**
    * Set function that obtains new access token when server rejects current one. Request rejected
    * with status 401 is repeated once if authenticator returns true.
    */
   void setAuthenticator(const std::function<bool ()>& authenticator) { m_authenticator = authenticator; }

   const char *getErrorText() const { return m_errorText.c_str(); }
   int getHttpStatus() const { return m_httpStatus; }

   /**
    * Get document returned by server with error response to last call (can be nullptr)
    */
   json_t *getErrorDocument() const { return m_errorDocument; }

   /**
    * Check if any call failed because server cannot be reached or does not accept user's credentials
    */
   bool isConnectionFailed() const { return m_connectionFailed; }

   void setTimeout(uint32_t timeout) { m_timeout = timeout; }
   void setResponseTimeout(uint32_t timeout) { m_responseTimeout = timeout; }

   /**
    * Cancel current operation. Can be called from signal handler or another thread.
    */
   void cancel() { InterlockedIncrement(&m_cancellationFlag); }
   void resetCancellation() { InterlockedAnd(&m_cancellationFlag, 0); }
   bool isCancelled() const { return m_cancellationFlag > 0; }

   bool call(const char *method, const char *path, json_t *request, json_t **response);
   bool connectWebSocket(const char *path, WebSocketClient *socket);

   bool login(const char *username, const char *password);
   bool checkSession();

   bool createChat(uint32_t incidentId, uint32_t objectId, uint32_t *chatId);
   bool clearChat(uint32_t chatId);
   bool deleteChat(uint32_t chatId);

   bool sendMessage(uint32_t chatId, const char *message, json_t *context, ChatResponse *response,
         const std::function<void (const char*)>& progressCallback = nullptr);
   bool getStatus(uint32_t chatId, ChatStatus *status);
   bool waitForResponse(uint32_t chatId, ChatResponse *response,
         const std::function<void (const char*)>& progressCallback = nullptr);

   bool pollQuestion(uint32_t chatId, Question *question);
   bool answerQuestion(uint32_t chatId, uint64_t questionId, bool positive, int selectedOption);

   bool getObject(uint32_t id, ObjectInfo *object);
   bool getChildObjects(uint32_t parentId, std::vector<ObjectInfo> *objects);
   bool findObjects(const char *name, std::vector<ObjectInfo> *objects);
};

/**
 * Cached list of child objects
 */
struct ObjectListing
{
   int64_t timestamp;
   std::vector<ObjectInfo> objects;
};

/**
 * Shell
 */
class Shell
{
private:
   WebApiClient *m_client;
   std::string m_serverName;
   bool m_terminalInput;      // Standard input is a terminal, user can be asked questions
   bool m_interactive;        // Commands are read from terminal
   bool m_exitRequested;
   ShellMode m_mode;
   ShellMode m_homeMode;
   std::vector<ObjectInfo> m_location;
   std::vector<ObjectInfo> m_previousLocation;
   std::map<uint32_t, ObjectListing> m_listingCache;
   uint32_t m_chatId;
   uint32_t m_incidentId;
   WebSocketClient *m_consoleSocket;
   std::map<std::string, std::string> m_aliases;
   std::function<void (const char*)> m_progressCallback;

   // Command processing
   bool executeCommand(const std::string& line);
   bool executeModeCommand(const std::string& command, bool *success);
   bool executeAliasCommand(const std::string& arguments);
   bool removeAlias(const std::string& name);
   bool runAlias(const std::string& name, const std::string& body, const std::string& arguments);
   void showHelp();
   void showStatus();

   // Navigation
   bool getChildObjects(uint32_t parentId, const std::vector<ObjectInfo> **objects);
   bool resolvePath(const std::string& text, std::vector<ObjectInfo> *location, bool reportErrors);
   bool buildLocation(uint32_t objectId, std::vector<ObjectInfo> *location);
   bool selectObject(const std::string& name, uint32_t *objectId);
   bool changeLocation(const std::string& arguments);
   bool listObjects(const std::string& arguments);
   void validateLocation();

   // AI assistant
   json_t *createChatContext() const;
   bool setIncidentContext(const std::string& arguments);
   bool clearChat();

   // Server debug console
   bool openConsoleSession();
   void closeConsoleSession();
   bool executeConsoleCommand(const std::string& command);
   bool executeConsoleCommandBuffered(const std::string& command);

   // Scripts
   bool executeScript(const std::string& source, const std::vector<std::string>& parameters, const char *aliasName);
   void reportScriptError(const char *aliasName);

public:
   Shell(WebApiClient *client, ShellMode homeMode, bool terminalInput, bool interactive);
   ~Shell();

   WebApiClient *getClient() const { return m_client; }
   ShellMode getMode() const { return m_mode; }
   bool isExitRequested() const { return m_exitRequested; }
   uint32_t getCurrentObjectId() const { return m_location.empty() ? 0 : m_location.back().id; }

   void setIncidentId(uint32_t incidentId) { m_incidentId = incidentId; }
   void setProgressCallback(const std::function<void (const char*)>& callback) { m_progressCallback = callback; }
   bool setLocationByObjectId(uint32_t objectId);
   bool setLocationByObjectName(const char *name);

   std::string getLocationPath() const;
   std::string getPrompt() const;

   bool isInputComplete(const std::string& input) const { return IsInputComplete(m_mode, input); }
   bool execute(const std::string& input);
   bool executeStream(FILE *stream, const char *name, bool stopOnError);
   void executeStartupFile();
   bool sendChatMessage(const char *message);
   void leaveMode();

   void complete(const std::string& input, std::string *completion, std::vector<std::string> *candidates);
};

/**
 * Plain output mode - no colors and no markdown formatting
 */
extern bool g_plainOutput;

void PrintStatus(const char *format, ...);
void PrintSuccess(const char *format, ...);
void PrintWarning(const char *format, ...);
void PrintError(const char *format, ...);
void RenderResponse(const char *text);

bool ReadInputLine(const char *prompt, std::string *line);
void WriteOutput(const char *text);
void AppendHighlightedText(std::string *output, const char *attributes, const char *text);

void ProgressIndicatorUpdate(const char *currentFunction);
void ProgressIndicatorStop();

bool GetConfigFilePath(const TCHAR *name, TCHAR *path, size_t size);
bool LoadSessionToken(const char *server, std::string *token);
bool SaveSessionToken(const char *server, const char *token);
void ClearSessionToken(const char *server);

int RunInteractiveSession(Shell *shell);
bool PromptForAnswer(const Question& question, bool *positive, int *selectedOption);
bool AskConfirmation(const char *question);

WebSocketReadResult ReadWebSocketMessage(WebSocketClient *socket, uint32_t timeout, json_t **message);

#endif   /* _nxshell_h_ */
