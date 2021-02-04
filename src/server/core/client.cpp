/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
static RWLOCK s_sessionListLock = RWLockCreate();
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
   RWLockWriteLock(s_sessionListLock);
   if (s_freePos < g_maxClientSessions)
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
   RWLockUnlock(s_sessionListLock);

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
   RWLockWriteLock(s_sessionListLock);
   if (s_sessions.contains(id))
   {
      s_sessions.remove(id);
      s_freeList[--s_freePos] = id;
   }
   RWLockUnlock(s_sessionListLock);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Client session with ID %d unregistered"), id);
}

/**
 * Keep-alive thread
 */
static void ClientSessionManager()
{
   ThreadSetName("ClientManager");

   // Read configuration
   uint32_t interval = ConfigReadInt(_T("KeepAliveInterval"), 60);
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

      RWLockReadLock(s_sessionListLock);
      auto it = s_sessions.iterator();
      while(it->hasNext())
      {
         ClientSession *session = it->next();
         if (session->isAuthenticated())
         {
            session->postMessage(&msg);
            session->runHousekeeper();
         }
      }
      delete it;
      RWLockUnlock(s_sessionListLock);
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
   ClientListener(UINT16 port) : StreamSocketListener(port) { setName(_T("Clients")); }
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
   uint16_t listenPort = static_cast<uint16_t>(ConfigReadInt(_T("ClientListenerPort"), SERVER_LISTEN_PORT_FOR_CLIENTS));
   ClientListener listener(listenPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
      return;

   listener.mainLoop();
   listener.shutdown();

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Terminating client sessions that are still active..."));
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      it->next()->terminate();
   }
   delete it;
   RWLockUnlock(s_sessionListLock);

   while(GetSessionCount(true, true, -1, nullptr) > 0)
      ThreadSleepMs(100);

   s_pollers.clear();
}

/**
 * Dump client sessions to screen
 */
void DumpClientSessions(ServerConsole *pCtx)
{
   static const TCHAR *pszCipherName[] = { _T("NONE"), _T("AES-256"), _T("BF-256"), _T("IDEA"), _T("3DES"), _T("AES-128"), _T("BF-128") };
	static const TCHAR *pszClientType[] = { _T("DESKTOP"), _T("WEB"), _T("MOBILE"), _T("TABLET"), _T("APP") };

   ConsolePrintf(pCtx, _T("ID  CIPHER   CLTYPE  USER [CLIENT]\n"));
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      TCHAR webServer[256] = _T("");
      if (session->getClientType() == CLIENT_TYPE_WEB)
      {
         _sntprintf(webServer, 256, _T(" (%s)"), session->getWebServerAddress());
      }
      ConsolePrintf(pCtx, _T("%-3d %-8s %-7s %s%s [%s]\n"), session->getId(),
            pszCipherName[session->getCipher() + 1], pszClientType[session->getClientType()],
                    session->getSessionName(), webServer, session->getClientInfo());
   }
   delete it;
   int count = s_sessions.size();
   RWLockUnlock(s_sessionListLock);
   ConsolePrintf(pCtx, _T("\n%d active session%s\n\n"), count, count == 1 ? _T("") : _T("s"));
}

/**
 * Kill client session
 */
bool NXCORE_EXPORTABLE KillClientSession(session_id_t id)
{
   bool success = false;
   RWLockReadLock(s_sessionListLock);
   ClientSession *session = s_sessions.get(id);
   if (session != nullptr)
   {
      session->terminate();
      success = true;
   }
   RWLockUnlock(s_sessionListLock);
   return success;
}

/**
 * Enumeration callback for EnumerateClientSessions
 */
static EnumerationCallbackResult EnumerateClientSessionsCB(const session_id_t& id, ClientSession *session, std::pair<void (*)(ClientSession*, void*), void*> *context)
{
   if (!session->isTerminated())
      context->first(session, context->second);
   return _CONTINUE;
}

/**
 * Enumerate active sessions
 */
