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
** File: ai_messages.cpp
**
**/

#include "nxcore.h"
#include <iris.h>
#include <ai_messages.h>

#define DEBUG_TAG L"ai.messages"

/**
 * Global message list
 */
static SharedPointerIndex<AIMessage> s_messages;

/**
 * Last used message ID
 */
static VolatileCounter s_messageId = 0;

/**
 * Cleanup thread handle
 */
static THREAD s_cleanupThread = INVALID_THREAD_HANDLE;

/**
 * Cleanup thread
 */
static void CleanupThread()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"AI message cleanup thread started");

   uint32_t cleanupInterval = ConfigReadULong(L"AI.MessageCleanupInterval", 60) * 60; // Convert to seconds

   while(!SleepAndCheckForShutdown(cleanupInterval))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"Running AI message cleanup");

      time_t now = time(nullptr);
      IntegerArray<uint32_t> expiredMessages;

      s_messages.forEach(
         [now, &expiredMessages] (AIMessage *msg) -> EnumerationCallbackResult
         {
            if (msg->isExpired())
            {
               expiredMessages.add(msg->getId());
               nxlog_debug_tag(DEBUG_TAG, 5, L"AI message %u expired", msg->getId());
            }
            return _CONTINUE;
         });

      for (int i = 0; i < expiredMessages.size(); i++)
      {
         uint32_t msgId = expiredMessages.get(i);
         shared_ptr<AIMessage> msg = s_messages.get(msgId);
         if (msg != nullptr)
         {
            msg->setStatus(AI_MSG_STATUS_EXPIRED);
            msg->deleteFromDatabase();
            s_messages.remove(msgId);
            NotifyAIMessageRecipients(msgId, false);
         }
      }

      nxlog_debug_tag(DEBUG_TAG, 6, L"AI message cleanup completed, %d messages expired", expiredMessages.size());
   }

   nxlog_debug_tag(DEBUG_TAG, 1, L"AI message cleanup thread stopped");
}

/**
 * Initialize AI message manager
 */
void InitializeAIMessageManager()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"Initializing AI message manager");

   // Load existing messages from database
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,message_guid,message_type,creation_time,expiration_time,")
                                     _T("source_task_id,source_task_type,related_object_id,title,message_text,")
                                     _T("spawn_task_data,status FROM ai_messages"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         auto message = make_shared<AIMessage>(hResult, i);
         if (message->getId() > s_messageId)
            s_messageId = message->getId();
         s_messages.put(message->getId(), message);
      }
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG, 2, L"Loaded %d AI messages from database", count);
   }

   // Load recipients
   hResult = DBSelect(hdb, _T("SELECT message_id,user_id,is_initiator,read_time FROM ai_message_recipients"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         uint32_t msgId = DBGetFieldUInt32(hResult, i, 0);
         uint32_t userId = DBGetFieldUInt32(hResult, i, 1);
         bool isInitiator = DBGetFieldInt32(hResult, i, 2) != 0;

         shared_ptr<AIMessage> msg = s_messages.get(msgId);
         if (msg != nullptr)
         {
            msg->addRecipient(userId, isInitiator, DBGetFieldTime(hResult, i, 3));
         }
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);

   // Start cleanup thread
   s_cleanupThread = ThreadCreateEx(CleanupThread);

   nxlog_debug_tag(DEBUG_TAG, 1, L"AI message manager initialized");
}

/**
 * Shutdown AI message manager
 */
void ShutdownAIMessageManager()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"Shutting down AI message manager");

   if (s_cleanupThread != INVALID_THREAD_HANDLE)
   {
      ThreadJoin(s_cleanupThread);
      s_cleanupThread = INVALID_THREAD_HANDLE;
   }

   nxlog_debug_tag(DEBUG_TAG, 1, L"AI message manager shutdown complete");
}

/*** AIMessage implementation ***/

/**
 * Constructor from data
 */
AIMessage::AIMessage(uint32_t id, AIMessageType type, const TCHAR *title, const TCHAR *text,
                     uint32_t relatedObjectId, uint32_t sourceTaskId, const TCHAR *sourceTaskType,
                     const TCHAR *spawnTaskData, uint32_t expirationMinutes) :
   m_recipients(4, 4, Ownership::True), m_mutex(MutexType::FAST)
{
   m_id = id;
   m_guid = uuid::generate();
   m_type = type;
   m_creationTime = time(nullptr);
   m_expirationTime = m_creationTime + (expirationMinutes * 60);
   m_sourceTaskId = sourceTaskId;
   _tcslcpy(m_sourceTaskType, CHECK_NULL_EX(sourceTaskType), 64);
   m_relatedObjectId = relatedObjectId;
   _tcslcpy(m_title, title, 256);
   m_text = MemCopyString(text);
   m_spawnTaskData = (spawnTaskData != nullptr) ? MemCopyString(spawnTaskData) : nullptr;
   m_status = AI_MSG_STATUS_PENDING;
}

