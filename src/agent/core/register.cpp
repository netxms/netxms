/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: register.cpp
**
**/

#include "nxagentd.h"
#include <netxms-version.h>

/**
 * Externals
 */
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Constants
 */
#define MAX_MSG_SIZE    262144

/**
 * Register agent on management server
 */
bool RegisterOnServer(const TCHAR *server, int32_t zoneUIN)
{
   InetAddress addr = InetAddress::resolveHostName(server);
   if (!addr.isValidUnicast())
   {
		nxlog_write(NXLOG_WARNING, _T("Registration on management server failed (unable to resolve name of management server)"));
      return false;
   }

   bool success = false;
   SOCKET hSocket = CreateSocket(addr.getFamily(), SOCK_STREAM, 0);
   if (hSocket != INVALID_SOCKET)
   {
      SockAddrBuffer sa;
      addr.fillSockAddr(&sa, 4701);
      if (connect(hSocket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) != -1)
      {
         // Prepare request
         NXCPMessage msg(CMD_REGISTER_AGENT, 2, 4); // Use version 4

         TCHAR buffer[MAX_RESULT_LENGTH];
         if (H_PlatformName(nullptr, nullptr, buffer, nullptr) != SYSINFO_RC_SUCCESS)
            _tcscpy(buffer, _T("error"));
         msg.setField(VID_PLATFORM_NAME, buffer);
         msg.setField(VID_VERSION_MAJOR, static_cast<uint16_t>(NETXMS_VERSION_MAJOR));
         msg.setField(VID_VERSION_MINOR, static_cast<uint16_t>(NETXMS_VERSION_MINOR));
         msg.setField(VID_VERSION_RELEASE, static_cast<uint16_t>(NETXMS_VERSION_RELEASE));
         msg.setField(VID_ZONE_UIN, zoneUIN);

         // Send request
         NXCP_MESSAGE *rawMsg = msg.serialize();
         size_t msgLen = ntohl(rawMsg->size);
         if (SendEx(hSocket, rawMsg, msgLen, 0, nullptr) == msgLen)
         {
            SocketMessageReceiver receiver(hSocket, 4096, MAX_MSG_SIZE);
            MessageReceiverResult result;
            NXCPMessage *response = receiver.readMessage(30000, &result);
            if (response != nullptr)
            {
               if ((response->getCode() == CMD_REQUEST_COMPLETED) &&
                   (response->getId() == 2) &&
                   (response->getFieldAsUInt32(VID_RCC) == 0))
               {
                  success = true;
               }
               delete response;
            }
         }
         MemFree(rawMsg);

			if (success)
			{
				nxlog_write(NXLOG_INFO, _T("Successfully registered on management server %s"), server);
			}
			else
			{
		      nxlog_write(NXLOG_WARNING, _T("Registration on management server failed (communication failure)"));
			}
      }
      else
      {
         nxlog_write(NXLOG_WARNING, _T("Registration on management server failed (unable to connect to management server)"));
      }
      closesocket(hSocket);
   }

   return success;
}
