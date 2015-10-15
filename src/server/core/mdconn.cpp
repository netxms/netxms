/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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

/**
 * Static data
 */
static MobileDeviceSession *m_pSessionList[MAX_DEVICE_SESSIONS];
static RWLOCK m_rwlockSessionListAccess;

/**
 * Register new session in list
 */
static BOOL RegisterMobileDeviceSession(MobileDeviceSession *pSession)
{
   UINT32 i;

   RWLockWriteLock(m_rwlockSessionListAccess, INFINITE);
   for(i = 0; i < MAX_DEVICE_SESSIONS; i++)
      if (m_pSessionList[i] == NULL)
      {
         m_pSessionList[i] = pSession;
         pSession->setId(i + MAX_CLIENT_SESSIONS);
         RWLockUnlock(m_rwlockSessionListAccess);
         return TRUE;
      }

   RWLockUnlock(m_rwlockSessionListAccess);
   nxlog_write(MSG_TOO_MANY_MD_SESSIONS, EVENTLOG_WARNING_TYPE, NULL);
   return FALSE;
}

/**
 * Unregister session
 */
void UnregisterMobileDeviceSession(int id)
{
   RWLockWriteLock(m_rwlockSessionListAccess, INFINITE);
   m_pSessionList[id - MAX_CLIENT_SESSIONS] = NULL;
   RWLockUnlock(m_rwlockSessionListAccess);
}

/**
 * Initialize mobile device listener(s)
 */
void InitMobileDeviceListeners()
{
   // Create session list access rwlock
   m_rwlockSessionListAccess = RWLockCreate();
}

/**
 * Listener thread
 */
THREAD_RESULT THREAD_CALL MobileDeviceListener(void *arg)
{
   SOCKET sock, sockClient;
   struct sockaddr_in servAddr;
   int errorCount = 0;
   socklen_t iSize;
   WORD wListenPort;
   MobileDeviceSession *pSession;

   // Read configuration
   wListenPort = (WORD)ConfigReadInt(_T("MobileDeviceListenerPort"), SERVER_LISTEN_PORT_FOR_MOBILES);

   // Create socket
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("MobileDeviceListener"));
      return THREAD_OK;
   }

	SetSocketExclusiveAddrUse(sock);
	SetSocketReuseFlag(sock);
#ifndef _WIN32
   fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);
#endif

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = !_tcscmp(g_szListenAddress, _T("*")) ? 0 : ResolveHostName(g_szListenAddress);
   servAddr.sin_port = htons(wListenPort);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", wListenPort, _T("MobileDeviceListener"), WSAGetLastError());
      closesocket(sock);
      /* TODO: we should initiate shutdown procedure here */
      return THREAD_OK;
   }

   // Set up queue
   listen(sock, SOMAXCONN);
	nxlog_write(MSG_LISTENING_FOR_MOBILE_DEVICES, EVENTLOG_INFORMATION_TYPE, "ad", ntohl(servAddr.sin_addr.s_addr), wListenPort);

   // Wait for connection requests
   while(!IsShutdownInProgress())
   {
      iSize = sizeof(struct sockaddr_in);
      if ((sockClient = accept(sock, (struct sockaddr *)&servAddr, &iSize)) == -1)
      {
         int error;

#ifdef _WIN32
         error = WSAGetLastError();
         if (error != WSAEINTR)
#else
         error = errno;
         if (error != EINTR)
#endif
            nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
         errorCount++;
         if (errorCount > 1000)
         {
            nxlog_write(MSG_TOO_MANY_ACCEPT_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
            errorCount = 0;
         }
         ThreadSleepMs(500);
			continue;
      }

      errorCount = 0;     // Reset consecutive errors counter
		SetSocketNonBlocking(sockClient);

      // Create new session structure and threads
      pSession = new MobileDeviceSession(sockClient, (struct sockaddr *)&servAddr);
      if (!RegisterMobileDeviceSession(pSession))
      {
         delete pSession;
      }
      else
      {
         pSession->run();
      }
   }

   closesocket(sock);
   return THREAD_OK;
}

