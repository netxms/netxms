/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: chatbot.cpp
**
**/

#include "nxcore.h"
#include <nms_users.h>
#include <ncdrv.h>
#include <chatdrv.h>
#include <nxai.h>
#include <unordered_map>

#define DEBUG_TAG L"chatbot"

/**
 * Default session idle timeout in seconds
 */
#define DEFAULT_SESSION_IDLE_TIMEOUT   1800

/**
 * Chat bot driver descriptor
 */
struct ChatBotDriverDescriptor
{
   ChatBotDriverFactory instanceFactory;
   wchar_t name[MAX_OBJECT_NAME];
};

/**
 * Registered chat bot drivers
 */
static StringObjectMap<ChatBotDriverDescriptor> s_driverList(Ownership::True);

/**
 * Register chat bot driver. Intended to be called during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterChatBotDriver(const wchar_t *name, ChatBotDriver *(*instanceFactory)(Config*, NCDriverStorageManager*))
{
   ChatBotDriverDescriptor *dd = new ChatBotDriverDescriptor();
   dd->instanceFactory = instanceFactory;
   wcslcpy(dd->name, name, MAX_OBJECT_NAME);
   s_driverList.set(dd->name, dd);
   nxlog_debug_tag(DEBUG_TAG, 4, L"Chat bot driver %s registered successfully", dd->name);
}

/**
 * Chat session with single peer
 */
struct ChatBotSession
{
   std::string peerId;
   uint32_t userId;
   shared_ptr<Chat> chat;
   time_t lastActivity;
   bool busy;                          // AI request in progress
   std::string pendingInput;           // messages received while busy, fed into next request
   uint64_t activeQuestionId;          // pending question delivered to peer (0 = none)
   bool activeQuestionMultipleChoice;
   int activeQuestionOptionCount;

   ChatBotSession(const char *peerId, uint32_t userId) : peerId(peerId)
   {
      this->userId = userId;
      lastActivity = time(nullptr);
      busy = false;
      activeQuestionId = 0;
      activeQuestionMultipleChoice = false;
      activeQuestionOptionCount = 0;
   }
};

/**
 * Question data captured from chat's pending question for asynchronous delivery to the platform
 */
struct QuestionDelivery
{
   std::string peerId;
   uint64_t questionId;
   std::string text;
   bool multipleChoice;
   ConfirmationType confirmationType;
   StringList options;

   QuestionDelivery(const std::string& peerId, const PendingQuestion& question) : peerId(peerId), options(question.options)
   {
      questionId = question.id;
      text = question.text;
      multipleChoice = question.isMultipleChoice;
      confirmationType = question.confirmationType;
   }
};

/**
 * Configured chat bot
 */
class ChatBot : public ChatBotMessageSink
{
private:
   wchar_t m_name[MAX_OBJECT_NAME];
   wchar_t m_description[MAX_NC_DESCRIPTION];
   wchar_t m_driverName[MAX_OBJECT_NAME];
   char *m_configuration;
   uint32_t m_idleTimeout;    // Session idle timeout in seconds
   char m_providerSlot[32];   // AI provider slot for sessions (empty = chat default)
   ChatBotDriver *m_driver;
   Mutex m_driverLock;
   NCDriverStorageManager *m_storageManager;
   bool m_healthCheckStatus;
   time_t m_lastInboundMessageTime;
   wchar_t m_errorMessage[MAX_NC_ERROR_MESSAGE];
   std::unordered_map<std::string, uint32_t> m_userMapping;
   std::unordered_map<std::string, shared_ptr<ChatBotSession>> m_sessions;
   Mutex m_lock;
   weak_ptr<ChatBot> m_self;

   void setError(const wchar_t *message)
   {
      wcslcpy(m_errorMessage, message, MAX_NC_ERROR_MESSAGE);
   }

   shared_ptr<Chat> acquireChat(const shared_ptr<ChatBotSession>& session);
   void processRequest(shared_ptr<ChatBotSession> session, std::string input);
   void processCommand(const shared_ptr<ChatBotSession>& session, const char *command);
   bool processQuestionReply(const shared_ptr<ChatBotSession>& session, const char *text);
   void submitQuestionResponse(const shared_ptr<ChatBotSession>& session, uint64_t questionId, int selectedOption);
   void replyAsync(const std::string& peerId, const std::string& text);
   void closeSession(const shared_ptr<ChatBotSession>& session, const wchar_t *reason);

public:
   ChatBot(const wchar_t *name, const wchar_t *description, const wchar_t *driverName, char *configuration,
         uint32_t idleTimeout, const char *providerSlot);
   virtual ~ChatBot();

   void setSelf(const shared_ptr<ChatBot>& self) { m_self = self; }

   virtual void onMessage(const char *peerId, const char *displayName, const char *text) override;
   virtual void onChoiceResponse(const char *peerId, uint64_t questionId, int selectedOption) override;

   bool startDriver();
   void stopDriver();
   void shutdown();

   const wchar_t *getName() const { return m_name; }
   const wchar_t *getDescription() const { return m_description; }
   const wchar_t *getDriverName() const { return m_driverName; }
   bool isHealthy() const { return m_healthCheckStatus; }

   void setUserMapping(const StructArray<ChatBotUserMapping>& userMappings);
   void update(const wchar_t *description, const wchar_t *driverName, char *configuration, uint32_t idleTimeout,
         const char *providerSlot, const StructArray<ChatBotUserMapping>& userMappings);
   void updateName(const wchar_t *newName) { wcslcpy(m_name, newName, MAX_OBJECT_NAME); }

   bool sendMessageToPeer(const char *peerId, const char *text);
   void deliverQuestion(const QuestionDelivery& question);

   void checkHealth();
   void expireIdleSessions(time_t now);

