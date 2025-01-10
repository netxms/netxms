/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: client.cpp
**
**/

#include "nxcore.h"
#include <socket_listener.h>
#include <nms_users.h>

#define DEBUG_TAG _T("client.session")

/**
 * Client thread pool
 */
NXCORE_EXPORTABLE_VAR(ThreadPool *g_clientThreadPool) = nullptr;

/**
 * Maximum number of client sessions
 */
int32_t g_maxClientSessions = 256;

/**
 * Static data
 */
static HashMap<session_id_t, ClientSession> s_sessions;
static RWLock s_sessionListLock;
static session_id_t *s_freeList = nullptr;
static size_t s_freePos = 0;
static ObjectArray<BackgroundSocketPollerHandle> s_pollers(8, 8, Ownership::True);
static uint32_t s_maxClientSessionsPerPoller = std::min(256, SOCKET_POLLER_MAX_SOCKETS - 1);

/**
 * Register new session in list
 */
static bool RegisterClientSession(ClientSession *session)
{
   bool success;
   s_sessionListLock.writeLock();
   if (s_freePos < static_cast<size_t>(g_maxClientSessions))
   {
      // Select socket poller
      BackgroundSocketPollerHandle *sp = nullptr;
      for(int i = 0; i < s_pollers.size(); i++)
      {
         BackgroundSocketPollerHandle *p = s_pollers.get(i);
         if (static_cast<uint32_t>(InterlockedIncrement(&p->usageCount)) < s_maxClientSessionsPerPoller)
         {
            sp = p;
            break;
         }
         InterlockedDecrement(&p->usageCount);
      }
      if (sp == nullptr)
      {
         sp = new BackgroundSocketPollerHandle();
         sp->usageCount = 1;
         s_pollers.add(sp);
      }
      session->setSocketPoller(sp);

      session->setId(s_freeList[s_freePos++]);
      s_sessions.set(session->getId(), session);
      success = true;
   }
   else
   {
      success = false;
   }
   s_sessionListLock.unlock();

   if (success)
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Client session with ID %d registered"), session->getId());
   else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Too many client sessions open - unable to accept new client connection"));

   return success;
}

/**
 * Unregister session
 */
void UnregisterClientSession(session_id_t id)
{
   s_sessionListLock.writeLock();
   if (s_sessions.contains(id))
   {
      s_sessions.remove(id);
      s_freeList[--s_freePos] = id;
   }
   s_sessionListLock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Client session with ID %d unregistered"), id);
}

/**
 * Keep-alive thread
 */
static void ClientSessionManager()
{
   ThreadSetName("ClientManager");

   // Read configuration
   uint32_t interval = ConfigReadInt(_T("Client.KeepAliveInterval"), 60);
   if (interval > 300)
      interval = 300;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Client session manager started (check interval %u seconds)"), interval);

   // Prepare keepalive message
   NXCPMessage msg(CMD_KEEPALIVE, 0);

   while(true)
   {
      if (SleepAndCheckForShutdown(interval))
         break;

      msg.setFieldFromTime(VID_TIMESTAMP, time(nullptr));

      s_sessionListLock.readLock();
      auto it = s_sessions.begin();
      while(it.hasNext())
      {
         ClientSession *session = it.next();
         if (session->isAuthenticated())
         {
            session->postMessage(&msg);
            session->runHousekeeper();
         }
      }
      s_sessionListLock.unlock();
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Client session manager thread stopped"));
}

/**
 * Initialize client listener(s)
 */
void InitClientListeners()
{
   s_maxClientSessionsPerPoller = ConfigReadULong(_T("ClientConnector.MaxSessionsPerPoller"), s_maxClientSessionsPerPoller);
   if (s_maxClientSessionsPerPoller > SOCKET_POLLER_MAX_SOCKETS - 1)
      s_maxClientSessionsPerPoller = SOCKET_POLLER_MAX_SOCKETS - 1;

   s_freeList = MemAllocArrayNoInit<session_id_t>(g_maxClientSessions);
   for(int i = 0; i < g_maxClientSessions; i++)
      s_freeList[i] = i;

   g_clientThreadPool = ThreadPoolCreate(_T("CLIENT"),
            ConfigReadInt(_T("ThreadPool.Client.BaseSize"), 16),
            ConfigReadInt(_T("ThreadPool.Client.MaxSize"), g_maxClientSessions * 8));

   // Start client keep-alive thread
   ThreadCreate(ClientSessionManager);
}

/**
 * Client listener class
 */
class ClientListener : public StreamSocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer);
   virtual bool isStopConditionReached();