/**
 * Constructor from database
 */
AIMessage::AIMessage(DB_RESULT hResult, int row) :
   m_recipients(4, 4, Ownership::True), m_mutex(MutexType::FAST)
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   m_type = static_cast<AIMessageType>(DBGetFieldInt32(hResult, row, 2));
   m_creationTime = DBGetFieldTime(hResult, row, 3);
   m_expirationTime = DBGetFieldTime(hResult, row, 4);
   m_sourceTaskId = DBGetFieldUInt32(hResult, row, 5);
   DBGetField(hResult, row, 6, m_sourceTaskType, 64);
   m_relatedObjectId = DBGetFieldUInt32(hResult, row, 7);
   DBGetField(hResult, row, 8, m_title, 256);
   m_text = DBGetField(hResult, row, 9, nullptr, 0);
   m_spawnTaskData = DBGetField(hResult, row, 10, nullptr, 0);
   m_status = static_cast<AIMessageStatus>(DBGetFieldInt32(hResult, row, 11));
}

/**
 * Destructor
 */
AIMessage::~AIMessage()
{
   MemFree(m_text);
   MemFree(m_spawnTaskData);
}

/**
 * Add recipient
 */
void AIMessage::addRecipient(uint32_t userId, bool isInitiator, time_t readTime)
{
   lock();
   auto recipient = new AIMessageRecipient();
   recipient->userId = userId;
   recipient->isInitiator = isInitiator;
   recipient->readTime = readTime;
   m_recipients.add(recipient);
   unlock();
}

/**
 * Check if user is recipient
 */
bool AIMessage::isRecipient(uint32_t userId) const
{
   for (int i = 0; i < m_recipients.size(); i++)
   {
      if (m_recipients.get(i)->userId == userId)
         return true;
   }
   return false;
}

/**
 * Check if user is initiator
 */
bool AIMessage::isInitiator(uint32_t userId) const
{
   for (int i = 0; i < m_recipients.size(); i++)
   {
      AIMessageRecipient *r = m_recipients.get(i);
      if (r->userId == userId && r->isInitiator)
         return true;
   }
   return false;
}

/**
 * Mark message as read by user
 */
bool AIMessage::markAsRead(uint32_t userId)
{
   lock();
   for (int i = 0; i < m_recipients.size(); i++)
   {
      AIMessageRecipient *r = m_recipients.get(i);
      if (r->userId == userId)
      {
         if (r->readTime == 0)
         {
            r->readTime = time(nullptr);

            // Update database
            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
            DB_STATEMENT hStmt = DBPrepare(hdb, L"UPDATE ai_message_recipients SET read_time=? WHERE message_id=? AND user_id=?");
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(r->readTime));
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, userId);
               DBExecute(hStmt);
               DBFreeStatement(hStmt);
            }
            DBConnectionPoolReleaseConnection(hdb);
         }
         unlock();
         return true;
      }
   }
   unlock();
   return false;
}

/**
 * Set message status
 */
bool AIMessage::setStatus(AIMessageStatus status)
{
   lock();
   m_status = status;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE ai_messages SET status=? WHERE id=?"));
   bool success = false;
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int>(status));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   unlock();
   return success;
}

/**
 * Fill NXCP message
 */
void AIMessage::fillMessage(NXCPMessage *msg, uint32_t baseId, uint32_t userId) const
{
   lock();
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_guid);
   msg->setField(baseId + 2, static_cast<uint16_t>(m_type));
   msg->setFieldFromTime(baseId + 3, m_creationTime);
   msg->setFieldFromTime(baseId + 4, m_expirationTime);
   msg->setField(baseId + 5, m_sourceTaskId);
   msg->setField(baseId + 6, m_sourceTaskType);
   msg->setField(baseId + 7, m_relatedObjectId);
   msg->setField(baseId + 8, m_title);
   msg->setField(baseId + 9, m_text);
   msg->setField(baseId + 10, CHECK_NULL_EX(m_spawnTaskData));
   if (m_status == AI_MSG_STATUS_PENDING)
   {
      // Check if message is read by user
      bool isRead = false;
      for (int i = 0; i < m_recipients.size(); i++)
      {
         AIMessageRecipient *r = m_recipients.get(i);
         if (r->userId == userId)
         {
            isRead = (r->readTime != 0);
            break;
         }
      }
      msg->setField(baseId + 11, static_cast<uint16_t>(isRead ? AI_MSG_STATUS_READ : AI_MSG_STATUS_PENDING));
   }
   else
   {
      msg->setField(baseId + 11, static_cast<uint16_t>(m_status));
   }
   unlock();
}

