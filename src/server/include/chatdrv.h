/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: chatdrv.h
**
**/

#ifndef _chatdrv_h_
#define _chatdrv_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxconfig.h>
#include <ncdrv.h>

/**
 * Message sink for chat bot drivers, implemented by server core. Drivers call sink methods
 * from their transport threads; implementations return quickly and never block. Peer ID is
 * a platform-specific stable identifier of the message sender (chat ID, JID, etc.).
 * All strings are UTF-8.
 */
class ChatBotMessageSink
{
public:
   virtual ~ChatBotMessageSink() { }

   /**
    * Incoming direct message from given peer. Display name is informational and can be empty.
    */
   virtual void onMessage(const char *peerId, const char *displayName, const char *text) = 0;

   /**
    * Response to a question previously delivered with ChatBotDriver::sendQuestion
    * (platform-native interactive element pressed). Selected option is a 0-based index
    * into the option list.
    */
   virtual void onChoiceResponse(const char *peerId, uint64_t questionId, int selectedOption) = 0;
};

/**
 * Chat bot driver base class - bidirectional interface to a messaging platform. The driver
 * owns the platform connection and its transport thread(s), started by start() and stopped
 * by stop(). Peer IDs and message texts are UTF-8; question option labels are wide strings
 * (consistent with StringList).
 */
class ChatBotDriver
{
protected:
   ChatBotDriver() { }

public:
   virtual ~ChatBotDriver() { }

   /**
    * Start platform connection. Sink is owned by the caller and remains valid until stop() returns.
    */
   virtual bool start(ChatBotMessageSink *sink) = 0;

   /**
    * Stop platform connection. Returns after all transport threads are stopped
    * (no sink calls will be made after return).
    */
   virtual void stop() = 0;

   /**
    * Send plain text message to given peer.
    */
   virtual bool sendMessage(const char *peerId, const char *text) = 0;

   /**
    * Send question with selectable options to given peer using platform-native interactive
    * elements. Responses are reported via ChatBotMessageSink::onChoiceResponse with the same
    * question ID. Return false if the platform has no interactive elements - the caller then
    * falls back to plain text rendering of the question.
    */
   virtual bool sendQuestion(const char *peerId, const char *text, const StringList& options, uint64_t questionId) = 0;

   /**
    * Optional health check. Returns true if the driver is operational.
    */
   virtual bool checkHealth() { return true; }
};

/**
 * Chat bot driver factory
 */
typedef ChatBotDriver *(*ChatBotDriverFactory)(Config *config, NCDriverStorageManager *storageManager);

/**
 * Chat bot entry point - additional optional symbol exported by a notification channel
 * driver module alongside DECLARE_NCD_ENTRY_POINT. Absence of the symbol means the module
 * is a pure notification driver.
 */
#define DECLARE_CHATBOT_ENTRY_POINT \
extern "C" __EXPORT ChatBotDriver *NcdCreateChatBotInstance(Config *config, NCDriverStorageManager *storageManager)

#endif   /* _chatdrv_h_ */