   void fillMessage(NXCPMessage *msg, uint32_t baseId);
   json_t *toJson(bool includeSensitiveData);
   void saveToDatabase();
   void deleteFromDatabase();
};

/**
 * Configured chat bots
 */
static SharedStringObjectMap<ChatBot> s_bots;
static Mutex s_botsLock;

/**
 * Monitoring thread (health checks and session expiration)
 */
static THREAD s_monitorThread = INVALID_THREAD_HANDLE;

/**
 * Notification channel driver adapter wrapping live chat bot. Registered as backing driver
 * for the automatically created notification channel with the same name as the bot.
 */
class ChatBotChannelDriver : public NCDriver
{
private:
   shared_ptr<ChatBot> m_bot;

public:
   ChatBotChannelDriver(const shared_ptr<ChatBot>& bot) : NCDriver(), m_bot(bot) { }

   virtual int send(const NotificationContext& context) override
   {
      if ((context.recipient == nullptr) || (*context.recipient == 0))
         return -1;

      std::string text;
      if ((context.subject != nullptr) && (*context.subject != 0))
      {
         text.append(context.subject);
         text.append("\n\n");
      }
      if (context.body != nullptr)
         text.append(context.body);
      return m_bot->sendMessageToPeer(context.recipient, text.c_str()) ? 0 : -1;
   }

   virtual bool checkHealth() override
   {
      return m_bot->isHealthy();
   }
};

/**
 * Chat bot constructor. Takes ownership of configuration string.
 */
ChatBot::ChatBot(const wchar_t *name, const wchar_t *description, const wchar_t *driverName, char *configuration,
      uint32_t idleTimeout, const char *providerSlot) : m_driverLock(MutexType::FAST), m_lock(MutexType::FAST)
{
   wcslcpy(m_name, name, MAX_OBJECT_NAME);
   wcslcpy(m_description, CHECK_NULL_EX(description), MAX_NC_DESCRIPTION);
   wcslcpy(m_driverName, driverName, MAX_OBJECT_NAME);
   m_configuration = (configuration != nullptr) ? configuration : MemCopyStringA("");
   m_idleTimeout = (idleTimeout > 0) ? idleTimeout : DEFAULT_SESSION_IDLE_TIMEOUT;
   strlcpy(m_providerSlot, CHECK_NULL_EX_A(providerSlot), sizeof(m_providerSlot));
   m_driver = nullptr;
   m_storageManager = CreateNCDriverStorageManager(name);
   m_healthCheckStatus = false;
   m_lastInboundMessageTime = 0;
   m_errorMessage[0] = 0;
}

/**
 * Chat bot destructor
 */
ChatBot::~ChatBot()
{
   delete m_driver;
   DestroyNCDriverStorageManager(m_storageManager);
   MemFree(m_configuration);
}

/**
 * Create driver instance and start platform connection
 */
bool ChatBot::startDriver()
{
   ChatBotDriverDescriptor *dd = s_driverList.get(m_driverName);
   if (dd == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot find chat bot driver %s for chat bot %s", m_driverName, m_name);
      setError(L"Cannot find driver");
      return false;
   }

   Config config(false);
   if (!config.loadConfigFromMemory(m_configuration, strlen(m_configuration), m_driverName, nullptr, true, false))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot parse driver configuration for chat bot %s", m_name);
      setError(L"Cannot parse driver configuration");
      return false;
   }

   ChatBotDriver *driver = dd->instanceFactory(&config, m_storageManager);
   if (driver == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Unable to create instance of chat bot driver %s for chat bot %s", m_driverName, m_name);
      setError(L"Unable to create driver instance");
      return false;
   }

   if (!driver->start(this))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Unable to start chat bot driver %s for chat bot %s", m_driverName, m_name);
      setError(L"Unable to start driver");
      delete driver;
      return false;
   }

   m_driverLock.lock();
   m_driver = driver;
   m_driverLock.unlock();
   m_errorMessage[0] = 0;
   m_healthCheckStatus = true;
   nxlog_debug_tag(DEBUG_TAG, 3, L"Chat bot \"%s\" started (driver %s)", m_name, m_driverName);
   return true;
}

/**
 * Stop platform connection and destroy driver instance
 */
void ChatBot::stopDriver()
{
   m_driverLock.lock();
   ChatBotDriver *driver = m_driver;
   m_driver = nullptr;
   m_driverLock.unlock();
   if (driver != nullptr)
   {
      driver->stop();
      delete driver;
   }
   m_healthCheckStatus = false;
}

/**
 * Shutdown chat bot - stop driver and close all sessions
 */
void ChatBot::shutdown()
{
   stopDriver();

   m_lock.lock();
   std::unordered_map<std::string, shared_ptr<ChatBotSession>> sessions;
   sessions.swap(m_sessions);
   m_lock.unlock();

   for(auto it = sessions.begin(); it != sessions.end(); ++it)
      closeSession(it->second, L"chat bot shutdown");
}

/**
 * Close session - delete associated AI chat
 */
void ChatBot::closeSession(const shared_ptr<ChatBotSession>& session, const wchar_t *reason)
{
   if (session->chat != nullptr)
   {
      DeleteAIAssistantChat(session->chat->getId(), session->userId);
      session->chat.reset();
   }
   nxlog_debug_tag(DEBUG_TAG, 4, L"Chat bot \"%s\": session with peer %hs closed (%s)", m_name, session->peerId.c_str(), reason);
   WriteAuditLog(AUDIT_SECURITY, true, session->userId, nullptr, AUDIT_SYSTEM_SID, 0,
         L"Chat bot \"%s\" session with peer \"%hs\" closed (%s)", m_name, session->peerId.c_str(), reason);
}

/**
 * Replace user mapping (called with configuration coming from client or database)
 */