/**
 * Save message to database
 */
bool AIMessage::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   bool success = DBBegin(hdb);
   if (success)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb,
         _T("INSERT INTO ai_messages (id,message_guid,message_type,creation_time,expiration_time,")
         _T("source_task_id,source_task_type,related_object_id,title,message_text,spawn_task_data,status) ")
         _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?)"));

      if (hStmt != nullptr)
      {
         lock();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_guid);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int>(m_type));
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_expirationTime));
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_sourceTaskId);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_sourceTaskType, DB_BIND_TRANSIENT);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_relatedObjectId);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_title, DB_BIND_TRANSIENT);
         DBBind(hStmt, 10, DB_SQLTYPE_TEXT, m_text, DB_BIND_TRANSIENT);
         DBBind(hStmt, 11, DB_SQLTYPE_TEXT, m_spawnTaskData, DB_BIND_TRANSIENT);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, static_cast<int>(m_status));
         unlock();
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Save recipients
   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_message_recipients WHERE message_id=?");

      lock();
      if (success && !m_recipients.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO ai_message_recipients (message_id,user_id,is_initiator,read_time) VALUES (?,?,?,?)", m_recipients.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for (int i = 0; (i < m_recipients.size()) && success; i++)
            {
               AIMessageRecipient *r = m_recipients.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, r->userId);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, r->isInitiator ? L"1" : L"0", DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(r->readTime));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlock();
   }

   if (success)
      success = DBCommit(hdb);
   else
      DBRollback(hdb);

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Delete message from database
 */
bool AIMessage::deleteFromDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM ai_message_recipients WHERE message_id=%u"), m_id);
   DBQuery(hdb, query);

   _sntprintf(query, 256, _T("DELETE FROM ai_messages WHERE id=%u"), m_id);
   bool success = DBQuery(hdb, query);

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/*** Global functions ***/

/**
 * Create AI message
 */
uint32_t NXCORE_EXPORTABLE CreateAIMessage(AIMessageType type, const TCHAR *title, const TCHAR *text,
   uint32_t initiatorUserId, uint32_t relatedObjectId, uint32_t sourceTaskId,
   const TCHAR *sourceTaskType, const TCHAR *spawnTaskData,
   uint32_t expirationMinutes, bool includeResponsibleUsers)
{
   auto message = make_shared<AIMessage>(InterlockedIncrement(&s_messageId), type, title, text, relatedObjectId,
      sourceTaskId, sourceTaskType, spawnTaskData, expirationMinutes);

   // Add initiator as recipient
   message->addRecipient(initiatorUserId, true);

   // Add responsible users if requested
   if (includeResponsibleUsers && relatedObjectId != 0)
   {
      shared_ptr<NetObj> object = FindObjectById(relatedObjectId);
      if (object != nullptr)
      {
         unique_ptr<StructArray<ResponsibleUser>> responsibleUsers = object->getAllResponsibleUsers();
         for (int i = 0; i < responsibleUsers->size(); i++)
         {
            uint32_t userId = responsibleUsers->get(i)->userId;
            if (userId != initiatorUserId)
               message->addRecipient(userId, false);
         }
      }
   }

   if (!message->saveToDatabase())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Failed to save AI message %u to database", message->getId());
      return 0;
   }

   s_messages.put(message->getId(), message);

   NotifyAIMessageRecipients(message->getId(), true);

   nxlog_debug_tag(DEBUG_TAG, 4, L"Created AI message %u: type=%d, title=\"%s\"", message->getId(), type, title);
   return message->getId();
}

/**
 * Get AI messages for user
 */
unique_ptr<SharedObjectArray<AIMessage>> NXCORE_EXPORTABLE GetAIMessagesForUser(uint32_t userId)
{
   return s_messages.findAll(
      [userId] (AIMessage *msg) -> bool
      {
         return msg->isRecipient(userId) && !msg->isExpired();
      });
}

/**
 * Get AI message by ID
 */
shared_ptr<AIMessage> NXCORE_EXPORTABLE GetAIMessage(uint32_t messageId)
{
   return s_messages.get(messageId);
}

