/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: mdconn.cpp
**
**/

#include "nxcore.h"
#include <socket_listener.h>

/**
 * Static data
 */
static MobileDeviceSession *s_sessionList[MAX_DEVICE_SESSIONS];
static RWLOCK s_sessionListLock;

/**
 * Register new session in list
 */
static BOOL RegisterMobileDeviceSession(MobileDeviceSession *pSession)
{
   RWLockWriteLock(s_sessionListLock, INFINITE);
   for(int i = 0; i < MAX_DEVICE_SESSIONS; i++)
      if (s_sessionList[i] == NULL)
      {
         s_sessionList[i] = pSession;
         pSession->setId(i + MAX_CLIENT_SESSIONS);
         RWLockUnlock(s_sessionListLock);
         return TRUE;
      }

   RWLockUnlock(s_sessionListLock);
   nxlog_write(MSG_TOO_MANY_MD_SESSIONS, EVENTLOG_WARNING_TYPE, NULL);
   return FALSE;
}

/**
 * Unregister session
 */
void UnregisterMobileDeviceSession(int id)
{
   RWLockWriteLock(s_sessionListLock, INFINITE);
   s_sessionList[id - MAX_CLIENT_SESSIONS] = NULL;
   RWLockUnlock(s_sessionListLock);
}

/**
 * Initialize mobile device listener(s)
 */
void InitMobileDeviceListeners()
{
   memset(s_sessionList, 0, sizeof(s_sessionList));
   s_sessionListLock = RWLockCreate();
}

/**
 * Mobile device listener class
 */
class MobileDeviceListener : public StreamSocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer);
   virtual bool isStopConditionReached();

public:
   MobileDeviceListener(UINT16 port) : StreamSocketListener(port) { setName(_T("MobileDevices")); }
};

/**
 * Listener stop condition
 */
bool MobileDeviceListener::isStopConditionReached()
{
   return IsShutdownInProgress();
}

/**
 * Process incoming connection
 */
ConnectionProcessingResult MobileDeviceListener::processConnection(SOCKET s, const InetAddress& peer)
{
   SetSocketNonBlocking(s);
   MobileDeviceSession *session = new MobileDeviceSession(s, peer);
   if (RegisterMobileDeviceSession(session))
   {
      session->run();
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
THREAD_RESULT THREAD_CALL MobileDeviceListenerThread(void *arg)
{
   ThreadSetName("MDevListener");
   UINT16 listenPort = (UINT16)ConfigReadInt(_T("MobileDeviceListenerPort"), SERVER_LISTEN_PORT_FOR_MOBILES);
   MobileDeviceListener listener(listenPort);
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
void DumpMobileDeviceSessions(CONSOLE_CTX pCtx)
{
   int i, iCount;
   TCHAR szBuffer[256];
   static const TCHAR *pszStateName[] = { _T("init"), _T("idle"), _T("processing") };
   static const TCHAR *pszCipherName[] = { _T("NONE"), _T("AES-256"), _T("BLOWFISH"), _T("IDEA"), _T("3DES"), _T("AES-128") };

   ConsolePrintf(pCtx, _T("ID  STATE                    CIPHER   USER [CLIENT]\n"));
   RWLockReadLock(s_sessionListLock, INFINITE);
   for(i = 0, iCount = 0; i < MAX_DEVICE_SESSIONS; i++)
      if (s_sessionList[i] != NULL)
      {
         ConsolePrintf(pCtx, _T("%-3d %-24s %-8s %s [%s]\n"), i, 
                       (s_sessionList[i]->getState() != SESSION_STATE_PROCESSING) ?
                         pszStateName[s_sessionList[i]->getState()] :
                         NXCPMessageCodeName(s_sessionList[i]->getCurrentCmd(), szBuffer),
					        pszCipherName[s_sessionList[i]->getCipher() + 1],
                       s_sessionList[i]->getUserName(),
                       s_sessionList[i]->getClientInfo());
         iCount++;
      }
   RWLockUnlock(s_sessionListLock);
   ConsolePrintf(pCtx, _T("\n%d active session%s\n\n"), iCount, iCount == 1 ? _T("") : _T("s"));
}