void ChatBot::setUserMapping(const StructArray<ChatBotUserMapping>& userMappings)
{
   m_lock.lock();
   m_userMapping.clear();
   for(int i = 0; i < userMappings.size(); i++)
   {
      const ChatBotUserMapping *m = userMappings.get(i);
      m_userMapping[m->peerId] = m->userId;
   }

   // Drop sessions for peers that are no longer mapped or are mapped to a different user
   std::vector<shared_ptr<ChatBotSession>> droppedSessions;
   for(auto it = m_sessions.begin(); it != m_sessions.end(); )
   {
      auto m = m_userMapping.find(it->first);
      if ((m == m_userMapping.end()) || (m->second != it->second->userId))
      {
         droppedSessions.push_back(it->second);
         it = m_sessions.erase(it);
      }
      else
      {
         ++it;
      }
   }
   m_lock.unlock();

   for(size_t i = 0; i < droppedSessions.size(); i++)
      closeSession(droppedSessions[i], L"user mapping changed");
}

/**
 * Update chat bot configuration. Takes ownership of configuration string.
 */
void ChatBot::update(const wchar_t *description, const wchar_t *driverName, char *configuration, uint32_t idleTimeout,
      const char *providerSlot, const StructArray<ChatBotUserMapping>& userMappings)
{
   wcslcpy(m_description, CHECK_NULL_EX(description), MAX_NC_DESCRIPTION);
   m_idleTimeout = (idleTimeout > 0) ? idleTimeout : DEFAULT_SESSION_IDLE_TIMEOUT;
   strlcpy(m_providerSlot, CHECK_NULL_EX_A(providerSlot), sizeof(m_providerSlot));

   if (wcscmp(m_driverName, driverName) || strcmp(m_configuration, configuration))
   {
      stopDriver();
      wcslcpy(m_driverName, driverName, MAX_OBJECT_NAME);
      MemFree(m_configuration);
      m_configuration = configuration;
      startDriver();
   }
   else
   {
      MemFree(configuration);
   }

   setUserMapping(userMappings);
   saveToDatabase();
}

/**
 * Send message to given peer through platform driver
 */
bool ChatBot::sendMessageToPeer(const char *peerId, const char *text)
{
   bool success = false;
   m_driverLock.lock();
   if (m_driver != nullptr)
      success = m_driver->sendMessage(peerId, text);
   m_driverLock.unlock();
   return success;
}

/**
 * Send reply to peer asynchronously (used from driver transport thread which must not block)
 */
void ChatBot::replyAsync(const std::string& peerId, const std::string& text)
{
   shared_ptr<ChatBot> bot = m_self.lock();
   if (bot == nullptr)
      return;
   ThreadPoolExecute(g_mainThreadPool,
      [bot, peerId, text] ()
      {
         bot->sendMessageToPeer(peerId.c_str(), text.c_str());
      });
}

/**
 * Get or create AI assistant chat for session
 */
shared_ptr<Chat> ChatBot::acquireChat(const shared_ptr<ChatBotSession>& session)
{
   m_lock.lock();
   shared_ptr<Chat> chat = session->chat;
   m_lock.unlock();
   if (chat != nullptr)
      return chat;

   uint32_t rcc;
   chat = CreateAIAssistantChat(session->userId, 0, &rcc);
   if (chat == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Chat bot \"%s\": cannot create AI assistant chat for user [%u] (RCC=%u)", m_name, session->userId, rcc);
      return shared_ptr<Chat>();
   }

   if (m_providerSlot[0] != 0)
      chat->setSlot(m_providerSlot);

   weak_ptr<ChatBot> weakBot = m_self;
   std::string peerId = session->peerId;
   chat->setQuestionListener(
      [weakBot, peerId] (const PendingQuestion& question)
      {
         shared_ptr<ChatBot> bot = weakBot.lock();
         if (bot == nullptr)
            return;
         auto delivery = make_shared<QuestionDelivery>(peerId, question);
         ThreadPoolExecute(g_mainThreadPool,
            [bot, delivery] ()
            {
               bot->deliverQuestion(*delivery);
            });
      });

   m_lock.lock();
   session->chat = chat;
   m_lock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 4, L"Chat bot \"%s\": AI assistant chat [%u] created for peer %hs (user [%u])",
         m_name, chat->getId(), session->peerId.c_str(), session->userId);
   WriteAuditLog(AUDIT_SECURITY, true, session->userId, nullptr, AUDIT_SYSTEM_SID, 0,
         L"Chat bot \"%s\" session opened for peer \"%hs\"", m_name, session->peerId.c_str());
   return chat;
}

/**
 * Process AI assistant request (executed in thread pool)
 */
void ChatBot::processRequest(shared_ptr<ChatBotSession> session, std::string input)
{
   while(true)
   {
      shared_ptr<Chat> chat = acquireChat(session);
      if (chat == nullptr)
      {
         sendMessageToPeer(session->peerId.c_str(), "Cannot connect to AI assistant. Please contact your NetXMS administrator.");
         m_lock.lock();
         session->busy = false;
         session->pendingInput.clear();
         m_lock.unlock();
         return;
      }

      char *response = chat->sendRequest(input.c_str());
      if (response != nullptr)
      {
         sendMessageToPeer(session->peerId.c_str(), response);
         MemFree(response);
      }
      else
      {
         sendMessageToPeer(session->peerId.c_str(), "Request to AI assistant failed. Please try again later.");
      }

      m_lock.lock();
      session->lastActivity = time(nullptr);
      if (session->pendingInput.empty())
      {
         session->busy = false;
         m_lock.unlock();
         break;
      }
      input = session->pendingInput;
      session->pendingInput.clear();
      m_lock.unlock();
   }
}

/**
 * Process built-in command
 */
