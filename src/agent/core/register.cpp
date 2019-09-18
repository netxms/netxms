/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
BOOL RegisterOnServer(const TCHAR *pszServer, UINT32 zoneUIN)
{
   SOCKET hSocket;
   BOOL bRet = FALSE;
   TCHAR szBuffer[MAX_RESULT_LENGTH];
   NXCP_MESSAGE *pRawMsg;
   NXCP_BUFFER *pBuffer;
   NXCPEncryptionContext *pDummyCtx = NULL;
   int nLen;

   InetAddress addr = InetAddress::resolveHostName(pszServer);
   if (!addr.isValidUnicast())
   {
		nxlog_write(NXLOG_WARNING, _T("Registration on management server failed (unable to resolve name of management server)"));
      return FALSE;
   }

   hSocket = CreateSocket(addr.getFamily(), SOCK_STREAM, 0);
   if (hSocket != INVALID_SOCKET)
   {
      SockAddrBuffer sa;
      addr.fillSockAddr(&sa, 4701);
      if (connect(hSocket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) != -1)
      {
         // Prepare request
         NXCPMessage msg(CMD_REGISTER_AGENT, 2, 2); // User version 2
         if (H_PlatformName(NULL, NULL, szBuffer, NULL) != SYSINFO_RC_SUCCESS)
            _tcscpy(szBuffer, _T("error"));
         msg.setField(VID_PLATFORM_NAME, szBuffer);
         msg.setField(VID_VERSION_MAJOR, (WORD)NETXMS_VERSION_MAJOR);
         msg.setField(VID_VERSION_MINOR, (WORD)NETXMS_VERSION_MINOR);
         msg.setField(VID_VERSION_RELEASE, (WORD)NETXMS_VERSION_BUILD);
         msg.setField(VID_ZONE_UIN, zoneUIN);

         // Send request
         pRawMsg = msg.serialize();
         nLen = ntohl(pRawMsg->size);
         if (SendEx(hSocket, pRawMsg, nLen, 0, NULL) == nLen)
         {
            pRawMsg = (NXCP_MESSAGE *)realloc(pRawMsg, MAX_MSG_SIZE);
            pBuffer = (NXCP_BUFFER *)malloc(sizeof(NXCP_BUFFER));
            NXCPInitBuffer(pBuffer);

            nLen = RecvNXCPMessage(hSocket, pRawMsg, pBuffer, MAX_MSG_SIZE,
                                   &pDummyCtx, NULL, 30000);
            if (nLen >= 16)
            {
               NXCPMessage *pResponse = NXCPMessage::deserialize(pRawMsg);
               if (pResponse != NULL)
               {
                  if ((pResponse->getCode() == CMD_REQUEST_COMPLETED) &&
                      (pResponse->getId() == 2) &&
                      (pResponse->getFieldAsUInt32(VID_RCC) == 0))
                  {
                     bRet = TRUE;
                  }
                  delete pResponse;
               }
            }
            free(pBuffer);
         }
         free(pRawMsg);

			if (bRet)
			{
				nxlog_write(NXLOG_INFO, _T("Successfully registered on management server %s"), pszServer);
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

   return bRet;
}