public:
   ClientListener(uint16_t port) : StreamSocketListener(port) { setName(_T("Clients")); }
};

/**
 * Listener stop condition
 */
bool ClientListener::isStopConditionReached()
{
   return IsShutdownInProgress();
}

/**
 * Process incoming connection
 */
ConnectionProcessingResult ClientListener::processConnection(SOCKET s, const InetAddress& peer)
{
   SetSocketNonBlocking(s);
   ClientSession *session = new ClientSession(s, peer);
   if (RegisterClientSession(session))
   {
      if (!session->start())
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot start client session"));
         UnregisterClientSession(session->getId());
         delete session;
      }
   }
   else
   {
      delete session;
   }
   return CPR_BACKGROUND;
}

/**
 * Listener thread
 */
void ClientListenerThread()
{
   ThreadSetName("ClientListener");
   uint16_t listenPort = static_cast<uint16_t>(ConfigReadInt(_T("Client.ListenerPort"), SERVER_LISTEN_PORT_FOR_CLIENTS));
   ClientListener listener(listenPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
      return;

   listener.mainLoop();
   listener.shutdown();

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Terminating client sessions that are still active..."));
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      it.next()->terminate();
   }
   s_sessionListLock.unlock();

   while(GetSessionCount(true, true, -1, nullptr) > 0)
      ThreadSleepMs(100);

   s_pollers.clear();
}

/**
 * Dump client sessions to screen
 */
void DumpClientSessions(ServerConsole *pCtx)
{
   static const TCHAR *cipherName[] = { _T("NONE"), _T("AES-256"), _T("BF-256"), _T("IDEA"), _T("3DES"), _T("AES-128"), _T("BF-128") };
	static const TCHAR *clientType[] = { _T("DESKTOP"), _T("WEB"), _T("MOBILE"), _T("TABLET"), _T("APP") };

   ConsolePrintf(pCtx, _T(" ID  | ADDRESS                        | WEB SERVER           | TYPE    | CIPHER   | USER                 | CLIENT\n"));
   ConsolePrintf(pCtx, _T("-----+--------------------------------+----------------------+---------+----------+----------------------+----------------------------\n"));
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      ConsolePrintf(pCtx, _T(" %-3d | %-30s | %-20s | %-7s | %-8s | %-20s | %s\n"), session->getId(), session->getWorkstation(), session->getWebServerAddress(),
         clientType[session->getClientType()], cipherName[session->getCipher() + 1], session->getLoginName(), session->getClientInfo());
   }
   int count = s_sessions.size();
   s_sessionListLock.unlock();
   ConsolePrintf(pCtx, _T("\n%d active session%s\n\n"), count, count == 1 ? _T("") : _T("s"));
}

/**
 * Kill client session
 */
bool NXCORE_EXPORTABLE KillClientSession(session_id_t id)
{
   bool success = false;
   s_sessionListLock.readLock();
   ClientSession *session = s_sessions.get(id);
   if (session != nullptr)
   {
      session->terminate();
      success = true;
   }
   s_sessionListLock.unlock();
   return success;
}

/**
 * Enumerate active sessions
 */
void NXCORE_EXPORTABLE EnumerateClientSessions(void (*callback)(ClientSession*, void*), void *context)
{
   s_sessionListLock.readLock();
   s_sessions.forEach(
      [callback, context] (const session_id_t& id, ClientSession *session) -> EnumerationCallbackResult
      {
         if (!session->isTerminated())
            callback(session, context);
         return _CONTINUE;
      });
   s_sessionListLock.unlock();
}

/**
 * Enumerate active sessions
 */
void NXCORE_EXPORTABLE EnumerateClientSessions(std::function<void(ClientSession*)> callback)
{
   s_sessionListLock.readLock();
   s_sessions.forEach(
      [callback] (const session_id_t& id, ClientSession *session) -> EnumerationCallbackResult
      {
         if (!session->isTerminated())
            callback(session);
         return _CONTINUE;
      });
   s_sessionListLock.unlock();
}

/**
 * Send user database update notification to all clients
 */
void SendUserDBUpdate(uint16_t code, uint32_t id, UserDatabaseObject *object)
{
   NXCPMessage msg(CMD_USER_DB_UPDATE, 0);
   msg.setField(VID_UPDATE_TYPE, code);
   switch(code)
   {
      case USER_DB_CREATE:
      case USER_DB_MODIFY:
         object->fillMessage(&msg);
         break;
      default:
         msg.setField(VID_USER_ID, id);
         break;
   }

   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if (session->isAuthenticated() && !session->isTerminated() && session->isSubscribedTo(NXC_CHANNEL_USERDB))
         session->postMessage(msg);
   }
   s_sessionListLock.unlock();
}