void ChatBot::processCommand(const shared_ptr<ChatBotSession>& session, const char *command)
{
   if (!strcmp(command, "/new"))
   {
      m_lock.lock();
      shared_ptr<Chat> chat = session->chat;
      session->activeQuestionId = 0;
      m_lock.unlock();
      if (chat != nullptr)
         chat->clear();
      replyAsync(session->peerId, "Started new conversation.");
   }
   else if (!strcmp(command, "/whoami"))
   {
      wchar_t userName[MAX_USER_NAME];
      ResolveUserId(session->userId, userName, true);
      char *userNameUtf8 = UTF8StringFromWideString(userName);
      char buffer[512];
      snprintf(buffer, sizeof(buffer), "You are mapped to NetXMS user \"%s\" (ID %u). Your peer ID is \"%s\".",
            userNameUtf8, session->userId, session->peerId.c_str());
      MemFree(userNameUtf8);
      replyAsync(session->peerId, buffer);
   }
   else if (!strcmp(command, "/help"))
   {
      replyAsync(session->peerId,
            "Available commands:\n"
            "/new - start a new conversation\n"
            "/whoami - show mapped NetXMS user identity\n"
            "/help - show this message\n"
            "Any other message is sent to the AI assistant.");
   }
   else
   {
      replyAsync(session->peerId, "Unknown command. Send /help for the list of available commands.");
   }
}

/**
 * Check if text message is a numeric reply to a pending question and process it if so.
 * Returns true if message was consumed as question reply. Caller holds bot lock.
 */
bool ChatBot::processQuestionReply(const shared_ptr<ChatBotSession>& session, const char *text)
{
   if ((session->activeQuestionId == 0) || (session->chat == nullptr) || !session->chat->hasPendingQuestion())
      return false;

   char *eptr;
   long n = strtol(text, &eptr, 10);
   while(*eptr == ' ')
      eptr++;
   if ((*eptr != 0) || (n < 1) || (n > session->activeQuestionOptionCount))
      return false;

   uint64_t questionId = session->activeQuestionId;
   session->activeQuestionId = 0;

   shared_ptr<ChatBot> bot = m_self.lock();
   if (bot != nullptr)
   {
      int selectedOption = static_cast<int>(n) - 1;
      ThreadPoolExecute(g_mainThreadPool,
         [bot, session, questionId, selectedOption] ()
         {
            bot->submitQuestionResponse(session, questionId, selectedOption);
         });
   }
   return true;
}

/**
 * Submit response to pending question in session's chat
 */
void ChatBot::submitQuestionResponse(const shared_ptr<ChatBotSession>& session, uint64_t questionId, int selectedOption)
{
   m_lock.lock();
   shared_ptr<Chat> chat = session->chat;
   bool multipleChoice = session->activeQuestionMultipleChoice;
   m_lock.unlock();
   if (chat == nullptr)
      return;

   if (multipleChoice)
      chat->handleQuestionResponse(questionId, true, selectedOption);
   else
      chat->handleQuestionResponse(questionId, selectedOption == 0, -1);

   WriteAuditLog(AUDIT_SECURITY, true, session->userId, nullptr, AUDIT_SYSTEM_SID, 0,
         L"Chat bot \"%s\": question " UINT64_FMT L" answered by peer \"%hs\" (option %d)",
         m_name, questionId, session->peerId.c_str(), selectedOption + 1);
}

/**
 * Deliver pending question to peer, using platform interactive elements when available
 * and falling back to numbered plain text options otherwise
 */
void ChatBot::deliverQuestion(const QuestionDelivery& question)
{
   m_lock.lock();
   auto it = m_sessions.find(question.peerId);
   if (it == m_sessions.end())
   {
      m_lock.unlock();
      return;
   }
   shared_ptr<ChatBotSession> session = it->second;

   StringList options;
   if (question.multipleChoice)
   {
      options.addAll(&question.options);
   }
   else
   {
      switch(question.confirmationType)
      {
         case ConfirmationType::YES_NO:
            options.add(L"Yes");
            options.add(L"No");
            break;
         case ConfirmationType::CONFIRM_CANCEL:
            options.add(L"Confirm");
            options.add(L"Cancel");
            break;
         default:
            options.add(L"Approve");
            options.add(L"Reject");
            break;
      }
   }

   session->activeQuestionId = question.questionId;
   session->activeQuestionMultipleChoice = question.multipleChoice;
   session->activeQuestionOptionCount = options.size();
   m_lock.unlock();

   m_driverLock.lock();
   bool delivered = (m_driver != nullptr) ? m_driver->sendQuestion(question.peerId.c_str(), question.text.c_str(), options, question.questionId) : false;
   m_driverLock.unlock();

   if (!delivered)
   {
      // Numbered list fallback - next numeric reply from the peer is interpreted as the answer
      std::string text(question.text);
      text.append("\n");
      for(int i = 0; i < options.size(); i++)
      {
         char buffer[16];
         snprintf(buffer, sizeof(buffer), "\n%d. ", i + 1);
         text.append(buffer);
         char *option = UTF8StringFromWideString(options.get(i));
         text.append(option);
         MemFree(option);
      }
      text.append("\n\nReply with the option number.");
      sendMessageToPeer(question.peerId.c_str(), text.c_str());
   }
}

/**
 * Incoming message from platform (called from driver transport thread)
 */
