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
** File: nxai.h
**
**/

#ifndef _nxai_h_
#define _nxai_h_

#include <nms_util.h>
#include <netxms-version.h>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <functional>

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
 * Object summary as returned by object search
 */
struct ObjectInfo
{
   uint32_t id;
   std::string name;
   std::string className;

   ObjectInfo()
   {
      id = 0;
   }
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
   int m_httpStatus;
   uint32_t m_timeout;
   uint32_t m_responseTimeout;
   bool m_verifyPeer;
   VolatileCounter m_cancellationFlag;

   bool call(const char *method, const char *path, json_t *request, json_t **response);
   void setErrorFromResponse(json_t *response, const char *rawResponse);

public:
   WebApiClient(const char *server, bool verifyPeer);
   ~WebApiClient();

   const char *getServerUrl() const { return m_baseUrl.c_str(); }
   const char *getToken() const { return m_token.c_str(); }
   void setToken(const char *token) { m_token = CHECK_NULL_EX_A(token); }

   const char *getErrorText() const { return m_errorText.c_str(); }
   int getHttpStatus() const { return m_httpStatus; }
   bool isAuthenticationError() const { return (m_httpStatus == 401) || (m_httpStatus == 403); }

   void setTimeout(uint32_t timeout) { m_timeout = timeout; }
   void setResponseTimeout(uint32_t timeout) { m_responseTimeout = timeout; }

   /**
    * Cancel wait for assistant response. Can be called from signal handler or another thread.
    */
   void cancel() { InterlockedIncrement(&m_cancellationFlag); }
   void resetCancellation() { InterlockedAnd(&m_cancellationFlag, 0); }
   bool isCancelled() const { return m_cancellationFlag > 0; }

   bool login(const char *username, const char *password);

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

   /**
    * Find object by name. Returns false if request failed. On success object ID is set to 0
    * if no matching object was found.
    */
   bool findObject(const char *name, ObjectInfo *object);
};

/**
 * Chat session state
 */
struct ChatSession
{
   WebApiClient *client;
   std::string server;
   uint32_t chatId;
   uint32_t objectId;
   uint32_t incidentId;
   bool interactive;
   std::function<void (const char*)> progressCallback;

   ChatSession(WebApiClient *client, const char *server)
   {
      this->client = client;
      this->server = server;
      chatId = 0;
      objectId = 0;
      incidentId = 0;
      interactive = false;
   }

   json_t *createContextDocument() const;
   bool sendMessage(const char *message);
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

void TrimString(std::string *text);
bool ReadInputLine(const char *prompt, std::string *line);

void ProgressIndicatorUpdate(const char *currentFunction);
void ProgressIndicatorStop();

bool GetConfigFilePath(const TCHAR *name, TCHAR *path, size_t size);
bool LoadSessionToken(const char *server, std::string *token);
bool SaveSessionToken(const char *server, const char *token);
void ClearSessionToken(const char *server);

int RunChatSession(ChatSession *session);
bool PromptForAnswer(const Question& question, bool *positive, int *selectedOption);

#endif   /* _nxai_h_ */
