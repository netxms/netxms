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
** File: ai_messages.h
**
**/

#ifndef _ai_messages_h_
#define _ai_messages_h_

/**
 * AI message types
 */
enum AIMessageType
{
   AI_MESSAGE_INFORMATIONAL = 0,
   AI_MESSAGE_ALERT = 1,
   AI_MESSAGE_APPROVAL_REQUEST = 2
};

/**
 * AI message status
 */
enum AIMessageStatus
{
   AI_MSG_STATUS_PENDING = 0,
   AI_MSG_STATUS_READ = 1,
   AI_MSG_STATUS_APPROVED = 2,
   AI_MSG_STATUS_REJECTED = 3,
   AI_MSG_STATUS_EXPIRED = 4
};

/**
 * Helper function for converting AI message status form int to enum
 */
static inline AIMessageStatus AIMessageStatusFromInt(int v)
{
   return ((v >= 0) && (v <= AI_MSG_STATUS_EXPIRED)) ? static_cast<AIMessageStatus>(v) : AI_MSG_STATUS_PENDING;
}

/**
 * AI message recipient
 */
struct AIMessageRecipient
{
   uint32_t userId;
   bool isInitiator;
   time_t readTime;
};

/**
 * AI agent message
 */
class AIMessage
{
private:
   uint32_t m_id;
   uuid m_guid;
   AIMessageType m_type;
   time_t m_creationTime;
   time_t m_expirationTime;
   uint32_t m_sourceTaskId;
   TCHAR m_sourceTaskType[64];
   uint32_t m_relatedObjectId;
   TCHAR m_title[256];
   TCHAR *m_text;
   TCHAR *m_spawnTaskData;
   AIMessageStatus m_status;
   ObjectArray<AIMessageRecipient> m_recipients;
   Mutex m_mutex;

   void lock() const { m_mutex.lock(); }
   void unlock() const { m_mutex.unlock(); }

public:
   AIMessage(uint32_t id, AIMessageType type, const TCHAR *title, const TCHAR *text,
             uint32_t relatedObjectId = 0, uint32_t sourceTaskId = 0, const TCHAR *sourceTaskType = nullptr,
             const TCHAR *spawnTaskData = nullptr, uint32_t expirationMinutes = 10080);
   AIMessage(DB_RESULT hResult, int row);
   ~AIMessage();

   uint32_t getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   AIMessageType getType() const { return m_type; }
   time_t getCreationTime() const { return m_creationTime; }
   time_t getExpirationTime() const { return m_expirationTime; }
   uint32_t getSourceTaskId() const { return m_sourceTaskId; }
   const TCHAR *getSourceTaskType() const { return m_sourceTaskType; }
   uint32_t getRelatedObjectId() const { return m_relatedObjectId; }
   const TCHAR *getTitle() const { return m_title; }
   const TCHAR *getText() const { return m_text; }
   const TCHAR *getSpawnTaskData() const { return m_spawnTaskData; }
   AIMessageStatus getStatus() const { return m_status; }
   bool isExpired() const { return time(nullptr) > m_expirationTime; }

   void addRecipient(uint32_t userId, bool isInitiator, time_t readTime = 0);
   bool isRecipient(uint32_t userId) const;
   bool isInitiator(uint32_t userId) const;

   bool markAsRead(uint32_t userId);
   bool setStatus(AIMessageStatus status);

   void fillMessage(NXCPMessage *msg, uint32_t baseId, uint32_t userId) const;
   bool saveToDatabase() const;
   bool deleteFromDatabase();
};

/**
 * Initialize AI message manager
 */
void InitializeAIMessageManager();

/**
 * Shutdown AI message manager
 */
void ShutdownAIMessageManager();

/**
 * Create AI message
 * @param type Message type
 * @param title Short title
 * @param text Full message text
 * @param initiatorUserId ID of user who initiated the task
 * @param relatedObjectId Related NetXMS object ID (optional)
 * @param sourceTaskId Source task ID (optional)
 * @param sourceTaskType Source task type (optional)
 * @param spawnTaskData JSON data for task spawning on approval (optional)
 * @param expirationMinutes Minutes until message expires
 * @param includeResponsibleUsers Add responsible users as recipients
 * @return Message ID on success, 0 on failure
 */
uint32_t NXCORE_EXPORTABLE CreateAIMessage(AIMessageType type, const TCHAR *title, const TCHAR *text,
   uint32_t initiatorUserId, uint32_t relatedObjectId = 0, uint32_t sourceTaskId = 0,
   const TCHAR *sourceTaskType = nullptr, const TCHAR *spawnTaskData = nullptr,
   uint32_t expirationMinutes = 10080, bool includeResponsibleUsers = false);

/**
 * Get AI messages for user
 * @param userId User ID
 * @return Array of messages (caller must delete)
 */
unique_ptr<SharedObjectArray<AIMessage>> NXCORE_EXPORTABLE GetAIMessagesForUser(uint32_t userId);

/**
 * Get AI message by ID
 * @param messageId Message ID
 * @return Message or nullptr if not found
 */
shared_ptr<AIMessage> NXCORE_EXPORTABLE GetAIMessage(uint32_t messageId);

/**
 * Mark AI message as read
 * @param messageId Message ID
 * @param userId User ID
 * @return RCC code
 */
uint32_t NXCORE_EXPORTABLE MarkAIMessageAsRead(uint32_t messageId, uint32_t userId);

/**
 * Approve AI message (for approval requests)
 * @param messageId Message ID
 * @param userId User ID
 * @return RCC code
 */
uint32_t NXCORE_EXPORTABLE ApproveAIMessage(uint32_t messageId, uint32_t userId);

/**
 * Reject AI message (for approval requests)
 * @param messageId Message ID
 * @param userId User ID
 * @return RCC code
 */
uint32_t NXCORE_EXPORTABLE RejectAIMessage(uint32_t messageId, uint32_t userId);

/**
 * Delete AI message
 * @param messageId Message ID
 * @param userId User ID (for permission check)
 * @return RCC code
 */
uint32_t NXCORE_EXPORTABLE DeleteAIMessage(uint32_t messageId, uint32_t userId);

/**
 * Notify recipients about new or updated message
 * @param messageId Message ID
 * @param isNew true if new message, false if update
 */
void NotifyAIMessageRecipients(uint32_t messageId, bool isNew);

/**
 * Create AI message (AI function)
 */
std::string F_CreateAIMessage(json_t *arguments, uint32_t userId);

/**
 * Create approval request AI message (AI function)
 */
std::string F_CreateApprovalRequest(json_t *arguments, uint32_t userId);

#endif