void ChatBot::onMessage(const char *peerId, const char *displayName, const char *text)
{
   if ((peerId == nullptr) || (text == nullptr))
      return;

   m_lastInboundMessageTime = time(nullptr);

   m_lock.lock();
   auto mapping = m_userMapping.find(peerId);
   if (mapping == m_userMapping.end())
   {
      m_lock.unlock();
      // Silently ignore unmapped peers - the bot does not confirm its existence to strangers
      nxlog_debug_tag(DEBUG_TAG, 5, L"Chat bot \"%s\": message from unmapped peer %hs (%hs) ignored", m_name, peerId, CHECK_NULL_EX_A(displayName));
      return;
   }
   uint32_t userId = mapping->second;

   shared_ptr<ChatBotSession> session;
   auto it = m_sessions.find(peerId);
   if (it != m_sessions.end())
   {
      session = it->second;
   }
   else
   {
      session = make_shared<ChatBotSession>(peerId, userId);
      m_sessions[peerId] = session;
      nxlog_debug_tag(DEBUG_TAG, 4, L"Chat bot \"%s\": new session for peer %hs (user [%u])", m_name, peerId, userId);
   }
   session->lastActivity = time(nullptr);

   // Skip leading whitespace
   while((*text == ' ') || (*text == '\t'))
      text++;

   if (*text == '/')
   {
      m_lock.unlock();
      nxlog_debug_tag(DEBUG_TAG, 6, L"Chat bot \"%s\": command \"%hs\" from peer %hs", m_name, text, peerId);
      processCommand(session, text);
      return;
   }

   if (processQuestionReply(session, text))
   {
      m_lock.unlock();
      return;
   }

   if (session->busy)
   {
      bool notify = session->pendingInput.empty();
      if (!session->pendingInput.empty())
         session->pendingInput.append("\n");
      session->pendingInput.append(text);
      m_lock.unlock();
      if (notify)
         replyAsync(session->peerId, "Still working on your previous request. Your message will be processed next.");
      return;
   }

   session->busy = true;
   m_lock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 6, L"Chat bot \"%s\": dispatching message from peer %hs to AI assistant", m_name, peerId);
   shared_ptr<ChatBot> bot = m_self.lock();
   if (bot != nullptr)
   {
      std::string input(text);
      ThreadPoolExecute(g_mainThreadPool,
         [bot, session, input] ()
         {
            bot->processRequest(session, input);
         });
   }
}

/**
 * Response to interactive question (called from driver transport thread)
 */
void ChatBot::onChoiceResponse(const char *peerId, uint64_t questionId, int selectedOption)
{
   if (peerId == nullptr)
      return;

   m_lock.lock();
   auto it = m_sessions.find(peerId);
   if (it == m_sessions.end())
   {
      m_lock.unlock();
      return;
   }
   shared_ptr<ChatBotSession> session = it->second;
   if ((session->activeQuestionId != questionId) || (selectedOption < 0) || (selectedOption >= session->activeQuestionOptionCount))
   {
      m_lock.unlock();
      nxlog_debug_tag(DEBUG_TAG, 5, L"Chat bot \"%s\": stale or invalid choice response from peer %hs (question " UINT64_FMT L", option %d)",
            m_name, peerId, questionId, selectedOption);
      return;
   }
   session->activeQuestionId = 0;
   session->lastActivity = time(nullptr);
   m_lock.unlock();

   shared_ptr<ChatBot> bot = m_self.lock();
   if (bot != nullptr)
   {
      ThreadPoolExecute(g_mainThreadPool,
         [bot, session, questionId, selectedOption] ()
         {
            bot->submitQuestionResponse(session, questionId, selectedOption);
         });
   }
}

/**
 * Check driver health
 */
void ChatBot::checkHealth()
{
   bool status = false;
   m_driverLock.lock();
   if (m_driver != nullptr)
      status = m_driver->checkHealth();
   m_driverLock.unlock();
   if (status != m_healthCheckStatus)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Chat bot \"%s\": health check status changed to %s", m_name, status ? L"OK" : L"FAILED");
      m_healthCheckStatus = status;
      NotifyClientSessions(NX_NOTIFY_CHATBOT_CHANGED, 0);
   }
}

/**
 * Close sessions idle past the configured timeout
 */
void ChatBot::expireIdleSessions(time_t now)
{
   std::vector<shared_ptr<ChatBotSession>> expiredSessions;
   m_lock.lock();
   for(auto it = m_sessions.begin(); it != m_sessions.end(); )
   {
      shared_ptr<ChatBotSession> session = it->second;
      if (!session->busy && (now - session->lastActivity > static_cast<time_t>(m_idleTimeout)) &&
          ((session->chat == nullptr) || !session->chat->hasPendingQuestion()))
      {
         expiredSessions.push_back(session);
         it = m_sessions.erase(it);
      }
      else
      {
         ++it;
      }
   }
   m_lock.unlock();

   for(size_t i = 0; i < expiredSessions.size(); i++)
      closeSession(expiredSessions[i], L"idle timeout");
}

/**
 * Fill NXCP message with chat bot data
 */
void ChatBot::fillMessage(NXCPMessage *msg, uint32_t baseId)
{
   msg->setField(baseId, m_name);
   msg->setField(baseId + 1, m_description);
   msg->setField(baseId + 2, m_driverName);
   msg->setFieldFromMBString(baseId + 3, m_configuration);
   m_driverLock.lock();
   msg->setField(baseId + 4, m_driver != nullptr);
   m_driverLock.unlock();
   msg->setField(baseId + 5, m_healthCheckStatus);
   msg->setField(baseId + 6, m_idleTimeout);
   msg->setFieldFromUtf8String(baseId + 7, m_providerSlot);
   m_lock.lock();
   msg->setField(baseId + 8, static_cast<uint32_t>(m_sessions.size()));
   m_lock.unlock();
   msg->setFieldFromTime(baseId + 9, m_lastInboundMessageTime);
   msg->setField(baseId + 10, m_errorMessage);

   m_lock.lock();
   msg->setField(baseId + 11, static_cast<uint32_t>(m_userMapping.size()));
   uint32_t fieldId = baseId + 12;
   for(auto it = m_userMapping.begin(); it != m_userMapping.end(); ++it)
   {
      msg->setFieldFromUtf8String(fieldId++, it->first.c_str());
      msg->setField(fieldId++, it->second);
   }
   m_lock.unlock();
}

