/*
** NetXMS - Network Management System
** Client Library API
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: nxclient.h
**
**/

#ifndef _nxclient_h_
#define _nxclient_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxevent.h>
#include <nxcpapi.h>
#include <nxtools.h>
#include <nxlog.h>
#include <uuid.h>
#include <nxcldefs.h>

#ifdef LIBNXCLIENT_EXPORTS
#define LIBNXCLIENT_EXPORTABLE __EXPORT
#else
#define LIBNXCLIENT_EXPORTABLE __IMPORT
#endif

#include "nxclobj.h"

#define MAX_TZ_LEN         32

/**
 * NXCSession::connect flags
 */
#define NXCF_DEFAULT						   0
#define NXCF_EXACT_VERSION_MATCH		   0x0002
#define NXCF_USE_CERTIFICATE			   0x0004
#define NXCF_IGNORE_PROTOCOL_VERSION   0x0008

/**
 * Standard controller names
 */
#define CONTROLLER_ALARMS           _T("ALARMS")
#define CONTROLLER_DATA_COLLECTION  _T("DATACOLL")
#define CONTROLLER_EVENTS           _T("EVENTS")
#define CONTROLLER_OBJECTS          _T("OBJECTS")
#define CONTROLLER_SERVER           _T("SERVER")

/**
 * DCI push data
 */
struct NXCPushData
{
   uint32_t dciId;          // DCI ID or 0 if name is used
   TCHAR *dciName;
   uint32_t nodeId;      // Node ID or 0 if name is used
   TCHAR *nodeName;
   TCHAR *value;
};

/**
 * Debug callback
 */
typedef void (*NXC_DEBUG_CALLBACK)(const TCHAR *msg);

class NXCSession;

/**
 * Abstract controller class
 */
class LIBNXCLIENT_EXPORTABLE Controller
{
protected:
   NXCSession *m_session;

public:
   Controller(NXCSession *session) { m_session = session; }
   virtual ~Controller();

   virtual bool handleMessage(NXCPMessage *msg);
};

/**
 * Alarm comment
 */
class LIBNXCLIENT_EXPORTABLE AlarmComment
{
private:
   UINT32 m_id;
   UINT32 m_alarmId;
   UINT32 m_userId;
   TCHAR *m_userName;
   time_t m_timestamp;
   TCHAR *m_text;

public:
   AlarmComment(NXCPMessage *msg, UINT32 baseId);
   ~AlarmComment();

   UINT32 getId() { return m_id; }
   UINT32 getAlarmId() { return m_alarmId; }
   UINT32 getUserId() { return m_userId; }
   const TCHAR *getUserName() { return m_userName; }
   time_t getTimestamp() { return m_timestamp; }
   const TCHAR *getText() { return m_text; }
};

/**
 * Alarm controller
 */
class LIBNXCLIENT_EXPORTABLE AlarmController : public Controller
{
private:
   NXC_ALARM *createAlarmFromMessage(NXCPMessage *msg);

public:
   AlarmController(NXCSession *session) : Controller(session) { }

   UINT32 getAll(ObjectArray<NXC_ALARM> **alarms);

   UINT32 acknowledge(UINT32 alarmId, bool sticky = false, UINT32 timeout = 0);
   UINT32 resolve(UINT32 alarmId);
   UINT32 terminate(UINT32 alarmId);
   UINT32 openHelpdeskIssue(UINT32 alarmId, TCHAR *helpdeskRef);
   
   UINT32 getComments(UINT32 alarmId, ObjectArray<AlarmComment> **comments);
   UINT32 addComment(UINT32 alarmId, const TCHAR *text);
   UINT32 updateComment(UINT32 alarmId, UINT32 commentId, const TCHAR *text);

   TCHAR *formatAlarmText(NXC_ALARM *alarm, const TCHAR *format);
};

/**
 * Data collection controller
 */
class LIBNXCLIENT_EXPORTABLE DataCollectionController : public Controller
{
public:
   DataCollectionController(NXCSession *session) : Controller(session) { }

   uint32_t pushData(ObjectArray<NXCPushData> *data, Timestamp timestamp, uint32_t *failedIndex = nullptr);
};

/**
 * Event template
 */
class LIBNXCLIENT_EXPORTABLE EventTemplate
{
private:
   uint32_t m_code;
   uuid m_guid;
   TCHAR m_name[MAX_EVENT_NAME];
   int m_severity;
   uint32_t m_flags;
   TCHAR *m_messageTemplate;
   TCHAR *m_description;
   TCHAR *m_tags;

public:
   EventTemplate(const NXCPMessage& msg, uint32_t baseId);
   ~EventTemplate();

   uint32_t getCode() const { return m_code; }
   const uuid& getGuid() const { return m_guid; }
   const TCHAR *getName() const { return m_name; }
   int getSeverity() const { return m_severity; }
   uint32_t getFlags() const { return m_flags; }
   const TCHAR *getMessageTemplate() const { return CHECK_NULL_EX(m_messageTemplate); }
   const TCHAR *getDescription() const { return CHECK_NULL_EX(m_description); }
   const TCHAR *getTags() const { return CHECK_NULL_EX(m_tags); }
};