/**
 * Listener thread - IPv6
 */
#ifdef WITH_IPV6

THREAD_RESULT THREAD_CALL MobileDeviceListenerIPv6(void *arg)
{
   SOCKET sock, sockClient;
   struct sockaddr_in6 servAddr;
   int errorCount = 0;
   socklen_t iSize;
   WORD wListenPort;
   MobileDeviceSession *pSession;

   // Read configuration
   wListenPort = (WORD)ConfigReadInt(_T("MobileDeviceListenerPort"), SERVER_LISTEN_PORT_FOR_MOBILES);

   // Create socket
   if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("MobileDeviceListenerIPv6"));
      return THREAD_OK;
   }

   SetSocketExclusiveAddrUse(sock);
   SetSocketReuseFlag(sock);
#ifdef IPV6_V6ONLY
   int on = 1;
   setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
#endif
#ifndef _WIN32
   fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);
#endif

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in6));
   servAddr.sin6_family = AF_INET6;
   //servAddr.sin6_addr.s6_addr = ResolveHostName(g_szListenAddress);
   servAddr.sin6_port = htons(wListenPort);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in6)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", wListenPort, _T("MobileDeviceListenerIPv6"), WSAGetLastError());
      closesocket(sock);
      /* TODO: we should initiate shutdown procedure here */
      return THREAD_OK;
   }

   // Set up queue
   listen(sock, SOMAXCONN);
	nxlog_write(MSG_LISTENING_FOR_MOBILE_DEVICES, EVENTLOG_INFORMATION_TYPE, "Ad", servAddr.sin6_addr.s6_addr, wListenPort);

   // Wait for connection requests
   while(!IsShutdownInProgress())
   {
      iSize = sizeof(struct sockaddr_in6);
      if ((sockClient = accept(sock, (struct sockaddr *)&servAddr, &iSize)) == -1)
      {
         int error;

#ifdef _WIN32
         error = WSAGetLastError();
         if (error != WSAEINTR)
#else
         error = errno;
         if (error != EINTR)
#endif
            nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
         errorCount++;
         if (errorCount > 1000)
         {
            nxlog_write(MSG_TOO_MANY_ACCEPT_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
            errorCount = 0;
         }
         ThreadSleepMs(500);
			continue;
      }

      errorCount = 0;     // Reset consecutive errors counter
		SetSocketNonBlocking(sockClient);

      // Create new session structure and threads
      pSession = new MobileDeviceSession(sockClient, (struct sockaddr *)&servAddr);
      if (!RegisterMobileDeviceSession(pSession))
      {
         delete pSession;
      }
      else
      {
         pSession->run();
      }
   }

   closesocket(sock);
   return THREAD_OK;
}

#endif

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
   RWLockReadLock(m_rwlockSessionListAccess, INFINITE);
   for(i = 0, iCount = 0; i < MAX_DEVICE_SESSIONS; i++)
      if (m_pSessionList[i] != NULL)
      {
         ConsolePrintf(pCtx, _T("%-3d %-24s %-8s %s [%s]\n"), i, 
                       (m_pSessionList[i]->getState() != SESSION_STATE_PROCESSING) ?
                         pszStateName[m_pSessionList[i]->getState()] :
                         NXCPMessageCodeName(m_pSessionList[i]->getCurrentCmd(), szBuffer),
					        pszCipherName[m_pSessionList[i]->getCipher() + 1],
                       m_pSessionList[i]->getUserName(),
                       m_pSessionList[i]->getClientInfo());
         iCount++;
      }
   RWLockUnlock(m_rwlockSessionListAccess);
   ConsolePrintf(pCtx, _T("\n%d active session%s\n\n"), iCount, iCount == 1 ? _T("") : _T("s"));
}