/**
 * Convert chat bot to JSON
 */
json_t *ChatBot::toJson(bool includeSensitiveData)
{
   json_t *root = json_object();
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "driverName", json_string_t(m_driverName));
   if (includeSensitiveData)
      json_object_set_new(root, "configuration", json_string_a(m_configuration));
   m_driverLock.lock();
   json_object_set_new(root, "driverInitialized", json_boolean(m_driver != nullptr));
   m_driverLock.unlock();
   json_object_set_new(root, "healthCheckStatus", json_boolean(m_healthCheckStatus));
   json_object_set_new(root, "idleTimeout", json_integer(m_idleTimeout));
   json_object_set_new(root, "providerSlot", json_string(m_providerSlot));
   json_object_set_new(root, "lastInboundMessageTime", json_time_string(m_lastInboundMessageTime));
   json_object_set_new(root, "errorMessage", json_string_t(m_errorMessage));

   m_lock.lock();
   json_object_set_new(root, "activeSessions", json_integer(m_sessions.size()));
   json_t *userMappings = json_array();
   for(auto it = m_userMapping.begin(); it != m_userMapping.end(); ++it)
   {
      json_t *mapping = json_object();
      json_object_set_new(mapping, "peerId", json_string(it->first.c_str()));
      json_object_set_new(mapping, "userId", json_integer(it->second));
      json_array_append_new(userMappings, mapping);
   }
   m_lock.unlock();
   json_object_set_new(root, "userMappings", userMappings);
   return root;
}

/**
 * Save chat bot configuration to database
 */
void ChatBot::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = DBBegin(hdb);
   if (success)
   {
      static const wchar_t *columns[] = { L"driver_name", L"description", L"configuration", L"idle_timeout", L"provider_slot", nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"chat_bots", L"name", m_name, columns);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_driverName, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_configuration, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_idleTimeout);
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_providerSlot, DB_BIND_STATIC);
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, L"DELETE FROM chat_bot_users WHERE channel_name=?");
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      if (success)
      {
         m_lock.lock();
         if (!m_userMapping.empty())
         {
            hStmt = DBPrepare(hdb, L"INSERT INTO chat_bot_users (channel_name,peer_id,user_id) VALUES (?,?,?)", m_userMapping.size() > 1);
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
               for(auto it = m_userMapping.begin(); success && (it != m_userMapping.end()); ++it)
               {
                  DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, it->first.c_str(), DB_BIND_STATIC);
                  DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, it->second);
                  success = DBExecute(hStmt);
               }
               DBFreeStatement(hStmt);
            }
            else
            {
               success = false;
            }
         }
         m_lock.unlock();
      }

      if (success)
         DBCommit(hdb);
      else
         DBRollback(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Delete chat bot from database (including driver persistent storage)
 */
void ChatBot::deleteFromDatabase()
{
   static const wchar_t *queries[] = {
      L"DELETE FROM chat_bots WHERE name=?",
      L"DELETE FROM chat_bot_users WHERE channel_name=?",
      L"DELETE FROM nc_persistent_storage WHERE channel_name=?",
      nullptr
   };
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   for(int i = 0; queries[i] != nullptr; i++)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, queries[i]);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Create chat bot object, start its driver, and register companion notification channel
 */
static shared_ptr<ChatBot> CreateChatBotObject(const wchar_t *name, const wchar_t *description, const wchar_t *driverName,
      char *configuration, uint32_t idleTimeout, const char *providerSlot, const StructArray<ChatBotUserMapping>& userMappings)
{
   shared_ptr<ChatBot> bot = make_shared<ChatBot>(name, description, driverName, configuration, idleTimeout, providerSlot);
   bot->setSelf(bot);
   bot->setUserMapping(userMappings);
   bot->startDriver();
   RegisterChatBotNotificationChannel(name, description, new ChatBotChannelDriver(bot));
   return bot;
}

/**
 * Check if chat bot with given name exists
 */
bool NXCORE_EXPORTABLE IsChatBotExists(const wchar_t *name)
{
   s_botsLock.lock();
   bool exists = s_bots.contains(name);
   s_botsLock.unlock();
   return exists;
}

/**
 * Create new chat bot. Takes ownership of configuration string.
 */
uint32_t NXCORE_EXPORTABLE CreateChatBot(const wchar_t *name, const wchar_t *description, const wchar_t *driverName, char *configuration,
      uint32_t idleTimeout, const char *providerSlot, const StructArray<ChatBotUserMapping>& userMappings)
{
   s_botsLock.lock();
   if (s_bots.contains(name))
   {
      s_botsLock.unlock();
      MemFree(configuration);
      return RCC_CHANNEL_ALREADY_EXIST;
   }
   shared_ptr<ChatBot> bot = CreateChatBotObject(name, description, driverName,
         (configuration != nullptr) ? configuration : MemCopyStringA(""), idleTimeout, providerSlot, userMappings);
   s_bots.set(name, bot);
   bot->saveToDatabase();
   s_botsLock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 3, L"Chat bot \"%s\" created", name);
   NotifyClientSessions(NX_NOTIFY_CHATBOT_CHANGED, 0);
   return RCC_SUCCESS;
}

/**
 * Update existing chat bot. Takes ownership of configuration string.
 */
uint32_t NXCORE_EXPORTABLE UpdateChatBot(const wchar_t *name, const wchar_t *description, const wchar_t *driverName, char *configuration,
      uint32_t idleTimeout, const char *providerSlot, const StructArray<ChatBotUserMapping>& userMappings)
{
   s_botsLock.lock();
   shared_ptr<ChatBot> bot = s_bots.getShared(name);
   s_botsLock.unlock();
   if (bot == nullptr)
   {
      MemFree(configuration);
      return RCC_NO_CHANNEL_NAME;
   }
   bot->update(description, driverName, (configuration != nullptr) ? configuration : MemCopyStringA(""), idleTimeout, providerSlot, userMappings);
   NotifyClientSessions(NX_NOTIFY_CHATBOT_CHANGED, 0);
   return RCC_SUCCESS;
}

