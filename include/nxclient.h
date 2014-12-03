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
};

/**
 * Alarm controller
 */
class LIBNXCLIENT_EXPORTABLE AlarmController : public Controller
{
public:
   AlarmController(NXCSession *session) : Controller(session) { }

   UINT32 acknowledge(UINT32 alarmId, bool sticky = false);
   UINT32 resolve(UINT32 alarmId);
   UINT32 terminate(UINT32 alarmId);
   ObjectArray<NXC_ALARM> *get();
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
 * Event controller
 */
class LIBNXCLIENT_EXPORTABLE EventController : public Controller
{
public:
   EventController(NXCSession *session) : Controller(session) { }
};

/**
 * Object controller
 */
class LIBNXCLIENT_EXPORTABLE ObjectController : public Controller
{
public:
   ObjectController(NXCSession *session) : Controller(session) { }

   UINT32 manage(UINT32 objectId);
   UINT32 unmanage(UINT32 objectId);
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
   TCHAR m_serverTimeZone[MAX_TZ_LEN];
   UINT32 m_userId;
   UINT64 m_systemRights;
   bool m_passwordChangeNeeded;

   // data
   MUTEX m_dataLock;
   MUTEX m_msgSendLock;
   StringObjectMap<Controller> *m_controllers;

   void onNotify(NXCPMessage *msg);

public:
   NXCSession();
   virtual ~NXCSession();

   UINT32 connect(const TCHAR *host, const TCHAR *login, const TCHAR *password, UINT32 flags = NXCF_DEFAULT, const TCHAR *clientInfo = NULL);
   void disconnect();

   UINT32 createMessageId() { return InterlockedIncrement(&m_msgId); }
   bool sendMessage(NXCPMessage *msg);
   NXCPMessage *waitForMessage(UINT16 code, UINT32 id, UINT32 timeout = 0);
   UINT32 waitForRCC(UINT32 id, UINT32 timeout = 0);

   void setCommandTimeout(UINT32 timeout) { m_commandTimeout = timeout; }
   UINT32 getCommandTimeout() { return m_commandTimeout; }

   const TCHAR *getServerVersion() { return m_serverVersion; }
   const TCHAR *getServerTimeZone() { return m_serverTimeZone; }
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
