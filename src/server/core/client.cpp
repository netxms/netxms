/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
NXCORE_EXPORTABLE_VAR(ThreadPool *g_clientThreadPool) = NULL;

/**
 * Static data
 */
static ClientSession *s_sessionList[MAX_CLIENT_SESSIONS];
static RWLOCK s_sessionListLock;

/**
 * Register new session in list
 */
static BOOL RegisterClientSession(ClientSession *pSession)
{
   RWLockWriteLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if (s_sessionList[i] == NULL)
      {
         s_sessionList[i] = pSession;
         pSession->setId(i);
         RWLockUnlock(s_sessionListLock);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Client session with ID %d registered"), i);
         return TRUE;
      }
   RWLockUnlock(s_sessionListLock);
   nxlog_write(MSG_TOO_MANY_SESSIONS, EVENTLOG_WARNING_TYPE, NULL);
   return FALSE;
}

/**
 * Unregister session
 */
void UnregisterClientSession(int id)
{
   RWLockWriteLock(s_sessionListLock, INFINITE);
   s_sessionList[id] = NULL;
   RWLockUnlock(s_sessionListLock);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Client session with ID %d unregistered"), id);
}

/**
 * Keep-alive thread
 */
static THREAD_RESULT THREAD_CALL ClientKeepAliveThread(void *)
{
   ThreadSetName("ClientKeepAlive");

   // Read configuration
   int iSleepTime = ConfigReadInt(_T("KeepAliveInterval"), 60);

   // Prepare keepalive message
   NXCPMessage msg;
   msg.setCode(CMD_KEEPALIVE);
   msg.setId(0);

   while(true)
   {
      if (SleepAndCheckForShutdown(iSleepTime))
         break;

      msg.setField(VID_TIMESTAMP, (UINT32)time(NULL));
      RWLockReadLock(s_sessionListLock, INFINITE);
      for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
         if (s_sessionList[i] != NULL)
            if (s_sessionList[i]->isAuthenticated())
               s_sessionList[i]->postMessage(&msg);
      RWLockUnlock(s_sessionListLock);
   }

   DbgPrintf(1, _T("Client keep-alive thread terminated"));
   return THREAD_OK;
}

/**
 * Initialize client listener(s)
 */
void InitClientListeners()
{
   g_clientThreadPool = ThreadPoolCreate(_T("CLIENT"),
            ConfigReadInt(_T("ThreadPool.Client.BaseSize"), 16),
            ConfigReadInt(_T("ThreadPool.Client.MaxSize"), MAX_CLIENT_SESSIONS * 8));

   memset(s_sessionList, 0, sizeof(s_sessionList));

   // Create session list access rwlock
   s_sessionListLock = RWLockCreate();

   // Start client keep-alive thread
   ThreadCreate(ClientKeepAliveThread, 0, NULL);
}

/**
 * Client listener class
 */
class ClientListener : public SocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer);
   virtual bool isStopConditionReached();

