/*
** NetXMS multiplatform core agent
** Copyright (C) 2014-2017 Raden Solutions
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
** File: syslog.cpp
**
**/

#include "nxagentd.h"

#define MAX_SYSLOG_MSG_LEN    1024

/**
 * Syslog record
 */
class SyslogRecord
{
public:
   InetAddress addr;
   time_t timestamp;
   char *message;
   INT32 messageLength;

   SyslogRecord(const InetAddress& a, const char *m, int len)
   {
      addr = a;
      timestamp = time(NULL);
      message = strdup(m);
      messageLength = len;
   }
   ~SyslogRecord()
   {
      free(message);
   }
};

/**
 * Sender queue
 */
static Queue s_syslogSenderQueue;

/**
 * Counter for received messages
 */
static UINT64 s_receivedMessages = 0;

/**
 * Handler for syslog proxy information parameters
 */
LONG H_SyslogStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   switch(*arg)
   {
      case 'R':   // received messages
         ret_uint64(value, s_receivedMessages);
         break;
      case 'Q':   // queue size
         ret_int(value, s_syslogSenderQueue.size());
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Shutdown trap sender
 */
void ShutdownSyslogSender()
{
   s_syslogSenderQueue.setShutdownMode();
}

/**
 * Syslog messages sender thread
 */
THREAD_RESULT THREAD_CALL SyslogSender(void *)
{
   nxlog_debug(1, _T("Syslog sender thread started"));
   UINT64 id = (UINT64)time(NULL) << 32;
   while(true)
   {
      nxlog_debug(8, _T("SyslogSender: waiting for message"));
      SyslogRecord *rec = (SyslogRecord *)s_syslogSenderQueue.getOrBlock();
      if (rec == INVALID_POINTER_VALUE)
         break;

      nxlog_debug(6, _T("SyslogSender: got message from queue"));
      bool sent = false;

      NXCPMessage msg;
      msg.setCode(CMD_SYSLOG_RECORDS);
      msg.setId(GenerateMessageId());
      msg.setField(VID_REQUEST_ID, id++);
      msg.setField(VID_IP_ADDRESS, rec->addr);
      msg.setFieldFromTime(VID_TIMESTAMP, rec->timestamp);
      msg.setField(VID_MESSAGE, (BYTE *)rec->message, rec->messageLength + 1);
      msg.setField(VID_MESSAGE_LENGTH, rec->messageLength);
      msg.setField(VID_ZONE_UIN, g_zoneUIN);

      if (g_dwFlags & AF_SUBAGENT_LOADER)
      {
         sent = SendMessageToMasterAgent(&msg);
      }
      else
      {
         MutexLock(g_hSessionListAccess);
         for(UINT32 i = 0; i < g_dwMaxSessions; i++)
         {
            if (g_pSessionList[i] != NULL)
            {
               if (g_pSessionList[i]->canAcceptTraps())
               {
                  g_pSessionList[i]->sendMessage(&msg);
                  sent = true;
               }
            }
         }
         MutexUnlock(g_hSessionListAccess);
      }

      if (!sent)
      {
         nxlog_debug(6, _T("Cannot forward syslog message to server"));
         s_syslogSenderQueue.insert(rec);
         ThreadSleep(5);
      }
      else
      {
         nxlog_debug(6, _T("Syslog message successfully forwarded to server"));
         delete rec;
      }
   }
   nxlog_debug(1, _T("Syslog sender thread terminated"));
   return THREAD_OK;
}

/**
 * Syslog messages receiver thread
 */
THREAD_RESULT THREAD_CALL SyslogReceiver(void *)
{
   SOCKET hSocket = (g_dwFlags & AF_DISABLE_IPV4) ? INVALID_SOCKET : socket(AF_INET, SOCK_DGRAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = (g_dwFlags & AF_DISABLE_IPV6) ? INVALID_SOCKET : socket(AF_INET6, SOCK_DGRAM, 0);
#endif

#ifdef WITH_IPV6
   if ((hSocket == INVALID_SOCKET) && (hSocket6 == INVALID_SOCKET))
#else
   if (hSocket == INVALID_SOCKET)
#endif
   {
      nxlog_debug(1, _T("SyslogReceiver: cannot create socket"));
      return THREAD_OK;
   }

   SetSocketExclusiveAddrUse(hSocket);
   SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef WITH_IPV6
   SetSocketExclusiveAddrUse(hSocket6);
   SetSocketReuseFlag(hSocket6);
#ifndef _WIN32
   fcntl(hSocket6, F_SETFD, fcntl(hSocket6, F_GETFD) | FD_CLOEXEC);
#endif
#ifdef IPV6_V6ONLY
   int on = 1;
   setsockopt(hSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
#endif
#endif

   // Fill in local address structure
   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;

#ifdef WITH_IPV6
   struct sockaddr_in6 servAddr6;
   memset(&servAddr6, 0, sizeof(struct sockaddr_in6));
   servAddr6.sin6_family = AF_INET6;
#endif

   if (!_tcscmp(g_szListenAddress, _T("*")))
   {
      servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WITH_IPV6
      memset(servAddr6.sin6_addr.s6_addr, 0, 16);
#endif
   }
   else
   {
      InetAddress bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
         servAddr.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
      }
      else
      {
         servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
#ifdef WITH_IPV6
      bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET6);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET6))
      {
         memcpy(servAddr6.sin6_addr.s6_addr, bindAddress.getAddressV6(), 16);
      }
      else
      {
         memset(servAddr6.sin6_addr.s6_addr, 0, 15);
         servAddr6.sin6_addr.s6_addr[15] = 1;
      }
#endif
   }
   servAddr.sin_port = htons(g_syslogListenPort);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(g_syslogListenPort);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug(5, _T("SyslogReceiver: trying to bind on UDP %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", g_syslogListenPort, _T("SyslogReceiver"), WSAGetLastError());
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   nxlog_debug(5, _T("SyslogReceiver: trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", g_syslogListenPort, _T("SyslogReceiver"), WSAGetLastError());
      bindFailures++;
      closesocket(hSocket6);
      hSocket6 = INVALID_SOCKET;
   }
#else
   bindFailures++;
#endif

   // Abort if cannot bind to at least one socket
   if (bindFailures == 2)
   {
      nxlog_debug(1, _T("Syslog receiver aborted - cannot bind at least one socket"));
      return THREAD_OK;
   }

   if (hSocket != INVALID_SOCKET)
      nxlog_debug(1, _T("Listening for syslog messages on %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
      nxlog_debug(1, _T("Listening for syslog messages on [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
#endif

   nxlog_debug(1, _T("Syslog receiver thread started"));

   // Wait for packets
   SocketPoller sp;
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      sp.reset();
      if (hSocket != INVALID_SOCKET)
         sp.add(hSocket);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         sp.add(hSocket6);
#endif

      int rc = sp.poll(1000);
      if (rc > 0)
      {
         char syslogMessage[MAX_SYSLOG_MSG_LEN + 1];
         SockAddrBuffer addr;
         socklen_t addrLen = sizeof(SockAddrBuffer);
#ifdef WITH_IPV6
         SOCKET s = sp.isSet(hSocket) ? hSocket : hSocket6;
#else
         SOCKET s = hSocket;
#endif
         int bytes = recvfrom(s, syslogMessage, MAX_SYSLOG_MSG_LEN, 0, (struct sockaddr *)&addr, &addrLen);
         if (bytes > 0)
         {
            syslogMessage[bytes] = 0;
            s_syslogSenderQueue.put(new SyslogRecord(InetAddress::createFromSockaddr((struct sockaddr *)&addr), syslogMessage, bytes));
            s_receivedMessages++;
         }
         else
         {
            // Sleep on error
            ThreadSleepMs(100);
         }
      }
      else if (rc == -1)
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   if (hSocket != INVALID_SOCKET)
      closesocket(hSocket);
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
      closesocket(hSocket6);
#endif

   nxlog_debug(1, _T("Syslog receiver thread stopped"));
   return THREAD_OK;
}