/**
 * Send graph update to all active sessions
 */
void NXCORE_EXPORTABLE NotifyClientsOnGraphUpdate(const NXCPMessage& msg, uint32_t graphId)
{
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if (session->isAuthenticated() && !session->isTerminated() && (CheckGraphAccess(graphId, session->getUserId(), NXGRAPH_ACCESS_READ) == RCC_SUCCESS))
         session->postMessage(msg);
   }
   s_sessionListLock.unlock();
}

/**
 * Send policy update/create to all sessions
 */
void NotifyClientsOnPolicyUpdate(const NXCPMessage& msg, const Template& object)
{
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if (session->isAuthenticated() && !session->isTerminated() && object.checkAccessRights(session->getUserId(), OBJECT_ACCESS_MODIFY))
         session->postMessage(msg);
   }
   s_sessionListLock.unlock();
}

/**
 * Send policy update/create to all sessions
 */
void NotifyClientsOnPolicyDelete(uuid guid, const Template& object)
{
   NXCPMessage msg(CMD_DELETE_AGENT_POLICY, 0);
   msg.setField(VID_GUID, guid);
   msg.setField(VID_TEMPLATE_ID, object.getId());
   NotifyClientsOnPolicyUpdate(msg, object);
}

/**
 * Send business service checks update/delete to all active sessions
 */
void NotifyClientsOnBusinessServiceCheckUpdate(const NXCPMessage& msg, const NetObj& object)
{
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if (session->isAuthenticated() && !session->isTerminated() && object.checkAccessRights(session->getUserId(), OBJECT_ACCESS_READ))
      {
         session->postMessage(msg);
      }
   }
   s_sessionListLock.unlock();
}

/**
 * Send business service check update to all active sessions
 */
void NotifyClientsOnBusinessServiceCheckUpdate(const NetObj& service, const shared_ptr<BusinessServiceCheck>& check)
{
   NXCPMessage msg(CMD_UPDATE_BIZSVC_CHECK, 0);
   check->fillMessage(&msg, VID_CHECK_LIST_BASE);
   NotifyClientsOnBusinessServiceCheckUpdate(msg, service);
}

/**
 * Send business service check delete to all active sessions
 */
void NotifyClientsOnBusinessServiceCheckDelete(const NetObj& service, uint32_t checkId)
{
   NXCPMessage msg(CMD_DELETE_BIZSVC_CHECK, 0);
   msg.setField(VID_OBJECT_ID, service.getId());
   msg.setField(VID_CHECK_ID, checkId);
   NotifyClientsOnBusinessServiceCheckUpdate(msg, service);
}

/**
 * Send DCI update to all active sessions
 */
void NotifyClientsOnDCIUpdate(const DataCollectionOwner& object, DCObject *dco)
{
   NXCPMessage msg(CMD_MODIFY_NODE_DCI, 0);
   msg.setField(VID_OBJECT_ID, object.getId());
   dco->createMessage(&msg);
   NotifyClientsOnDCIUpdate(msg, object);
}

/**
 * Send graph update to all active sessions
 */
void NotifyClientsOnDCIDelete(const DataCollectionOwner& object, uint32_t dcoId)
{
   NXCPMessage msg(CMD_DELETE_NODE_DCI, 0);
   msg.setField(VID_OBJECT_ID, object.getId());
   msg.setField(VID_DCI_ID, dcoId);
   NotifyClientsOnDCIUpdate(msg, object);
}

/**
 * Send DCI delete to all active sessions
 */
void NotifyClientsOnDCIStatusChange(const DataCollectionOwner& object, uint32_t dcoId, int status)
{
   NXCPMessage msg(CMD_SET_DCI_STATUS, 0);
   msg.setField(VID_OBJECT_ID, object.getId());
   msg.setField(VID_DCI_STATUS, status);
   msg.setField(VID_NUM_ITEMS, 1);
   msg.setField(VID_ITEM_LIST, dcoId);
   NotifyClientsOnDCIUpdate(msg, object);
}

/**
 * Send DCI update/delete to all active sessions
 */
void NotifyClientsOnDCIUpdate(const NXCPMessage& msg, const NetObj& object)
{
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if (session->isAuthenticated() && !session->isTerminated() &&
          session->isDataCollectionConfigurationOpen(object.getId()) &&
          object.checkAccessRights(session->getUserId(), OBJECT_ACCESS_MODIFY))
      {
         session->postMessage(msg);
      }
   }
   s_sessionListLock.unlock();
}

/**
 * Notify clients on threshold change
 */