/**
 * Rename chat bot (also renames companion notification channel)
 */
uint32_t NXCORE_EXPORTABLE RenameChatBot(const wchar_t *name, const wchar_t *newName)
{
   s_botsLock.lock();
   if (s_bots.contains(newName))
   {
      s_botsLock.unlock();
      return RCC_CHANNEL_ALREADY_EXIST;
   }
   shared_ptr<ChatBot> bot = s_bots.unlink(name);
   if (bot == nullptr)
   {
      s_botsLock.unlock();
      return RCC_NO_CHANNEL_NAME;
   }
   bot->updateName(newName);
   s_bots.set(newName, bot);
   s_botsLock.unlock();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   static const wchar_t *queries[] = {
      L"UPDATE chat_bots SET name=? WHERE name=?",
      L"UPDATE chat_bot_users SET channel_name=? WHERE channel_name=?",
      L"UPDATE nc_persistent_storage SET channel_name=? WHERE channel_name=?",
      nullptr
   };
   bool success = DBBegin(hdb);
   for(int i = 0; success && (queries[i] != nullptr); i++)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, queries[i]);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, newName, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }
   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
   DBConnectionPoolReleaseConnection(hdb);

   RenameChatBotNotificationChannel(name, newName);
   NotifyClientSessions(NX_NOTIFY_CHATBOT_CHANGED, 0);
   return RCC_SUCCESS;
}

/**
 * Delete chat bot
 */
uint32_t NXCORE_EXPORTABLE DeleteChatBot(const wchar_t *name)
{
   s_botsLock.lock();
   shared_ptr<ChatBot> bot = s_bots.unlink(name);
   s_botsLock.unlock();
   if (bot == nullptr)
      return RCC_NO_CHANNEL_NAME;

   UnregisterChatBotNotificationChannel(name);
   bot->shutdown();
   bot->deleteFromDatabase();
   nxlog_debug_tag(DEBUG_TAG, 3, L"Chat bot \"%s\" deleted", name);
   NotifyClientSessions(NX_NOTIFY_CHATBOT_CHANGED, 0);
   return RCC_SUCCESS;
}

/**
 * Fill NXCP message with list of configured chat bots
 */
void NXCORE_EXPORTABLE GetChatBots(NXCPMessage *msg)
{
   s_botsLock.lock();
   msg->setField(VID_NUM_ELEMENTS, s_bots.size());
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   auto it = s_bots.begin();
   while(it.hasNext())
   {
      shared_ptr<ChatBot> bot = *it.next()->value;
      bot->fillMessage(msg, fieldId);
      fieldId += 4096;
   }
   s_botsLock.unlock();
}

/**
 * Get all chat bots as JSON array
 */
json_t NXCORE_EXPORTABLE *GetChatBots(bool includeSensitiveData)
{
   json_t *bots = json_array();
   s_botsLock.lock();
   auto it = s_bots.begin();
   while(it.hasNext())
   {
      shared_ptr<ChatBot> bot = *it.next()->value;
      json_array_append_new(bots, bot->toJson(includeSensitiveData));
   }
   s_botsLock.unlock();
   return bots;
}

/**
 * Get single chat bot as JSON by name
 */
json_t NXCORE_EXPORTABLE *GetChatBotByName(const wchar_t *name, bool includeSensitiveData)
{
   json_t *result = nullptr;
   s_botsLock.lock();
   ChatBot *bot = s_bots.get(name);
   if (bot != nullptr)
      result = bot->toJson(includeSensitiveData);
   s_botsLock.unlock();
   return result;
}

/**
 * Fill NXCP message with list of registered chat bot drivers
 */
void NXCORE_EXPORTABLE GetChatBotDrivers(NXCPMessage *msg)
{
   StringList driverNames = s_driverList.keys();
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < driverNames.size(); i++)
      msg->setField(fieldId++, driverNames.get(i));
   msg->setField(VID_DRIVER_COUNT, s_driverList.size());
}

/**
 * Get chat bot drivers as JSON array
 */
json_t NXCORE_EXPORTABLE *GetChatBotDriversAsJson()
{
   json_t *drivers = json_array();
   StringList driverNames = s_driverList.keys();
   for(int i = 0; i < driverNames.size(); i++)
      json_array_append_new(drivers, json_string_t(driverNames.get(i)));
   return drivers;
}

/**
 * Read user mappings from JSON array of { "peerId": ..., "userId": ... } objects
 */
static void UserMappingsFromJson(json_t *userMappings, StructArray<ChatBotUserMapping> *mappings)
{
   if (!json_is_array(userMappings))
      return;

   size_t i;
   json_t *element;
   json_array_foreach(userMappings, i, element)
   {
      const char *peerId = json_object_get_string_utf8(element, "peerId", nullptr);
      if ((peerId == nullptr) || (*peerId == 0))
         continue;
      ChatBotUserMapping *m = mappings->addPlaceholder();
      memset(m, 0, sizeof(ChatBotUserMapping));
      strlcpy(m->peerId, peerId, sizeof(m->peerId));
      m->userId = json_object_get_uint32(element, "userId", 0);
   }
}

/**
 * Create chat bot from JSON configuration (WebAPI)
 */