public:
   ClientListener(UINT16 port) : SocketListener(port) { setName(_T("Clients")); }
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
         nxlog_write(MSG_CANNOT_CREATE_SESSION_THREAD, NXLOG_ERROR, NULL);
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
THREAD_RESULT THREAD_CALL ClientListenerThread(void *arg)
{
   ThreadSetName("ClientListener");
   UINT16 listenPort = (UINT16)ConfigReadInt(_T("ClientListenerPort"), SERVER_LISTEN_PORT_FOR_CLIENTS);
   ClientListener listener(listenPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
      return THREAD_OK;

   listener.mainLoop();
   listener.shutdown();
   return THREAD_OK;
}

/**
 * Dump client sessions to screen
 */
void DumpClientSessions(CONSOLE_CTX pCtx)
{
   int i, iCount;
   static const TCHAR *pszStateName[] = { _T("init"), _T("idle"), _T("processing") };
   static const TCHAR *pszCipherName[] = { _T("NONE"), _T("AES-256"), _T("BF-256"), _T("IDEA"), _T("3DES"), _T("AES-128"), _T("BF-128") };
	static const TCHAR *pszClientType[] = { _T("DESKTOP"), _T("WEB"), _T("MOBILE"), _T("TABLET"), _T("APP") };

   ConsolePrintf(pCtx, _T("ID  CIPHER   CLTYPE  USER [CLIENT]\n"));
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(i = 0, iCount = 0; i < MAX_CLIENT_SESSIONS; i++)
      if (s_sessionList[i] != NULL)
      {
         TCHAR webServer[256] = _T("");
         if (s_sessionList[i]->getClientType() == CLIENT_TYPE_WEB)
         {
            _sntprintf(webServer, 256, _T(" (%s)"), s_sessionList[i]->getWebServerAddress());
         }
         ConsolePrintf(pCtx, _T("%-3d %-8s %-7s %s%s [%s]\n"), i,
					        pszCipherName[s_sessionList[i]->getCipher() + 1],
							  pszClientType[s_sessionList[i]->getClientType()],
                       s_sessionList[i]->getSessionName(), webServer,
                       s_sessionList[i]->getClientInfo());
         iCount++;
      }
   RWLockUnlock(s_sessionListLock);
   ConsolePrintf(pCtx, _T("\n%d active session%s\n\n"), iCount, iCount == 1 ? _T("") : _T("s"));
}

/**
 * Kill client session
 */
bool NXCORE_EXPORTABLE KillClientSession(int id)
{
   bool success = false;
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
   {
      if ((s_sessionList[i] != NULL) && (s_sessionList[i]->getId() == id))
      {
         s_sessionList[i]->kill();
         success = true;
         break;
      }
   }
   RWLockUnlock(s_sessionListLock);
   return success;
}

/**
 * Enumerate active sessions
 */
void NXCORE_EXPORTABLE EnumerateClientSessions(void (*pHandler)(ClientSession *, void *), void *pArg)
{
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
   {
      if ((s_sessionList[i] != NULL) && !s_sessionList[i]->isTerminated())
      {
         pHandler(s_sessionList[i], pArg);
      }
   }
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

   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if ((s_sessionList[i] != NULL) &&
          s_sessionList[i]->isAuthenticated() &&
          !s_sessionList[i]->isTerminated() &&
          s_sessionList[i]->isSubscribedTo(NXC_CHANNEL_USERDB))
         s_sessionList[i]->postMessage(&msg);
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send graph update to all active sessions
 */
void NXCORE_EXPORTABLE NotifyClientGraphUpdate(NXCPMessage *update, UINT32 graphId)
{
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if ((s_sessionList[i] != NULL) &&
          s_sessionList[i]->isAuthenticated() &&
          !s_sessionList[i]->isTerminated() &&
          (GetGraphAccessCheckResult(graphId, s_sessionList[i]->getUserId()) == RCC_SUCCESS))
         s_sessionList[i]->postMessage(update);
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send policy update/create to all sessions
 */
void NotifyClientPolicyUpdate(NXCPMessage *msg, Template *object)
{
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if ((s_sessionList[i] != NULL) &&
          s_sessionList[i]->isAuthenticated() &&
          !s_sessionList[i]->isTerminated() &&
          object->checkAccessRights(s_sessionList[i]->getUserId(), OBJECT_ACCESS_MODIFY))
         s_sessionList[i]->postMessage(msg);
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send policy update/create to all sessions
 */
void NotifyClientPolicyDelete(uuid guid, Template *object)
{
   NXCPMessage msg;
   msg.setCode(CMD_DELETE_AGENT_POLICY);
   msg.setField(VID_GUID, guid);
   msg.setField(VID_TEMPLATE_ID, object->getId());
   NotifyClientPolicyUpdate(&msg, object);
}

/**
 * Send DCI update to all active sessions
 */
void NotifyClientDCIUpdate(DataCollectionOwner *object, DCObject *dco)
{
   NXCPMessage msg;
   msg.setCode(CMD_MODIFY_NODE_DCI);
   msg.setField(VID_OBJECT_ID, object->getId());
   dco->createMessage(&msg);
   NotifyClientDCIUpdate(&msg, object);
}

/**
 * Send graph update to all active sessions
 */
void NotifyClientDCIDelete(DataCollectionOwner *object, UINT32 dcoId)
{
   NXCPMessage msg;
   msg.setCode(CMD_DELETE_NODE_DCI);
   msg.setField(VID_OBJECT_ID, object->getId());
   msg.setField(VID_DCI_ID, dcoId);
   NotifyClientDCIUpdate(&msg, object);
}

/**
 * Send DCI delete to all active sessions
 */
void NotifyClientDCIStatusChange(DataCollectionOwner *object, UINT32 dcoId, int status)
{
   NXCPMessage msg;
   msg.setCode(CMD_SET_DCI_STATUS);
   msg.setField(VID_OBJECT_ID, object->getId());
   msg.setField(VID_DCI_STATUS, status);
   msg.setField(VID_NUM_ITEMS, 1);
   msg.setField(VID_ITEM_LIST, dcoId);
   NotifyClientDCIUpdate(&msg, object);
}

/**
 * Send DCI update/delete to all active sessions
 */
void NotifyClientDCIUpdate(NXCPMessage *update, NetObj *object)
{
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if ((s_sessionList[i] != NULL) &&
          s_sessionList[i]->isAuthenticated() &&
          !s_sessionList[i]->isTerminated() &&
          object->checkAccessRights(s_sessionList[i]->getUserId(), OBJECT_ACCESS_MODIFY) &&
          s_sessionList[i]->isDCOpened(object->getId()))
         s_sessionList[i]->postMessage(update);
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send notification to all active user sessions
 */
void NXCORE_EXPORTABLE NotifyClientSessions(UINT32 dwCode, UINT32 dwData)
{
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
   {
      if ((s_sessionList[i] != NULL) &&
          s_sessionList[i]->isAuthenticated() &&
          !s_sessionList[i]->isTerminated())
      {
         s_sessionList[i]->notify(dwCode, dwData);
      }
   }
   RWLockUnlock(s_sessionListLock);
}

/**
 * Send notification to specified user session
 */
void NXCORE_EXPORTABLE NotifyClientSession(UINT32 sessionId, UINT32 dwCode, UINT32 dwData)
{
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if ((s_sessionList[i] != NULL) && (s_sessionList[i]->getId() == sessionId))
         s_sessionList[i]->notify(dwCode, dwData);
   RWLockUnlock(s_sessionListLock);
}

/**
 * Get number of active user sessions
 */
int GetSessionCount(bool includeSystemAccount)
{
   int i, nCount;

   RWLockReadLock(s_sessionListLock, INFINITE);
   for(i = 0, nCount = 0; i < MAX_CLIENT_SESSIONS; i++)
   {
      if ((s_sessionList[i] != NULL) &&
          (includeSystemAccount || (s_sessionList[i]->getUserId() != 0)))
      {
         nCount++;
      }
   }
   RWLockUnlock(s_sessionListLock);
   return nCount;
}

/**
 * Check if given user is currenly logged in
 */
bool IsLoggedIn(UINT32 dwUserId)
{
   bool result = false;
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if (s_sessionList[i] != NULL && s_sessionList[i]->getUserId() == dwUserId)
      {
         result = true;
         break;
      }
   RWLockUnlock(s_sessionListLock);
   return result;
}

/**
 * Close all user's sessions except given one
 */
void CloseOtherSessions(UINT32 userId, UINT32 thisSession)
{
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_CLIENT_SESSIONS; i++)
   {
      if ((s_sessionList[i] != NULL) &&
          (s_sessionList[i]->getUserId() == userId) &&
          (s_sessionList[i]->getId() != thisSession))
      {
         nxlog_debug(4, _T("CloseOtherSessions(%d,%d): disconnecting session %d"), userId, thisSession, s_sessionList[i]->getId());
         s_sessionList[i]->kill();
      }
   }
   RWLockUnlock(s_sessionListLock);
}
