/*
** NetXMS multiplatform core agent
** Copyright (C) 2014-2026 Raden Solutions
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
#include <socket_listener.h>

#define MAX_SYSLOG_MSG_LEN    8192

/**
 * Counter for received messages
 */
static uint64_t s_receivedMessages = 0;

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
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Syslog messages receiver
 */
class SyslogReceiverListener : public DatagramSocketListener
{
private:
   uint64_t m_requestId;

protected:
   virtual bool isStopConditionReached() override
   {
      return (g_dwFlags & AF_SHUTDOWN) != 0;
   }

   virtual ConnectionProcessingResult processDatagram(SOCKET s) override;

public:
   SyslogReceiverListener() : DatagramSocketListener(g_syslogListenPort, (g_dwFlags & AF_DISABLE_IPV4) == 0, (g_dwFlags & AF_DISABLE_IPV6) == 0)
   {
      m_requestId = static_cast<uint64_t>(time(nullptr)) << 32;
      setName(_T("Syslog"));
   }
};

/**
 * Process incoming syslog message
 */
ConnectionProcessingResult SyslogReceiverListener::processDatagram(SOCKET s)
{
   char syslogMessage[MAX_SYSLOG_MSG_LEN + 1];
   SockAddrBuffer addr;
   socklen_t addrLen = sizeof(SockAddrBuffer);
   int bytes = recvfrom(s, syslogMessage, MAX_SYSLOG_MSG_LEN, 0, (struct sockaddr *)&addr, &addrLen);
   if (bytes > 0)
   {
      syslogMessage[bytes] = 0;

      NXCPMessage *msg = new NXCPMessage(CMD_SYSLOG_RECORDS, GenerateMessageId(), 4);   // Use version 4
      msg->setField(VID_REQUEST_ID, m_requestId++);
      msg->setField(VID_IP_ADDRESS, InetAddress::createFromSockaddr((struct sockaddr *)&addr));
      msg->setFieldFromTime(VID_TIMESTAMP, time(nullptr));
      msg->setField(VID_MESSAGE, reinterpret_cast<BYTE*>(syslogMessage), bytes + 1);
      msg->setField(VID_MESSAGE_LENGTH, static_cast<uint32_t>(bytes));
      msg->setField(VID_ZONE_UIN, g_zoneUIN);
      g_notificationProcessorQueue.put(msg);
      s_receivedMessages++;
   }
   else
   {
      // Sleep on error
      ThreadSleepMs(100);
   }
   return CPR_COMPLETED;
}

/**
 * Syslog messages receiver thread
 */
void SyslogReceiver()
{
   SyslogReceiverListener listener;
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
   {
      nxlog_debug(1, _T("Syslog receiver aborted - cannot bind at least one socket"));
      return;
   }

   nxlog_debug(1, _T("Syslog receiver thread started"));
   listener.mainLoop();
   nxlog_debug(1, _T("Syslog receiver thread stopped"));
}