void NotifyClientsOnThresholdChange(UINT32 objectId, UINT32 dciId, UINT32 thresholdId, const TCHAR *instance, ThresholdCheckResult change)
{
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return;

   NXCPMessage msg(CMD_THRESHOLD_UPDATE, 0);
   msg.setField(VID_OBJECT_ID, objectId);
   msg.setField(VID_DCI_ID, dciId);
   msg.setField(VID_THRESHOLD_ID, thresholdId);
   msg.setField(VID_INSTANCE, instance);
   msg.setField(VID_STATE, change == ThresholdCheckResult::ACTIVATED);

   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if (session->isAuthenticated() && !session->isTerminated() &&
          session->isSubscribedTo(NXC_CHANNEL_DC_THRESHOLDS) &&
          object->checkAccessRights(session->getUserId(), OBJECT_ACCESS_MODIFY))
      {
         session->postMessage(msg);
      }
   }
   s_sessionListLock.unlock();
}

/**
 * Notify client sessions that match filter
 */
void NXCORE_EXPORTABLE NotifyClientSessions(const NXCPMessage& msg, std::function<bool (ClientSession*)> filter)
{
   s_sessionListLock.readLock();
   if (s_sessions.size() > 0)
   {
      auto it = s_sessions.begin();
      while(it.hasNext())
      {
         ClientSession *session = it.next();
         if (session->isAuthenticated() && !session->isTerminated() && filter(session))
         {
            session->postMessage(msg);
         }
      }
   }
   s_sessionListLock.unlock();
}

/**
 * Send notification message to all active user sessions with given subscription
 */
void NXCORE_EXPORTABLE NotifyClientSessions(const NXCPMessage& msg, const wchar_t *channel)
{
   NotifyClientSessions(msg,
      [channel] (ClientSession *session) -> bool
      {
         return (channel == nullptr) || session->isSubscribedTo(channel);
      });
}

/**
 * Send notification to all active user sessions
 */
void NXCORE_EXPORTABLE NotifyClientSessions(uint32_t code, uint32_t data, const wchar_t *channel)
{
   NXCPMessage msg(CMD_NOTIFY, 0);
   msg.setField(VID_NOTIFICATION_CODE, code);
   msg.setField(VID_NOTIFICATION_DATA, data);
   NotifyClientSessions(msg,
      [channel] (ClientSession *session) -> bool
      {
         return (channel == nullptr) || session->isSubscribedTo(channel);
      });
}

/**
 * Send notification to specified user session
 */
void NXCORE_EXPORTABLE NotifyClientSession(session_id_t sessionId, uint32_t code, uint32_t data)
{
   s_sessionListLock.readLock();
   ClientSession *session = s_sessions.get(sessionId);
   if (session != nullptr)
      session->notify(code, data);
   s_sessionListLock.unlock();
}

/**
 * Send notification to specified user session
 */
void NXCORE_EXPORTABLE NotifyClientSession(session_id_t sessionId, NXCPMessage *data)
{
   s_sessionListLock.readLock();
   ClientSession *session = s_sessions.get(sessionId);
   if (session != nullptr)
      session->sendMessage(data);
   s_sessionListLock.unlock();
}

/**
 * Get number of active user sessions
 */
int GetSessionCount(bool includeSystemAccount, bool includeNonAuthenticated, int typeFilter, const TCHAR *loginFilter)
{
   int count = 0;

   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if ((includeSystemAccount || (session->getUserId() != 0)) &&
          (includeNonAuthenticated || session->isAuthenticated()) &&
          ((typeFilter == -1) || (session->getClientType() == typeFilter)) &&
          ((loginFilter == nullptr) || !_tcscmp(loginFilter, session->getLoginName())))
      {
         count++;
      }
   }
   s_sessionListLock.unlock();
   return count;
}

/**
 * Check if given user is currenly logged in
 */
bool IsLoggedIn(uint32_t userId)
{
   bool result = false;
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if (session->getUserId() == userId)
      {
         result = true;
         break;
      }
   }
   s_sessionListLock.unlock();
   return result;
}

/**
 * Close all user's sessions except given one
 */
void NXCORE_EXPORTABLE CloseOtherSessions(uint32_t userId, session_id_t thisSession)
{
   s_sessionListLock.readLock();
   auto it = s_sessions.begin();
   while(it.hasNext())
   {
      ClientSession *session = it.next();
      if ((session->getUserId() == userId) && (session->getId() != thisSession))
      {
         nxlog_debug(4, _T("CloseOtherSessions(%u,%u): disconnecting session %u"), userId, thisSession, session->getId());
         session->terminate();
      }
   }
   s_sessionListLock.unlock();
}