/**
 * Mark AI message as read
 */
uint32_t NXCORE_EXPORTABLE MarkAIMessageAsRead(uint32_t messageId, uint32_t userId)
{
   shared_ptr<AIMessage> msg = s_messages.get(messageId);
   if (msg == nullptr)
      return RCC_INVALID_MESSAGE_ID;

   if (!msg->isRecipient(userId))
      return RCC_ACCESS_DENIED;

   if (!msg->markAsRead(userId))
      return RCC_INTERNAL_ERROR;

   NotifyAIMessageRecipients(messageId, false);
   return RCC_SUCCESS;
}

/**
 * Approve AI message
 */
uint32_t NXCORE_EXPORTABLE ApproveAIMessage(uint32_t messageId, uint32_t userId)
{
   shared_ptr<AIMessage> msg = s_messages.get(messageId);
   if (msg == nullptr)
      return RCC_INVALID_MESSAGE_ID;

   if (!msg->isRecipient(userId))
      return RCC_ACCESS_DENIED;

   if (msg->getType() != AI_MESSAGE_APPROVAL_REQUEST)
      return RCC_INCOMPATIBLE_OPERATION;

   if (msg->getStatus() != AI_MSG_STATUS_PENDING && msg->getStatus() != AI_MSG_STATUS_READ)
      return RCC_INCOMPATIBLE_OPERATION;

   msg->setStatus(AI_MSG_STATUS_APPROVED);
   NotifyAIMessageRecipients(messageId, false);

   // Spawn task if spawn data is available
   const TCHAR *spawnData = msg->getSpawnTaskData();
   if (spawnData != nullptr && spawnData[0] != 0)
   {
      // Parse spawn data JSON (stored as UTF-8 in database)
#ifdef UNICODE
      char *spawnDataUtf8 = UTF8StringFromWideString(spawnData);
      json_t *json = json_loads(spawnDataUtf8, 0, nullptr);
      MemFree(spawnDataUtf8);
#else
      json_t *json = json_loads(spawnData, 0, nullptr);
#endif
      if (json != nullptr)
      {
         const char *userPrompt = json_object_get_string_utf8(json, "userPrompt", nullptr);
         if (userPrompt != nullptr && userPrompt[0] != 0)
         {
            TCHAR description[300];
            _sntprintf(description, 300, _T("Approved: %s"), msg->getTitle());
            uint32_t taskId = RegisterAITask(description, userId, String(userPrompt, "utf-8"), 0);
            nxlog_debug_tag(DEBUG_TAG, 4, L"Spawned AI task %u from approved message %u", taskId, messageId);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"AI message %u approved but no userPrompt in spawn data", messageId);
         }
         json_decref(json);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"AI message %u approved but failed to parse spawn data JSON", messageId);
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"AI message %u approved by user %u", messageId, userId);
   return RCC_SUCCESS;
}

/**
 * Reject AI message
 */
uint32_t NXCORE_EXPORTABLE RejectAIMessage(uint32_t messageId, uint32_t userId)
{
   shared_ptr<AIMessage> msg = s_messages.get(messageId);
   if (msg == nullptr)
      return RCC_INVALID_MESSAGE_ID;

   if (!msg->isRecipient(userId))
      return RCC_ACCESS_DENIED;

   if (msg->getType() != AI_MESSAGE_APPROVAL_REQUEST)
      return RCC_INCOMPATIBLE_OPERATION;

   if (msg->getStatus() != AI_MSG_STATUS_PENDING && msg->getStatus() != AI_MSG_STATUS_READ)
      return RCC_INCOMPATIBLE_OPERATION;

   msg->setStatus(AI_MSG_STATUS_REJECTED);
   NotifyAIMessageRecipients(messageId, false);

   nxlog_debug_tag(DEBUG_TAG, 4, L"AI message %u rejected by user %u", messageId, userId);
   return RCC_SUCCESS;
}

/**
 * Delete AI message
 */
uint32_t NXCORE_EXPORTABLE DeleteAIMessage(uint32_t messageId, uint32_t userId)
{
   shared_ptr<AIMessage> msg = s_messages.get(messageId);
   if (msg == nullptr)
      return RCC_INVALID_MESSAGE_ID;

   // Only initiator or admin can delete
   if (!msg->isInitiator(userId))
   {
      // Check if user is admin
      // For now, just allow if user is recipient
      if (!msg->isRecipient(userId))
         return RCC_ACCESS_DENIED;
   }

   msg->deleteFromDatabase();
   s_messages.remove(messageId);
   NotifyAIMessageRecipients(messageId, false);

   nxlog_debug_tag(DEBUG_TAG, 4, L"AI message %u deleted by user %u", messageId, userId);
   return RCC_SUCCESS;
}

