/*
** NetXMS - Network Management System
** Client Library API
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

#ifdef _WIN32
#ifdef LIBNXCLIENT_EXPORTS
#define LIBNXCLIENT_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXCLIENT_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXCLIENT_EXPORTABLE
#endif

#include "nxclobj.h"

#define MAX_TZ_LEN         32

/**
 * NXCSession::connect flags
 */
#define NXCF_DEFAULT						   0
#define NXCF_ENCRYPT						   0x0001
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
   UINT32 dciId;          // DCI ID or 0 if name is used
   TCHAR *dciName;
   UINT32 nodeId;      // Node ID or 0 if name is used
   TCHAR *nodeName;
   TCHAR *value;
};

/**
 * Debug callback
 */
typedef void (* NXC_DEBUG_CALLBACK)(const TCHAR *msg);

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

   UINT32 pushData(ObjectArray<NXCPushData> *data, UINT32 *failedIndex = NULL);
};

/**
 * Event template
 */
class LIBNXCLIENT_EXPORTABLE EventTemplate
{
private:
   UINT32 m_code;
   TCHAR m_name[MAX_EVENT_NAME];
   int m_severity;
   UINT32 m_flags;
   TCHAR *m_messageTemplate;
   TCHAR *m_description;

public:
   EventTemplate(NXCPMessage *msg);
   ~EventTemplate();

   UINT32 getCode() { return m_code; }
   const TCHAR *getName() { return m_name; }
   int getSeverity() { return m_severity; }
   UINT32 getFlags() { return m_flags; }
   const TCHAR *getMessageTemplate() { return CHECK_NULL_EX(m_messageTemplate); }
   const TCHAR *getDescription() { return CHECK_NULL_EX(m_description); }
};

/**
 * Event controller
 */
class LIBNXCLIENT_EXPORTABLE EventController : public Controller
{
private:
   MUTEX m_eventTemplateLock;
   ObjectArray<EventTemplate> *m_eventTemplates;

public:
   EventController(NXCSession *session);
   virtual ~EventController();

   UINT32 syncEventTemplates();
   UINT32 getEventTemplates(ObjectArray<EventTemplate> *templates);
   TCHAR *getEventName(UINT32 code, TCHAR *buffer, size_t bufferSize);

   UINT32 sendEvent(UINT32 code, const TCHAR *name, UINT32 objectId, int argc, TCHAR **argv, const TCHAR *userTag);
};

struct ObjectCacheEntry;

/**
 * Object controller
 */
class LIBNXCLIENT_EXPORTABLE ObjectController : public Controller
{
private:
   ObjectCacheEntry *m_cache;
   MUTEX m_cacheLock;

   void addObject(AbstractObject *object);

public:
   ObjectController(NXCSession *session);
   virtual ~ObjectController();

   virtual bool handleMessage(NXCPMessage *msg);

   UINT32 sync();
   UINT32 syncObjectSet(UINT32 *idList, size_t length, bool syncComments, UINT16 flags);
   UINT32 syncSingleObject(UINT32 id);

   AbstractObject *findObjectById(UINT32 id);

   UINT32 manage(UINT32 objectId);
   UINT32 unmanage(UINT32 objectId);
};

/**
 * Server controller
 */
class LIBNXCLIENT_EXPORTABLE ServerController : public Controller
{
public:
   ServerController(NXCSession *session) : Controller(session) { }

   UINT32 sendSMS(const TCHAR *recipient, const TCHAR *text);
};

/**
 * Session
 */
class LIBNXCLIENT_EXPORTABLE NXCSession
{
private:
   THREAD m_receiverThread;

   static THREAD_RESULT THREAD_CALL receiverThreadStarter(void *arg);
   void receiverThread();

protected:
   // communications
   bool m_connected;
   bool m_disconnected;
   VolatileCounter m_msgId;
   SOCKET m_hSocket;
   MsgWaitQueue *m_msgWaitQueue;
   NXCPEncryptionContext *m_encryptionContext;
   UINT32 m_commandTimeout;

   // server information
   BYTE m_serverId[8];
   TCHAR m_serverVersion[64];
   IntegerArray<UINT32> *m_protocolVersions;
   TCHAR m_serverTimeZone[MAX_TZ_LEN];
   UINT32 m_userId;
   UINT64 m_systemRights;
   bool m_passwordChangeNeeded;

   // data
   MUTEX m_dataLock;
   MUTEX m_msgSendLock;
   StringObjectMap<Controller> *m_controllers;

   void onNotify(NXCPMessage *msg);
   bool handleMessage(NXCPMessage *msg);

public:
   NXCSession();
   virtual ~NXCSession();

   UINT32 connect(const TCHAR *host, const TCHAR *login, const TCHAR *password, 
      UINT32 flags = NXCF_DEFAULT, const TCHAR *clientInfo = NULL, 
      const UINT32 *cpvIndexList = NULL, size_t cpvIndexListSize = 0);
   void disconnect();

   UINT32 createMessageId() { return InterlockedIncrement(&m_msgId); }
   bool sendMessage(NXCPMessage *msg);
   NXCPMessage *waitForMessage(UINT16 code, UINT32 id, UINT32 timeout = 0);
   UINT32 waitForRCC(UINT32 id, UINT32 timeout = 0);

   void setCommandTimeout(UINT32 timeout) { m_commandTimeout = timeout; }
   UINT32 getCommandTimeout() { return m_commandTimeout; }

   const TCHAR *getServerVersion() { return m_serverVersion; }
   const TCHAR *getServerTimeZone() { return m_serverTimeZone; }
   UINT32 getProtocolVersion(int index) { return m_protocolVersions->get(index); }
   UINT32 getUserId() { return m_userId; }
   UINT64 getSystemRights() { return m_systemRights; }
   bool isPasswordChangeNeeded() { return m_passwordChangeNeeded; }

   Controller *getController(const TCHAR *name);
};

/**
 * Functions
 */
UINT32 LIBNXCLIENT_EXPORTABLE NXCGetVersion();
const TCHAR LIBNXCLIENT_EXPORTABLE *NXCGetErrorText(UINT32 rcc);

bool LIBNXCLIENT_EXPORTABLE NXCInitialize();
void LIBNXCLIENT_EXPORTABLE NXCShutdown();
void LIBNXCLIENT_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK cb);

#endif   /* _nxclient_h_ */