void NXCORE_EXPORTABLE EnumerateClientSessions(void (*handler)(ClientSession *, void *), void *context)
{
   std::pair<void (*)(ClientSession*, void*), void*> data(handler, context);
   RWLockReadLock(s_sessionListLock);
   s_sessions.forEach(EnumerateClientSessionsCB, &data);
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send user database update notification to all clients
 */
void SendUserDBUpdate(int code, UINT32 id, UserDatabaseObject *object)
{
   NXCPMessage msg;
   msg.setCode(CMD_USER_DB_UPDATE);
   msg.setId(0);
   msg.setField(VID_UPDATE_TYPE, (WORD)code);
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

   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->isAuthenticated() && !session->isTerminated() && session->isSubscribedTo(NXC_CHANNEL_USERDB))
         session->postMessage(msg);
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send graph update to all active sessions
 */
void NXCORE_EXPORTABLE NotifyClientsOnGraphUpdate(const NXCPMessage& msg, uint32_t graphId)
{
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->isAuthenticated() && !session->isTerminated() && (GetGraphAccessCheckResult(graphId, session->getUserId()) == RCC_SUCCESS))
         session->postMessage(msg);
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send policy update/create to all sessions
 */
void NotifyClientsOnPolicyUpdate(const NXCPMessage& msg, const Template& object)
{
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->isAuthenticated() && !session->isTerminated() && object.checkAccessRights(session->getUserId(), OBJECT_ACCESS_MODIFY))
         session->postMessage(msg);
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send policy update/create to all sessions
 */
void NotifyClientsOnPolicyDelete(uuid guid, const Template& object)
{
   NXCPMessage msg;
   msg.setCode(CMD_DELETE_AGENT_POLICY);
   msg.setField(VID_GUID, guid);
   msg.setField(VID_TEMPLATE_ID, object.getId());
   NotifyClientsOnPolicyUpdate(msg, object);
}

/**
 * Send DCI update to all active sessions
 */
void NotifyClientsOnDCIUpdate(const DataCollectionOwner& object, DCObject *dco)
{
   NXCPMessage msg;
   msg.setCode(CMD_MODIFY_NODE_DCI);
   msg.setField(VID_OBJECT_ID, object.getId());
   dco->createMessage(&msg);
   NotifyClientsOnDCIUpdate(msg, object);
}

/**
 * Send graph update to all active sessions
 */
void NotifyClientsOnDCIDelete(const DataCollectionOwner& object, uint32_t dcoId)
{
   NXCPMessage msg;
   msg.setCode(CMD_DELETE_NODE_DCI);
   msg.setField(VID_OBJECT_ID, object.getId());
   msg.setField(VID_DCI_ID, dcoId);
   NotifyClientsOnDCIUpdate(msg, object);
}

/**
 * Send DCI delete to all active sessions
 */
void NotifyClientsOnDCIStatusChange(const DataCollectionOwner& object, uint32_t dcoId, int status)
{
   NXCPMessage msg;
   msg.setCode(CMD_SET_DCI_STATUS);
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
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->isAuthenticated() && !session->isTerminated() &&
          object.checkAccessRights(session->getUserId(), OBJECT_ACCESS_MODIFY) &&
          session->isDataCollectionConfigurationOpen(object.getId()))
      {
         session->postMessage(msg);
      }
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
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
   if (instance != NULL)
      msg.setField(VID_INSTANCE, instance);
   msg.setField(VID_STATE, change == ThresholdCheckResult::ACTIVATED);

   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->isAuthenticated() && !session->isTerminated() &&
          object->checkAccessRights(session->getUserId(), OBJECT_ACCESS_MODIFY) &&
          session->isSubscribedTo(NXC_CHANNEL_DC_THRESHOLDS))
      {
         session->postMessage(msg);
      }
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send notification message to all active user sessions with given subscription
 */
void NXCORE_EXPORTABLE NotifyClientSessions(const NXCPMessage& msg, const TCHAR *channel)
{
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->isAuthenticated() && !session->isTerminated() && ((channel == nullptr) || session->isSubscribedTo(channel)))
      {
         session->postMessage(msg);
      }
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send notification to all active user sessions
 */
void NXCORE_EXPORTABLE NotifyClientSessions(uint32_t code, uint32_t data)
{
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->isAuthenticated() && !session->isTerminated())
      {
         session->notify(code, data);
      }
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send notification to specified user session
 */
void NXCORE_EXPORTABLE NotifyClientSession(session_id_t sessionId, uint32_t code, uint32_t data)
{
   RWLockReadLock(s_sessionListLock);
   ClientSession *session = s_sessions.get(sessionId);
   if (session != nullptr)
      session->notify(code, data);
   RWLockUnlock(s_sessionListLock);
}

/**
 * Get number of active user sessions
 */
int GetSessionCount(bool includeSystemAccount, bool includeNonAuthenticated, int typeFilter, const TCHAR *loginFilter)
{
   int count = 0;

   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if ((includeSystemAccount || (session->getUserId() != 0)) &&
          (includeNonAuthenticated || session->isAuthenticated()) &&
          ((typeFilter == -1) || (session->getClientType() == typeFilter)) &&
          ((loginFilter == nullptr) || !_tcscmp(loginFilter, session->getLoginName())))
      {
         count++;
      }
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
   return count;
}

/**
 * Check if given user is currenly logged in
 */
bool IsLoggedIn(uint32_t userId)
{
   bool result = false;
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if (session->getUserId() == userId)
      {
         result = true;
         break;
      }
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
   return result;
}

/**
 * Close all user's sessions except given one
 */
void CloseOtherSessions(uint32_t userId, session_id_t thisSession)
{
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      ClientSession *session = it->next();
      if ((session->getUserId() == userId) && (session->getId() != thisSession))
      {
         nxlog_debug(4, _T("CloseOtherSessions(%u,%u): disconnecting session %u"), userId, thisSession, session->getId());
         session->terminate();
      }
   }
   delete it;
   RWLockUnlock(s_sessionListLock);
}