/**
 * Event controller
 */
class LIBNXCLIENT_EXPORTABLE EventController : public Controller
{
private:
   Mutex m_eventTemplateLock;
   ObjectArray<EventTemplate> *m_eventTemplates;

public:
   EventController(NXCSession *session);
   virtual ~EventController();

   uint32_t syncEventTemplates();
   uint32_t getEventTemplates(ObjectArray<EventTemplate> *templates);
   TCHAR *getEventName(uint32_t code, TCHAR *buffer, size_t bufferSize);

   uint32_t sendEvent(uint32_t code, const TCHAR *name, uint32_t objectId, int argc, TCHAR **argv, const TCHAR *userTag, bool useNamedParameters);
};

struct ObjectCacheEntry;

/**
 * Object controller
 */
class LIBNXCLIENT_EXPORTABLE ObjectController : public Controller
{
private:
   ObjectCacheEntry *m_cache;
   Mutex m_cacheLock;

   void addObject(const shared_ptr<AbstractObject>& object);

public:
   ObjectController(NXCSession *session);
   virtual ~ObjectController();

   virtual bool handleMessage(NXCPMessage *msg);

   uint32_t sync();
   uint32_t syncObjectSet(const uint32_t *idList, size_t length, uint16_t flags);
   uint32_t syncSingleObject(uint32_t id);

   shared_ptr<AbstractObject> findObjectById(uint32_t id);

   uint32_t manage(uint32_t objectId);
   uint32_t unmanage(uint32_t objectId);
};

/**
 * Server controller
 */
class LIBNXCLIENT_EXPORTABLE ServerController : public Controller
{
public:
   ServerController(NXCSession *session) : Controller(session) { }

   UINT32 sendNotification(const TCHAR *channelName, const TCHAR *recipient, const TCHAR *subject, const TCHAR *text);
};

/**
 * Session
 */
class LIBNXCLIENT_EXPORTABLE NXCSession
{
private:
   THREAD m_receiverThread;
   SocketMessageReceiver *m_receiver;

   void receiverThread();

protected:
   // communications
   bool m_connected;
   bool m_disconnected;
   VolatileCounter m_msgId;
   SOCKET m_hSocket;
   MsgWaitQueue *m_msgWaitQueue;
   shared_ptr<NXCPEncryptionContext> m_encryptionContext;
   uint32_t m_commandTimeout;
   bool m_compressionEnabled;

   // server information
   BYTE m_serverId[8];
   TCHAR m_serverVersion[64];
   IntegerArray<UINT32> *m_protocolVersions;
   TCHAR m_serverTimeZone[MAX_TZ_LEN];
   uint32_t m_userId;
   uint64_t m_systemRights;
   bool m_passwordChangeNeeded;

   // data
   Mutex m_dataLock;
   Mutex m_msgSendLock;
   StringObjectMap<Controller> *m_controllers;

   void onNotify(NXCPMessage *msg);
   bool handleMessage(NXCPMessage *msg);

public:
   NXCSession();
   virtual ~NXCSession();

   uint32_t connect(const TCHAR *host, const TCHAR *login, const TCHAR *password,
      uint32_t flags = NXCF_DEFAULT, const TCHAR *clientInfo = nullptr,
      const uint32_t *cpvIndexList = nullptr, size_t cpvIndexListSize = 0);
   void disconnect();

   uint32_t createMessageId() { return InterlockedIncrement(&m_msgId); }
   bool sendMessage(NXCPMessage *msg);
   NXCPMessage *waitForMessage(uint16_t code, uint32_t id, uint32_t timeout = 0);
   uint32_t waitForRCC(uint32_t id, uint32_t timeout = 0);

   void setCommandTimeout(uint32_t timeout) { m_commandTimeout = timeout; }
   uint32_t getCommandTimeout() const { return m_commandTimeout; }

   const TCHAR *getServerVersion() const { return m_serverVersion; }
   const TCHAR *getServerTimeZone() const { return m_serverTimeZone; }
   uint32_t getProtocolVersion(int index) const { return m_protocolVersions->get(index); }
   uint32_t getUserId() const { return m_userId; }
   uint64_t getSystemRights() const { return m_systemRights; }
   bool isPasswordChangeNeeded() const { return m_passwordChangeNeeded; }

   Controller *getController(const TCHAR *name);
};

/**
 * Functions
 */
uint32_t LIBNXCLIENT_EXPORTABLE NXCGetVersion();
const TCHAR LIBNXCLIENT_EXPORTABLE *NXCGetErrorText(uint32_t rcc);

bool LIBNXCLIENT_EXPORTABLE NXCInitialize();
void LIBNXCLIENT_EXPORTABLE NXCShutdown();
void LIBNXCLIENT_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK cb);

#endif   /* _nxclient_h_ */
