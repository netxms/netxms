/* 
** NetXMS - Network Management System
** Copyright (C) 2019-2026 Raden Solutions
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
** File: ncdrv.h
**
**/

#ifndef _ncdrv_h_
#define _ncdrv_h_

#include <nms_common.h>
#include <nxconfig.h>
#include <functional>

class Event;
class NetObj;

/**
 * API version
 */
#define NCDRV_API_VERSION           5

/**
 * Notification channel status
 */
struct NotificationChannelStatus
{
   bool sendStatus;           // Status of last send
   bool healthCheckStatus;    // Status of driver health check
   uint32_t queueSize;        // Current queue size
   uint32_t messageCount;     // Total number of sent messages
   uint32_t failedSendCount;  // Number of failed send operations
   uint32_t digestedCount;    // Number of messages absorbed into digest
   time_t lastMessageTime;    // Timestamp of last message send attempt
};

/**
 * Notification channel configuration template
 */
struct NCConfigurationTemplate
{
   NCConfigurationTemplate(bool needSubject, bool needRecipient)
   {
      this->needSubject = needSubject;
      this->needRecipient = needRecipient;
   }

   bool needSubject;
   bool needRecipient;
};

/**
 * Storage interface for notification channel drivers. All keys and values are UTF-8 strings.
 */
class NCDriverStorageManager
{
protected:
   NCDriverStorageManager() { }
   virtual ~NCDriverStorageManager() { }

public:
   virtual char *get(const char *key) = 0;
   virtual void getAll(std::function<void (const char*, const char*)> callback) = 0;
   virtual void set(const char *key, const char *value) = 0;
   virtual void clear(const char *key) = 0;
};

/**
 * Notification sending context. Message text is provided both as UTF-8 and as original wide character
 * strings. Event context is optional - drivers must handle absence of event and source object
 * (test sends, digest messages, and notifications carrying only an event code have neither).
 */
struct NotificationContext
{
   const char *recipient;           // Recipient (UTF-8)
   const char *subject;             // Subject (UTF-8)
   const char *body;                // Message body (UTF-8)
   const wchar_t *recipientW;       // Recipient (wide string original)
   const wchar_t *subjectW;         // Subject (wide string original)
   const wchar_t *bodyW;            // Message body (wide string original)
   const Event *event;              // Event that triggered the notification (can be nullptr)
   shared_ptr<NetObj> sourceObject; // Source object of the event (can be nullptr)
   const wchar_t *channelName;      // Name of the notification channel
   uuid ruleId;                     // ID of the EPP rule that generated the notification (can be null UUID)

   NotificationContext()
   {
      recipient = nullptr;
      subject = nullptr;
      body = nullptr;
      recipientW = nullptr;
      subjectW = nullptr;
      bodyW = nullptr;
      event = nullptr;
      channelName = nullptr;
   }
};

/**
 * Notification Channel Driver base class. Return value of send(): 0 = success,
 * positive value = retry after that number of seconds, negative value = permanent failure.
 */
class NCDriver
{
protected:
   NCDriver() { }

public:
   virtual ~NCDriver() { }

   virtual int send(const NotificationContext& context) = 0;

   virtual bool checkHealth() { return true; }
};

/**
 * NCD module entry point
 */
#define DECLARE_NCD_ENTRY_POINT(name, configTemplate) \
extern "C" __EXPORT_VAR(int NcdAPIVersion); \
__EXPORT_VAR(int NcdAPIVersion) = NCDRV_API_VERSION; \
extern "C" __EXPORT_VAR(const char *NcdName); \
__EXPORT_VAR(const char *NcdName) = #name; \
extern "C" __EXPORT const NCConfigurationTemplate *NcdGetConfigurationTemplate() { return (configTemplate); } \
extern "C" __EXPORT NCDriver *NcdCreateInstance(Config *config, NCDriverStorageManager *storageManager)

#endif   /* _ncdrv_h_ */