uint32_t NXCORE_EXPORTABLE CreateChatBotFromJson(json_t *config)
{
   wchar_t *name = json_object_get_string_w(config, "name", nullptr);
   wchar_t *driverName = json_object_get_string_w(config, "driverName", nullptr);
   if ((name == nullptr) || (*name == 0) || (driverName == nullptr) || (*driverName == 0))
   {
      MemFree(name);
      MemFree(driverName);
      return RCC_INVALID_ARGUMENT;
   }

   wchar_t *description = json_object_get_string_w(config, "description", L"");
   char *configuration = MemCopyStringA(json_object_get_string_utf8(config, "configuration", ""));
   uint32_t idleTimeout = json_object_get_uint32(config, "idleTimeout", 0);
   const char *providerSlot = json_object_get_string_utf8(config, "providerSlot", "");

   StructArray<ChatBotUserMapping> mappings;
   UserMappingsFromJson(json_object_get(config, "userMappings"), &mappings);

   uint32_t rcc = CreateChatBot(name, description, driverName, configuration, idleTimeout, providerSlot, mappings);
   MemFree(name);
   MemFree(driverName);
   MemFree(description);
   return rcc;
}

/**
 * Update chat bot from JSON configuration (WebAPI). Absent fields are taken from current configuration.
 */
uint32_t NXCORE_EXPORTABLE UpdateChatBotFromJson(const wchar_t *name, json_t *config)
{
   json_t *current = GetChatBotByName(name, true);
   if (current == nullptr)
      return RCC_NO_CHANNEL_NAME;

   json_t *effective = json_copy(current);
   json_object_update(effective, config);
   json_decref(current);

   wchar_t *driverName = json_object_get_string_w(effective, "driverName", L"");
   wchar_t *description = json_object_get_string_w(effective, "description", L"");
   char *configuration = MemCopyStringA(json_object_get_string_utf8(effective, "configuration", ""));
   uint32_t idleTimeout = json_object_get_uint32(effective, "idleTimeout", 0);
   const char *providerSlot = json_object_get_string_utf8(effective, "providerSlot", "");

   StructArray<ChatBotUserMapping> mappings;
   UserMappingsFromJson(json_object_get(effective, "userMappings"), &mappings);

   uint32_t rcc = UpdateChatBot(name, description, driverName, configuration, idleTimeout, providerSlot, mappings);
   MemFree(driverName);
   MemFree(description);
   json_decref(effective);
   return rcc;
}

/**
 * Chat bot monitoring thread - driver health checks and idle session expiration
 */
static void ChatBotMonitorThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"Chat bot monitoring thread started");
   while(!SleepAndCheckForShutdown(60))
   {
      SharedObjectArray<ChatBot> bots;
      s_botsLock.lock();
      auto it = s_bots.begin();
      while(it.hasNext())
         bots.add(*it.next()->value);
      s_botsLock.unlock();

      time_t now = time(nullptr);
      for(int i = 0; i < bots.size(); i++)
      {
         ChatBot *bot = bots.get(i);
         bot->checkHealth();
         bot->expireIdleSessions(now);
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 2, L"Chat bot monitoring thread stopped");
}

/**
 * Load chat bot configuration from database
 */
void LoadChatBots()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, L"SELECT name,driver_name,description,configuration,idle_timeout,provider_slot FROM chat_bots");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         wchar_t name[MAX_OBJECT_NAME], driverName[MAX_OBJECT_NAME], description[MAX_NC_DESCRIPTION];
         DBGetField(hResult, i, 0, name, MAX_OBJECT_NAME);
         DBGetField(hResult, i, 1, driverName, MAX_OBJECT_NAME);
         DBGetField(hResult, i, 2, description, MAX_NC_DESCRIPTION);
         char *configuration = DBGetFieldA(hResult, i, 3, nullptr, 0);
         uint32_t idleTimeout = DBGetFieldULong(hResult, i, 4);
         char providerSlot[32];
         DBGetFieldUTF8(hResult, i, 5, providerSlot, sizeof(providerSlot));

         StructArray<ChatBotUserMapping> mappings;
         DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT peer_id,user_id FROM chat_bot_users WHERE channel_name=?");
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
            DB_RESULT hMappingResult = DBSelectPrepared(hStmt);
            if (hMappingResult != nullptr)
            {
               int mappingCount = DBGetNumRows(hMappingResult);
               for(int j = 0; j < mappingCount; j++)
               {
                  ChatBotUserMapping *m = mappings.addPlaceholder();
                  memset(m, 0, sizeof(ChatBotUserMapping));
                  DBGetFieldUTF8(hMappingResult, j, 0, m->peerId, sizeof(m->peerId));
                  m->userId = DBGetFieldULong(hMappingResult, j, 1);
               }
               DBFreeResult(hMappingResult);
            }
            DBFreeStatement(hStmt);
         }

         shared_ptr<ChatBot> bot = CreateChatBotObject(name, description, driverName,
               (configuration != nullptr) ? configuration : MemCopyStringA(""), idleTimeout, providerSlot, mappings);
         s_botsLock.lock();
         s_bots.set(name, bot);
         s_botsLock.unlock();
         nxlog_debug_tag(DEBUG_TAG, 4, L"Chat bot \"%s\" successfully created", name);
      }
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG, 1, L"%d chat bots loaded", count);
   }
   DBConnectionPoolReleaseConnection(hdb);

   s_monitorThread = ThreadCreateEx(ChatBotMonitorThread);
}

/**
 * Shutdown all chat bots
 */
void ShutdownChatBots()
{
   ThreadJoin(s_monitorThread);
   s_monitorThread = INVALID_THREAD_HANDLE;

   SharedObjectArray<ChatBot> bots;
   s_botsLock.lock();
   auto it = s_bots.begin();
   while(it.hasNext())
      bots.add(*it.next()->value);
   s_bots.clear();
   s_botsLock.unlock();

   for(int i = 0; i < bots.size(); i++)
      bots.get(i)->shutdown();

   nxlog_debug_tag(DEBUG_TAG, 3, L"All chat bots stopped");
}
