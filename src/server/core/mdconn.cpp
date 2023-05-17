/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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

#define MAX_DEVICE_SESSIONS   256

/**
 * Mobile device thread pool
 */
NXCORE_EXPORTABLE_VAR(ThreadPool *g_mobileThreadPool) = nullptr;

/**
 * Static data
 */
static MobileDeviceSession *s_sessionList[MAX_DEVICE_SESSIONS];
static RWLock s_sessionListLock;

/**
 * Register new session in list
 */
static BOOL RegisterMobileDeviceSession(MobileDeviceSession *pSession)
{
   s_sessionListLock.writeLock();
   for(int i = 0; i < MAX_DEVICE_SESSIONS; i++)
      if (s_sessionList[i] == nullptr)
      {
         s_sessionList[i] = pSession;
         pSession->setId(i);
         s_sessionListLock.unlock();
         return TRUE;
      }

   s_sessionListLock.unlock();
   nxlog_write(NXLOG_WARNING, _T("Too many mobile device sessions open - unable to accept new client connection"));
   return FALSE;
}

/**
 * Unregister session
 */
void UnregisterMobileDeviceSession(int id)
{
   s_sessionListLock.writeLock();
   s_sessionList[id] = nullptr;
   s_sessionListLock.unlock();
}

/**
 * Initialize mobile device listener(s)
 */
void InitMobileDeviceListeners()
{
   memset(s_sessionList, 0, sizeof(s_sessionList));
   g_mobileThreadPool = ThreadPoolCreate(_T("MOBILE"),
         ConfigReadInt(_T("ThreadPool.MobileDevices.BaseSize"), 4),
         ConfigReadInt(_T("ThreadPool.MobileDevices.MaxSize"), MAX_DEVICE_SESSIONS));
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
   MobileDeviceListener(uint16_t port) : StreamSocketListener(port) { setName(_T("MobileDevices")); }
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
void MobileDeviceListenerThread()
{
   ThreadSetName("MDevListener");
   uint16_t listenPort = static_cast<uint16_t>(ConfigReadInt(_T("MobileDeviceListenerPort"), SERVER_LISTEN_PORT_FOR_MOBILES));
   MobileDeviceListener listener(listenPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
      return;

   listener.mainLoop();
   listener.shutdown();
}

/**
 * Dump client sessions to screen
 */
void DumpMobileDeviceSessions(CONSOLE_CTX pCtx)
{
   static const TCHAR *cipherName[] = { _T("NONE"), _T("AES-256"), _T("BLOWFISH"), _T("IDEA"), _T("3DES"), _T("AES-128") };

   ConsolePrintf(pCtx, _T(" ID  | CIPHER   | USER                 | CLIENT\n"));
   ConsolePrintf(pCtx, _T("-----+----------+----------------------+----------------------------\n"));
   s_sessionListLock.readLock();
   int count = 0;
   for(int i = 0; i < MAX_DEVICE_SESSIONS; i++)
      if (s_sessionList[i] != nullptr)
      {
         ConsolePrintf(pCtx, _T(" %-3d | %-8s | %-20s | %s\n"), i,
					        cipherName[s_sessionList[i]->getCipher() + 1],
                       s_sessionList[i]->getUserName(),
                       s_sessionList[i]->getClientInfo());
         count++;
      }
   s_sessionListLock.unlock();
   ConsolePrintf(pCtx, _T("\n%d active session%s\n\n"), count, count == 1 ? _T("") : _T("s"));
}