/**
 * Notify recipients about new or updated message
 */
void NotifyAIMessageRecipients(uint32_t messageId, bool isNew)
{
   shared_ptr<AIMessage> msg = s_messages.get(messageId);
   EnumerateClientSessions(
      [messageId, isNew, msg] (ClientSession *session) -> void
      {
         if (session->isAuthenticated())
         {
            uint32_t userId = session->getUserId();
            if (msg == nullptr || msg->isRecipient(userId))
            {
               NXCPMessage notification(CMD_AI_MESSAGE_UPDATE, 0);
               notification.setField(VID_AI_MESSAGE_ID, messageId);
               notification.setField(VID_IS_DELETED, !isNew && (msg == nullptr || msg->getStatus() == AI_MSG_STATUS_EXPIRED));

               if (msg != nullptr)
                  msg->fillMessage(&notification, VID_ELEMENT_LIST_BASE, session->getUserId());
               session->postMessage(notification);
            }
         }
      });
}

/**
 * Create AI message (AI function)
 */
std::string F_CreateAIMessage(json_t *arguments, uint32_t userId)
{
   const char *title = json_object_get_string_utf8(arguments, "title", nullptr);
   const char *text = json_object_get_string_utf8(arguments, "text", nullptr);
   const char *typeStr = json_object_get_string_utf8(arguments, "type", "informational");

   if ((title == nullptr) || (title[0] == 0))
      return std::string("Error: message title must be provided");
   if ((text == nullptr) || (text[0] == 0))
      return std::string("Error: message text must be provided");

   AIMessageType type = AI_MESSAGE_INFORMATIONAL;
   if (!stricmp(typeStr, "alert"))
      type = AI_MESSAGE_ALERT;

   uint32_t relatedObjectId = json_object_get_uint32(arguments, "relatedObjectId", 0);
   uint32_t expirationMinutes = json_object_get_uint32(arguments, "expirationMinutes", 10080);

   uint32_t messageId = CreateAIMessage(type, String(title, "utf-8"), String(text, "utf-8"),
      userId, relatedObjectId, 0, nullptr, nullptr, expirationMinutes, false);

   if (messageId == 0)
      return std::string("Error: failed to create message");

   json_t *result = json_object();
   json_object_set_new(result, "success", json_true());
   json_object_set_new(result, "messageId", json_integer(messageId));
   return JsonToString(result);
}

/**
 * Create approval request AI message (AI function)
 */
std::string F_CreateApprovalRequest(json_t *arguments, uint32_t userId)
{
   const char *title = json_object_get_string_utf8(arguments, "title", nullptr);
   const char *text = json_object_get_string_utf8(arguments, "text", nullptr);
   const char *taskPrompt = json_object_get_string_utf8(arguments, "taskPrompt", nullptr);

   if ((title == nullptr) || (title[0] == 0))
      return std::string("Error: request title must be provided");
   if ((text == nullptr) || (text[0] == 0))
      return std::string("Error: request description must be provided");
   if ((taskPrompt == nullptr) || (taskPrompt[0] == 0))
      return std::string("Error: task prompt must be provided");

   uint32_t relatedObjectId = json_object_get_uint32(arguments, "relatedObjectId", 0);
   uint32_t expirationMinutes = json_object_get_uint32(arguments, "expirationMinutes", 1440);

   // Build spawn task data JSON
   json_t *spawnData = json_object();
   json_object_set_new(spawnData, "taskType", json_string("ai_chat"));
   json_object_set_new(spawnData, "userPrompt", json_string(taskPrompt));
   json_object_set_new(spawnData, "contextObjectId", json_integer(relatedObjectId));

   char *spawnDataStr = json_dumps(spawnData, 0);
   json_decref(spawnData);

   uint32_t messageId = CreateAIMessage(AI_MESSAGE_APPROVAL_REQUEST, String(title, "utf-8"),
      String(text, "utf-8"), userId, relatedObjectId, 0, nullptr,
      String(spawnDataStr, "utf-8"), expirationMinutes, false);

   MemFree(spawnDataStr);

   if (messageId == 0)
      return std::string("Error: failed to create approval request");

   json_t *result = json_object();
   json_object_set_new(result, "success", json_true());
   json_object_set_new(result, "messageId", json_integer(messageId));
   return JsonToString(result);
}
